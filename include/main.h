#ifndef MAIN_H    
#define MAIN_H

#include <stdio.h>
#include <signal.h>

void signal_handler(int signo, siginfo_t *siginfo, void *context);

void create_logfile();

/**
 * Creates Child child process
 * @param program name of the process
 * @param arg_list arguments of process
 * @returns PID of the child process
*/
int create_child(const char *program, char **arg_list);

/**
 * Closes all pipes created during the program's execution and file descriptors
*/
void cleanup();

void get_args(int argc, char *argv[], char *program_type, char *socket_data);


#endif // MAIN_H
