#include "constants.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>


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
    // When all is done unlink from shm
}