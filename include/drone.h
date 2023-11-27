#ifndef DRONE_H
#define DRONE_H   

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define TIME_INTERVAL 0.1 // Time interval in seconds

void eulerMethod(double *x, double *Vx, double forceX, double *y, double *Vy, double forceY);
void stepMethod(int *x, int *y, int actionX, int actionY);

#endif //DRONE_H