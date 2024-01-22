#include "targets.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <signal.h>


int main() {
    int counter = 0;
    while(1){
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

        // String variable to store the targets in the specified format
        char targets_msg[100]; // Adjust the size accordingly

        // Generate targets within each sector
        for (int i = 0; i < MAX_TARGETS; ++i) {
            Target target;

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

        // Construct the targets_msg string
        int offset = sprintf(targets_msg, "T[%d]", MAX_TARGETS);
        for (int i = 0; i < MAX_TARGETS; ++i) {
            offset += sprintf(targets_msg + offset, "%d,%d", targets[i].x, targets[i].y);

            // Add a separator unless it's the last element
            if (i < MAX_TARGETS - 1) {
                offset += sprintf(targets_msg + offset, "|");
            }
        }
        targets_msg[offset] = '\0'; // Null-terminate the string

        // Print the generated targets in the specified format
        printf("%s\n", targets_msg); // Instead of print, this string should be sent to server.c

        //TODO: Send the targets_msg string using pipes to (main.c) with final destination to be (interface.c)
        // *

        // Temporary Delay (TEMPORARY)
        sleep(1);
    }
    return 0;
}


// Function to generate random coordinates within a given sector
void generateRandomCoordinates(int sectorWidth, int sectorHeight, int *x, int *y)
{
    *x = rand() % sectorWidth;
    *y = rand() % sectorHeight;
}
