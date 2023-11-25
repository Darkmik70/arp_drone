#ifndef DRONE_H
#define DRONE_H   

#define MASS 1.0    // Mass of the object
#define DAMPING 0.1 // Damping coefficient
#define TIME_INTERVAL 0.1 // Time interval in seconds

// Shared Memory key for positions
#define SHM_KEY_2 5678

void eulerMethod(double *x, double *Vx, double forceX, double *y, double *Vy, double forceY);

#endif //DRONE_H