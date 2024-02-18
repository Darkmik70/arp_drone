#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>
#include "constants.h"


int findLowestID(Targets *targets, int numTargets);

void removeTarget(Targets *targets, int *numTargets, int indexToRemove);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * Read the arguments passed to the main function
*/
void get_args(int argc, char *argv[]);

/**
* Function to extract the coordinates from the targets_msg string into the struct Targets.
*/
void parseTargetMsg(char *targets_msg, Targets *targets, int *numTargets, Targets *original_targets);


/**
 *  Function to check if drone is at the same coordinates as any obstacle.
*/
int isDroneAtObstacle(Obstacles obstacles[], int numObstacles, int droneX, int droneY);


/**
 * Extracts the coordinates from obstacles_msg and puts into strct Obstacles
 * @param obstacles_msg - message to be read
 * @param obstacles - struct Obstacles containing the obstacles
 * @param numObstacles - number of obstacles 
*/
void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles);

/**
 * Draws end-game message
 * 
 * @param score - game score
*/
void draw_final_window(int score);

/**
 * Draw the window on the screen.
 * Draws dynamical box which adapts to the actual size of window
 *
 * @param droneX    Drone's position in x
 * @param droneY    Drone's position in y
 */
void draw_window(int droneX, int droneY, Targets *targets, int numTargets, Obstacles *obstacles, int numObstacles, const char *score_msg);

/**
 * Determine what action should be performed from user input.
 * 
 * Function takes the the user input and translates it into command for drone.
 * drone_action is the memory segment to which command is written as a message.
 * For the steering, the layout of US Keyboard is assumed
 * 
 * @param pressedKey input provided by the user
 * @returns Status of the action taken
*/
char* determineAction(int pressedKey, char *sharedAct);

#endif //INTERFACE_H