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
    // Initialize shared memory
    int sharedKey;
    int *sharedMemory;

    // Try to create a new shared memory segment
    if ((sharedKey = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    if ((sharedMemory = shmat(sharedKey, NULL, 0)) == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize semaphore
    sem_t *semaphore = sem_open(SEM_KEY, O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    // Main loop
    while (1) {
        // Wait for the semaphore to be signaled
        sem_wait(semaphore);

        // Read the pressed key from shared memory
        int pressedKey = *sharedMemory;
        printf("Server: Received key: %d\n", pressedKey);

        // TODO: Handle the received key or dispatch it to other processes

        // Clear the shared memory after processing the key
        *sharedMemory = NO_KEY_PRESSED;

        // TODO: Signal other processes or perform any necessary actions
        // ...

        usleep(500000);  // Adjust the delay as needed
    }

    // Detach the shared memory segment
    shmdt(sharedMemory);

    // Close and unlink the semaphore
    sem_close(semaphore);
    sem_unlink(SEM_KEY);

    return 0;
}
