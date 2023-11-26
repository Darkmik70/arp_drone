#include "interface.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <semaphore.h>
#include <ctype.h>
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
    
    // Initial drone position (middle of the blackboard)
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    // Write initial drone position in its corresponding shared memory
    sprintf((char *)ptr_pos, "%d,%d", droneX, droneY);


    initscr();
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    createBlackboard();


    // Initialize color
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);

    while (1) {
        createBlackboard(); // Redraw blackboard in case the screen changed
        drawDrone(droneX, droneY);
        handleInput((int*) ptr_key, sem_key);
        /* Update drone position */
        //sem_wait(semaphorePos);
        sscanf((char*) ptr_pos, "%d,%d", &droneX, &droneY); // Obtain the values of X,Y from shared memory
        //sem_post(semaphorePos);
        usleep(20000);
        continue;
    }
    
    endwin();

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



void createBlackboard()
{
    // Clear the screen
    clear();

    // Get the dimensions of the terminal window
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);

    // Print a title in the center of the blackboard
    mvprintw(0, (maxX - 11) / 2, "Drone Control");

    // Refresh the screen to apply changes
    refresh();
}

void drawDrone(int droneX, int droneY)
{

    // Draw the center of the cross
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));

    refresh();
}

void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch;

    // Disable echoing
    noecho();

    if ((ch = getch()) != ERR)
    {
        // Debugging: Commented out the print statement
        // printf("Pressed key: %d\n", ch);

        // Store the pressed key in shared memory
        *sharedKey = ch;

        // Signal the semaphore to notify the server
        sem_post(semaphore);
    }

    // Enable echoing
    echo();

    // Clear the input buffer
    flushinp();
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