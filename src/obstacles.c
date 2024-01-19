#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// User-defined parameters
#define MAX_OBSTACLES 7
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100
#define MIN_SPAWN_TIME 2
#define MAX_SPAWN_TIME 7
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)

typedef struct {
    int x;
    int y;
    time_t spawnTime;
} Obstacle;

void printObstacles(Obstacle obstacles[], int numObstacles, char obstacles_msg[]) {
    // Append the letter O and the total number of obstacles to obstacles_msg
    sprintf(obstacles_msg, "O[%d]", numObstacles);

    for (int i = 0; i < numObstacles; ++i) {
        // Append obstacle information to obstacles_msg
        sprintf(obstacles_msg + strlen(obstacles_msg), "%d,%d", obstacles[i].x, obstacles[i].y);

        // Add a separator if there are more obstacles
        if (i < numObstacles - 1) {
            sprintf(obstacles_msg + strlen(obstacles_msg), "|");
        }
    }
    printf("%s\n", obstacles_msg);
}

int main() {
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];
    int numObstacles = 0;
    char obstacles_msg[1000]; // Adjust the size based on your requirements

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
        obstacles_msg[0] = '\0'; // Clear the string
        printObstacles(obstacles, numObstacles, obstacles_msg); // Instead of print, this string should be sent to server.c
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

        // TODO: Send the obstacles_msg using pipes to (server.c), and from the server then to (interface.c) and (drone.c)

    }

    return 0;
}
