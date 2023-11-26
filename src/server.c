#include "server.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>


int main() 
{   
    // Attach to shared memory for key presses
    void *ptr_key;        // Shared memory for Key pressing
    void *ptr_pos;        // Shared memory for Drone Position      
    void *ptr_action;     // Shared memory ptr for actions
    sem_t *sem_key;       // Semaphore for key presses
    sem_t *sem_pos;       // Semaphore for drone positions
    sem_t *sem_action;    // Semaphore for actions

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


    // Main loop
    while (1)
    {
        /* SERVER HANDLING OF KEY PRESSED */
        // sem_wait(semaphore);
        // int pressedKey = *sharedMemory;
        // printf("Server: Received key: %d\n", pressedKey);

        // TODO: Handle the received key or dispatch it to other processes
        // Here goes RESET
        // Here goes SUSPEND
        // Here goes QUIT

        // Clear the shared memory after processing the key
        // *sharedMemory = NO_KEY_PRESSED;

        /*SERVER HANDLING OF DRONE POSITION*/
        // sem_wait(semaphorePos);
        // int droneX = sharedPosition[0];
        // int droneY = sharedPosition[1];
        // sem_post(semaphorePos);

        // TODO: Logic for handling drone positions...

        usleep(50000);  // Delay for testing purposes
    }

    // Detach the shared memory segments
    shmdt(SHM_KEY);
    shmdt(SHM_POS);
    
    // Close and unlink the semaphores
    sem_close(sem_key);
    sem_close(sem_pos);
    sem_close(sem_action);
    sem_unlink(SEM_KEY);
    sem_unlink(SEM_POS);
    sem_unlink(SEM_ACTION);
    return 0;
}

void *create_shm(char *name)
{
    const int SIZE = 4096;  // the size (in bytes) of shared memory object

    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open of shm_key");
        exit(1);
    }
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE);

    /* memory map the shared memory object */
    void *shm_ptr = mmap(0, SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0); 
    if (shm_ptr == MAP_FAILED)
    {
        perror("Map Failed");
        exit(1);
    }
    return shm_ptr;
}
