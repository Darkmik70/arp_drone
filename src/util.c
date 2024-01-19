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

// TODO: Edit according to usage regarding PIPES

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
void write_to_pipe(int pipe_fd, char *message)
{
    ssize_t bytes_written = write(pipe_fd, message, sizeof(message));
    if (bytes_written == -1)
    {
        perror("Write went wrong");
        exit(1);
    }
}

// bool ask_to_write_on_pipe(int pipe)
// {
//     while (1)
//     {
//         if (!in_progress)
//         {
//             printf("Sent write request\n");
//             write(send_pipe, ask_char, strlen(ask_char) + 1);
//             in_progress = 1;
//         }

//     }

// }
// {

//     while (1) 
//     {   
//         // send ask
//         if(!in_progress){
//             printf("Sent write request\n");
//             write(send_pipe, ask_char, strlen(ask_char) + 1);
//             in_progress = 1;
//         }
//         else //wait for reply
//         {
//             read(receive_pipe, read_str, MSG_LEN);
//             if(read_str[0] == 'K')
//             {
//                 send_int = random() % 255;
//                 sprintf(send_str, "%i", send_int);

//                 printf("Acknowledged, sent %i \n", send_int);

//                 write(send_pipe, send_str, strlen(send_str)+1);
//             }
//             else if (read_str[0] == 'R')
//             {
//                 printf("Rejected \n");
//             }
//             in_progress = 0;
//         }
//         sleep(2);
//     } 
//     return 0; 
// } 

// }

