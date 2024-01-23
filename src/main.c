#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

// Serverless pipes (fd)
int key_press_fd[2];
int lowest_target_fd[2];

// New pipes working with server (fd)
int km_server[2];
int server_drone[2];
int interface_server[2];
int drone_server[2];
int server_interface[2];
int server_obstacles[2];
int obstacles_server[2];
int server_targets[2];
int targets_server[2];

// PIDs
pid_t server_pid;
pid_t window_pid;
pid_t km_pid;
pid_t drone_pid;
pid_t wd_pid;
pid_t logger_pid;
pid_t targets_pid;
pid_t obstacles_pid;


void signal_handler(int signo, siginfo_t *siginfo, void *context)
{
    if (signo == SIGINT)
    {
        printf("Caught SIGINT, killing all children... \n");
        kill(server_pid, SIGKILL);
        kill(window_pid, SIGKILL);
        kill(km_pid, SIGKILL);
        kill(drone_pid, SIGKILL);
        kill(wd_pid, SIGKILL);
        kill(logger_pid, SIGKILL);
        kill(targets_pid, SIGKILL);
        kill(obstacles_pid, SIGKILL);

        printf("Closing all pipes.. \n");
        close_all_pipes();

        exit(1);
    }
}

int main(int argc, char *argv[])
{
    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);


    // Serverless pipe creation
    if (pipe(key_press_fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(lowest_target_fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Pipe creation: To server
    if (pipe(km_server) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(drone_server) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(interface_server) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(obstacles_server) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(targets_server) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Pipe creation: From server
    if (pipe(server_drone) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_interface) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_obstacles) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(server_targets) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Passing file descriptors for pipes used on server.c
    char server_fds[80];
    sprintf(server_fds, "%d %d %d %d %d %d %d %d %d", km_server[0], server_drone[1],
            interface_server[0], drone_server[0], server_interface[1], server_obstacles[1],
            obstacles_server[0], server_targets[1], targets_server[0]);

    // Passing file descriptors for pipes used on key_manager.c
    char key_manager_fds[80];
    sprintf(key_manager_fds, "%d %d", key_press_fd[0], km_server[1]);

    // Passing file descriptors for pipes used on interface.c
    char interface_fds[80];
    sprintf(interface_fds, "%d %d %d %d", key_press_fd[1], server_interface[0],
            interface_server[1], lowest_target_fd[1]);

    // Passing file descriptors for pipes used on drone.c
    char drone_fds[80];
    sprintf(drone_fds, "%d %d %d", server_drone[0], drone_server[1], lowest_target_fd[0]);

    // Passing file descriptors for pipes used on obstacles.c
    char obstacles_fds[80];
    sprintf(obstacles_fds, "%d %d", server_obstacles[0], obstacles_server[1]);

    // Passing file descriptors for pipes used on targets.c
    char targets_fds[80];
    sprintf(targets_fds, "%d %d", server_targets[0], targets_server[1]);

    int delay = 100000; // Time delay between next spawns
    int p_num = 0;      // number of processes

    /* Server */
    char *server_args[] = {"konsole", "-e", "./build/server", server_fds, NULL};
    server_pid = create_child(server_args[0], server_args);
    p_num++;
    usleep(delay * 10); // little bit more time for server

    /* Targets */
    char *targets_args[] = {"konsole", "-e", "./build/targets", targets_fds, NULL};
    targets_pid = create_child(targets_args[0], targets_args);
    p_num++;
    usleep(delay);

    /* Obstacles */
    char *obstacles_args[] = {"konsole", "-e", "./build/obstacles", obstacles_fds, NULL};
    obstacles_pid = create_child(obstacles_args[0], obstacles_args);
    p_num++;
    usleep(delay);

    /* Keyboard manager */
    char *km_args[] = {"konsole", "-e", "./build/key_manager", key_manager_fds, NULL};
    km_pid = create_child(km_args[0], km_args);
    p_num++;
    usleep(delay);

    /* Drone */
    char *drone_args[] = {"konsole", "-e", "./build/drone", drone_fds, NULL};
    drone_pid = create_child(drone_args[0], drone_args);
    p_num++;
    usleep(delay);

    /* Watchdog */
    char *wd_args[] = {"konsole", "-e", "./build/watchdog", NULL};
    wd_pid = create_child(wd_args[0], wd_args);
    p_num++;
    printf("Watchdog Created\n");

    /* Window - Interface */
    char *window_args[] = {"konsole", "-e", "./build/interface", interface_fds, NULL};
    window_pid = create_child(window_args[0], window_args);
    p_num++;
    usleep(delay);

    // /* Logger */
    // char* logger_args[] = {"konsole", "-e", "./build/logger", NULL};
    // logger_pid = create_child(logger_args[0], logger_args);
    // p_num++;
    // usleep(delay);

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

    close_all_pipes();

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

void close_all_pipes()
{
    // Close all of the pipes
    // Serverless pipes (fd)
    close(key_press_fd[0]);
    close(lowest_target_fd[0]);
    close(key_press_fd[1]);
    close(lowest_target_fd[1]);

    // New pipes working with server (fd)
    close(km_server[0]);
    close(server_drone[0]);
    close(interface_server[0]);
    close(drone_server[0]);
    close(server_interface[0]);
    close(server_obstacles[0]);
    close(obstacles_server[0]);
    close(server_targets[0]);
    close(targets_server[0]);
    close(km_server[1]);
    close(server_drone[1]);
    close(interface_server[1]);
    close(drone_server[1]);
    close(server_interface[1]);
    close(server_obstacles[1]);
    close(obstacles_server[1]);
    close(server_targets[1]);
    close(targets_server[1]);
}