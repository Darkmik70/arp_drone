# Assignment 1
This assignment represents the first part of the project for Advanced and Robot Programming course on UniGe in the winter semester 2023/2024. 

The work has been performed by a team of two: Josue Tinoco and Michał Krępa *coolGoose*

## Installation & Usage
For project's configuration we have used `Makefile`.

To build executables simply hit:
```
make
```
in the project directory.

To run the game hit:
```
make run
```
when this happens a 5 windows with konsole processes will launch, each one for different segment.

To remove the executables simply hit 
```
make clean
```

### Operational instructions, controls
To operate the drone use following keybindings

    `q` `w` `e`       `u` `i` `o`
    
    `a` `s` `d`   or  `j` `k` `l`
    
    `z` `x` `c`       `m` `,` `.`
    

The given bindings represent 8 different directions for the drone movement.
- `w` or `i` : UP
- `x` or `,` : DOWN
- `a` or `j`: LEFT
- `d` or `l` : RIGHT

- `q` or `u` : UP-LEFT
- `e` or `o` : UP-RIGHT
- `z` or `m` : DOWN-LEFT
- `c` or `.` : DOWN- RIGHT

The center keys `s` and `k` is used to STOP all forces on the drone. However, because the drone physics has been programed with inertia, it will keep moving for a short time before completely stopping.

## Overview 

![Sketch of the architecture](architecture.jpg)

The first part assumes first 6 components:
- Main
- Server (blackboard using Posix Shared Memory)
- Window (User interface)
- Watchdog
- Drone
- Keyboard Manager

All of above mentioned components were created. For further details please refer to the description below.

General overview of first assignment, tasks, what was accomplished

short definitions of all active components : what they do, which primitives used, which algorithms)

### Main
Main process is the father of all processes. It creates child processes by using `fork()` and runs them inside a wrapper program `Konsole` to display the current status, debugging messages until an additional thread/process for colleceting log messages.

After creating children, process stays in a infinite while loop awaiting termination of all processes, and when that happens - it terminates itself.

### Server
Server process is the heart of this project. It creates all segments of the shared memory, and semaphores. It is also responsible of cleaning them up, when being interrupted.

Server operates by creating the semaphores and shared memory segments, then enters an infinite while loop awaiting for a signal from watchdog. After getting out of the loop (which should never happen, at the moment we assume the server closes only when interrupted) it cleans up the segments and semaphores, making sure no artifacts are left behind. 


### Watchdog
Watchdog's job is to monitor the "health" of all of the processes, which means at this point if processes are running and not closed.

During initialization it gets from special shared memory segment the PIDs of processes, (remember that in main we are running the wrapper process Konsole, so its not possible to get to know the actual PID from `fork()` in `main`, at this point at least).

Then the process enters while loop where it sends `SIGUSR1` to all of the processes checking if they are alive. They respond with `SIGUSR2` back to watchdog, knowing its PID thanks to `siginfo_t`. This zeroes the counter for programs to response. If the counter for any of them reaches the threshold, Watchdog sends `SIGINT` to all of the processes, and exits, making sure all of the semaphores it was using are closed. The same thing happens when Watchdog is interrupted.

### Interface (Window)
The interface process handles user interaction within the `Konsole` program by creating a graphical environment. This environment is designed to receive user inputs, represented by key presses, and subsequently display the updated state of the program.

To initiate this process, the necessary signals, shared memory, and semaphores are initialized, setting the groundwork for execution. Two shared memory addresses are utilized to manage both the input (key pressed) and the output (drone position).

The ncurses library is utilized to build the graphical interface. The program first employs the `initscr()` function, then the terminal dimensions are captured using `getmaxyx()`, and a box is drawn around the interface using `box()`. The initial drone position is set at the center of this box, representing the boundaries of the functional area within which the drone can navigate freely.

Afterwards the program enters an infinite loop, continuously monitoring user key presses with the help of `getch()`. Simultaneously, it updates the drone's position with `mvaddch()` on the screen whenever a movement action presents itself. This ensures real-time user interaction and visualization of the drone's movements.

### Keyboard manager
The keyboard manager process serves as the bridge between the interface and the computation of the drone's movements. It retrieves the key pressed by the user from shared memory and subsequently translates them into integer values representing the force applied in each possible direction, which are then communicated to the drone's process.

Within an infinite while loop, the iteration time is controlled through `sem_wait()`, synchronizing with the semaphore shared with the interface process. The specific action to be taken is determined by a function containing multiple `if` statements. Based on these conditions, values are assigned to variables x or y, which represent movement along the horizontal or vertical axis within the window. Three possible values `[-1, 0, 1]` are sent, indicating the magnitude and direction of movement, while any other value outside this range would indicate a special non-movement action understood by the drone process.

### Drone
The drone process is the one responsable for the drone's physics, movement, boundaries and conditional events within the interface working area (box). It ensures the drone behaves correctly and within certain pre-defined parameters, which in further improvements will include user personalized selection. 

Like the previous processes, at first, all signals, shared memory and semaphores are initialized. Also multiples variables are defined and initial values are given; such as position, velocity and force. 

Inside the main loop, no special functions are used besides the ones needed for memory read/write and semaphores. In order to move the drone trough the screen, two methods are used. The `step method` which was only used for internal testing and its selection is hard-coded, moves the drone one block (or step) at the time. Then we have the `euler method` which is the default choice at the beginning of the program, contains the drone dynamic formulas which make the drone move with inertia and viscous resistance.

### Additional Comments
- Watchdog gets the PID from all of the process through a shared memory segment. This operation requires 3 semaphores (In author's opinion 2 would be enough, yet at this moment we needed tu ensure the stability of logic flow, and did not have time to polish this solution.)

- We assume processes to be run in order it happens in `main.c`. If someone wishes to run processes seperatly, they may encounter segfaults that happen due to the fact that processes wish to access a non-existing shared memory components. Therefore we suggest to run project the way it is described in **Installation**


### Further Improvements
- [ ] Add new functionalities to window:
    - [ ] Freeze window
    - [ ] Menu window
    - [ ] Real-time information box
- [ ] Logging messages, Monitoring process
- [ ] Add special keycaps handling (eg. esc)
    - [ ] ESC: Spawn the menu. Suspend or Quit the program.
    - [ ] Shift: Increase speed secondary method. 
- [ ] Targets
- [ ] Obstacles with repulsion forces
- [ ] Walls of the box with repulsion forces
- [ ] Scoring system
