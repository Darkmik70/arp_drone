#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void createBlackboard();
void drawDrone(int droneX, int droneY);
void handleInput();

int main() {
    initscr();
    timeout(0); // Set non-blocking getch
    createBlackboard();

    // Initial drone position (middle of the blackboard)
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    // Initialize color
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);

    while (1) {

        drawDrone(droneX, droneY);
        handleInput();
        usleep(100000); // Add a small delay to control the speed
        continue;
    }

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


void handleInput() {
    int ch;

    while ((ch = getch()) != ERR) {
        // Print the pressed key to the standard output
        printf("Pressed key: %c\n", ch);
    }

    // Clear the input buffer
    flushinp();
}
