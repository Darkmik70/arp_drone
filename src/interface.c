#include "interface.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>

#include <signal.h>

/* Global variables */
// Attach to shared memory for key presses
int *ptr_key;   // Shared memory for Key pressing
char *ptr_pos;  // Shared memory for Drone Position
sem_t *sem_key; // Semaphore for key presses
sem_t *sem_pos; // Semaphore for drone positions


void signal_handler(int signo)
{
    printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_pos);

        printf("Succesfully closed all semaphores\n");
        sleep(10);
        exit(1);
    }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigaction (SIGINT, &sa, NULL);    

    publish_pid_to_wd(WINDOW_SYM, getpid());

    // Shared memory for KEY PRESSING
    int shm_key_fd = shm_open(SHM_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_key_fd, 0);


    // Shared memory for DRONE POSITION
    int shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    sem_key = sem_open(SEM_KEY, 0);
    sem_pos = sem_open(SEM_POS, 0);

    
    /* INITIALIZATION AND EXECUTION OF NCURSES CODE */

    initscr();
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    // Initialize color for drawing drone
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);

    // Set the initial drone position (middle of the window)
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    // Write initial drone position in its corresponding shared memory
    sprintf(ptr_pos, "%d,%d,%d,%d", droneX, droneY, maxX, maxY);


    while (1) {
        // Obtain the positionvalues stored in shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &droneX, &droneY, &maxX, &maxY);
        // Create the window
        // Rewrite the maximum values if necessary
        if (createWindow(maxX, maxY) == 1){
            getmaxyx(stdscr, maxY, maxX);
            sprintf(ptr_pos, "%d,%d,%d,%d", droneX, droneY, maxX, maxY);
        }
        drawDrone(droneX, droneY);
        handleInput(ptr_key, sem_key);
        usleep(20000);
        continue;
    }
    
    endwin(); // Clean up and finish up resources taken by ncurses

    // Close shared memories
    close(shm_key_fd);
    close(shm_pos_fd);

    // Close and unlink semaphores
    sem_close(sem_key);
    sem_close(sem_pos);

    return 0;
}


int createWindow(int maxX, int maxY){
    // Clear the screen
    clear();
    // Get the dimensions of the terminal window
    int new_maxY, new_maxX;
    getmaxyx(stdscr, new_maxY, new_maxX);
    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);
    // Print a title in the top center part of the window
    mvprintw(0, (new_maxX - 11) / 2, "Drone Control");
    // Refresh the screen to apply changes
    refresh();
    if(new_maxX != maxX || new_maxY != maxY){return 1;}
    else{return 0;}
}

void drawDrone(int droneX, int droneY)
{
    // Draw a plus sign to represent the drone
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));
    refresh();
}

void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch;
    noecho(); // Disable echoing: no key character will be shown when pressed.
    if ((ch = getch()) != ERR)
    {
        // Store the pressed key in shared memory
        *sharedKey = ch;
        // Signal the semaphore to notify the server
        sem_post(semaphore);
    }
    echo(); // Re-enable echoing
    flushinp(); // Clear the input buffer
}