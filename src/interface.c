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
int shm_pos_fd;         // File descriptor for drone pos shm
char *ptr_pos;          // Shared memory for Drone Position
sem_t *sem_pos;         // Semaphore for drone positions

// Serverless pipes
int key_press_fd[2];

// Pipes working with the server
int interface_server[2];

int main(int argc, char *argv[])
{
    get_args(argc, argv);

    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);  
    sigaction (SIGUSR1, &sa, NULL);    
  
    publish_pid_to_wd(WINDOW_SYM, getpid());

    // DELETE: Everything related to shared memory and semaphores
    // Shared memory for DRONE POSITION
    shm_pos_fd = shm_open(SHM_POS, O_RDWR, 0666);
    ptr_pos = mmap(0, SIZE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, shm_pos_fd, 0);
    // Semaphores open
    sem_pos = sem_open(SEM_POS, 0);

    /* INITIALIZATION AND EXECUTION OF NCURSES FUNCTIONS */
    initscr();
    timeout(0);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone color
    noecho(); // Disable echoing

    /* Set initial drone position */
    // Obtain the screen dimensions
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX); 

    // Initial drone position will be the middle of the screen.
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    // DELETE: Write initial drone position in its corresponding shared memory
    sprintf(ptr_pos, "%d,%d,%d,%d", droneX, droneY, maxX, maxY);
    
    // TODO: Write the position data into a pipe to be read from (drone.c)
    // *

    char initial_msg[MSG_LEN];
    //sprintf(initial_msg, "I1:%d,%d,%d,%d", droneX, droneY, maxX, maxY);
    sprintf(initial_msg, "123456789012345");
    write_to_pipe(interface_server[1], initial_msg);


    // TODO: Should be obtained from a pipe from (server.c), that gets it from (targets.c)
    char targets_msg[] = "T[8]140,23|105,5|62,4|38,6|50,16|6,25|89,34|149,11";
    Targets targets[30];
    int numTargets;
    parseTargetMsg(targets_msg, targets, &numTargets);

    // For game score calculation
    int score = 0;
    int counter = 0;

    while (1)
    {   
        sem_wait(sem_pos); // Waits for drone process

        // TODO: Should be obtained from a pipe from (server.c), that gets it from (obstacles.c)
        char obstacles_msg[] = "O[7]35,11|100,5|16,30|88,7|130,40|53,15|60,10";
        Obstacles obstacles[30];
        int numObstacles;
        parseObstaclesMsg(obstacles_msg, obstacles, &numObstacles);

        // DELETE: Obtain the position values from shared memory
        sscanf(ptr_pos, "%d,%d,%d,%d", &droneX, &droneY, &maxX, &maxY);
        // TODO: Obtain this values from a pipe from (drone.c)
        // *

        /* UPDATE THE TARGETS ONCE THE DRONE REACHES THE CURRENT LOWEST NUMBER */
        // Find the index of the target with the lowest ID
        int lowestIndex = findLowestID(targets, numTargets);
        // Obtain the coordinates of that target
        char lowestTarget[20];
        sprintf(lowestTarget, "%d,%d", targets[lowestIndex].x, targets[lowestIndex].y);
        // TODO: send this string called 'lowestTarget' to (drone.c)
        // *


        // Check if the coordinates of the lowest ID target match the drone coordinates
        if (targets[lowestIndex].x == droneX && targets[lowestIndex].y == droneY) {
            // Update score and counter (reset timer)
            if (counter > 400) {  // 400 * 50ms = 20,000ms = 20 seconds 
                score += 2;
            }
            else if (counter > 200) {  // 200 * 50ms = 10,000ms = 10 seconds 
                score += 6;
            }
            else if (counter > 100) {  // 100 * 50ms = 5000ms = 5 seconds 
                score += 8;
            }
            else {  // Les than 5 seconds
                score += 10;
            }
            counter = 0;
            // Remove the target with the lowest ID
            removeTarget(targets, &numTargets, lowestIndex);
        }

        // Check if the drone has crashed into an obstacle
        if (isDroneAtObstacle(obstacles, numObstacles, droneX, droneY)) {
            // Update score
            score -= 5;
        }

        // Create a string for the player's current score
        char score_msg[50];
        sprintf(score_msg, "Your current score: %d", score);
    
        // Draws the window with the updated information of the terminal size, drone, targets, obstacles and score.
        draw_window(droneX, droneY, targets, numTargets, obstacles, numObstacles, score_msg);

        /* HANDLE THE KEY PRESSED BY THE USER */
        int ch;
        if ((ch = getch()) != ERR)
        {
            // Write char to the pipe
            char msg[MSG_LEN];
            sprintf(msg, "%c", ch);
            write_to_pipe(key_press_fd[1], msg);
        }
        flushinp(); // Clear the input buffer

        // Timer that is linked to score calculations, and ncurses window stability.
        counter++;
        usleep(50000);

    }

    /* cleanup */
    endwin(); 

    // DELETE: Everything related to semaphores and shared memory
    /* Close shared memories and semaphores */
    close(shm_pos_fd);
    sem_close(sem_pos);

    return 0;
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &key_press_fd[1], &interface_server[1]);
}


// Function to find the index of the target with the lowest ID.
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


// Function to remove a target at a given index from the array.
void removeTarget(Targets *targets, int *numTargets, int indexToRemove)
{
    // Shift elements to fill the gap
    for (int i = indexToRemove; i < (*numTargets - 1); i++)
    {
        targets[i] = targets[i + 1];
    }

    // Decrement the number of targets
    (*numTargets)--;
}


// Function to extract the coordinates from the obstacles_msg string into the struct Obstacles.
void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles)
{
    int totalObstacles;
    sscanf(obstacles_msg, "O[%d]", &totalObstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *numObstacles = 0;

    while (token != NULL && *numObstacles < totalObstacles)
    {
        sscanf(token, "%d,%d", &obstacles[*numObstacles].x, &obstacles[*numObstacles].y);
        obstacles[*numObstacles].total = *numObstacles + 1;

        token = strtok(NULL, "|");
        (*numObstacles)++;
    }
}


// Function to extract the coordinates from the targets_msg string into the struct Targets.
void parseTargetMsg(char *targets_msg, Targets *targets, int *numTargets)
{
    char *token = strtok(targets_msg + 4, "|");
    *numTargets = 0;

    while (token != NULL)
    {
        sscanf(token, "%d,%d", &targets[*numTargets].x, &targets[*numTargets].y);
        targets[*numTargets].id = *numTargets + 1;

        token = strtok(NULL, "|");
        (*numTargets)++;
    }
}


// Function to check if drone is at the same coordinates as any obstacle.
int isDroneAtObstacle(Obstacles obstacles[], int numObstacles, int droneX, int droneY)
{
    for (int i = 0; i < numObstacles; i++) {
        if (obstacles[i].x == droneX && obstacles[i].y == droneY)
        {
            return 1;  // Drone is at the same coordinates as an obstacle
        }
    }
    return 0;  // Drone is not at the same coordinates as any obstacle
}

void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf(" Received signal number: %d \n", signo);
    if ( signo == SIGINT)
    {
        printf("Caught SIGINT \n");
        // close all semaphores
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

void draw_window(int droneX, int droneY, Targets *targets, int numTargets,
                 Obstacles *obstacles, int numObstacles, const char *score_msg)
{
    clear();

    /* get dimensions of the screen */
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    box(stdscr, 0, 0);  // Draw a rectangular border using the box function
    refresh();

    // Print a title in the top center part of the window
    mvprintw(0, (max_x - 11) / 2, "%s", score_msg);

    // Draw a plus sign to represent the drone
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));
    refresh();

    // Draw obstacles on the board
    for (int i = 0; i < numObstacles; i++) {
        mvaddch(obstacles[i].y, obstacles[i].x, 'O' ); // Assuming 'O' represents obstacles
    }
    refresh();

    // Draw targets on the board
    for (int i = 0; i < numTargets; i++) {
        mvaddch(targets[i].y, targets[i].x, targets[i].id + '0');
    }
    refresh(); // Always refresh when adding or changing the window.
}
