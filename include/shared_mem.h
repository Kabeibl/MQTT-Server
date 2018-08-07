#ifndef SHARED_MEM_H_
#define SHARED_MEM_H_

#include <pthread.h>

void create_shared_mutex		(void);				// Create a shared mutex
void destroy_shared_mutex		(void);				// Destroy shared mutex

extern int                 		shared_mutex_fd;    // File descriptor for shared memory mutex
extern pthread_mutex_t     		*shared_mutex;      // Pointer to mute

#endif