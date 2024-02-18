#include "interface.h"
#include "common.h"
#include <ncurses.h>

// PIPES: File descriptor arrays
int interface_km[2];  // Serverless pipe
int interface_drone[2];  // Serverless pipe
int server_interface[2];
int interface_server[2];


int main(int argc, char *argv[])
{
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    publish_pid_to_wd(WINDOW_SYM, getpid());

    //////////////////////////////////////////////////////
    /* INITIALIZATION OF NCURSES FUNCTIONS */
    /////////////////////////////////////////////////////

    initscr();
    timeout(0);
    curs_set(0);
    noecho();                          
    // Enable colors
    start_color();                         
    init_pair(1, COLOR_BLUE, COLOR_BLACK); // Drone is blue
    init_pair(2, COLOR_RED, COLOR_BLACK);  // Obstacles are red
    init_color(COLOR_YELLOW, 1000, 647, 0);  // Define an orange color
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Obstacles are orange
    init_pair(3, COLOR_GREEN, COLOR_BLACK); // Targets are green

    //////////////////////////////////////////////////////
    /* SET THE INITIAL DRONE POSITION */
    /////////////////////////////////////////////////////

    // Obtain the screen dimensions
    int screen_size_y;
    int screen_size_x;
    getmaxyx(stdscr, screen_size_y, screen_size_x);
    // Initial drone position will be the middle of the screen.
    int droneX = screen_size_x / 2;
    int droneY = screen_size_y / 2;
    // Write the initial position and screen size data into the server
    char initial_msg[MSG_LEN];
    sprintf(initial_msg, "I1:%d,%d,%d,%d", droneX, droneY, screen_size_x, screen_size_y);
    write_to_pipe(interface_server[1], initial_msg);

    //////////////////////////////////////////////////////
    /* VARIABLE INITIALIZATION */
    /////////////////////////////////////////////////////

    // To compare current and previous data
    int iteration = 0;
    int obtained_targets = 0;
    int obtained_obstacles = 0;
    int prev_screen_size_y = 0;
    int prev_screen_size_x = 0;
    int original_screen_size_x;
    int original_screen_size_y;
    // Game logic
    char score_msg[MSG_LEN];
    int score = 0;
    int counter = 0;
    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    // Targets and obstacles
    Targets targets[80];
    Targets original_targets[80];
    int numTargets;
    Obstacles obstacles[80];
    int numObstacles;


    while (1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: SCREEN DIMENSIONS: SEND & RE-SCALE */
        /////////////////////////////////////////////////////

        getmaxyx(stdscr, screen_size_y, screen_size_x);

        /*SEND x,y  to server ONLY when screen size changes*/
        if (screen_size_x != prev_screen_size_x || screen_size_y != prev_screen_size_y)
        {
            // To avoid division by 0 on first iteration
            if (iteration == 0)
            {
                prev_screen_size_x = screen_size_x;
                prev_screen_size_y = screen_size_y;
                original_screen_size_x = screen_size_x;
                original_screen_size_y = screen_size_y;
                iteration++;
            }
            // Send data
            char screen_msg[MSG_LEN];
            sprintf(screen_msg, "I2:%.3f,%.3f", (float)screen_size_x, (float)screen_size_y);
            write_to_pipe(interface_server[1], screen_msg);
            // Re-scale the targets on the screen
            if (iteration > 0 && obtained_targets == 1)
            {
                float scale_x = (float)screen_size_x / (float)original_screen_size_x;
                float scale_y = (float)screen_size_y / (float)original_screen_size_y;
                for (int i = 1; i < numTargets; i++)
                {
                    targets[i].x = (int)((float)original_targets[i].x * scale_x);
                    targets[i].y = (int)((float)original_targets[i].y * scale_y);
                }
            }
            // Update previous values
            prev_screen_size_x = screen_size_x;
            prev_screen_size_y = screen_size_y;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER*/
        /////////////////////////////////////////////////////

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_interface[0], &read_fds);

        char server_msg[MSG_LEN];

        int ready;
        do
        {
            ready = select(server_interface[0] + 1, &read_fds, NULL, NULL, NULL);
            if (ready == -1)
            {
                perror("Error in select");
            }
        } while (ready == -1 && errno == EINTR);

        ssize_t bytes_read_drone = read(server_interface[0], server_msg, MSG_LEN);
        if (bytes_read_drone > 0)
        {
            if (server_msg[0] == 'D')
            {
                sscanf(server_msg, "D:%d,%d", &droneX, &droneY);
            }
            else if (server_msg[0] == 'O')
            {
                parseObstaclesMsg(server_msg, obstacles, &numObstacles);
                obtained_obstacles = 1;
            }
            else if (server_msg[0] == 'T')
            {
                parseTargetMsg(server_msg, targets, &numTargets, original_targets);
                obtained_targets = 1;
            }
        }

        if (obtained_targets == 0 && obtained_obstacles == 0)
        {
            continue;
        }

        printf("TARGETS OBTAINED. NOW TO SECTION 3!");
        //////////////////////////////////////////////////////
        /* SECTION 3: DATA ANALYSIS & GAME SCORE CALCULATION*/
        /////////////////////////////////////////////////////

        /* UPDATE THE TARGETS ONCE THE DRONE REACHES THE CURRENT LOWEST NUMBER */
        // Find the index of the target with the lowest ID
        int lowestIndex = findLowestID(targets, numTargets);

        // When there are no more targets, the game is finished.
        if (numTargets == 0)
        {
            score = 0;
            obtained_targets = 0;
            write_to_pipe(interface_server[1], "GE");
            continue;
        }
        // Obtain the coordinates of that target
        char lowestTarget[20];
        sprintf(lowestTarget, "%d,%d", targets[lowestIndex].x, targets[lowestIndex].y);
        // Send to drone w/ serverless pipe lowest_target
        write_to_pipe(interface_drone[1], lowestTarget);

        // Check if the coordinates of the lowest ID target match the drone coordinates
        if (targets[lowestIndex].x == droneX && targets[lowestIndex].y == droneY)
        {
            // Update score and counter (reset timer)
            if (counter > 1500)
            { // 1500 * 10ms = 15 seconds
                score += 2;
            }
            else if (counter > 1000)
            { // 1000 * 10ms = 10 seconds
                score += 4;
            }
            else if (counter > 500)
            { // 500 * 10ms = 5 seconds
                score += 6;
            }
            else
            { // Les than 5 seconds
                score += 10;
            }
            counter = 0;
            // Remove the target with the lowest ID
            removeTarget(targets, &numTargets, lowestIndex);
        }

        // Check if the drone has crashed into an obstacle
        if (isDroneAtObstacle(obstacles, numObstacles, droneX, droneY))
        {
            // Update score
            score -= 5;
        }

        //////////////////////////////////////////////////////
        /* SECTION 4: DRAW THE WINDOW & OBTAIN INPUT*/
        /////////////////////////////////////////////////////

        // Create a string for the player's current score
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
            // Send it directly to key_manager.c
            write_to_pipe(interface_km[1], msg);
        }
        flushinp(); // Clear the input buffer

        // Counter/Timer linked to score calculations
        counter++;
        usleep(10000);
    }

    /* cleanup */
    endwin();

    return 0;
}

// Function to find the index of the target with the lowest ID.
int findLowestID(Targets *targets, int numTargets)
{
    int lowestID = targets[0].id;
    int lowestIndex = 0;
    for (int i = 1; i < numTargets; i++)
    {
        if (targets[i].id < lowestID)
        {
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

// Function to interpret the string of obstacles
void parseObstaclesMsg(char *obstacles_msg, Obstacles *obstacles, int *numObstacles)
{
    int totalObstacles;
    sscanf(obstacles_msg, "O[%d]", &totalObstacles);

    char *token = strtok(obstacles_msg + 4, "|");
    *numObstacles = 0;

    while (token != NULL && *numObstacles < totalObstacles)
    {
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);

        // Convert float to int (rounding is acceptable)
        obstacles[*numObstacles].x = (int)(x_float + 0.5);
        obstacles[*numObstacles].y = (int)(y_float + 0.5);

        obstacles[*numObstacles].total = *numObstacles + 1;

        token = strtok(NULL, "|");
        (*numObstacles)++;
    }
}

// Function to interpret the string of targets
void parseTargetMsg(char *targets_msg, Targets *targets, int *numTargets, Targets *original_targets)
{
    char *token = strtok(targets_msg + 4, "|");
    *numTargets = 0;

    while (token != NULL)
    {
        float x_float, y_float;
        sscanf(token, "%f,%f", &x_float, &y_float);

        // Convert float to int (rounding is acceptable)
        targets[*numTargets].x = (int)(x_float + 0.5);
        targets[*numTargets].y = (int)(y_float + 0.5);
        targets[*numTargets].id = *numTargets + 1;

        // Save original values
        original_targets[*numTargets].x = targets[*numTargets].x;
        original_targets[*numTargets].y = targets[*numTargets].y;
        original_targets[*numTargets].id = targets[*numTargets].id;

        // Handle token
        token = strtok(NULL, "|");
        (*numTargets)++;
    }
}

// Function to determine wether the drone is on top of an obstacle
int isDroneAtObstacle(Obstacles obstacles[], int numObstacles, int droneX, int droneY)
{
    for (int i = 0; i < numObstacles; i++)
    {
        if (obstacles[i].x == droneX && obstacles[i].y == droneY)
        {
            return 1; // Drone is at the same coordinates as an obstacle
        }
    }
    return 0; // Drone is not at the same coordinates as any obstacle
}

// Function to handle signals within the process
void signal_handler(int signo, siginfo_t *siginfo, void *context)
{
    // printf(" Received signal number: %d \n", signo);
    if (signo == SIGINT)
    {
        printf("Caught SIGINT \n");

        // Close file desciptors
        close(interface_km[1]);
        close(interface_drone[1]);
        close(server_interface[0]);
        close(interface_server[1]);

        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // TODO REFRESH SCREEN
        //  Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}

// Function to draw the regular game window
void draw_window(int droneX, int droneY, Targets *targets, int numTargets,
                 Obstacles *obstacles, int numObstacles, const char *score_msg)
{
    clear();
    /* get dimensions of the screen */
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    // Draw border on the screen
    box(stdscr, 0, 0);
    // Print a title in the top center part of the window
    mvprintw(0, (max_x - 11) / 2, "%s", score_msg);
    // Draw a plus sign to represent the drone
    mvaddch(droneY, droneX, '+' | COLOR_PAIR(1));

    // Draw  targets and obstacles on the board
    for (int i = 0; i < numObstacles; i++)
    {
        mvaddch(obstacles[i].y, obstacles[i].x, '*' | COLOR_PAIR(2)); // Assuming 'O' represents obstacles
    }
    for (int i = 0; i < numTargets; i++)
    {
        mvaddch(targets[i].y, targets[i].x, targets[i].id + '0' | COLOR_PAIR(3));
    }
    refresh();
}

// Function to obtain the file descriptors for all pipes
void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %d %d", &interface_km[1], &server_interface[0],
           &interface_server[1], &interface_drone[1]);
}
