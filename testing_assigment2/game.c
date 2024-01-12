/*THIS CODE IS USED TO OBTAIN A GAME POINT SCORE BASED ON THE ACTIONS 
OF THE USER. IT STILL NEEDS LOTS OF CHANGES AND REFINEMENT. IT IS NOT READY AT ALL.

WE TAKE INTO CONSIDERATION:
1. Time transcurred until target reached
3. Correct target reached
4. Incorrect target reached (penalty)
3. Entered obstacle min distance (penalty)

* A target is reached when a drone's x,y coordinates are equal to the target coordinates. 
* A target has this format ID,X,Y. The ID is a number. The correct target is the lowest ID possible. 
* The minimin distance of an obstacle is 2.
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>

// Structure to represent a target
struct Target {
    int id;
    int x;
    int y;
};

// Structure to represent an obstacle
struct Obstacle {
    int x;
    int y;
};

/*Regular functions needed thoughout the scoring process*/


double calculateDistance(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

int isTargetReached(int droneX, int droneY, const struct Target* target) {
    return (droneX == target->x && droneY == target->y);
}


int isObstacleTooClose(int droneX, int droneY, const struct Obstacle* obstacle) {
    int minDistance = 2; // This can be obtained from the constants txt file
    double distance = calculateDistance(droneX, droneY, obstacle->x, obstacle->y);
    return (distance < minDistance);
}

// Time penalty based on a formula I personally obtained by trial-error
int calculateTimePenalty(int timeElapsed) {
    int maxPenalty = 20;
    double penalty = (1.0 / 2000) * pow(timeElapsed, 4);
    // Ensure the penalty does not exceed the maximum penalty
    penalty = (penalty > maxPenalty) ? maxPenalty : penalty;
    return (int)penalty;
}

// Most important function: kinda..
int getScore(const struct Target targets[], int numTargets, const struct Obstacle* obstacle) {
    int timeElapsed = 0; // Time elapsed until target is reached. Shld start at zero!
    int score = 0; 

    for (int i = 0; i < numTargets; ++i) {
        int droneX = 0, droneY = 0; // Drone coordinates are set to (0, 0) for now. ASK SERVER.

        printf("Current drone position: (%d, %d)\n", droneX, droneY);

        if (isObstacleTooClose(droneX, droneY, obstacle)) {
            printf("Penalty: Entered obstacle zone!\n");
            score -= 10; // Apply penalty for entering obstacle zone
        }

        if (isTargetReached(droneX, droneY, &targets[i])) {
            printf("Target %d reached!\n", targets[i].id);
            score += 20; // Reward for reaching the target

            // Calculate the time elapsed until the target is reached
            timeElapsed += i + 1; // Idk what gpt is doing here, will change it later.
            // The time stuff makes no sense at the moment.

        } else {
            printf("Incorrect target reached! Penalty applied.\n");
            score -= 5; // Penalty for reaching the wrong target
        }

        usleep(500000);
    }

    // Calculate time penalty
    int timePenalty = calculateTimePenalty(timeElapsed);
    printf("Time penalty: %d\n", timePenalty);

    // Apply time penalty to the final score
    score -= timePenalty;

    printf("Game over! Total time elapsed: %d\n", timeElapsed);

    return score;
}

int main() {
    // For now this requires targets and obstacle positions
    struct Target targets[] = { {1, 3, 5}, {2, 7, 9}, {3, 10, 12} };
    int numTargets = sizeof(targets) / sizeof(targets[0]);

    struct Obstacle obstacle = { 5, 8 };

    // A loop to continously update the score.
    while (1) {
        int finalScore = getScore(targets, numTargets, &obstacle);
        printf("Score: %d\n", finalScore);

        // WORK IN PROGRESS...
        break;
    }

    return 0;
}