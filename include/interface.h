#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>
#include <signal.h>
#include "constants.h"


int findLowestID(Targets *targets, int numTargets);

void removeTarget(Targets *targets, int *numTargets, int indexToRemove);

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void get_args(int argc, char *argv[]);

void parseTargetMsg(char *targets_msg, Targets *targets, int *numTargets);

int isDroneAtObstacle(Obstacles obstacles[], int numObstacles, int droneX, int droneY);

void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles);


/**
 * Draw the window on the screen.
 * Draws dynamical box which adapts to the actual size of window
 *
 * @param maxX size in x
 * @param maxY size in Y
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