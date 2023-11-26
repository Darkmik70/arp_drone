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
/**
 * Create shared memory
 * @param name path to shared memory
 * @returns ptr to shared memory
*/
void *create_shm(char *name);

void handleInput(int *sharedKey, sem_t *semaphore);

#endif // SERVER_H
