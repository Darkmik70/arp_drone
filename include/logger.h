#ifndef LOGGER_H
#define LOGGER_H

#include <signal.h>
#include <stdio.h>


void signal_handler(int signo, siginfo_t *siginfo, void *context);

void write_to_file(FILE *fp, char *msg);

void create_logfile_name(char *name);

#endif // LOGGER_H
