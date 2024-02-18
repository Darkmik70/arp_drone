#ifndef MAIN_H    
#define MAIN_H


/**
 * Creates Child child process
 * @param program name of the process
 * @param arg_list arguments of process
 * @returns PID of the child process
*/
int create_child(const char *program, char **arg_list);

/**
 * Closes all pipes created during the program's execution
*/
void close_all_pipes();


#endif // MAIN_H
