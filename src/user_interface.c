
#include "user_interface.h"
#include "shared_mem.h"
#include "error_handler.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <form.h>
#include <time.h>
#include <stdio.h>

/* GLOBAL VARIABLES */

/* PROTOTYPES */
void init_screen		(void);

void init_items			(ITEM*** items, char** options, int n_options);

void init_form			(FORM** form, WINDOW** win, FIELD*** field, MENU_TYPE type);
void init_sub_fields	(FIELD*** field);
void init_unsub_fields	(FIELD*** field, CLIENT_INFO* clients, int n_clients);

void print_border		(WINDOW* win, int width, MENU_TYPE menu_type);
void print_center		(WINDOW* win, int starty, int startx, int width, char *string, chtype color);
void set_pos 			(POSITION *pos, int height, int width, int startx, int starty);
void destroy_win 		(WINDOW** win);
char* trim 				(char* str);

void next_field 		(FORM* form, MENU_TYPE type);
void prev_field 		(FORM* form, MENU_TYPE type);
void highlight_on 		(FORM* form, MENU_TYPE type);
void highlight_off 		(FORM* form, MENU_TYPE type);

/* Initialise curses */
void init_screen		(void) {

	initscr();									// Start curses mode
	start_color();								// Enable colors
	cbreak();									// Disable buffering, retain signals
	noecho();									// Disable input echoing
	curs_set(0);								// Set cursor invisible
	keypad(stdscr, TRUE);						// Enable function keys
	set_escdelay(0);							// Disable esc-delay

	init_pair(1, COLOR_GREEN, COLOR_BLACK);		// Create color pair
	init_pair(2, COLOR_BLUE, COLOR_BLACK);		// Create color pair
	init_pair(3, COLOR_YELLOW, COLOR_RED);		// Create color pair
	init_pair(4, COLOR_BLACK, COLOR_GREEN);		// Create color pair
}

/* Initialise main menu */
void init_main_menu		(MENU** menu, WINDOW** win) {

	/* Declare variables*/
	ITEM 			**items 	= {0};		// Items -array
	POSITION 		pos 		= {0};		// Position -struct
	int 			n_options	= 3;		// Number of options

	/* Set menu options */
	char 			*options[3] = 		  {"[S]ubscribe",
								   		   "[U]nsubscribe",
								   		   "[Q]uit"};

	init_items(&items, options, n_options);	// Initialise menu items

	/* Set position */
	set_pos(&pos,							// Position -struct
			LINES / 3,						// Height
			COLS / 5,						// Width
			1,								// X-origo
			3);								// Y-origo

	*menu = new_menu(items);				// Create menu

	*win = newwin(pos.height, 				// Create window
				  pos.width,				//
				  pos.starty,				//
				  pos.startx);				//

	keypad(*win, TRUE);						// Enable keypad
	set_menu_mark(*menu, "");				// Disable menu mark

	set_menu_win(*menu, *win);				// Set main-window
	set_menu_sub(*menu, derwin(*win,		// Set sub-window
						pos.height - 4,		//
						pos.width - 2,		//
						3, 1));				//

	post_menu(*menu);						// Post menu to window
	print_header();							// Print header
	print_footer();							// Print footer							
	print_border(*win, 						// Print border to window
				pos.width, 					//
				MAIN_MENU);					//

	wrefresh(*win);					// Display window
}

/* Initialise main menu items*/
void init_items			(ITEM*** items, char **options, int n_options) {

	*items = (ITEM**)calloc(n_options + 1, 		// Allocate memory for menu items
							sizeof(ITEM *));	//

	/* Create menu items */
	for(int i = 0; i < n_options; ++i) {
		(*items)[i] = new_item(options[i], 		// Create item
							   NULL);			//
	}

	(*items)[n_options] = (ITEM *)NULL;			// Set the last item to NULL
}

/* Initialise message window */
void init_msg_win 		(WINDOW** win, WINDOW** frame) {

	/* Declare variables */
	POSITION 		pos 		= {0};		// Position -struct

	/* Set position */
	set_pos(&pos,							// Position -struct
			LINES - 6,						// Height
			(COLS / 5)*4 - 2,				// Width
			COLS / 5 + 1,					// X-origo
			3);								// Y-origo

	/* Create frame */
	*frame = newwin(pos.height, 			// Window height
				 pos.width,					// Window width
				 pos.starty,				// Window y-origo
				 pos.startx);				// Window x-origo

	/* Print border to window */
	print_border(*frame, 					// Window
				pos.width, 					// Width
				MESSAGE_WINDOW);			// Type


	/* Create window */
	*win = derwin(*frame,
				  pos.height - 4,			// Sub-window height
				  pos.width - 2,			// Sub-window width
				  3, 						// Sub-window y-origo
				  1);						// Sub-window x-origo

	scrollok(*win ,1);						// Enable scrolling on window

	wrefresh(*win);					// Refresh window
	wrefresh(*frame);					// Refresh frame
}

/* Initialise clients window */
void init_client_win 	(WINDOW** win, WINDOW** frame) {

	/* Declare variables */
	POSITION 		pos 		= {0};		// Position -struct

	/* Set position */
	set_pos(&pos,							// Position -struct
			(LINES / 3) * 2 - 6,			// Height
			(COLS / 5),						// Width
			1,					 			// X-origo
			(LINES / 3) + 3);				// Y-origo

	/* Create frame */
	*frame = newwin(pos.height, 			// Window height
				 pos.width,					// Window width
				 pos.starty,				// Window y-origo
				 pos.startx);				// Window x-origo

	/* Print border to window */
	print_border(*frame, 					// Window
				pos.width, 					// Width
				CLIENTS_WINDOW);			// Type


	/* Create window */
	*win = derwin(*frame,
				  pos.height - 4,			// Sub-window height
				  pos.width - 2,			// Sub-window width
				  3, 						// Sub-window y-origo
				  1);						// Sub-window x-origo

	scrollok(*win ,1);						// Enable scrolling on window

	wrefresh(*win);					// Refresh window
	wrefresh(*frame);					// Refresh frame
}

/* Initialise subscription form */
void init_form			(FORM** form, WINDOW** win, FIELD*** field, MENU_TYPE type) {

	/* Declare variables */
	int 				height = 0,			// Window height
						width  = 0;			// Window width
	POSITION 			form_pos;			// Position settings

	*form = new_form(*field);				// Create form

	/* Get form size */
	scale_form(*form, 						// Form
			   &(height),					// Height
			   &(width));					// Width

	/* Set position */
	set_pos(&form_pos,						// Form
			height,							// Height
			width,							// Width
			(COLS - width) / 2,				// X-origo
			(LINES - height) / 2);			// Y-origo

	/* Create form window */
	*win = newwin(form_pos.height + 4, 		// Window height
				  form_pos.width + 2,		// Window width
				  form_pos.starty,			// Y-origo
				  form_pos.startx);			// X-origo

	keypad(*win, TRUE);						// Enable keypad for form-windows
	set_form_win(*form, *win);				// Set main-window

	/* Set sub-window */
	set_form_sub(*form, derwin(*win,		// Window
						form_pos.height,	// Sub-window height
						form_pos.width,		// Sub-window width
						3, 					// Y-offset from main window
						1));				// X-offset from main window

	/* Print borders */
	print_border(*win, 						// Window
				 form_pos.width + 2,		// Width
				 type);						// Menu type

	post_form(*form);						// Post form on window
	set_current_field(*form, *field[0]);	// Set first field as active
	highlight_on(*form, SUB_MENU);			// Highlight current field
	wrefresh(*win);					// Refresh window
}

/* Initialise subscription form fields */
void init_sub_fields	(FIELD*** field) {

		int 		n_fields = 12;
		char 		*option[5] = {
					"  Address:",
					"Client ID:",
					"    Topic:",
					"      QoS:", 
					"  Timeout:"
					};

		*field = (FIELD**)calloc(n_fields + 1, 				// Allocate memory for the fields
								 sizeof(FIELD*));			//
		
		for(int i = 0; i < n_fields - 2; i++) {

			if((i%2) == 0) {

				/* Set field size and position */
				(*field)[i] = new_field(1,					// Height
										12,					// Width
										i+1,				// Row
										0,					// Column
										0,					// Off-screen rows
										0);					// Additional buffers	

				/* Set field buffers */
				set_field_buffer((*field)[i],				// Field index
								 0, 						// Buffer index
								 option[i-(i/2)]);			// Buffer value

				/* Set field attributes */
				set_field_back((*field)[i], COLOR_PAIR(1));	// Set background color
			
				/* Set field options */
				set_field_opts((*field)[i], O_VISIBLE |		// Visible
											O_PUBLIC |		// Plain text
											O_AUTOSKIP);	// Auto-skip field
			}
			
			else {
				/* Set field size and position */
				(*field)[i] = new_field(1,					// Height
										40,					// Width
										i,					// Row
										12,					// Column
										0,					// Off-screen rows
										0);					// Additional buffers	

				set_field_back((*field)[i], COLOR_PAIR(1) |	// Set background color
											A_UNDERLINE);	// Underline field
			}	
		}

		/* QoS field settings */
		set_field_type((*field)[7],							// QoS field
					   TYPE_INTEGER,						// Set field type to integer
					   0,									// Disable zero-padding
					   0, 2);								// Set valid range

		/* Timeout field settings */
		set_field_type((*field)[9],							// Timeout field
					   TYPE_INTEGER,						// Set field type to integer
					   0,									// Disable zero-padding
					   0, 100000);							// Set valid range

		/* Create "DONE" -field */
		(*field)[n_fields - 2] = new_field(1,				// Height
											52/4,			// Width
											n_fields - 1,	// Row
											(52 - 52/4)/2,	// Column
											0,				// Off-screen rows
											0);				// Additional buffers

		/* Set field buffer */
		set_field_buffer((*field)[n_fields - 2],			// Field index
						 0, 								// Buffer index
						 "DONE");							// Buffer value
		
		/* Set field justification */
		set_field_just((*field)[n_fields - 2],				// Field to justify
					   JUSTIFY_CENTER);						// Justification mode

		/* Set field color */
		set_field_back((*field)[n_fields - 2],				// Field
					   COLOR_PAIR(1) |						// Set background color
					   A_STANDOUT |							// Enable highlight
					   A_DIM);								// Dim

		/* Disable field options */
		field_opts_off((*field)[n_fields - 2],				// Field
					   O_EDIT);								// Disable editing

		(*field)[n_fields - 1] = NULL;						// Set last field to NULL
}

/* Initialise unsubscription form fields */
void init_unsub_fields	(FIELD*** field, CLIENT_INFO* clients, int n_clients) {

	/* Declare variables */
	char 				*options[n_clients];
	int 				width, max_width;

	/* Set form options */
	for(int i = 0; i < n_clients; i++) {

		/* Set field width */
		width = strlen(clients[i].client_id) + 13;

		/* Check max width */
		if(max_width < width) max_width = width;
		
		/* Allocate mem for array items */
		options[i] = malloc(sizeof(char) * width);

		/* Format option */
		sprintf(options[i],							// Option
				"[CLIENT ID] %s",  					// String
				clients[i].client_id); 				// Source
	}

	*field = (FIELD**)calloc(n_clients + 1, 		// Allocate memory for the fields
							 sizeof(FIELD*));		//
	
	for(int i = 0; i < n_clients; i++) {

		/* Set field size and position */
		(*field)[i] = new_field(1,					// Height
								max_width,			// Width
								i,					// Row
								0,					// Column
								0,					// Off-screen rows
								0);					// Additional buffers	

		/* Set field buffers */
		set_field_buffer((*field)[i],				// Field index
						 0, 						// Buffer index
						 options[i]);				// Buffer value

		/* Set field attributes */
		set_field_back((*field)[i], COLOR_PAIR(1));	// Set background color

		/* Set field justification */
		set_field_just((*field)[i],					// Field to justify
					   JUSTIFY_CENTER);				// Justification mode
	}

	(*field)[n_clients] = NULL;						// Set last field to NULL

	for(int i = 0; i < n_clients; i++) {
		free(options[i]);
	}
}

/* Drive main menu */
int drive_main_menu		(MENU** menu, WINDOW** win) {

	/* Declare variables */
	int c;												// Character -buffer

	redrawwin(*win);	                      			// Re-draw main menu
	wrefresh(*win);										// Refresh window

	do {
		c = getch();									// Get user input
		switch(c) {
			case KEY_DOWN:
				menu_driver(*menu, REQ_DOWN_ITEM);		// Move cursor down
				break;
			case KEY_UP:
				menu_driver(*menu, REQ_UP_ITEM);		// Move cursor up
				break;
			case ENTER:
				return item_index(current_item(*menu));	// Return item index
				break;
			case 'S':
			case 's':
				return SUBSCRIBE;
				break;
			case 'U':
			case 'u':
				return UNSUBSCRIBE;
				break;
			case 'Q':
			case 'q':
				return QUIT;
				break;
		}
		redrawwin(*win);								// Redraw window
		wrefresh(*win);									// Refresh window
	} while(c != 'e' && c != 'E');						// Exit with 'e' or 'E'

	return QUIT;
}

/* Drive subscription menu */
int drive_sub_menu		(CLIENT_INFO* client_info) {

	/* Declare variables */
	WINDOW*			win 		= {0};	// Form window
	FORM 			*form 		= {0};	// Form
	FIELD 			**field 	= {0},	// Fields -array
					*cur_field 	= {0};	// Current field
	int 			c			= 0,	// Character -buffer
		 			index		= 0;	// Switch-case index

    init_sub_fields(&field); 				
	init_form(&form, &win, &field, SUB_MENU);

	do {
		c = wgetch(win);
		switch(c) {

			case KEY_DOWN:
				next_field(form, SUB_MENU); 		// Move to next field
				break;

			case KEY_UP:
				prev_field(form, SUB_MENU);			// Move to previous field
				break;

			case KEY_BACKSPACE:
				form_driver(form, REQ_DEL_PREV);	// Delete previous character
				break;

			case KEY_DC:
				form_driver(form, REQ_DEL_CHAR);	// Delete current character
				break;

			case ENTER:
				cur_field = current_field(form);	// Get current field
				index = field_index(cur_field);		// Get current field index
				next_field(form, SUB_MENU); 		// Move to next field

				switch(index) {

					case ADDRESS:
						/* Set address */
						strcpy(client_info->address, 
								trim(field_buffer(cur_field, 0)));
						break;

					case CLIENTID:
						/* Set client ID */
						strcpy(client_info->client_id, 
								trim(field_buffer(cur_field, 0)));
						break;

					case TOPIC:
						/* Set topic */
						strcpy(client_info->topic, 
								trim(field_buffer(cur_field, 0)));
						break;

					case QOS:
						/* Set quality of service level */
						client_info->qos = ((int)field_buffer(cur_field, 0)[0]) - '0';
						break;

					case TIMEOUT:
						/* Set connection timeout */
						client_info->timeout = atoi(field_buffer(cur_field, 0));
						break;

					case DONE:

						unpost_form(form);				// Remove form from screen
						free_form(form);				// Deallocate memory from form
						
						for(int i = 0; i < field_count(form); i++) {
							free(field[i]);				// Deallocate memory from fields
						}

						destroy_win(&win);				// Destroy window
						return DONE;
						break;

					default:
						break;
				}

				
				break;

			case ESC:

				unpost_form(form);				// Remove form from screen
				free_form(form);				// Deallocate memory from form
				
				for(int i = 0; i < field_count(form); i++) {
					free(field[i]);				// Deallocate memory from fields
				}

				destroy_win(&win);				// Destroy window
				return -1;
				break;

			default:
				form_driver(form, c); 			// Print character
				break;
		}
		wrefresh(win);
	} while(1);

	return QUIT;	
}

/* Drive unsubscription menu */
int drive_unsub_menu	(CLIENT_INFO* clients, int n_clients) {

	/* Declare variables */
	WINDOW*			win 		= {0};			// Form window
	FORM 			*form 		= {0};			// Form
	FIELD 			**field 	= {0},			// Fields -array
					*cur_field 	= {0};			// Current field
	int 			c			= 0,			// Character -buffer
					index		= 0;			// Switch-case index

	init_unsub_fields(&field, clients, n_clients);
	init_form(&form, &win, &field, UNSUB_MENU);

	do {
		c = wgetch(win);
		switch(c) {

			case KEY_DOWN:
				next_field(form, UNSUB_MENU); 		// Move to next field
				break;

			case KEY_UP:
				prev_field(form, UNSUB_MENU);		// Move to previous field
				break;

			case ENTER:
				cur_field = current_field(form);	// Get current field
				index = field_index(cur_field);		// Get current field index
				next_field(form, UNSUB_MENU); 		// Move to next field

				return index; 						// Return client number
				break;

			case ESC:

				unpost_form(form);				// Remove form from screen
				free_form(form);				// Deallocate memory from form
				
				for(int i = 0; i < field_count(form); i++) {
					free(field[i]);				// Deallocate memory from fields
				}

				destroy_win(&win);				// Destroy window
				return -1;
				break;

			default:
				break;
		}
		wrefresh(win);
	} while(1);

	return -1;	
}

/* Display connection window */
int wait_for_connection	(void) {

	/* Declare variables */
	WINDOW 				*win; 
	POSITION 			pos;
	int 				i 						= 0;
	char 				success[]				= "Connected.";
	char 				fail[] 					= "Connection timed out.";
	char 				*msg[] 					= {
												"Connecting ...",
												"Connecting .  ",
												"Connecting .. "
												};

	/* Set position */
	set_pos(&pos,							// Position -struct
			5,								// Height
			40,								// Width
			(COLS - 40) / 2,				// X-origo
			(LINES - 5) / 2);				// Y-origo

	/* Create a new window */
	win = newwin(pos.height, 				// Window height
				 pos.width,					// Window width
				 pos.starty,				// Window y-origo
				 pos.startx);				// Window x-origo

	box(win, 0, 0);							// Create borders
	wrefresh(win);						// Display window

	/* Wait for connection */
	while(client_conn_status == 0) {

		/* Print message */
		mvwprintw(win,						// Window
				 2,							// Y-coordinate
				 12,						// X-coordinate
				 "%s", msg[i]);				// Message

		wrefresh(win);					// Refresh screen
		sleep(1);							// Sleep for 1 second
		if(++i > 2) i = 0;					// Increment/reset counter
	}

	/* Check subscription status */
	switch(client_conn_status) {

		case CONN_SUCCESS:

			/* Print status */
			redrawwin(win);			// Redraw window

			mvwprintw(win,					// Window
					 2,						// Y-coordinate
					 12,					// X-coordinate
				 	 "%s", success);		// Status message

			wrefresh(win);				// Refresh screen

			sleep(1);						// Sleep for 2 seconds

			client_conn_status = 0;			// Reset sub-conn status flag
			destroy_win(&win);				// Destroy window

			return CONN_SUCCESS;
			break;

		case CONN_FAIL:

			/* Print status */
			redrawwin(win);			// Redraw window

			pthread_mutex_lock(shared_mutex);
			mvwprintw(win,					// Window
					 2,						// Y-coordinate
					 9,						// X-coordinate
				 	 "%s", fail);			// Status message
			pthread_mutex_unlock(shared_mutex);

			wrefresh(win);				// Refresh screen

			sleep(2);						// Sleep for 2 seconds

			client_conn_status = 0;			// Reset sub-conn status flag
			destroy_win(&win);				// Destroy window

			return CONN_FAIL;
			break;
	}

	return -1;
}

/*	 Prompt exit */
int prompt_exit			(void) {

	/* Declare variables */
	MENU 				*menu 			= NULL;
	ITEM 				**items 		= NULL;
	WINDOW 				*win 			= NULL;
	POSITION 			pos 			= {0};

	int 				c,
		  				index,
						n_options 		= 2;
	char 				*options[2] 	= {"     [Y]es     ",
										   "     [N]o      "};

	init_items(&items, options, n_options);

	/* Set position */
	set_pos(&pos,							// Position -struct
			5,								// Height
			32,								// Width
			(COLS/2)-(32/2),				// X-origo
			(LINES/2) - 5);					// Y-origo

	menu = new_menu(items);					// Create menu

	/* Create a new window */
	win = newwin(pos.height, 				// Window height
				 pos.width,					// Window width
				 pos.starty,				// Window y-origo
				 pos.startx);				// Window x-origo

	keypad(win, TRUE);						// Enable keypad
	set_menu_mark(menu, "");				// Disable menu mark

	set_menu_win(menu, win);				// Set main-window
	set_menu_sub(menu, derwin(win,			// Set sub-window
						pos.height - 4,		// Height
						pos.width - 2,		// Width
						3, 					// Y-origo
						1));				// X-origo

	set_menu_format(menu,					// Menu
					1,						// Rows
					2);						// Columns

	post_menu(menu);						// Post menu to window
	print_border(win, 						// Print border to window
				pos.width, 					//
				EXIT);						//

	wrefresh(win);						// Display window


	do {
		c = wgetch(win);

		switch(c) {

			case KEY_LEFT:
				menu_driver(menu, REQ_LEFT_ITEM);			// Move left
				break;

			case KEY_RIGHT:
				menu_driver(menu, REQ_RIGHT_ITEM);			// Move right
				break;

			case ENTER:
				index = item_index(current_item(menu));		// Get index
				close_menu(&menu);
				destroy_win(&win);							// Destroy window

				switch(index) {
					case YES:
						return 1;
						break;
					case NO:
						return 0;
						break;
				}
				break;

			case 'Y':
			case 'y':
				return 1;
				break;

			default:
			case 'N':
			case 'n':
				return 0;
				break;
		}
	} while(1);
	return 0;
}

/* Close main menu */
void close_menu			(MENU** menu) {

	
	ITEM** items = menu_items(*menu);	// Get items
	int n_items = item_count(*menu);	// Get amount of items

	unpost_menu(*menu);					// Erase menu from subwindow
	free_menu(*menu);					// Deallocate mem from menu

	for(int i = 0; i < n_items; i++) {
		free_item(items[i]);			// Deallocate mem from items
	}
}

/* Destroy window */
void destroy_win		(WINDOW **win) {

	werase(*win);										// Clear window
	wrefresh(*win);								// Refresh window
	delwin(*win);										// Delete window
}

/* Print clients */
void print_clients 		(CLIENT_INFO *clients, int n, WINDOW **win) {

	werase(*win);										// Clear window
	//wrefresh(*win);										// Refresh window


	for(int i = 0; i < n; i++) {
		wprintw(*win, "[CLIENT ID] <%s>\n",
				clients[i].client_id);
	}

	wrefresh(*win);										// Refresh window
}

/* Print MQTT message */
void print_mqtt_message	(CLIENT_INFO *client, WINDOW *win) {

	/* Print message */
	//wprintw(win, "MESSAGE ARRIVED!");
    wprintw(win, "%s\n", client->payload);
    wrefresh(win);
}

/* Print header */
void print_header 		(void) 	{

	char header[] = "MQTT SERVER v0.1";				// Header

	mvhline(2, 1, ACS_HLINE, COLS);					// Print header line
	mvprintw(1, (COLS - strlen(header))/2, header); // Print header
}

/* Print footer */
void print_footer		(void) {

	char *footer_item[] = {"ARROW UP - Move up",
						  "| ARROW DOWN - Move down",
						  "| ENTER - Select option"};

	mvhline(LINES - 3, 1, ACS_HLINE, COLS);			// Print footer line

	/* Print footer items */
	mvprintw(LINES - 2, 2, footer_item[0]);
	mvprintw(LINES - 2, 
			 3 + strlen(footer_item[0]), 
			   footer_item[1]);
	mvprintw(LINES - 2, 
			 4 + strlen(footer_item[0]) 
			   + strlen(footer_item[1]), 
		 	   footer_item[2]);
}

/* Print border */
void print_border		(WINDOW* win, int width, MENU_TYPE menu_type) {
	box(win, 0, 0);

	switch(menu_type) {
		case MAIN_MENU:
			print_center(win, 1, 0, width, "MAIN MENU", COLOR_PAIR(1));
			break;
		case SUB_MENU:
			print_center(win, 1, 0, width, "SUBSCRIBE", COLOR_PAIR(1));
			break;
		case UNSUB_MENU:
			print_center(win, 1, 0, width, "UNSUBSCRIBE", COLOR_PAIR(1));
			break;
		case EXIT:
			print_center(win, 1, 0, width, "QUIT?", COLOR_PAIR(1));
			break;
		case CLIENTS_WINDOW:
			print_center(win, 1, 0, width, "CONNECTED CLIENTS", COLOR_PAIR(1));
			break;
		case ERROR_WINDOW:
			print_center(win, 1, 0, width, "ERROR", COLOR_PAIR(3));
			break;
		case MESSAGE_WINDOW:
			print_center(win, 1, 0, width, "MESSAGES", COLOR_PAIR(2));
			break;
	}

	mvwaddch(win, 2, 0, ACS_LTEE);
	mvwhline(win, 2, 1, ACS_HLINE, width - 2);
	mvwaddch(win, 2, width - 1, ACS_RTEE);
	wrefresh(win);
}

/* Print to center of the screen */
void print_center		(WINDOW *win, int starty, int startx, int width, char *string, chtype color) {	
	int length, x, y;
	float temp;

	getyx(win, y, x);

	if(win == NULL) win = stdscr;
	if(startx != 0) x = startx;
	if(starty != 0) y = starty;
	if(width == 0) width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

/* Print error message */
void print_error 		(int error, int type, WINDOW** original_win) {

	/* Declare variables */
	ERROR_TYPE 				e_type 			= type;
	WINDOW 					*win 			= NULL;
	POSITION 				pos 			= {0};
	MENU 					*menu 			= NULL;
	ITEM 					**items 		= NULL;
	int 					c,
							width,
							n_options 		= 1;
	char 					*options[1] 	= {"      OK      "};
	char 					err_buf[64]		= {0};

    /* Check error type */
    switch(e_type) {
        case CLI_CONN_ERR:
            switch(error) {
            case 1:
            	sprintf(err_buf, "Connection refused: Unacceptable protocol version");
                break;
            case 2:
                sprintf(err_buf, "Connection refused: Identifier rejected");
                break;
            case 3:
                sprintf(err_buf, "Connection refused: Server unavailable");
                break;
            case 4:
                sprintf(err_buf, "Connection refused: Bad user name or password");
                break;
            case 5:
                sprintf(err_buf, "Connection refused: Not authorized");
                break;
            default:
                sprintf(err_buf, "Connection refused: Undefined error");
        	}
        	break;

        case SUB_ERR:
        	switch(error) {
        		case MQTTCLIENT_BAD_QOS:
        		    sprintf(err_buf, "Subscription failed: Invalid QoS");
        			break;
        		case CLIENT_ID_CONFLICT:
        			sprintf(err_buf, "Subscription failed: ID already in use");
        			break;
        		default:
                	sprintf(err_buf, "Subscription failed: Undefined error");
                	break;
        	}
        	break;

        case MQTT_ERR:
        	switch(error) {
        		case MQTTCLIENT_FAILURE:
        			break;
        		default:
        			sprintf(err_buf, "Error: Undefined error");
        			break;
        	}
            break;

        case GENERAL_ERR:
        	break;
    }

    /* Set position */
    width = strlen(err_buf) + 4;
	set_pos(&pos,							// Position -struct
			8,								// Height
			width,							// Width
			(COLS/2)-(width/2),				// X-origo
			(LINES/2) - 5);					// Y-origo

	init_items(&items, options, n_options);	// Initialise menu item(s)

	menu = new_menu(items);					// Create menu

	/* Create a new window */
	win = newwin(pos.height, 				// Window height
				 pos.width,					// Window width
				 pos.starty,				// Window y-origo
				 pos.startx);				// Window x-origo

	keypad(win, TRUE);						// Enable keypad
	set_menu_mark(menu, "");				// Disable menu mark

	set_menu_win(menu, win);				// Set main-window
	set_menu_sub(menu, derwin(win,			// Set sub-window
					1,						// Height
					pos.width - 2,			// Width
					6, 						// Y-origo
					1));					// X-origo

	wattron(win, COLOR_PAIR(3));
	wbkgd(win, COLOR_PAIR(3));

	set_menu_format(menu,					// Menu
					1,						// Rows
					2);						// Columns

	post_menu(menu);						// Post menu to window
	print_border(win, 						// Print border to window
				pos.width, 					//
				ERROR_WINDOW);				//

	mvwprintw(win, 4, 1, err_buf); 			// Print error
	wrefresh(win);							// Display window

	/* Wait for user to press enter */
    while(1) {
		c = wgetch(win);

		if(c == ENTER) {

			close_menu(&menu);
			destroy_win(&win);
			wbkgd(win, COLOR_PAIR(1));
			redrawwin(*original_win);
			return;
		}
	}
}

/* Set menu position settings */
void set_pos 			(POSITION *pos, int height, int width, int startx, int starty) {

	(*pos).height = height;
	(*pos).width = width;
	(*pos).startx = startx;
	(*pos).starty = starty;
}

/* Trim whitespaces from string */
char* trim 				(char* str) {

	char* 			end;				// End of string

	/* Trim leading spaces */
	while(isspace(*str)) {
		str++;							// Move index of first element forward
	}

	/* Return if string is all spaces */
	if(*str == 0) {
		return str;
	} 		

	end = str + strnlen(str, 128) - 1;	// Set end to point to the end of the given string

	/* Trim trailing spaces */
	while(end > str && isspace(*end)) {
		end--;							//
	}

	*(end+1) = '\0';					// Set null terminator

	return str;
}

#if 0
/* Refresh window with mutex */
void wrefresh_safe 		(WINDOW* win) {

	pthread_mutex_lock(shared_mutex);			// Lock mutex
	wrefresh(win);								// Refresh window
	pthread_mutex_unlock(shared_mutex);			// Unlock mutex
}

/* Refresh screen with mutex */
void refresh_safe 		(void) {

	pthread_mutex_lock(shared_mutex);			// Lock mutex
	refresh();									// Refresh window
	pthread_mutex_unlock(shared_mutex);			// Unlock mutex
}

/* Print with mutex */
void wprintw_safe 		(WINDOW* win, char *str) {

	pthread_mutex_lock(shared_mutex);			// Lock mutex
	wprintw(win, str);							// Refresh window
	pthread_mutex_unlock(shared_mutex);			// Unlock mutex
}

/* Redraw window with mutex */
void redrawwin_safe 	(WINDOW* win) {

	pthread_mutex_lock(shared_mutex);			// Lock mutex
	redrawwin(win);								// Refresh window
	pthread_mutex_unlock(shared_mutex);			// Unlock mutex
}
#endif

/* Move to next form field */
void next_field 		(FORM* form, MENU_TYPE type) {

	highlight_off(form, type);					// Disable highlight on current field
	form_driver(form, REQ_NEXT_FIELD);			// Go to next field
	//form_driver(form, REQ_END_LINE);			// Go to end of buffer
	highlight_on(form, type);					// Enable highlight on current field
}

/* Move to previous form field */
void prev_field 		(FORM* form, MENU_TYPE type) {

	highlight_off(form, type);					// Disable highlight on current field
	form_driver(form, REQ_PREV_FIELD);			// Go to next field
	//form_driver(form, REQ_END_LINE);			// Go to end of buffer;
	highlight_on(form, type);					// Enable highlight on current field
}

/* Enable highlight on form field */
void highlight_on 		(FORM* form, MENU_TYPE type) {

	FIELD *cur_field;

	switch(type) {

		case SUB_MENU:
			cur_field = current_field(form);			// Get current field

			if(field_index(cur_field) == DONE) {
				set_field_back(cur_field,				// Field
							   COLOR_PAIR(1) |			// Set background color
							   A_STANDOUT);				// Enable highlight
			}
			
			else {
				set_field_back(cur_field, COLOR_PAIR(1) |	// Set background color
									  A_UNDERLINE |		// Underline field
									  A_STANDOUT);		// Enable highlightd
			}
			break;

		case UNSUB_MENU:
			cur_field = current_field(form);			// Get current field
			set_field_back(cur_field, COLOR_PAIR(1) |	// Set background color
								  A_STANDOUT);			// Enable highlight
			break;

		default:
			break;
	}
}

/* Disable highlight on form field */
void highlight_off 		(FORM* form, MENU_TYPE type) {

	FIELD *cur_field;

	switch(type) {

		case SUB_MENU:
			cur_field = current_field(form);			// Get current field

			if(field_index(cur_field) == DONE) {
				set_field_back(cur_field,				// Field
							   COLOR_PAIR(1) |			// Set background color
							   A_STANDOUT |				// Enable highlight
							   A_DIM);					// Dim
			}

			else {
				set_field_back(cur_field, COLOR_PAIR(1) |	// Set background color
									  A_UNDERLINE | 	// Underline field
									  A_NORMAL);		// Disable highlight
			}
			break;

		case UNSUB_MENU:
			cur_field = current_field(form);			// Get current field
			set_field_back(cur_field, COLOR_PAIR(1));	// Set background color
			break;

		default:
			break;
	}
}