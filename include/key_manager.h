#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

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
