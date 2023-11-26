#include "key_manager.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <fcntl.h>
#include <semaphore.h>


int main() 
{
    int sharedKey;
    void *ptr_key;        // Shared memory for Key pressing
    void *ptr_action;        // Shared memory for Drone Position      
    
    int sharedAction;
    sem_t *sem_key;       // Semaphore for key presses
    sem_t *sem_action;       // Semaphore for drone positions

    // Shared memory for KEY PRESSING
    sharedKey = shm_open(SHM_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, sharedKey, 0);


    // Shared memory for DRONE CONTROL - ACTION
    sharedAction = shm_open(SHM_ACTION, O_RDWR, 0666);
    ptr_action = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, sharedAction, 0);


    sem_key = sem_open(SEM_KEY, 0);
    sem_action = sem_open(SEM_ACTION, 0);


    // Main loop
    while (1)
    {
        /*THIS SECTION IS FOR OBTAINING KEY INPUT*/

        // Wait for the semaphore to be signaled
        sem_wait(sem_key);
        // Read the pressed key from shared memory
        int pressedKey = *(int*)ptr_key; 
        //printf("Pressed key: %c\n", (char)pressedKey);
        printf("Pressed key: %c\n", (char)pressedKey);
        // Clear the shared memory after processing the key
        clearSharedMemory(ptr_key);

        /*THIS SECTION IS FOR DRONE ACTION DECISION*/
        // Determine the action based on the pressed key
        char *action = determineAction(pressedKey, ptr_action);
        // Print the action taken
        printf("Action sent to drone: %s\n\n", action);
        fflush(stdout);
    }

    // close shared memories
    close(sharedKey);
    close(sharedAction);

    // Close and unlink the semaphore
    sem_close(sem_key);
    sem_close(sem_action);

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
