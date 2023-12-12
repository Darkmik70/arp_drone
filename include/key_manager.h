#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

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


#endif // KEY_MANAGER_H
