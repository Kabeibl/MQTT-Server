#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

#include <menu.h>
#include "MQTTClient.h"
#include "mqtt_client.h"
#include "signal_handler.h"

typedef enum {
	SUBSCRIBE 	= 0,
	UNSUBSCRIBE,
	QUIT
} MAIN_MENU_FIELDS;

typedef enum {
	YES = 0,
	NO
} QUIT_MENU_FIELDS;

typedef enum {
	MAIN_MENU 	= 0,
	SUB_MENU,
	UNSUB_MENU,
	EXIT,
	CLIENTS_WINDOW,
	ERROR_WINDOW,
	MESSAGE_WINDOW
} MENU_TYPE;

typedef struct{
	int height; 
	int width;
	int startx; 
	int starty;
} POSITION;

typedef enum {
	ADDRESS 	= 1,
	CLIENTID 	= 3,
	TOPIC		= 5,
	QOS 		= 7,
	TIMEOUT 	= 9,
	DONE 		= 10 
} SUB_FIELDS;

#define ENTER 	10
#define ESC 	27

extern void init_screen			(void);
extern void init_main_menu		(MENU** menu, WINDOW** win);
extern void init_msg_win 		(WINDOW** win, WINDOW** frame);
extern void init_client_win 	(WINDOW** win, WINDOW** frame);

extern int drive_main_menu		(MENU** menu, WINDOW** win);
extern int drive_sub_menu		(CLIENT_INFO* client_info);
extern int drive_unsub_menu	 	(CLIENT_INFO* clients, int n_clients);

extern int wait_for_connection	(void);
extern int prompt_exit			(void);

extern void destroy_win 		(WINDOW** win);
extern void close_menu			(MENU** menu);

extern void print_clients 		(CLIENT_INFO *clients, int n, WINDOW **win);
extern void print_mqtt_message	(CLIENT_INFO *client, WINDOW *win);
extern void print_error 		(int error, int type, WINDOW** original_win);
extern void print_header 		(void);
extern void print_footer		(void);

#if 0
extern void wrefresh_safe 		(WINDOW* win);
extern void refresh_safe 		(void);
extern void wprintw_safe 		(WINDOW* win, char *str);
extern void redrawwin_safe 		(WINDOW* win);
#endif

#endif