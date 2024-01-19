#ifndef LOGGER_H
#define LOGGER_H

#include <signal.h>
#include <stdio.h>

// TODO ADD COMMENTS
void signal_handler(int signo, siginfo_t *siginfo, void *context);

// TODO ADD COMMENTS
void write_to_file(FILE *fp,char *msg);
//TODO ADD COMMENTS
void create_logfile(FILE *fp);




#endif // LOGGER_H
