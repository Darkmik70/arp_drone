/*THIS CODE GENERATES THE COORDINATES FOR THE TARGETS. IT WORKS THE FOLLOWING WAY:
1. It analyzes the screen dimensions so it can create even sections.
2. While the coordinates are random, they are still distributed by section, so it 
is way less messy.
(SECTIONS) Hardcoded into 6 sections: top center, bottom left, top right, etc...

THIS IS MISSING:
1. THE CONNECTION TO THE INTERFACE.C SO THE OBSTACLES CAN BE DRAWN
2. THE CONNECTION TO THE MAIN, SERVER, WATCHDOG */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// User-defined parameters
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100
#define MAX_TARGETS 8

// Structure to represent a target
typedef struct {
    int id;
    int x;
    int y;
} Target;

// Function to generate random coordinates within a given sector
void generateRandomCoordinates(int sectorWidth, int sectorHeight, int *x, int *y) {
    *x = rand() % sectorWidth;
    *y = rand() % sectorHeight;
}

// Function to print target information
void printTarget(Target target) {
    printf("Target %d: (%d, %d)\n", target.id, target.x, target.y);
}

int main() {
    // Set seed for random number generation
    srand(time(NULL));

    // Sector dimensions
    const int sectorWidth = SCREEN_WIDTH / 3;
    const int sectorHeight = SCREEN_HEIGHT / 2;

    // Array to store generated targets
    Target targets[MAX_TARGETS];

    // Generate random order for distributing targets across sectors
    int order[MAX_TARGETS];
    for (int i = 0; i < MAX_TARGETS; ++i) {
        order[i] = i;
    }
    for (int i = MAX_TARGETS - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }

    // Generate targets within each sector
    for (int i = 0; i < MAX_TARGETS; ++i) {
        Target target;
        target.id = i + 1;

        // Determine sector based on the random order
        int sectorX = (order[i] % 3) * sectorWidth;
        int sectorY = (order[i] / 3) * sectorHeight;

        // Generate random coordinates within the sector
        generateRandomCoordinates(sectorWidth, sectorHeight, &target.x, &target.y);

        // Adjust coordinates based on the sector position
        target.x += sectorX;
        target.y += sectorY;

        // Ensure coordinates do not exceed the screen size
        target.x = target.x % SCREEN_WIDTH;
        target.y = target.y % SCREEN_HEIGHT;

        // Store the target
        targets[i] = target;
    }

    // Print the generated targets
    for (int i = 0; i < MAX_TARGETS; ++i) {
        printTarget(targets[i]);
    }

    // Add a 20-second delay
    sleep(20);

    return 0;
}
