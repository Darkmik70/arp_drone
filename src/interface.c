#include "interface.h"
#include "constants.h"

#include <ncurses.h>
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
    initscr();
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    createBlackboard();

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

    // Initial drone position (middle of the blackboard)
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    // Initialize color
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);

    while (1) {
        createBlackboard();
        drawDrone(droneX, droneY);
        handleInput(sharedMemory, semaphore);
        usleep(200000); // Add a small delay to control the speed
        continue;
    }
    
    // Detach the shared memory segment
    shmdt(sharedMemory);

    // Close and unlink the semaphore
    sem_close(semaphore);

    // End the shared memory segment
    shmctl(sharedKey, IPC_RMID, NULL);

    endwin();
    return 0;
}



void createBlackboard()
{
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);

    // Print a title in the center of the blackboard
    mvprintw(0, (maxX - 11) / 2, "Drone Control");

    // Refresh the screen to apply changes
    refresh();
}


void drawDrone(int droneX, int droneY)
{

    // Draw the center of the cross
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));

    refresh();
}


void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch;

    // Disable echoing
    noecho();

    if ((ch = getch()) != ERR)
    {
        // Debugging: Commented out the print statement
        // printf("Pressed key: %d\n", ch);

        // Store the pressed key in shared memory
        *sharedKey = ch;

        // Signal the semaphore to notify the server
        sem_post(semaphore);
    }

    // Enable echoing
    echo();

    // Clear the input buffer
    flushinp();
}

