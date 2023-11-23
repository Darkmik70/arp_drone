# Compile the C program and save the executable in the build folder
gcc -o build/interface interface.c -lncurses
gcc -o build/km km.c
gcc -o build/main main.c

# Run the compiled program
./build/main