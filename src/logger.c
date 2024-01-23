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
sem_t *sem_logs_1;              // Semphore for logging info and errors
sem_t *sem_logs_2;              // Semphore for logging info and errors
sem_t *sem_logs_3;              // Semphore for logging info and errors
FILE *logfile;               // Pointer to the logfile


int main(int argc, char *argv[]) 
{   
    printf("Yo got here");
    sleep(5);
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   

    // when all shm are created publish your pid to WD
    // publish_pid_to_wd(LOGGER_SYM, getpid());


    shm_logs_fd = shm_open(SHM_LOGS, O_RDWR, 0666);
    ptr_logs = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_logs_fd, 0);

    sem_logs_1 = sem_open(SEM_LOGS_1, 0);
    sem_logs_2 = sem_open(SEM_LOGS_2, 0);
    sem_logs_3 = sem_open(SEM_LOGS_3, 0);

    char path_file[256];
    create_logfile_name(path_file);

    // Create the logfile
    logfile = fopen(path_file, "a");
    if (logfile == NULL)
    {
        perror("Unable to open the log file.");
        exit(1);
    }
    // Inform that you can log the message
    sem_post(sem_logs_1);

    // Main loop
    while (1)
    {
        // Wait for the new message to arrive
        sem_wait(sem_logs_2);

        // Write the message
        write_to_file(logfile, ptr_logs);

        // clear the ptr
        ptr_logs = "";

        // inform that you have finished writing
        sem_post(sem_logs_3);

    }
    
    sem_close(sem_logs_1);         // close all connections
    sem_close(sem_logs_2);         // close all connections
    sem_close(sem_logs_3);

    close(shm_logs_fd);          // close fd
    fclose(logfile);             // Close the file

    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_logs_1);         // close all connections
        sem_close(sem_logs_2);         // close all connections

        sem_unlink(SEM_LOGS_1);        // unlink semaphores
        sem_unlink(SEM_LOGS_2);        // unlink semaphores
        sem_unlink(SEM_LOGS_3);        // unlink semaphores
        
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

void create_logfile_name(char *path_file)
{
    // Get current time
    time_t current_time;
    struct tm *time_info;
    char filename[40];  // Buffer to hold the filename

    time(&current_time);
    time_info = localtime(&current_time);

    // Format time into a string: "logfile_YYYYMMDD_HHMMSS.txt"
    strftime(filename, sizeof(filename), "logfile_%Y%m%d_%H%M%S.txt", time_info);


    char directory[] = "build/logs";
    sprintf(path_file, "%s/%s", directory, filename);
}

void write_to_file(FILE *fp,char *msg)
{
    int process_symbol;
    int msg_type;
    char msg_content[80], who[80], what[80];
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