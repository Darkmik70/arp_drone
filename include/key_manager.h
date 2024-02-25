#ifndef KEY_MANAGER_H    
#define KEY_MANAGER_H

#include <signal.h>


/**
 * Takes the character from the user input and translates it into a command for the drone.
 * 
 * @param pressed_key   Input provided by the user
 * @returns Status of the action taken
*/
char* determine_action(char pressed_key);

/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);

/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);


#endif // KEY_MANAGER_H
