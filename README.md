# Assignment 3
This assignment represents the last part of the project for Advanced and Robot Programming course on UniGe in the winter semester 2023/2024. 

The work has been performed by a team of two: Josue Tinoco and Michał Krępa *coolGoose*

## Pre-requisites
The program have been tested to work on Ubuntu operating system V22. For code compilation the following external libraries are needed: konsole, ncurses.
```
sudo apt-get install libncurses5-dev libncursesw5-dev
```
```
sudo apt-get install konsole
```

## Installation & execution
For the project's configuration we have used `Makefile`.

To build executables, in the project directory, simply write:
```
make
```
To run the game:
```
make run
```
After this happens, two processes in a konsole terminal will launch, while the rest of them will be executed in the background.

To remove the executables: 
```
make clean
```

## Usage
This program has three main modes:
- Standalone: The game is able to be played locally.
- Server: The game is able to be played but requires the server to accept connections with two external clients (obstacles and targets).
- Client: Only the obstacles/targets processes will be operational. They will provide a remote server with the expected data. No game can be played on the local machine.

To change the execution mode, open the ```config.txt``` file, and make the necessary changes to the only uncommented line.
The instructions are included at the top of the same file.

## Game Description
A plus symbol (+) representing a drone will spawn in the middle of the konsole window running the interface process. 
On the screen we will also see targets represented by numbers (1,2,3) and obstacles represented by asterisks (*).

The goal of the game is to reach all the targets in a sequential order, while trying to avoid the obstacles which
spawn in a random manner throughout the entire duration of the game.

A score calculation will be updated in real time. Maximum points can be acquired if a target is reached within
a certain amount of time. If the player takes too long to reach a target, less points will be awarded. Also, if
a player collides with an obstacle, points will be deducted.

Once all targets are reached, the game will reset and the player will be presented with a new set of targets.


## Operational instructions
To operate the drone use the following keybindings:

    `q` `w` `e`       
    
    `a` `s` `d`    
    
    `z` `x` `c`       
    

The given bindings represent 8 different directions for the drone to move on the window.
- `w` : UP
- `x` : DOWN
- `a` : LEFT
- `d` : RIGHT

- `q` : UP-LEFT
- `e` : UP-RIGHT
- `z` : DOWN-LEFT
- `c` : DOWN- RIGHT

The center key `s` is used to STOP all forces on the drone. However, because the drone physics has been programed with inertia, it will keep moving for a short time before completely stopping.

The special key `p` is used to STOP the program itself.

## Overview 

![Sketch of the architecture](Architecture.jpeg)

The program consists on the following main 8 components:
- Main
- Server
- Interface
- Keyboard Manager
- Drone
- Targets
- Obstacles
- Watchdog

For further details on all of the above mentioned components. please refer to the description below.

### Main
Main process is the father of all processes. It creates child processes by using `fork()` and runs them inside a wrapper program `Konsole` to display the current status, debugging messages until an additional thread/process for colleceting log messages.

Every pipe for use in the program is created using `pipe()`, and each of the file descriptors are passed to the arguments of the processes created. The read or write end of each pipe is assigned to exclusively one process.

After creating children, process stays in a infinite while loop awaiting termination of all processes, and when that happens - it terminates itself.

### Server
Server process is the heart of this project. It manages all communications regarding pipes amongst all processes, so that when it reads the data, it is able to send it to the correct recipients. This is done in a controlled infinite loop that will terminate depending on the watchdog control of the program. 

The server also handle incoming connections from the network or from the same machine, using the ```socket``` function to communicate specifically with the obstacles and targets processes.

When necessary, the server may analyze the contents of each message received in order to understand the appropiate destination for it. This way, each of the processes will only read specific data intended for them. 


### Watchdog
Watchdog's job is to monitor the "health" of all of the processes, which means at this point if processes are running and not closed.

During initialization it gets from special shared memory segment the PIDs of processes, (remember that in main we are running the wrapper process Konsole, so its not possible to get to know the actual PID from `fork()` in `main`, at this point at least).

Then the process enters while loop where it sends `SIGUSR1` to all of the processes checking if they are alive. They respond with `SIGUSR2` back to watchdog, knowing its PID thanks to `siginfo_t`. This zeroes the counter for programs to response. If the counter for any of them reaches the threshold, Watchdog sends `SIGINT` to all of the processes, and exits, making sure all of the semaphores it was using are closed. The same thing happens when Watchdog is interrupted.

### Interface
The interface process handles user interaction within the `Konsole` program by creating a graphical environment. This environment is designed to receive user inputs, represented by key presses, and subsequently display the updated state of the program.

To initiate this process, the necessary signals are initialized, and pipes file descriptors are obtained, setting the groundwork for execution. The interface will receive data exclusively from the server, for which its origin are the drone, targets and obstacles processes.

The ncurses library is utilized to build the graphical interface. The program first employs the `initscr()` function, then the terminal dimensions are captured using `getmaxyx()`, and a box is drawn around the interface using `box()`. The initial drone position is set at the center of this box, representing the boundaries of the functional area within which the drone can navigate freely.

Afterwards the program enters an infinite loop, continuously monitoring user key presses with the help of `getch()`. Simultaneously, it updates the drone's position with `mvaddch()` on the screen whenever a movement action presents itself. This ensures real-time user interaction and visualization of the drone's movements.

This process also handles logic regarding the current state of the game, for which it continuosly analyzes the position of the drone, targets and obstacles in order to calculate a score that will be updated on the screen for the player to see.

### Keyboard manager
The keyboard manager process serves as the bridge between the interface and the computation of the drone's movements. It retrieves the key pressed by the user from shared memory and subsequently translates them into integer values representing the force applied in each possible direction, which are then communicated to the drone's process.

Within an infinite while loop, the iteration time is controlled through a `do_while`, in which the function `select()` is used inside of it, to wait for the readiness of the pipe to allow execution of the rest of the code. The specific action to be taken is determined by a function containing multiple `if` statements. Based on these conditions, values are assigned to variables x or y, which represent movement along the horizontal or vertical axis within the window. Three possible values `[-1, 0, 1]` are sent, indicating the magnitude and direction of movement, while any other value outside this range would indicate a special non-movement action understood by the drone process.

### Drone
The drone process is the one responsable for the drone's physics, movement, boundaries and conditional events within the interface working area (window). It ensures the drone behaves and moves correctly on the window drawn by the interface, for which physical equations have been computed to simulate movement withing certain pre-defined parameters. At the same time, the drone interacts with external elements (targets and obstacles) which exert additional forces that procide a more dynamic control for it.

Multiples variables are defined such as position, velocity, force and accelerationn. It only requires from the interface the initial position values where it drew the drone on the screen. From then onwards, it is this process alone that will determine all future positions for the drone.

Inside the main loop, no special functions are used besides the ones needed for reading or writing data to the server. In order to move the drone trough the screen, the `euler method` is the default choice at the beginning of the program, contains the drone dynamic formulas which make the drone move with inertia and viscous resistance. For the external forces, Kathib’s model was used to compute a formula to obtain both the repulsion and attraction forces acting up on the drone

### Targets
The targets process uses ```sockets``` for IPC with the server. It requires to read from the server the screen dimensions of the window. Once they are obtained, the logic for creating the target is executed once and inmediately sent. If a message containing "GE" is obtained, it will re-send new target coordinates to the server.

The targets position on the screen is calculated in a combination of randomness and order. Six sections for any given area are generated, so while the coordinates of the targets make use of the `rand()` function, they are still placed in a way so that they are distributed evenly alongside those sections. This way the player will never get entire targetless sections, making efficient use of all the window.

### Obstacles
The obstacles process uses ```sockets``` for IPC with the server. It also requires, before the main loop, to obtain the screen dimensions of the window. However, there are multiple key difference regarding how these are spawned, in comparison with the targets. 

At first, they are truly randomly generated throughout the entirety of the screen. Second, because the obstacles appear and dissapear at random intervals, it never stops sendind data to the server, updating the new coordinates for each obstacle. Many pre-defined variables directly affect the behavior of the creation of targets, which mostly relate to the minimun and maximum time they will appear on the screen, or the rate at which they are generated.

