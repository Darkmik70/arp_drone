#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

// Semaphore for key presses
sem_t *semaphore;
// Semaphore for drone positions
sem_t *semaphorePos;

// Function prototypes
// FIXME - why are these never used
void createSharedMemory();
void handleInput(int *sharedKey, sem_t *semaphore);

#endif // SERVER_H
