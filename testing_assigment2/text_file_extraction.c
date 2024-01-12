/*ALL CONSTANTS THAT COULD BE CHANGED FOR TESTING PURPOSES, MUST BE EXTRACTED
FROM A TEXT FILE TO AVOID RECOMPILATION. THEY CAN ALSO BE CHANGED IN REAL TIME!!

USE THE FOLLOWING CODE AS EXAMPLE*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    FILE *file;
    int time, distance;

    while (1) {
        file = fopen("constants.txt", "r");

        // Error checking
        if (file == NULL) {
            fprintf(stderr, "Error opening file\n");
            exit(1);
        }

        // Read values from the file
        fscanf(file, "time = %d\ndistance = %d", &time, &distance);
        fclose(file);
        // Printing for testing purposes
        printf("Time: %d, Distance: %d\n", time, distance);
        sleep(1);
    }

    return 0;
}
