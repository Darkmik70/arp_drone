#ifndef DRONE_H
#define DRONE_H   

#define SLEEP_DRONE 100000  // To let the interface.c process execute first write the initial positions.

#define MASS 1.0    // Mass of the object
#define DAMPING 1 // Damping coefficient
#define TIME_INTERVAL 0.1 // Time interval in seconds
#define F_MAX 20.0 // Maximal force that can be acted on the drone's motors

void eulerMethod(double *x, double *Vx, double forceX, double *y, double *Vy, double forceY, double *maxX, double *maxY);
void stepMethod(int *x, int *y, int actionX, int actionY);

#endif //DRONE_H