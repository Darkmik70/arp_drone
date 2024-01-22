#ifndef OBSTACLES_H    
#define OBSTACLES_H


// User-defined parameters
#define MAX_OBSTACLES 7
#define MIN_SPAWN_TIME 3
#define MAX_SPAWN_TIME 8
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)

#include "constants.h"
#include <time.h>

void get_args(int argc, char *argv[]);

typedef struct {
    int x;
    int y;
    time_t spawnTime;
} Obstacle; // Diferent from Obstacle

void printObstacles(Obstacle obstacles[], int numObstacles, char obstacles_msg[]);


#endif // OBSTACLES_H