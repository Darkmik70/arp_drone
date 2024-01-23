#ifndef MAIN_H    
#define MAIN_H


void create_pipes();

void close_all_pipes();

/**
 * Creates Child child process
 * @param program name of the process
 * @param arg_list arguments of process
 * @returns PID of the child process
*/
int create_child(const char *program, char **arg_list);


#endif // MAIN_H
