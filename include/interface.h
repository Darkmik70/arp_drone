#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>
#include "constants.h"


/**
 * Reads the arguments passed to the main function
*/
void get_args(int argc, char *argv[]);

/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * Obtains the lowest ID value from the targets data.
*/
int find_lowest_target_id(Targets *targets, int numTargets);

/**
 * Removes the specified target from the data stored inside the process.
*/
void remove_target(Targets *targets, int *numTargets, int indexToRemove);

/**
 *  Checks if drone is at the same coordinates as any obstacle.
*/
int is_drone_at_obstacle(Obstacles obstacles[], int numObstacles, int droneX, int droneY);

/**
* Extracts the coordinates from the targets string into the struct Targets.
*/
void parse_target_string(char *targets_msg, Targets *targets, int *numTargets, Targets *original_targets);

/**
 * Extracts the coordinates from obstacles_msg and puts into strct Obstacles
*/
void parse_obstacles_string(char *obstacles_msg, Obstacles *obstacles, int *numObstacles);

/**
 * Draw the window on the screen.
 * Draws dynamical box which adapts to the actual size of window
 *
 * @param droneX            Drone's position in x
 * @param droneY            Drone's position in y
 * @param targets           Array of targets saved in a data struct
 * @param numTargets        Number of targets
 * @param obstacles         Array of obstacles saved in a data struct
 * @param numObstacles      Number of obstacles
 * @param score_msg         A string containing the current score
 */
void draw_window(int droneX, int droneY, Targets *targets, int numTargets, Obstacles *obstacles, int numObstacles, const char *score_msg);


#endif //INTERFACE_H