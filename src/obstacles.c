#include "obstacles.h"
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

// Pipes working with the server
int server_obstacles[2];
int obstacles_server[2];

int main(int argc, char *argv[])
{
    get_args(argc, argv);
    srand(time(NULL));

    Obstacle obstacles[MAX_OBSTACLES];
    int numObstacles = 0;
    char obstacles_msg[MSG_LEN];

    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // To compare previous values
    int obtained_dimensions = 0;

    // Variables
    int screen_size_x; int screen_size_y;

    while (1) {

        //////////////////////////////////////////////////////
        /* SECTION 1: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_obstacles[0], &read_fds);
        
        char server_msg[MSG_LEN];

        int ready = select(server_obstacles[0] + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == -1) {perror("Error in select");}

        if (ready > 0 && FD_ISSET(server_obstacles[0], &read_fds)) {
            ssize_t bytes_read = read(server_obstacles[0], server_msg, MSG_LEN);
            if (bytes_read > 0) {
                sscanf(server_msg, "I2:%d,%d", &screen_size_x, &screen_size_y);
                printf("Obtained from server: %s\n", server_msg);
                fflush(stdout);
                obtained_dimensions = 1;
            }
        }
        if(obtained_dimensions == 0){continue;}


        //////////////////////////////////////////////////////
        /* SECTION 2: OBSTACLES GENERATION & SEND DATA */
        /////////////////////////////////////////////////////

        // Check if it's time to spawn a new obstacle
        if (numObstacles < MAX_OBSTACLES) {
            Obstacle newObstacle;
            newObstacle.x = rand() % screen_size_x;
            newObstacle.y = rand() % screen_size_y;
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
                obstacles[i].x = rand() % screen_size_x;
                obstacles[i].y = rand() % screen_size_y;
                obstacles[i].spawnTime = time(NULL) + (rand() % (MAX_SPAWN_TIME - MIN_SPAWN_TIME + 1) + MIN_SPAWN_TIME);
            }
        }

        // SEND DATA TO SERVER
        write_to_pipe(obstacles_server[1],obstacles_msg);

    }

    return 0;
}

void printObstacles(Obstacle obstacles[], int numObstacles, char obstacles_msg[])
{
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
    fflush(stdout);
}

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_obstacles[0], &obstacles_server[1]);
}