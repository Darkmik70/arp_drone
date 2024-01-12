/*THIS CODE SHOULD GO INSIDE THE DRONE.C FILE.

IT SHOULD BE PUT INSIDE A FUNCTION CALLED 'VISION_SYSTEM' OR SMTHG, AND IT 
SHOULD ASK THE SERVER FOR THE LIST OF ALL ACTIVE TARGETS AND OBSTACLES ON
THE SCREEN, CONSTANTLY INSIDE THE WHILE LOOP*/

#include <unistd.h>
#include <stdio.h>
#include <math.h>

// Constants
const double Coefficient = 400.0; // This was obtained by trial-error
double minDistance = 2;
double startDistance = 5;

// Function to calculate force in the X and Y directions
void calculateForce(double droneX, double droneY, double targetX, double targetY, double obstacleX, double obstacleY, double *forceX, double *forceY) {

    // ***Calculate ATTRACTION force towards the target***
    double distanceToTarget = sqrt(pow(targetX - droneX, 2) + pow(targetY - droneY, 2));
    double angleToTarget = atan2(targetY - droneY, targetX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToTarget < minDistance) {
        distanceToTarget = minDistance; // Keep the force value as it were on the minDistance
    }

    // Bellow 5m the attraction force will be calculated.
    if (distanceToTarget > startDistance){
    *forceX += Coefficient * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * cos(angleToTarget);
    *forceY += Coefficient * (1.0 / distanceToTarget - 1.0 / 5.0) * (1.0 / pow(distanceToTarget,2)) * sin(angleToTarget);
    }

    // ***Calculate REPULSION force from the obstacle***
    double distanceToObstacle = sqrt(pow(obstacleX - droneX, 2) + pow(obstacleY - droneY, 2));
    double angleToObstacle = atan2(obstacleY - droneY, obstacleX - droneX);

    // To avoid division by zero or extremely high forces
    if (distanceToObstacle < minDistance) {
            distanceToObstacle = minDistance; // Keep the force value as it were on the minDistance
        }

    if (distanceToObstacle > startDistance){
    *forceX -= Coefficient * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * cos(angleToObstacle);
    *forceY -= Coefficient * (1.0 / distanceToObstacle - 1.0 / 5.0) * (1.0 / pow(distanceToObstacle,2)) * sin(angleToObstacle);
    }
}

int main() {
    // Drone coordinates
    /*This should be asked to the server*/
    double droneX = 2.0, droneY = 3.0;

    // Obstacles and Targets coordinates
    /*This should be asked to the server and format it into arrays*/ 
    double obstacles[][2] = {{5.0, 1.0}};
    double targets[][2] = {{3.0, 5.0}};

    // Variables to store force in X and Y directions
    double forceX = 0.0, forceY = 0.0;

    // Calculate the force for each target
    for (int i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
        calculateForce(droneX, droneY, targets[i][0], targets[i][1], 0.0, 0.0, &forceX, &forceY);
    }

    // Calculate the force for each obstacle
    for (int i = 0; i < sizeof(obstacles) / sizeof(obstacles[0]); i++) {
        calculateForce(droneX, droneY, 0.0, 0.0, obstacles[i][0], obstacles[i][1], &forceX, &forceY);
    }

    // Display the results
    printf("Force in X direction: %lf\n", forceX);
    printf("Force in Y direction: %lf\n", forceY);

    // The results have been manually proven to be correct.

    sleep(20);
    return 0;
}
