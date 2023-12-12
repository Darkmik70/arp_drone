#include "interface.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>
// #include <ctype.h>


/* Global variables */
int shm_key_fd;         // File descriptor for key shm
int shm_pos_fd;         // File descriptor for drone pos shm
int *ptr_key;           // Shared memory for Key pressing
char *ptr_pos;          // Shared memory for Drone Position
sem_t *sem_key;         // Semaphore for key presses
sem_t *sem_pos;         // Semaphore for drone positions



void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
        sem_close(sem_key);
        sem_close(sem_pos);

        printf("Succesfully closed all semaphores\n");
        sleep(10);
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

int main()
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);  
    sigaction (SIGUSR1, &sa, NULL);    
  
    publish_pid_to_wd(WINDOW_SYM, getpid());

    // Shared memory for KEY PRESSING
    shm_key_fd = shm_open(SHM_KEY, O_RDWR, 0666);
    ptr_key = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_key_fd, 0);

    // Shared memory for DRONE POSITION
    shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);

    sem_key = sem_open(SEM_KEY, 0);
    sem_pos = sem_open(SEM_POS, 0);

    
    /* INITIALIZATION AND EXECUTION OF NCURSES FUNCTIONS */
    initscr();
    timeout(0);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone color
    noecho(); // Disable echoing

    /* Set initial drone position */
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2; // Middle of the screen
    int droneY = maxY / 2;
    // Write initial drone position in its corresponding shared memory
    sprintf(ptr_pos, "%d,%d,%d,%d", droneX, droneY, maxX, maxY);


    while (1)
    {
        sem_wait(sem_pos); // Waits for drone process
        
        // Obtain the position values from shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &droneX, &droneY, &maxX, &maxY);  

        createWindow(maxX, maxY);
        drawDrone(droneX, droneY);
        handleInput(ptr_key, sem_key);

        sem_post(sem_key);    // unlocks to allow keyboard manager
    }

    /* cleanup */
    endwin(); 

    /* Close shared memories and semaphores */
    close(shm_key_fd);
    close(shm_pos_fd);
    sem_close(sem_key);
    sem_close(sem_pos);

    return 0;
}



void createWindow(int maxX, int maxY)
{
    clear();

    /* get dimensions of the screen */ // Dynamical window frame
    int new_maxY, new_maxX;
    getmaxyx(stdscr, new_maxY, new_maxX);
    box(stdscr, 0, 0);  // Draw a rectangular border using the box function
   
    // Print a title in the top center part of the window
    mvprintw(0, (new_maxX - 11) / 2, "Drone Control"); 

    refresh(); // Apply changes 
}

void drawDrone(int droneX, int droneY)
{
    // Draw a plus sign to represent the drone
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));
    refresh();
}

void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch;
    if ((ch = getch()) != ERR)
    {
        *sharedKey = ch;    // Store the pressed key in shared memory
    }
    // echo(); // Re-enable echoing
    flushinp(); // Clear the input buffer
}