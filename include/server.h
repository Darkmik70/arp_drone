#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>


// Function prototypes
// FIXME - why are these never used
void createSharedMemory();
void handleInput(int *sharedKey, sem_t *semaphore);

void *create_shm(char *name);

/**
 * Close, unlink all semaphores and shared memory segments;
*/
void clean_up();

#endif // SERVER_H
