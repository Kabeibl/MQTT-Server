/* HEADERS */
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include "MQTTClient.h"
#include "signal_handler.h"
#include "stdlib.h"
#include "string.h"
#include "error_handler.h"
#include "user_interface.h"
#include "mqtt_client.h"

/* DEFINITIONS */

/* GLOBAL VARIABLES */
MENU*               main_menu;          // Main menu
WINDOW              *main_menu_win,     // Main menu window
                    *msg_win,           // Message window
                    *msg_win_frame,     // Message window frame
                    *clients_win,       // Connected clients' window
                    *clients_win_frame; // Connected clients' window frame
CLIENT_INFO*        clients;            // Client array
int                 n_clients = 0;      // Number of clients connected
int                 log_fd;             // Log file fd

/* PROTOTYPES */
void signal_handler         (int signo, siginfo_t *siginfo, void *context);
void handle_selection       (int selection);
void init_sigact            (struct sigaction *sigact, struct sigaction *oldact);
void open_log               (void);
void log_message            (char *msg, int len);
void add_client             (CLIENT_INFO client);
void disconnect_client      (int client_num);
void disconnect_clients     (void);
void redraw_windows         (void);
void quit                   (void);
int check_client_id         (CLIENT_INFO *client);
CLIENT_INFO get_client_info (int client_num);

/* MAIN */
int main(int argc, char* argv[]) {

    /* Declare variables */
    int                 select;                         // Main menu selection   
    struct sigaction    sigact, oldact;                 // Sigaction structs

    //create_shared_mutex();                              // Create mutex
    init_sigact(&sigact, &oldact);                      // Initiate signal handler
    init_screen();                                      // Initialise curses
    init_main_menu(&main_menu, &main_menu_win);         // Initialise menu
    init_msg_win(&msg_win, &msg_win_frame);             // Initialise message window
    init_client_win(&clients_win, &clients_win_frame);  // Initialise clients' window
    open_log();
    
    /* Run the server */
    while(1) {  

        /* Drive main menu */
        select = drive_main_menu(&main_menu,            // Menu
                                 &main_menu_win);       // Menu window

        handle_selection(select);                       // Handle selection
    }
    
    /* Code should never reach this point */
    return 0;
}

/* FUNCTIONS */
/* Signal handler */
void signal_handler         (int signo, siginfo_t *siginfo, void *context) {

    CLIENT_INFO     client;
    int             client_num,
                    success = 0;

    switch(signo) {

        case SIGINT:
            if(prompt_exit() == 1) quit();                  // Exit program
            redraw_windows();                               // Re-draw windows
            break;

        case SIGCONNSUCCESS:
            client_conn_status = CONN_SUCCESS;              // Set flag for client
            break;

        case SIGCONNFAIL:
            client_conn_status = CONN_FAIL;                 // Set flag for client
            break;

        case SIGMESSAGE:

            client_num = (int)siginfo->si_value.sival_int;  // Get client number
            client = get_client_info(client_num);           // Fetch client info

            memset(client.payload, 0, sizeof(client.payload));

            /* Read message from pipe */
            success = read(client.msg_pipe,                 // Pipe
                          client.payload,                   // Message buffer
                          sizeof(client.payload));          // Message length

            if(success > 0) {
                print_mqtt_message(&client, msg_win);       // Print message
                log_message(client.payload,                 // Write message to log file
                            sizeof(client.payload));        //             
            }
            else {
                handle_error(errno, GENERAL_ERR, &stdscr);  // Handle error
            }
            break;

        case SIGPIPE:
            printw("WRITE FAILED!");
            break;
    }
}

/* Main menu selection handler */
void handle_selection       (int select) {
    
    /* Declare variables */
    CLIENT_INFO             client;
    int                     client_num;

    memset(&client, 0, sizeof(CLIENT_INFO));

    switch(select) {

        case SUBSCRIBE:

            if(drive_sub_menu(&client) == DONE) {

                client.client_number = n_clients;              // Set client number

                /* Check if client id is already in use */
                if(check_client_id(&client) > 0) {

                    subscribe(&client);                            // Subscribe to given client
                    redraw_windows();                              // Re-draw windows

                    /* Wait for connection to succeed */
                    if(wait_for_connection() == CONN_SUCCESS) {
                       add_client(client);                         // Add client to array
                    } 
                }

                else {
                    handle_error(CLIENT_ID_CONFLICT, SUB_ERR, &main_menu_win);
                    redraw_windows();                              // Re-draw windows
                }                      
                
            }
            break;

        case UNSUBSCRIBE:

            if(n_clients > 0) {

                if((client_num = drive_unsub_menu(clients, n_clients)) >= 0) {

                disconnect_client(client_num);
                }
            }

            else{
                //handle_error()
            }

            break;

        case QUIT:

            if(prompt_exit() == 1) quit();      // Exit program
            break;
    }

    redraw_windows();                                               // Re-draw windows
}

/* Initiate signal action */
void init_sigact            (struct sigaction *sigact, struct sigaction *oldact) {

    sigemptyset(&sigact->sa_mask);              // Clear mask
    sigact->sa_sigaction = signal_handler;      // Set handler
    sigact->sa_flags = SA_SIGINFO;              // Set flags

    sigaction(SIGRTMAX, sigact, oldact);        // Save old set, set SIGRTMAX
    sigaction(SIGINT, sigact, NULL);            // Set SIGINT
    sigaction(SIGCONNSUCCESS, sigact, NULL);    // Set SIGCONNSUCCESS
    sigaction(SIGCONNFAIL, sigact, NULL);       // Set SIGCONNFAIL
    sigaction(SIGSUBSUCCESS, sigact, NULL);     // Set SIGSUBSUCCESS
    sigaction(SIGSUBFAIL, sigact, NULL);        // Set SIGSUBFAIL
    sigaction(SIGMESSAGE, sigact, NULL);        // Set SIGMESSAGE
    sigaction(SIGPIPE, sigact, NULL);           // Set SIGPIPE
}

/* Open log file for writing */
void open_log               (void) {

    char *log_path = "../log/mqtt_log";   // Path to log-file

    log_fd = open(log_path,
                O_CREAT |
                O_APPEND |
                O_WRONLY,
                0755);
}

/* Write message to log file */
void log_message            (char *msg, int len) {

    char buf[len];
    sprintf(buf, "%s\n", msg);
    write(log_fd, buf, strlen(buf));
}

/* Add a pipe file descriptor to pipe array */
void add_client             (CLIENT_INFO client) {

    n_clients++;                                        // Increment number of clients

    /* Declare variables */
    int                 n = n_clients;                  // Number of clients
    int                 size = sizeof(CLIENT_INFO);     // Size of one client info struct
    CLIENT_INFO         *temp;                          // Temporary client array
    char                msg[128],                       // Message buffer
                        time_buf[24];                   // Time buffer
    struct timespec     t1;

    temp = (CLIENT_INFO*) malloc((n + 1) * size);       // Allocate mem for temp arr
    memmove(temp, clients, (n - 1) * size);             // Copy old arr to temp arr
    temp[n - 1] = client;                               // Add new client to temp arr
    free(clients);                                      // Deallocate mem from old arr

    clients = temp;                                     // Copy new client array

    /* Get current time */
    timespec_get(&t1, TIME_UTC);

    /* Format time */
    strftime(time_buf,                                  // Buffer
             sizeof(time_buf),                          // Length
             "%d/%m/%Y %H:%M:%S",                       // Format
             gmtime(&t1.tv_sec));                       // Source

    /* Format message */
    snprintf(msg,
            sizeof(msg),
            "[%s] Client connected : [ID] %s [TOPIC] %s",
            time_buf, client.client_id, client.topic);

    print_clients(clients, n_clients, &clients_win);    // Print out clients' list
    wprintw(msg_win, "%s\n", msg);                      // Print message
    wrefresh(msg_win);                                  // Refresh
    log_message(msg, sizeof(msg));                      // Make a log entry
}

/* Disconnect a client */
void disconnect_client      (int client_num) {

    /* Declare variables */
    int                 n = n_clients,
                        size = sizeof(CLIENT_INFO),
                        i = 0, 
                        j = 0;
    char                status[1] = {TERMINATED + '0'}, // Set status
                        msg[128],                       // Message buffer
                        time_buf[24];                   // Time buffer
    CLIENT_INFO         *temp;
    struct timespec     t1;    

    /* Get current time */
    timespec_get(&t1, TIME_UTC);

    /* Format time */
    strftime(time_buf,                          // Buffer
             sizeof(time_buf),                  // Length
             "%d/%m/%Y %H:%M:%S",               // Format
             gmtime(&t1.tv_sec));               // Source

    /* Format message */
    snprintf(msg,
            sizeof(msg),
            "[%s] Client disconnected : [ID] %s [TOPIC] %s",
            time_buf, 
            clients[client_num].client_id, 
            clients[client_num].topic);               

    write(clients[client_num].status_pipe, status, 1);  // Write termination status to client

    temp = (CLIENT_INFO*) malloc((n - 1) * size);       // Allocate mem for temp arr

    /* Copy clients */
    for(i = 0; i < n_clients; i++) {
        if(i != client_num) {
            memmove(&temp[j++], &clients[i], size);     // Copy current clients to temp array
        }
    }

    free(clients);                                      // Deallocate mem from old arr

    clients = temp;                                     // Allocate mem for new arr

    n_clients--;                                        // Decrement client counter
    print_clients(clients, n_clients, &clients_win);    // Print client list
    wprintw(msg_win,"%s\n", msg);                       // Print message
    wrefresh(msg_win);                                  // Refresh screen
    log_message(msg, sizeof(msg));                      // Write to log file
}

/* Close connections to clients */
void disconnect_clients     (void) {

    char status[1] = {TERMINATED + '0'};                // Set status
    char buf[128];
    int success = 0;

    for(int i = 0; i < n_clients; i++) {

        write(clients[i].status_pipe,status, 1);                // Write status to pipe
        success = read(clients[i].msg_pipe, buf, sizeof(buf));  // Read term status from client

        if(success > 0) {
            log_message(buf, sizeof(buf));                      // Write message to log file
        }
    }

    free(clients);                                      // Deallocate mem from client array
}  

/* Redraw windows */
void redraw_windows         (void) {
    print_header();
    print_footer();
    redrawwin(main_menu_win);
    redrawwin(msg_win);
    redrawwin(msg_win_frame);
    redrawwin(clients_win);
    redrawwin(clients_win_frame);
    wrefresh(main_menu_win);
    wrefresh(msg_win);
    wrefresh(msg_win_frame);
    wrefresh(clients_win);
    wrefresh(clients_win_frame);
}

/* Close all processes */
void quit                   (void) {

    //destroy_shared_mutex();                 // Destroy shared mutex
    disconnect_clients();                   // Disconnect clients
    close_menu(&main_menu);                 // Close main menu
    destroy_win(&main_menu_win);            // Destroy main menu window
    destroy_win(&msg_win);                  // Destroy message window
    destroy_win(&msg_win_frame);            // Destroy message frame window 
    destroy_win(&clients_win);              // Destroy clients window
    destroy_win(&clients_win_frame);        // Destroy clients' frame window
    endwin();                               // Close ncurses
    exit(0);                                // Exit server
}

/* Check if given client ID is free */
int check_client_id         (CLIENT_INFO *client) {

    for(int i = 0; i < n_clients; i++) {

        if(strcmp(client->client_id, clients[i].client_id) == 0) {
            return 0;
        }
    }

    return 1;
}

/* Get client info */
CLIENT_INFO get_client_info (int client_num) {

    CLIENT_INFO client = {0};

    for(int i = 0; i < n_clients; i++) {

        if(clients[i].client_number == client_num) {
            client = clients[i];
        }
    }

    return(client);
}