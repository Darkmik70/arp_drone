#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

#define SLEEP_TIME 500000  

void get_args(int argc, char *argv[]);
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * Creates shared memory segments
 * 
 *  This is divided into two parts.
 *  The first part creates the shared memory segment with predefined functions and size. 
 *  The second part maps the memory and returns the ptr.
 * 
 * @param name Name of the shared memory segment
 *      
 * @returns pointer to the memory map of type void
*/
void *create_shm(char *name);

/**
 * Closes and unlinks semaphores, unmaps and unlinks shared memory segments
*/
void clean_up();

#endif // SERVER_H
