#ifndef DRONE_H
#define DRONE_H

#include <constants.h>
#include <signal.h>

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define D_T 0.1 // Time interval in seconds
#define F_MAX 30.0 // Maximum force that can be acted on the drone's motors
#define EXT_FORCE_MAX 40.0 // Maximum external force on any axis.
#define COEFFICIENT 400.0 // Obtained by testing and trial-error
#define MIN_DISTANCE 5.0  // Distance where the drone begins feeling force of an object
#define MAX_DISTANCE 2.0  // Distance where the drone caps out the force of an object
#define TOLERANCE 0.0044  // Below this value the process drone.c will not print the velocity anymore


/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);


/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);


/**
 * Calculates the forces on the drone originated from targets and obstacles
 * @param ext_forceX - Pointer to the force variable on the X axis
 * @param ext_forceX - Pointer to the force variable on the Y axis
 * @result - Updates the value for each of the force variables inside the drone process
*/
void calculate_external_forces(double droneX, double droneY, double targetX, double targetY,
                        double obstacleX, double obstacleY, double *ext_forceX, double *ext_forceY);


/**
 * Converts the string of obstacles defined in the protocol to its component parts.
 * @result - Adds values to each struct element in the Obstacles array
*/
void parse_obstacles_string(char *obstacles_msg, Obstacles *obstacles, int *numObstacles);

/**
 * Updates the drone position for a single axis, according to specified drone dynamics.
 * @param pos - Pointer to the drone position variable
 * @param vel - Pointer to the drone velocity variable
 * @param force - Force applied by the user
 * @param force - Force applied by external objects
 * @param maxPos - Pointer to the maximum position the drone can take (window's size).
 * @result - Updates the position of the drone in the given axis.
*/
void euler_method(double *pos, double *vel, double force, double extForce, double *maxPos);


#endif //DRONE_H