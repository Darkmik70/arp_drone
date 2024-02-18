#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

/**
 * Publish process pid to watchdog process
 *  @param process_symbol character that represent to which process PID will belong
 *  @param pid_t process pid
*/
void publish_pid_to_wd(int process_symbol, pid_t pid);

void write_to_pipe(int pipe_fd, char message[]);

void write_message_to_logger(int who, int type, char *msg);

void read_then_echo(int sockfd, char socket_msg[]);

int read_then_echo_unblocked(int sockfd, char socket_msg[]);

void write_then_wait_echo(int sockfd, char socket_msg[], size_t msg_size);

int read_pipe_unblocked(int pipefd, char msg[]);

#endif // UTIL_H