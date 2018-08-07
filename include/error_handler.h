#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_

#include "user_interface.h"

#define CUSTOM_ERR_MAX -11

typedef enum {
   	CLI_CONN_ERR = 0,
   	MQTT_ERR,
   	SUB_ERR,
   	GENERAL_ERR
} ERROR_TYPE;

typedef enum {

	CLIENT_ID_CONFLICT = CUSTOM_ERR_MAX
} CUSTOM_MQTT_ERROR;


extern void handle_error(int error, int type, WINDOW** original_win);

#endif