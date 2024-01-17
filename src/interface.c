#include "interface.h"
#include "constants.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>


/* Global variables */
int shm_key_fd;         // File descriptor for key shm
int shm_pos_fd;         // File descriptor for drone pos shm
int *ptr_key;           // Shared memory for Key pressing
char *ptr_pos;          // Shared memory for Drone Position
sem_t *sem_key;         // Semaphore for key presses
sem_t *sem_pos;         // Semaphore for drone positions

// Pipes
int key_pressing_pfd[2];

// Function to find the index of the target with the lowest ID
int findLowestID(Targets *targets, int numTargets) {
    int lowestID = targets[0].id;
    int lowestIndex = 0;

    for (int i = 1; i < numTargets; i++) {
        if (targets[i].id < lowestID) {
            lowestID = targets[i].id;
            lowestIndex = i;
        }
    }

    return lowestIndex;
}

// Function to remove a target at a given index from the array
void removeTarget(Targets *targets, int *numTargets, int indexToRemove) {
    // Shift elements to fill the gap
    for (int i = indexToRemove; i < (*numTargets - 1); i++) {
        targets[i] = targets[i + 1];
    }

    // Decrement the number of targets
    (*numTargets)--;
}

void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles) {
    int totalObstacles;
    sscanf(obstacles_msg, "O[%d]", &totalObstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *numObstacles = 0;

    while (token != NULL && *numObstacles < totalObstacles) {
        sscanf(token, "%d,%d", &obstacles[*numObstacles].x, &obstacles[*numObstacles].y);
        obstacles[*numObstacles].total = *numObstacles + 1;

        token = strtok(NULL, "|");
        (*numObstacles)++;
    }
}

void parseTargetMsg(char *targets_msg, Targets *targets, int *numTargets) {
    char *token = strtok(targets_msg + 4, "|");
    *numTargets = 0;

    while (token != NULL) {
        sscanf(token, "%d,%d", &targets[*numTargets].x, &targets[*numTargets].y);
        targets[*numTargets].id = *numTargets + 1;

        token = strtok(NULL, "|");
        (*numTargets)++;
    }
}

int main(int argc, char *argv[])
{
    get_args(argc, argv);

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

    // TARGETS: Should be obtained from a pipe from server.c
    char targets_msg[] = "T[8]140,23|105,5|62,4|38,6|50,16|6,25|89,34|149,11";
    Targets targets[30];
    int numTargets;
    parseTargetMsg(targets_msg, targets, &numTargets);


    while (1)
    {   
        sem_wait(sem_pos); // Waits for drone process

        // OBSTACLES: Should be obtained from a pipe from server.c
        char obstacles_msg[] = "O[7]35,11|100,5|16,30|88,7|130,40|53,15|60,10";
        Obstacles obstacles[30];
        int numObstacles;
        parseObstaclesMsg(obstacles_msg, obstacles, &numObstacles);

        // Obtain the position values from shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &droneX, &droneY, &maxX, &maxY); 

        // UPDATE THE TARGETS ONCE THE DRONE REACHES THE CURRENT LOWEST NUMBER 
        // ...

        // Find the index of the target with the lowest ID
        int lowestIndex = findLowestID(targets, numTargets);
        // Check if the coordinates of the lowest ID target match the drone coordinates
        if (targets[lowestIndex].x == droneX && targets[lowestIndex].y == droneY) {
            // Remove the target with the lowest ID
            removeTarget(targets, &numTargets, lowestIndex);
        }

        // Draws the window with the updated information of the terminal size, drone, targets and obstacles
        draw_window(maxX, maxY, droneX, droneY, targets, numTargets, obstacles, numObstacles);
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

void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &key_pressing_pfd[0], &key_pressing_pfd[1]);
}

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

void draw_window(int maxX, int maxY, int droneX, int droneY, Targets *targets, int numTargets,
                 Obstacles *obstacles, int numObstacles) {
    clear();

    /* get dimensions of the screen */
    int new_maxY, new_maxX;
    getmaxyx(stdscr, new_maxY, new_maxX);
    box(stdscr, 0, 0);  // Draw a rectangular border using the box function

    // Print a title in the top center part of the window
    mvprintw(0, (new_maxX - 11) / 2, "Drone Control");

    // Draw a plus sign to represent the drone
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));

    // Draw obstacles on the board
    for (int i = 0; i < numObstacles; i++) {
        mvaddch(obstacles[i].y, obstacles[i].x, 'O' ); // Assuming 'O' represents obstacles
    }

    // Draw targets on the board
    for (int i = 0; i < numTargets; i++) {
        mvaddch(targets[i].y, targets[i].x, targets[i].id + '0');
    }

    refresh(); // Apply changes
}


void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch;
    if ((ch = getch()) != ERR)
    {
        // write char to the pipe
        char msg[MSG_LEN];
        sprintf(msg, "%c", ch);

        write_to_pipe(key_pressing_pfd[1], msg);
        
        *sharedKey = ch;    // Store the pressed key in shared memory
    }
    // echo(); // Re-enable echoing
    flushinp(); // Clear the input buffer
}