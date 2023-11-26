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
    // Initialize shared memory 1: 'Key pressed'
    int sharedKey;
    int *sharedMemory;

    if ((sharedKey = shmget(SHM_KEY_1, sizeof(int), IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((sharedMemory = shmat(sharedKey, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    sem_t *semaphore = sem_open(SEM_KEY_1, O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
    
    // Initialize shared memory 3: 'Action'
    int sharedAct;
    char *sharedAction;

    if ((sharedAct = shmget(SHM_KEY_3, 80*sizeof(char), IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if ((sharedAction = shmat(sharedAct, NULL, 0)) == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }
    
    sem_t *semaphore_act = sem_open(SEM_KEY_3, O_CREAT, 0666, 0);
    if (semaphore_act == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }


    // Main loop
    while (1)
    {
        /*THIS SECTION IS FOR OBTAINING KEY INPUT*/

        // Wait for the semaphore to be signaled
        sem_wait(semaphore);
        // Read the pressed key from shared memory
        int pressedKey = *sharedMemory;
        //printf("Pressed key: %c\n", (char)pressedKey);
        printf("Pressed key: %c\n", (char)pressedKey);
        // Clear the shared memory after processing the key
        clearSharedMemory(sharedMemory);

        /*THUS SECTION IS FOR DRONE ACTION DECISION*/

        // Determine the action based on the pressed key
        char *action = determineAction(pressedKey, sharedAction);
        // Print the action taken
        printf("Action sent to drone: %s\n\n", action);
        fflush(stdout);
    }

    // Detach the shared memory segment
    shmdt(sharedMemory);
    shmdt(sharedAction);
    // Close and unlink the semaphore
    sem_close(semaphore);
    sem_close(semaphore_act);

    return 0;
}


// US Keyboard assumed
char* determineAction(int pressedKey, char *sharedAction)
{
    char key = toupper(pressedKey);
    int x; int y;

    if ( key == 'W' || key == 'I')
    {
        x = 0;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "UP";
    }
    if ( key == 'X' || key == ',')
    {
        x = 0;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "DOWN";
    }
    if ( key == 'A' || key == 'J')
    {
        x = -1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "RIGHT";
    }
    if ( key == 'D' || key == 'L')
    {
        x = 1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "LEFT";
    }
    if ( key == 'Q' || key == 'U')
    {
        x = -1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "UP-LEFT";
    }
    if ( key == 'E' || key == 'O')
    {
        x = 1;    // Movement on the X axis.
        y = -1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "UP-RIGHT";
    }
    if ( key == 'Z' || key == 'M')
    {
        x = -1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "DOWN-LEFT";
    }
    if ( key == 'C' || key == '.')
    {
        x = 1;    // Movement on the X axis.
        y = 1;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "DOWN-RIGHT";
    }
    if ( key == 'S' || key == 'K')
    {
        x = 0;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "STOP";
    }
    else
    {
        return "None";
    }

}


void clearSharedMemory(int* sharedMemory)
{
    // Set the shared memory to a special value to indicate no key pressed
    *sharedMemory = NO_KEY_PRESSED;
}
