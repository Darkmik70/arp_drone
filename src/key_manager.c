#include "key_manager.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>


int main() {

    // Initialize shared memory
    int sharedKey;
    int *sharedMemory;

    // Try to access the existing shared memory segment
    if ((sharedKey = shmget(SHM_KEY_1, sizeof(int), 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    if ((sharedMemory = shmat(sharedKey, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Initialize semaphore
    sem_t *semaphore = sem_open(SEM_KEY_1, O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }


    // Main loop
    while (1)
    {
        // Wait for the semaphore to be signaled
        sem_wait(semaphore);

        // Read the pressed key from shared memory
        int pressedKey = *sharedMemory;
        //printf("Pressed key: %c\n", (char)pressedKey);
        printf("Pressed key: %c\n", (char)pressedKey);

        // Determine the action based on the pressed key
        char *action = determineAction(pressedKey);

        // Send the action to the drone program
        sendActionToDrone(action);

        // Clear the shared memory after processing the key
        clearSharedMemory(sharedMemory);
        //usleep(500000);
    }


    // Detach the shared memory segment
    shmdt(sharedMemory);
    // Close and unlink the semaphore
    sem_close(semaphore);

    return 0;
}

// Function implementations

char* determineAction(int pressedKey)
{
    // Determine the action based on the pressed key
    switch (toupper((char)pressedKey))
    {
        case 'W':
            return "Up";
        case 'S':
            return "Down";
        case 'A':
            return "Left";
        case 'D':
            return "Right";
        case 'Q':
            return "Up-left";
        case 'E':
            return "Up-right";
        case 'Z':
            return "Down-left";
        case 'C':
            return "Down-right";
        case 'X':
            return "STOP";
        case 'R':
            return "Reset";
        case 'T':
            return "Suspend";
        case 'Y':
            return "Quit";
        default:
            return "None";
    }
}



void sendActionToDrone(char* action) 
{
    // Here you can implement the code to send the action to the drone program
    // For now, let's print the action to the standard output
    printf("Action sent to drone: %s\n\n", action);
    fflush(stdout);
}

void clearSharedMemory(int* sharedMemory)
{
    // Set the shared memory to a special value to indicate no key pressed
    *sharedMemory = NO_KEY_PRESSED;
}
