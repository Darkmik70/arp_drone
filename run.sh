# Compile the C program and save the executable in the build folder
gcc -o build/server server.c -pthread
gcc -o build/interface interface.c -lncurses -pthread
gcc -o build/km km.c -pthread
gcc -o build/main main.c

# Run the compiled program
./build/main