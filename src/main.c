#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>


// Serverless pipes (fd)
int key_press_fd[2];

// New pipes working with server (fd)
int km_server[2];
int server_drone[2];
int interface_server[2];
int drone_server[2];

// TODO: Only execute in Konsole the interface.c and the drone.c for monitoring of force, pos, vel.

int main(int argc, char *argv[])
{
    // PIDs
    pid_t server_pid;
    pid_t window_pid;
    pid_t km_pid;
    pid_t drone_pid;
    pid_t wd_pid;
    pid_t logger_pid;
    pid_t targets_pid;
    pid_t obstacles_pid;

    // Serverless pipe creation
    if (pipe(key_press_fd) == -1) {perror("pipe"); exit(EXIT_FAILURE);}

    // Pipe creation
    if (pipe(km_server) == -1) {perror("pipe"); exit(EXIT_FAILURE);}
    if (pipe(server_drone) == -1) {perror("pipe"); exit(EXIT_FAILURE);}
    if (pipe(interface_server) == -1) {perror("pipe"); exit(EXIT_FAILURE);}

    // Passing file descriptors for pipes used on key_manager.c
    char key_manager_fds[10];
    sprintf(key_manager_fds, "%d %d", key_press_fd[0], km_server[1]);

    // Passing file descriptors for pipes used on interface.c
    char interface_fds[10];
    sprintf(interface_fds, "%d %d", key_press_fd[1], interface_server[1]);

    // Passing file descriptors for pipes used on drone.c
    char drone_fds[10];
    sprintf(drone_fds, "%d", server_drone[0]);

    // Passing file descriptors for pipes used on server.c
    char server_fds[10];
    sprintf(server_fds, "%d %d %d", km_server[0], server_drone[1], interface_server[0]);


    int delay = 100000; // Time delay between next spawns
    int p_num = 0;  // number of processes


    /* Server */
    char* server_args[] = {"konsole", "-e", "./build/server", server_fds, NULL};
    server_pid = create_child(server_args[0], server_args);
    p_num++;
    usleep(delay*10); // little bit more time for server

    // /* Logger */ 
    // char* logger_args[] = {"konsole", "-e", "./build/logger", NULL};
    // logger_pid = create_child(logger_args[0], logger_args);
    // p_num++;
    // usleep(delay);

    /* Targets */
    char* targets_args[] = {"konsole", "-e", "./build/targets", NULL};
    targets_pid = create_child(targets_args[0], targets_args);
    p_num++;
    usleep(delay);

    /* Obstacles */
    char* obstacles_args[] = {"konsole", "-e", "./build/obstacles", NULL};
    obstacles_pid = create_child(obstacles_args[0], obstacles_args);
    p_num++;
    usleep(delay);

    /* Keyboard manager */
    char* km_args[] = {"konsole", "-e", "./build/key_manager", key_manager_fds, NULL};
    km_pid = create_child(km_args[0], km_args);
    p_num++;
    usleep(delay);

    /* Drone */
    char* drone_args[] = {"konsole", "-e", "./build/drone", drone_fds, NULL};
    drone_pid = create_child(drone_args[0], drone_args);
    p_num++;
    usleep(delay);

    /* Watchdog */
    char* wd_args[] = {"konsole", "-e", "./build/watchdog", NULL};
    wd_pid = create_child(wd_args[0], wd_args);
    p_num++;
    printf("Watchdog Created\n");

    /* Window - Interface */
    char* window_args[] = {"konsole", "-e", "./build/interface", interface_fds, NULL};
    window_pid = create_child(window_args[0], window_args);
    p_num++;
    usleep(delay);

    /* Wait for all children to close */
    for (int i = 0; i < p_num; i++)
    {
        int pid, status;
        pid = wait(&status);

        if (WIFEXITED(status))
        {
            printf("Child process with PID: %i terminated with exit status: %i\n", pid, WEXITSTATUS(status));
        }
    }
    printf("All child processes closed, closing main process...\n");
    return 0;
}


int create_child(const char *program, char **arg_list)
{
    pid_t child_pid = fork();
    if (child_pid != 0)
    {
        printf("Child process %s with PID: %d  was created\n", program, child_pid);
        return child_pid;
    }
    else if (child_pid == 0)
    {   
        execvp(program, arg_list);
    }
    else
    {
        perror("Fork failed");
    }
}