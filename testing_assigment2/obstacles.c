/*THIS CODE GENERATES THE COORDINATES FOR THE OBSTACLES. IT WORKS THE FOLLOWING WAY:
1. An obstacle is spawned at the beginning (it has a spawn time)
2. After an specified wait_time another obstacle will appear, until the max is reached
3. When an obstacle de-spawns, on the next cycle, a new one will take its place 

THIS IS MISSING:
1. THE CONNECTION TO THE INTERFACE.C SO THE OBSTACLES CAN BE DRAWN
2. THE CONNECTION TO THE MAIN, SERVER, WATCHDOG */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// User-defined parameters
#define MAX_OBSTACLES 7
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100
#define MIN_SPAWN_TIME 2
#define MAX_SPAWN_TIME 7
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is bellow MAX_OBSTACLES)

typedef struct {
    int x;
    int y;
    time_t spawnTime;
} Obstacle;

void printObstacles(Obstacle obstacles[], int numObstacles) {
    printf("Obstacles:\n");
    for (int i = 0; i < numObstacles; ++i) {
        printf("Obstacle %d: (%d, %d)\n", i + 1, obstacles[i].x, obstacles[i].y);
    }
}

int main() {
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];
    int numObstacles = 0;

    while (1) {
        // Check if it's time to spawn a new obstacle
        if (numObstacles < MAX_OBSTACLES) {
            Obstacle newObstacle;
            newObstacle.x = rand() % SCREEN_WIDTH;
            newObstacle.y = rand() % SCREEN_HEIGHT;
            newObstacle.spawnTime = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);

            obstacles[numObstacles] = newObstacle;
            numObstacles++;
        }

        // Print obstacles every second
        printObstacles(obstacles, numObstacles);
        sleep(WAIT_TIME);

        // Check if any obstacle has exceeded its spawn time
        time_t currentTime = time(NULL);
        for (int i = 0; i < numObstacles; ++i) {
            if (obstacles[i].spawnTime <= currentTime) {
                // Replace the obstacle with a new one
                obstacles[i].x = rand() % SCREEN_WIDTH;
                obstacles[i].y = rand() % SCREEN_HEIGHT;
                obstacles[i].spawnTime = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
            }
        }
    }

    return 0;
}