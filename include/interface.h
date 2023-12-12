#ifndef INTERFACE_H
#define INTERFACE_H   

#include <semaphore.h>


/**
 * Draw the window on the screen.
 * Draws dynamical box which adapts to the actual size of window
 * 
 * @param maxX size in x 
 * @param maxY size in Y
*/
void createWindow(int maxX, int maxY);

/**
 * Draw the Drone on the screen
 * 
 * @param droneX    Drone's position in x
 * @param droneY    Drone's position in y
*/
void drawDrone(int droneX, int droneY);

/**
 * Read the input from the keyboard, the pressed keys
 * 
 * @param sharedKey  pointer to shared memory
 * @param semphore   Semaphore to be signalled for unlock
*/
void handleInput(int *sharedKey, sem_t *semaphore);


#endif //INTERFACE_H