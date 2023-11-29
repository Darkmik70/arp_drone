#include "server.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#include <signal.h>

// GLOBAL VARIABLES
// Attach to shared memory for key presses
void *ptr_wd;                           // Shared memory for WD
void *ptr_key;                          // Shared memory for Key pressing
void *ptr_pos;                          // Shared memory for Drone Position
void *ptr_action;                       // Shared memory ptr for actions
sem_t *sem_key;                         // Semaphore for key presses
sem_t *sem_pos;                         // Semaphore for drone positions
sem_t *sem_action;                      // Semaphore for actions
sem_t *sem_wd_1, *sem_wd_2, *sem_wd_3;  // Semaphores for watchdog


void signal_handler(int signo)
{
    printf("Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        printf("Caught SIGINT\n");
        clean_up();
        sleep(5);
        exit(1);
    }
}



int main() 
{   
    struct sigaction sa;
    sa.sa_handler = signal_handler; 
    sigaction (SIGINT, &sa, NULL);    

    // Shared memory and semaphores for WATCHDOG
    ptr_wd = create_shm(SHM_WD);
    sem_wd_1 = sem_open(SEM_WD_1, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_1 == SEM_FAILED) {
        perror("sem_wd_1 failed");
        exit(1);
    }
    sem_wd_2 = sem_open(SEM_WD_2, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_2 == SEM_FAILED) {
        perror("sem_wd_2 failed");
        exit(1);
    }
    sem_wd_3 = sem_open(SEM_WD_3, O_CREAT, S_IRUSR | S_IWUSR, 0); // 0 for locked, this semaphore is unlocked by WD in order to get pids
    if (sem_wd_3 == SEM_FAILED) {
        perror("sem_wd_2 failed");
        exit(1);
    }

    // Shared memory for KEY PRESSING
    ptr_key = create_shm(SHM_KEY);
    sem_key = sem_open(SEM_KEY, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_key == SEM_FAILED) {
        perror("sem_key failed");
        exit(1);
    }
    // Shared memory for DRONE POSITION
    ptr_pos = create_shm(SHM_POS);
    sem_pos = sem_open(SEM_POS, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_pos == SEM_FAILED) {
        perror("sem_pos failed");
        exit(1);
    }
    // Shared memory for DRONE ACTION
    ptr_action = create_shm(SHM_ACTION);
    sem_action = sem_open(SEM_ACTION, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_pos == SEM_FAILED) {
        perror("sem_pos failed");
        exit(1);
    }

    // when all shm are created publish your pid to WD
    publish_pid_to_wd(SERVER_SYM, getpid());

    // Main loop
    while (1)
    {
        usleep(50000);  
    }

    
    // Close and unlink the semaphores
    clean_up();
    return 0;
}


void *create_shm(char *name)
{
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE_SHM);
    /* memory map the shared memory object */
    void *shm_ptr = mmap(0, SIZE_SHM, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        perror("Map Failed");
        exit(1);
    }
    // File descriptor is no longer needed
    close(shm_fd);

    return shm_ptr;
}

void clean_up()
{
    // close all connections
    sem_close(sem_key);
    sem_close(sem_pos);
    sem_close(sem_action);
    sem_close(sem_wd_1);
    sem_close(sem_wd_2);
    sem_close(sem_wd_3);

    // unlink semaphores
    sem_unlink(SEM_KEY);
    sem_unlink(SEM_POS);
    sem_unlink(SEM_ACTION);
    sem_unlink(SEM_WD_1);
    sem_unlink(SEM_WD_2);
    sem_unlink(SEM_WD_3);

    // unmap shared memory
    munmap(ptr_wd, SIZE_SHM);
    munmap(ptr_key, SIZE_SHM);
    munmap(ptr_pos, SIZE_SHM);
    munmap(ptr_action, SIZE_SHM);

    // unlink shared memories
    shm_unlink(SHM_WD);
    shm_unlink(SHM_KEY);
    shm_unlink(SHM_POS);
    shm_unlink(SHM_ACTION);

    printf("Clean up has been performed succesfully\n");
}
