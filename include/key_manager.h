#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>


/**
 * Determine what action should be performed from user input
 * 
 * @param pressedKey 
 * @returns Message with with pressed key
*/
char* determineAction(int pressedKey);

/**
 * Sends action to Drone
 * 
 * @param action translated action
*/
void sendActionToDrone(char* action);

/**
 * Clears shared Memory 
 * 
 * @param sharedMemory memory to be cleaned
*/
void clearSharedMemory(int* sharedMemory);


#endif // KEY_MANAGER_H
