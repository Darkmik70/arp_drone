#include "interface.h"
#include "constants.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <ctype.h>
#include <fcntl.h>


int main()
{
    void *ptr_key;        // Shared memory for Key pressing
    void *ptr_pos;        // Shared memory for Drone Position      
    sem_t *sem_key;       // Semaphore for key presses
    sem_t *sem_pos;       // Semaphore for drone positions

    ptr_key = access_shm_rdwr(SHM_KEY);
    ptr_pos = access_shm_rdwr(SHM_POS);

    sem_key = sem_open(SEM_KEY, 0);
    sem_pos = sem_open(SEM_POS, 0);

    // Initial drone position (middle of the blackboard)
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    int droneX = maxX / 2;
    int droneY = maxY / 2;

    initscr();
    timeout(0); // Set non-blocking getch
    curs_set(0); // Hide the cursor from the terminal
    noecho(); // Disable echoing
    
    draw_window();
    refresh(); // Refresh the screen to apply changes


    // Write initial drone position in its corresponding shared memory
    int dronePosition[2];
    dronePosition[0] = droneX;
    dronePosition[1] = droneY;
    set_drone_pos((int*)ptr_pos, dronePosition);
    

    // Initialize colors
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Obstacle
    init_pair(3, COLOR_GREEN, COLOR_BLACK); // Target


    while (1)
    {
        draw_window();
        // TODO: For all objects in objects
        drawObject(dronePosition[0], dronePosition[1]);
        // When all the objects are drawn
        refresh();

        handleInput(sharedMemory, sem_pos);

        //Update drone position
        //sem_wait(semaphorePos);
        dronePosition[0] = sharedPosition[0];
        dronePosition[1] = sharedPosition[1];
        //sem_post(semaphorePos);

        //TODO: CHANGE TO NANOSLEEP
        usleep(REFRESH_RATE);
    }
    endwin();


    if (shm_unlink(SHM_KEY) == 1)
    {
        printf("Error removing %s\n", SHM_KEY);
        exit(1);
    }
        if (shm_unlink(SHM_POS) == 1)
    {
        printf("Error removing %s\n", SHM_POS);
        exit(1);
    }
    sem_close(sem_key);
    sem_unlink(SEM_KEY);
    sem_close(sem_pos);
    sem_unlink(SEM_POS);

    return 0;
}


void draw_window()
{
    clear();
    // Get the dimensions of the terminal window
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);

    // Draw a rectangular border using the box function
    box(stdscr, 0, 0);

    // Print a title in the center of the blackboard
    mvprintw(0, (maxX - 11) / 2, "Drone Control");
}

void drawObject(struct Object *ob)
{
    switch (ob->type)
    {
    case DRONE:
        mvaddch(ob->pos_y, ob->pos_x, '+' | COLOR_PAIR(1));
        break;
    case OBSTACLE:
        mvaddch(ob->pos_y, ob->pos_x, '1' | COLOR_PAIR(2));
        break;
    case TARGET:
        mvaddch(ob->pos_y, ob->pos_x, ob->sym | COLOR_PAIR(3));
        break;
    }
}

void handleInput(int *sharedKey, sem_t *semaphore)
{
    int ch = getch();
    if (ch == -1)
    {
        perror("Error in getch()!");
    }
    else
    {
        // Store the pressed key in shared memory
        *sharedKey = ch;
        // Signal the semaphore to notify the server
        sem_post(semaphore);
    }
    // Clear the input buffer
    flushinp();
}

void set_drone_pos(int *sharedPos, int dronePosition[2]) {

    //sem_wait(semaphorePos);
    sharedPos[0] = dronePosition[0];
    sharedPos[1] = dronePosition[1];
    //sem_post(semaphorePos);
}

void *access_shm_rdonly(char *name)
{
    const int SIZE = 4096;
    /* get semaphores and shared memories */
    int shm_fd = shm_open(name, O_RDONLY, 0666);
    if (shm_fd == 1)
    {
        printf("Shared memory segment failed\n");
        exit(1);
    }
    
    // Memory-map the shared memory segment with read permissions
    void *ptr_key = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr_key == MAP_FAILED)
    {
        printf("Map failed\n");
        return 1;
    }
    return ptr_key;
}

void *access_shm_rdwr(char *name)
{
    const int SIZE = 4096;
    /* get semaphores and shared memories */
    int shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd == 1)
    {
        printf("Shared memory segment failed\n");
        exit(1);
    }
    
    // Memory-map the shared memory segment with read-write permissions
    void *ptr_key = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr_key == MAP_FAILED)
    {
        printf("Map failed\n");
        return 1;
    }
    return ptr_key;
}

