#ifndef OBSTACLES_H    
#define OBSTACLES_H

#include <time.h>

// User-defined parameters
#define MAX_OBSTACLES 5
#define MIN_SPAWN_TIME 10
#define MAX_SPAWN_TIME 15
#define WAIT_TIME 1 // How often a new obstacle is created (if < MAX_OBSTACLES)


// Structure to represente a single obstacle
typedef struct {
    int x;
    int y;
    time_t spawnTime;
} Obstacle; // Diferent from "Obstacles" in constants.h

/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);

/**
 * Generates a string for the obstacles according to protocol.
*/
void generate_obstacles_string(Obstacle obstacles[], int numObstacles, char obstacles_msg[]);


#endif // OBSTACLES_H