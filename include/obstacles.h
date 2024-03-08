#ifndef OBSTACLES_H    
#define OBSTACLES_H

#include <time.h>
#include <signal.h>

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
} Obstacle; // Diferent from OBS in constants.h

/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);

/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * Generates a string for the obstacles according to protocol.
*/
void generate_obstacles_string(Obstacle obstacles[], int numObstacles, char obstacles_msg[]);


#endif // OBSTACLES_H