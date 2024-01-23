#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>


void publish_pid_to_wd(int process_symbol, pid_t pid)
{   
    int shm_wd_fd = shm_open(SHM_WD, O_RDWR, 0666);
    char *ptr_wd = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_wd_fd, 0);
    sem_t *sem_wd_1 = sem_open(SEM_WD_1, 0);    // Protects shm_wd
    sem_t *sem_wd_2 = sem_open(SEM_WD_2, 0);    // Allows WD to use SHM_WD
    sem_t *sem_wd_3 = sem_open(SEM_WD_3, 0);    // WD allows processes to unlock sem_wd_1

    // Wait for signal from WD / other process to give your pid to WD
    sem_wait(sem_wd_1);
    snprintf(ptr_wd, sizeof(ptr_wd), "%i %i", process_symbol, pid);   
    // shm is set, signal WD so it can get the PID
    sem_post(sem_wd_2);
    // Wait for response from WD 
    sem_wait(sem_wd_3);
    // Allow other processes to do that. 
    sem_post(sem_wd_1);
    // PID's published, close your connections
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);
    // Detach from shared memorry
    munmap(ptr_wd,SIZE_SHM);
}

// Pipe functions
void write_to_pipe(int pipe_fd, char message[])
{
    ssize_t bytes_written = write(pipe_fd, message, MSG_LEN );
    if (bytes_written == -1)
    {
        perror("Write went wrong");
        exit(1);
    }
}

// TODO: add to util.h and add comments
void write_message_to_logger(int who, int type, char *msg)
{
    int shm_logs_fd = shm_open(SHM_LOGS, O_RDWR, 0666);
    void *ptr_logs = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_logs_fd, 0);

    sem_t *sem_logs_1 = sem_open(SEM_LOGS_1, 0);
    sem_t *sem_logs_2 = sem_open(SEM_LOGS_2, 0);
    sem_t *sem_logs_3 = sem_open(SEM_LOGS_3, 0);

    sem_wait(sem_logs_1);

    // unsure if it will work
    sprintf(ptr_logs, "%i|%i|%s", who, type, msg);

    // Message is ready to be read
    sem_post(sem_logs_2);

    // wait for logger to finish writing
    sem_wait(sem_logs_3);

    // allow other processes to log their messages
    sem_post(sem_logs_1);

    // Detach from shared memorry
    munmap(ptr_logs,SIZE_SHM);
    close(shm_logs_fd);
    sem_close(sem_logs_1);
    sem_close(sem_logs_2);
    sem_close(sem_logs_3);
}

