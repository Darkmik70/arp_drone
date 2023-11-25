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


int main() 
{
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


// US Keyboard assumed
char* determineAction(int pressedKey)
{
    char key = toupper(pressedKey);

    if ( key == 'W' || key == 'I')
    {
        return "Up";
    }
    if ( key == 'X' || key == ',')
    {
        return "Down";
    }
    if ( key == 'A' || key == 'J')
    {
        return "Right";
    }
    if ( key == 'D' || key == 'L')
    {
        return "Left";
    }
    if ( key == 'Q' || key == 'U')
    {
        return "UP-LEFT";
    }
    if ( key == 'E' || key == 'O')
    {
        return "UP-RIGHT";
    }
    if ( key == 'Z' || key == 'M')
    {
        return "DOWN-LEFT";
    }
    if ( key == 'C' || key == '.')
    {
        return "DOWN-RIGHT";
    }
    if ( key == 'S' || key == 'K')
    {
        return "STOP";
    }
    else
    {
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
