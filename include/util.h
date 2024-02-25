#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

/**
 * log message to the logfile
 * 
 * @param filepath filepath to the logfile
 * @param who name of the process, written in capital letters
 * @param message message
 * HOW TO WRITE A PROPER MESSAGE, follow this format:
 * [WHO] WHAT TIME
*/
void log_msg(char *filepath, char *who, char *message);

/**
 * Publish process pid to watchdog process
 *  @param process_symbol character that represent to which process PID will belong
 *  @param pid_t process pid
*/
void publish_pid_to_wd(int process_symbol, pid_t pid);

void write_to_pipe(int pipe_fd, char message[]);

void write_message_to_logger(int who, int type, char *msg);

void read_then_echo(int sockfd, char socket_msg[]);

int read_then_echo_unblocked(int sockfd, char socket_msg[], char *logfile, char *log_who);

void write_then_wait_echo(int sockfd, char socket_msg[], size_t msg_size, char *logfile, char *log_who);

int read_pipe_unblocked(int pipefd, char msg[]);

void read_args_from_file(const char *filename, char *type, char *data);

void parse_host_port(const char *str, char *host, int *port);

#endif // UTIL_H