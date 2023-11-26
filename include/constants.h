#ifndef CONSTANTS_H
#define CONSTANTS_H

// Shared memory key for key presses
#define SHM_KEY_1 1
#define SEM_KEY_1 "/my_semaphore_1"

// Shared memory key for drone positions
#define SHM_KEY_2 2
#define SEM_KEY_2 "/my_semaphore_2"

// Shared memory key for drone control (actions).
#define SHM_KEY_3 3
#define SEM_KEY_3 "/my_semaphore_3"

// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

#endif // CONSTANTS_H