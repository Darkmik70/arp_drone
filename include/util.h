#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

/**
 * Publish process pid to watchdog process
 *  @param process_symbol character that represent to which process PID will belong
 *  @param pid_t process pid
*/
void publish_pid_to_wd(char process_symbol, pid_t pid);

void write_to_pipe(int pipe_fd, char *message);

char *read_from_pipe(int pipe_fd);

#endif // UTIL_H