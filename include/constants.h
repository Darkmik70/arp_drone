#ifndef CONSTANTS_H
#define CONSTANTS_H

#define FLOAT_TOLERANCE 0.0044

// Shared memory key for key presses
#define SHM_KEY "/shared_keyboard"
#define SHM_KEY_1 1
#define SEM_KEY "/my_semaphore_1"

// Shared memory key for drone positions
#define SHM_POS "/shared_drone_pos"
#define SHM_KEY_2 2
#define SEM_POS "/my_semaphore_2"

// Shared memory key for drone control (actions).
#define SHM_ACTION "/shared_control"
#define SHM_KEY_3 3
#define SEM_ACTION "/my_semaphore_3"

// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

#endif // CONSTANTS_H