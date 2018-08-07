#include "shared_mem.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int                 		shared_mutex_fd;    // File descriptor for shared memory mutex
pthread_mutex_t     		*shared_mutex;      // Pointer to mute

/* Create shared memory mutex */
void create_shared_mutex           (void) {

    pthread_mutexattr_t     mutex_attr;

    /* Create shared memory object */
    shared_mutex_fd = shm_open("/shared_mutex.shm",
                      O_CREAT | O_RDWR, 0755);

    ftruncate(shared_mutex_fd, sizeof(pthread_mutex_t));            // Truncate to correct length

    shared_mutex = (pthread_mutex_t*)mmap(0,                        // Address
                                          sizeof(pthread_mutex_t),  // Length
                                          PROT_READ | PROT_WRITE,   // Memory protection
                                          MAP_SHARED,               // Mapping flags
                                          shared_mutex_fd,          // Shared memory fd
                                          0);                       // Offset

    /* Set mutex attribute to shared mode */
    pthread_mutexattr_setpshared(&mutex_attr, 
                                PTHREAD_PROCESS_SHARED);

    /* Initialize mutex */
    pthread_mutex_init(shared_mutex, &mutex_attr);
} 

/* Destroy shared mutex */
void destroy_shared_mutex 	(void) {

	pthread_mutex_destroy(shared_mutex);    // Destroy mutex
    close(shared_mutex_fd);                 // Close shared memory fd
    shm_unlink("/shared_mutex.shm");        // Release shared memory object
}