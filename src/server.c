#include "server.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>


int main() {
    // Attach to shared memory for key presses
    int sharedKey;
    int *sharedMemory;

    if ((sharedKey = shmget(SHM_KEY_1, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((sharedMemory = shmat(sharedKey, NULL, 0)) == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    // Attach to semaphore for key presses
    semaphore = sem_open(SEM_KEY_1, O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Attach to shared memory for drone positions
    int sharedPos;
    int *sharedPosition;

    if ((sharedPos = shmget(SHM_KEY_2, 2 * sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((sharedPosition = shmat(sharedPos, NULL, 0)) == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    // Attach to semaphore for drone positions
    semaphorePos = sem_open(SEM_KEY_2, O_CREAT, 0666, 0);
    if (semaphorePos == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Main loop
    while (1) {
        /* SERVER HANDLING OF KEY PRESSED */
        sem_wait(semaphore);
        int pressedKey = *sharedMemory;
        printf("Server: Received key: %d\n", pressedKey);

        // TODO: Handle the received key or dispatch it to other processes
        // Here goes RESET
        // Here goes SUSPEND
        // Here goes QUIT

        // Clear the shared memory after processing the key
        *sharedMemory = NO_KEY_PRESSED;

        /*SERVER HANDLING OF DRONE POSITION*/
        sem_wait(semaphorePos);
        int droneX = sharedPosition[0];
        int droneY = sharedPosition[1];
        sem_post(semaphorePos);

        // TODO: Logic for handling drone positions...

        usleep(50000);  // Delay for testing purposes
    }

    // Detach the shared memory segments
    shmdt(sharedMemory);
    shmdt(sharedPosition);

    // Close and unlink the semaphores
    sem_close(semaphore);
    sem_close(semaphorePos);
    sem_unlink(SEM_KEY_1);
    sem_unlink(SEM_KEY_2);

    return 0;
}
