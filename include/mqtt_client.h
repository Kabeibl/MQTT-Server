#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include <stdlib.h>
#include <ncurses.h>

typedef struct {
	char	address[32];
	char	client_id[32];
	char	topic[32];
	char	payload[128];
	int 	qos;
	int		timeout;
	int 	status_pipe;
	int 	msg_pipe;
	int 	client_number;
} CLIENT_INFO;

typedef enum {
	CONN_SUCCESS = 1,
	CONN_FAIL = 2
} CLIENT_CONN_STATUS;

extern pid_t 					server_pid;
extern CLIENT_CONN_STATUS 		client_conn_status;
extern void subscribe			(CLIENT_INFO *client_info);

#endif