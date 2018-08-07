#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

#include <signal.h>

#define SIGCONNSUCCESS			40
#define SIGCONNFAIL				41
#define SIGSUBSUCCESS 			42
#define SIGSUBFAIL				43
#define SIGMESSAGE				44

typedef enum {
	RUNNING = 0,
	TERMINATED
} PROGRAM_STATUS;

#endif