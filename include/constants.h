#ifndef CONSTANTS_H
#define CONSTANTS_H

// Shared memory key for key presses
#define SHM_KEY "/shared_key"
#define SEM_KEY "/sem_key"

// Shared memory key for drone positions
#define SHM_POS "/shared_pos"
#define SEM_POS "/sem_pos"

#define SHM_KEY_2 5678
#define SEM_KEY_2 "/my_semaphore_2"

// Special value to indicate no key pressed
#define NO_KEY_PRESSED 0

#define DELAY 50000
enum objects
{
    DRONE,
    TARGET,
    OBSTACLE,
    NONE
};

struct Object 
{   
  enum objects type;
  int pos_x;          
  int pos_y;
  char sym;       
};


#endif // CONSTANTS_H