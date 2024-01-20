#ifndef OBSTACLES_H    
#define OBSTACLES_H


// User-defined parameters
#define MAX_OBSTACLES 7
#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 100
#define MIN_SPAWN_TIME 2
#define MAX_SPAWN_TIME 7
#define WAIT_TIME 1 // How often a new obstacle is created (if the number is below MAX_OBSTACLES)


#include "constants.h"
#include <time.h>

typedef struct {
    int x;
    int y;
    time_t spawnTime;
} Obstacle; // Diferent from Obstacle

void printObstacles(Obstacle obstacles[], int numObstacles, char obstacles_msg[])


#endif // OBSTACLES_H