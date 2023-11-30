#include "key_manager.h"
#include "constants.h"
#include "util.h"

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
#include <signal.h>


// GLOBAL VARIABLES
int sharedKey;
int sharedAction;
void *ptr_key;              // Shared memory for Key pressing
void *ptr_action;           // Shared memory for Drone Position      
sem_t *sem_key;             // Semaphore for key presses
sem_t *sem_action;          // Semaphore for drone positions

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_action);

        printf("Succesfully closed all semaphores\n");
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}


int main() 
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);    

    
    publish_pid_to_wd(KM_SYM, getpid());

    // Initialize shared memory for KEY PRESSING
    sharedKey = shm_open(SHM_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, sharedKey, 0);

    // Initialize shared memory for DRONE CONTROL - ACTION
    sharedAction = shm_open(SHM_ACTION, O_RDWR, 0666);
    ptr_action = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, sharedAction, 0);

    // Initialize semaphores
    sem_key = sem_open(SEM_KEY, 0);
    sem_action = sem_open(SEM_ACTION, 0);

    while (1)
    {
        /*THIS SECTION IS FOR OBTAINING KEY INPUT*/

        sem_wait(sem_key);  // Wait for the semaphore to be signaled from interface.c process
        int pressedKey = *(int*)ptr_key;    // Read the pressed key from shared memory 
        printf("Pressed key: %c\n", (char)pressedKey);
        fflush(stdout);

        /*THIS SECTION IS FOR DRONE ACTION DECISION*/

        char *action = determineAction(pressedKey, ptr_action);
        printf("Action sent to drone: %s\n\n", action);
        fflush(stdout);

        sharedKey = NO_KEY_PRESSED; // Clear the shared memory after processing the key
    }

    // Close shared memories
    close(sharedKey);
    close(sharedAction);

    // Close and unlink the semaphores
    sem_close(sem_key);
    sem_close(sem_action);

    return 0;
}


// US Keyboard assumed
char* determineAction(int pressedKey, char *sharedAction)
{
    char key = toupper(pressedKey);
    int x; int y;

    // Disclaimer: Y axis is inverted on tested terminal.
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
        return "LEFT";
    }
    if ( key == 'D' || key == 'L')
    {
        x = 1;    // Movement on the X axis.
        y = 0;    // Movement on the Y axis.
        sprintf(sharedAction, "%d,%d", x, y);
        return "RIGHT";
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
        x = 900;    // Special value interpreted by drone.c process
        y = 0;
        sprintf(sharedAction, "%d,%d", x, y);
        return "STOP";
    }
    else
    {
        return "None";
    }

}