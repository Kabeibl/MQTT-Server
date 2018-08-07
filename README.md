# MQTT-Server
A lightweight MQTT server written with C Posix utilizing ncurses.

All project files described are found under "mqtt/src" or "mqtt/include"
Generally all .h -files define structs and data types related to that header.
This is still a work in progress, and may or may not be developed further.

FILE CONTENTS:
server.c:
	- Contains main
	- Runs menus
	- Takes care of logging
	- Responsible for killing client processes gracefully on exit

user_interface.c
	- Responsible of displaying everything

signal_handler.h
	- Defines manual signals and signal related data structures

mqtt_client.c
	- Utilizes paho-mqtt-c
	- Responsible of creating child/client processes
	- Responsible of connecting/subscribing
	- Passes received messages back to main process

error_handler.c
	- Defines error types and handles errors (passes them to user_interface to be checked and printed)
	- Error handling in general is lacking and not quite ready