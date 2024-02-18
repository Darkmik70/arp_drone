#ifndef SERVER_H
#define SERVER_H

#include <signal.h>


/**
 * Creates shared memory segments
 *  1. Creation of the shared memory segment with predefined functions and size. 
 *  2. Memory mapping and return of ptr.
 * 
 * @param name Name of the shared memory segment
 *   
 * @returns pointer to the memory map of type void
*/
void *create_shm(char *name);

/**
 * Obtain the file descriptors of the pipes given by the main process.
*/
void get_args(int argc, char *argv[]);

/**
 * Handles the signals monitored by the watchdog process.
*/
void signal_handler(int signo, siginfo_t *siginfo, void *context);

/**
 * Closes and unlinks semaphores, unmaps and unlinks shared memory segments
*/
void clean_up();


#endif // SERVER_H
