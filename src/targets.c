#include "targets.h"
#include "common.h"

// Pipes working with the server
int server_targets[0];
int targets_server[1];

// Sockets
int sockfd;

char logfile[80]; // path to logfile
char msg[1024];


int main(int argc, char *argv[]) 
{
    // Read the file descriptors from the arguments
    get_args(argc, argv);

    sleep(1);

    // Read the config.txt file
    char program_type[MSG_LEN];
    char socket_data[MSG_LEN];
    read_args_from_file("./src/config.txt", program_type, socket_data);
    char host_name[MSG_LEN];
    int port_number;
    sprintf(msg, "Program type: %s\n", program_type);
    log_msg(logfile, TARGETS, msg);

    parse_host_port(socket_data, host_name, &port_number);
    sprintf(msg, "Host name: %s\n", host_name);
    log_msg(logfile, TARGETS, msg);
    sprintf(msg, "Port number: %d\n", port_number);
    log_msg(logfile, TARGETS, msg);


    if (strcmp(program_type, "server") == 0)
    {
        exit(0);
    }

    // Signals
    struct sigaction sa;
    sa.sa_sigaction = signal_handler; 
    sa.sa_flags = SA_SIGINFO;
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGUSR1, &sa, NULL);   
    publish_pid_to_wd(TARGETS_SYM, getpid());

    // Seed random number generator with current time
    srand(time(NULL));

    // Variables
    int screen_size_x; 
    int screen_size_y;
    float scale_x;
    float scale_y;
    int obtained_dimensions = 0;
    int targets_created = 0;

    //////////////////////////////////////////////////////
    /* SOCKET INITIALIZATION */
    /////////////////////////////////////////////////////

    int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = port_number;  // Obtained from config.txt

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {log_err(logfile, TARGETS, "ERROR opening socket");}

    server = gethostbyname(host_name);  // Obtained from config.txt
    if (server == NULL) {fprintf(stderr,"ERROR, no such host\n");}

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    block_signal(SIGUSR1);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        log_err(logfile, TARGETS, "ERROR connecting");
    unblock_signal(SIGUSR1);


    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[] = "TI";
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg), logfile, TARGETS);
    log_msg(logfile, TARGETS, init_msg);


    //////////////////////////////////////////////////////
    /* OBTAIN DIMENSIONS */
    /////////////////////////////////////////////////////

    char dimension_msg[MSG_LEN];
    read_then_echo(sockfd, dimension_msg);

    float temp_scx, temp_scy;
    sscanf(dimension_msg, "%f,%f", &temp_scx, &temp_scy);
    screen_size_x = (int)temp_scx;
    screen_size_y = (int)temp_scy;


    while(1)
    {
        //////////////////////////////////////////////////////
        /* SECTION 1: TARGET GENERATION & SEND DATA*/
        /////////////////////////////////////////////////////

        // Array to store generated targets
        Target targets[MAX_TARGETS];

        if(targets_created == 0){
            // Sector dimensions
            const int sectorWidth = screen_size_x / 3;
            const int sectorHeight = screen_size_y / 2;

            // Generate random order for distributing targets across sectors
            int order[MAX_TARGETS];
            for (int i = 0; i < MAX_TARGETS; ++i) {
                order[i] = i;
            }
            for (int i = MAX_TARGETS - 1; i > 0; --i) {
                int j = rand() % (i + 1);
                int temp = order[i];
                order[i] = order[j];
                order[j] = temp;
            }

            // String variable to store the targets in the specified format
            char targets_msg[MSG_LEN]; // Adjust the size accordingly

            // Generate targets within each sector
            for (int i = 0; i < MAX_TARGETS; ++i) {
                Target target;

                // Determine sector based on the random order
                int sectorX = (order[i] % 3) * sectorWidth;
                int sectorY = (order[i] / 3) * sectorHeight;

                // Generate random coordinates within the sector
                target.x = rand() % sectorWidth;
                target.y = rand() % sectorHeight;

                // Adjust coordinates based on the sector position
                target.x += sectorX;
                target.y += sectorY;

                // Ensure coordinates do not exceed the screen size
                target.x = target.x % screen_size_x;
                target.y = target.y % screen_size_y;

                // Store the target
                targets[i] = target;
            }

            // Construct the targets_msg string
            int offset = sprintf(targets_msg, "T[%d]", MAX_TARGETS);
            for (int i = 0; i < MAX_TARGETS; ++i) {
                offset += sprintf(targets_msg + offset, "%.3f,%.3f", 
                                    (float)targets[i].x, (float)targets[i].y);

                // Add a separator unless it's the last element
                if (i < MAX_TARGETS - 1) {
                    offset += sprintf(targets_msg + offset, "|");
                }
            }
            targets_msg[offset] = '\0'; // Null-terminate the string

            // Send the data to the server
            write_then_wait_echo(sockfd, targets_msg, sizeof(targets_msg), logfile, TARGETS);

            targets_created = 1;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER */
        /////////////////////////////////////////////////////

        /* Because the targets process does not need to run continously, 
        we may use a blocking-read for the socket */
        char socket_msg[MSG_LEN];
        read_then_echo_unblocked(sockfd, socket_msg, logfile, TARGETS);

        if (strcmp(socket_msg, "STOP") == 0){
            sprintf(msg,"STOP RECEIVED FROM SERVER!\n");
            log_msg(logfile, TARGETS, msg);

            sprintf(msg, "This process will close in 5 seconds...\n");
            log_msg(logfile, TARGETS, msg);
            sleep(5);
            exit(0);
        }
        else if (strcmp(socket_msg, "GE") == 0){
            targets_created = 0;
        }
    }

    clean_up();

    return 0;
}


void signal_handler(int signo, siginfo_t *siginfo, void *context) 
{
    // printf("Received signal number: %d \n", signo);
    if  (signo == SIGINT)
    {
        sprintf(msg, "Caught SIGINT \n");
        log_msg(logfile, TARGETS, msg);
        clean_up();
        exit(1);
    }
    if (signo == SIGUSR1)
    {
        // Get watchdog's pid
        pid_t wd_pid = siginfo->si_pid;
        // inform on your condition
        kill(wd_pid, SIGUSR2);
        // printf("SIGUSR2 SENT SUCCESSFULLY\n");
    }
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d %s", &server_targets[0], &targets_server[1], logfile);
}

void clean_up()
{
    close(targets_server[1]);
    close(server_targets[0]);
    close(sockfd);
}

