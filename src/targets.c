#include "targets.h"
#include "common.h"

// Pipes working with the server
int server_targets[0];
int targets_server[1];


int main(int argc, char *argv[]) 
{
    sleep(1);
    // Read the file descriptors from the arguments
    get_args(argc, argv);
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

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = PORT_NUMBER;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {perror("ERROR opening socket");}

    server = gethostbyname("localhost");
    if (server == NULL) {fprintf(stderr,"ERROR, no such host\n");}

    bzero((char *) &serv_addr, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        perror("ERROR connecting");


    //////////////////////////////////////////////////////
    /* IDENTIFICATION WITH SERVER */
    /////////////////////////////////////////////////////

    char init_msg[] = "TI";
    write_then_wait_echo(sockfd, init_msg, sizeof(init_msg));


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
            write_then_wait_echo(sockfd, targets_msg, sizeof(targets_msg));
            targets_created = 1;
        }

        //////////////////////////////////////////////////////
        /* SECTION 2: READ THE DATA FROM SERVER */
        /////////////////////////////////////////////////////

        /* Because the targets process does not need to run continously, 
        we may use a blocking-read for the socket */
        char socket_msg[MSG_LEN];
        read_then_echo(sockfd, socket_msg);

        if (strcmp(socket_msg, "STOP") == 0){
            printf("STOP RECEIVED FROM SERVER!\n");
            printf("This process will close in 5 seconds...\n");
            fflush(stdout);
            sleep(5);
            exit(0);
        }
        else if (strcmp(socket_msg, "GE") == 0){
            targets_created = 0;
        }
    }

    return 0;
}


void get_args(int argc, char *argv[])
{
    sscanf(argv[1], "%d %d", &server_targets[0], &targets_server[1]);
}


