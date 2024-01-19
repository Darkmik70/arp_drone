#include "logger.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>


// GLOBAL VARIABLES
int shm_logs_fd;             // File descriptor for logs shm
void *ptr_logs;              // Shared memory for logs
sem_t *sem_logs;              // Semphore for logging info and errors
FILE *logfile;               // Pointer to the logfile


int main(int argc, char *argv[]) 
{   
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   

    // when all shm are created publish your pid to WD
    publish_pid_to_wd(LOGGER_SYM, getpid());

    // Initialize shared memory for Logging
    shm_logs_fd = shm_open(SHM_LOGS, O_RDWR, 0666);
    ptr_logs = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_logs_fd, 0);
    // Initialize semaphores
    sem_logs = sem_open(SEM_LOGS, 0);


    create_logfile(logfile);

    // Main loop
    while (1)
    {
        // Wait for the new message to arrive
        sem_wait(sem_logs);

        // Write the message
        write_to_file(logfile, ptr_logs);

        // clear the ptr
        ptr_logs = "";

        // unlock semaphore for other processes
        sem_post(sem_logs);
    }
    
    // close all connections
    sem_close(sem_logs);
    // unlink semaphores
    sem_unlink(SEM_LOGS);
    // close fd
    close(shm_logs_fd);
    // Close the file
    fclose(logfile);

    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_logs);
        // Close the file
        fclose(logfile);

        printf("Succesfully closed all semaphores\n");
        sleep(2);
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        //TODO REFRESH SCREEN
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}

void create_logfile(FILE *fp)
{
    // Get current time
    time_t current_time;
    struct tm *time_info;
    char filename[40];  // Buffer to hold the filename

    time(&current_time);
    time_info = localtime(&current_time);

    // Format time into a string: "logfile_YYYYMMDD_HHMMSS.txt"
    strftime(filename, sizeof(filename), "logfile_%Y%m%d_%H%M%S.txt", time_info);


    char path_file[256];
    char directory[] = "build/logs";
    sprintf(path_file, "%s/%s", directory, filename);

    // Create the logfile
    fp = fopen(path_file, "a");
    if (logfile == NULL)
    {
        perror("Unable to open the log file.");
        exit(1);
    }
}

void write_to_file(FILE *fp,char *msg)
{
    int process_symbol;
    int msg_type;
    char *msg_content, *who, *what;
    sscanf(msg, "%i|%i|%s", &process_symbol, &msg_type, msg_content);

    // decide who gave you the message
    switch (process_symbol)
    {
    case SERVER_SYM:
        sprintf(who,"[SERVER]");
        break;
    case WINDOW_SYM:
        sprintf(who,"[WINDOW]");
        break;
    case KM_SYM:
        sprintf(who,"[KEY_MANAGER]");
        break;
    case DRONE_SYM:
        sprintf(who,"[DRONE]");
        break;
    case WD_SYM:
        sprintf(who,"[WATCHDOG]");
        break;
    default:
        sprintf(who,"[UNKOWN]");
    }

    // Decide what is the message type
    switch(msg_type)
    {
    case INFO:
        sprintf(what,"INFO");
        break;
    case WARN:
        sprintf(what,"WARN");
        break;
    case ERROR:
        sprintf(what,"ERROR");
        break;
    default:
        sprintf(what,"TYPE_UKNOWN");
        break;
    }
    // Write message to the logfile
    fprintf(fp, "[%s] %s: %s", who, what, msg_content);
}