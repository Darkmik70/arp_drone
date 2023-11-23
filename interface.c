#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Shared memory key
#define SHM_KEY 1234

void createBlackboard();
void drawDrone(int droneX, int droneY);
void handleInput(int *sharedKey);

int main() {
    initscr();
    timeout(0); // Set non-blocking getch
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
        handleInput(sharedMemory);
        usleep(100000); // Add a small delay to control the speed
        continue;
    }
    
    // Detach the shared memory segment
    //shmdt(sharedMemory);

    // End the shared memory segment
    shmctl(sharedKey, IPC_RMID, NULL);
    endwin();
    return 0;
}


void createBlackboard() {
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


void drawDrone(int droneX, int droneY) {

    // Draw the center of the cross
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));

    refresh();
}


void handleInput(int *sharedKey) {
    int ch;

    if ((ch = getch()) != ERR) {
        // Debugging: Print the pressed key
        printf("Pressed key: %d\n", ch);

        // Store the pressed key in shared memory
        *sharedKey = ch;
    }

    // Clear the input buffer
    flushinp();
}
