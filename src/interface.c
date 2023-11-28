#include "interface.h"
#include "constants.h"
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


int main()
{
    // Attach to shared memory for key presses
    int *ptr_key;        // Shared memory for Key pressing
    char *ptr_pos;        // Shared memory for Drone Position      
    sem_t *sem_key;       // Semaphore for key presses
    sem_t *sem_pos;       // Semaphore for drone positions

    // Shared memory for KEY PRESSING
    int shm_key_fd = shm_open(SHM_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_key_fd, 0);


    // Shared memory for DRONE POSITION
    int shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    sem_key = sem_open(SEM_KEY, 0);
    sem_pos = sem_open(SEM_POS, 0);

    
    /* INITIALIZATION AND EXECUTION OF NCURSES CODE */

    initscr();
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    createWindow(); // Create the windows on the spawned 'Konsole' terminal.
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
        createWindow(); // Redraw window in case the screen changed
        drawDrone(droneX, droneY);
        handleInput(ptr_key, sem_key);
        /* Update drone position */
        //sem_wait(semaphorePos);
        sscanf(ptr_pos, "%d,%d,%d,%d", &droneX, &droneY, &maxX, &maxY); // Obtain the values of X,Y from shared memory
        //sem_post(semaphorePos);
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


void createWindow()
{
    // Clear the screen
    clear();
    // Get the dimensions of the terminal window
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);
    // Print a title in the top center part of the window
    mvprintw(0, (maxX - 11) / 2, "Drone Control");
    // Refresh the screen to apply changes
    refresh();
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