#include "mqtt_client.h"
#include "MQTTClient.h"
#include "error_handler.h"
#include "signal_handler.h"
#include "user_interface.h"
#include "shared_mem.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

/* PROTOTYPES */
pid_t 						server_pid;
CLIENT_CONN_STATUS 			client_conn_status;
int  message_arrived		(void *context, char *topic, int topic_len, MQTTClient_message *message);
void conn_lost 				(void *context, char *cause);
void message_delivered		(void *context, MQTTClient_deliveryToken dt);
//void open_mutex 			(int *mutex_fd, pthread_mutex_t **mutex);

/* Subscribe to client */
void subscribe 				(CLIENT_INFO* client_info) {

	/* Declare variables */
	pid_t			client_pid;				// Client PID
	int 			status_pipe[2],			// Pipe file descriptor array
					msg_pipe[2];			// Pipe file descriptor array

	pipe(status_pipe);						// Create status pipe
	pipe(msg_pipe);							// Create message pipe
	server_pid = getpid();					// Set server pid
	client_pid = fork();					// Fork

	/* Temp parent */
	if(client_pid == 0) {

		close(status_pipe[1]);				// Close write end of status pipe
		close(msg_pipe[0]); 				// Close read end of message pipe
		client_pid = fork();				// Fork

		/* Kill temp parent */
		if(client_pid != 0){
			close(status_pipe[0]);			// Close read end of status pipe
			close(msg_pipe[1]); 			// Close write end of message pipe
			exit(0);						// Kill process
		} 

		/* Declare variables */
		MQTTClient 						client;
	    MQTTClient_connectOptions 		conn_opts 	= MQTTClient_connectOptions_initializer;
	    CLIENT_INFO 					info 		= *client_info;
	   	PROGRAM_STATUS 					program_status;
	    int 							rc,
	    								term_status = 0;
	    char  							buf[1],
	    								term_buf[128],
	    								time_buf[24];
	    struct timespec 				t1;

		//open_mutex(&shared_mutex_fd, &shared_mutex);	// Open shared memory fd for mutex
		client_info->msg_pipe = msg_pipe[1];

	    /* Create client */
	    MQTTClient_create(&client, 						// Client
	    				  info.address,					// Address
	    				  info.client_id,				// Client ID
	    				  MQTTCLIENT_PERSISTENCE_NONE,	// Persistence level
	    				  NULL);						// Persistence file path

	    /* Set connection options */
	    conn_opts.keepAliveInterval = 20;				// Set keepalive interval
	    conn_opts.cleansession = 1;						//

	    /* Set callback functions */
	    MQTTClient_setCallbacks(client,					// Client
	    						client_info,			// Context
	    						conn_lost,				// Connection lost cb
	    						message_arrived, 		// Message arrived cb
	    						message_delivered);		// Message delivered cb
		

	    /* Connect to client */
	    rc = MQTTClient_connect(client, 				// Client
	    						&conn_opts);			// Connection options
	    
	    /* Handle error */
	    if(rc != MQTTCLIENT_SUCCESS){

	    	/* Send signal to server */
	        sigqueue(server_pid, 						// Server process ID
	        		 SIGCONNFAIL,						// Signal number
	        		 (union sigval)NULL);				// Info

			close(status_pipe[0]);						// Close read end of status pipe
			close(msg_pipe[1]); 						// Close write end of message pipe
        	exit(0);									// Exit process
	    }
	    /********/

        /* Subscribe to client */
        rc = MQTTClient_subscribe(client,				// Client 
        					 	  info.topic, 			// Topic
        					 	  info.qos);			// Quality of Service

        /* Handle error */
        if(rc != MQTTCLIENT_SUCCESS) {

        	/* Send signal to server */
	        sigqueue(server_pid, 						// Server process ID
	        		 SIGCONNFAIL,						// Signal number
	        		 (union sigval)NULL);				// Info

        	handle_error(rc, SUB_ERR, &stdscr);			// Handle error
        	close(status_pipe[0]);						// Close read end of status pipe
			close(msg_pipe[1]); 						// Close write end of message pipe
        	exit(0);									// Exit process
        }

        /* Send signal to server */
        sigqueue(server_pid, 							// Server process ID
	    		 SIGCONNSUCCESS,						// Signal number
	    		 (union sigval)NULL);					// Info

        /* Wait for termination */
        do {
        	read(status_pipe[0], buf, 1); 				// Read status
        	program_status = buf[0] - '0';				// Convert to status to int
        } while(program_status == RUNNING);
    
        /* Get current time */
		timespec_get(&t1, TIME_UTC);

		/* Format time */
		strftime(time_buf,  						// Buffer
				 sizeof(time_buf), 					// Length
				 "%d/%m/%Y %H:%M:%S",  				// Format
				 gmtime(&t1.tv_sec)); 				// Source

        sprintf(term_buf, "[%s] Process PID <%d> terminated with status <%d>",
        		time_buf,
        		getpid(),
        		term_status);

    	write(msg_pipe[1], term_buf, sizeof(term_buf));
	    
		close(status_pipe[0]);							// Close read end of status pipe
		close(msg_pipe[1]); 							// Close write end of message pipe        
		exit(term_status);								// Exit process
	}

	close(status_pipe[0]);								// Close read end of status pipe
	close(msg_pipe[1]); 								// Close write end of message pipe
	wait(NULL);											// Wait

	client_info->status_pipe = status_pipe[1];			// Save status pipe
	client_info->msg_pipe = msg_pipe[0];				// Save message pipe
}

/* Callback function for message arrival event */
int message_arrived			(void *context, char *topic, int topic_len, MQTTClient_message *msg) {

	/* Declare variables */
	CLIENT_INFO 		client 		 = *(CLIENT_INFO*)context; 
	char 				message[128] = {0},
	 					time_buf[24] = {0},
	 					payload[msg->payloadlen + 1];
	struct timespec 	t1;
	union sigval        sig_val;

	sig_val.sival_int = client.client_number;	// Set client number

	/* Get current time */
	timespec_get(&t1, TIME_UTC);

	/* Format time */
    strftime(time_buf,  						// Buffer
    		 sizeof(time_buf), 					// Length
    		 "%d/%m/%Y %H:%M:%S",  				// Format
    		 gmtime(&t1.tv_sec)); 				// Source

    /* Fetch message */
    snprintf(payload, 
    		 msg->payloadlen + 1, 
    		 "%s", 
    		 (char*)msg->payload);

    /* Format message */
	snprintf(message, 														
			 sizeof(message), 												
			 "[%s] Message arrived  : [ID] %s [TOPIC] %s [PAYLOAD] %s", 
			 time_buf,
			 client.client_id,
			 topic, 
			 payload);

	/* Write message to pipe */
    write(client.msg_pipe, message, strlen(message)); 
	
    /* Send a signal */
    sigqueue(server_pid,						// Server PID
    		 SIGMESSAGE, 						// Signal number
    		 sig_val); 							// Info to pass to server
   
    
    MQTTClient_freeMessage(&msg);			// Free memory allocated to message
    MQTTClient_free(topic);					// Free memory allocated to topic
    return 1;
}

/* Callback function for connection lost -event */
void conn_lost 				(void *context, char *cause) {

	//char 		*message 	= NULL;				// Message buffer

    /* Format message */
	//sprintf(message, 							// Message buffer
	//		"Connection lost: %s", 				// Message
	//		cause);								// Source
}

/* Callback function for message delivered -event */
void message_delivered		(void *context, MQTTClient_deliveryToken dt) {

	//char 		*message 	= NULL;				// Message buffer

    /* Format message */
	//sprintf(message, 							// Message buffer
	//		"Delivery confirmed on token %d", 	// Message
	//		dt);								// Source
}

#if 0
/* Open shared mutex */
void open_mutex 			(int *mutex_fd, pthread_mutex_t **mutex) {

	*mutex_fd = shm_open("/shared_mutex.shm", O_RDWR, 0755);

	*mutex = (pthread_mutex_t*)mmap(0,                        // Address
                                	sizeof(pthread_mutex_t),  // Length
                                	PROT_READ | PROT_WRITE,   // Memory protection
                                	MAP_SHARED,               // Mapping flags
                                	*mutex_fd,          	  // Shared memory fd
                                	0);                       // Offset
}
#endif