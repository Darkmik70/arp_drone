# Source directory and build directory
SRCDIR = src
BUILDDIR = build

# Default target
all: km drone interface main #server

# Run project
run:
	./$(BUILDDIR)/main

clean:
	rm -f $(BUILDDIR)/main
	rm -f $(BUILDDIR)/server
	rm -f $(BUILDDIR)/interface
	rm -f $(BUILDDIR)/key_manager
	rm -f $(BUILDDIR)/drone


drone:
	gcc -I include -o $(BUILDDIR)/drone $(SRCDIR)/drone.c

main:
	gcc -I include -o $(BUILDDIR)/main $(SRCDIR)/main.c

server:
	gcc -I include -o $(BUILDDIR)/server $(SRCDIR)/server.c -pthread

interface:
	gcc -I include -o $(BUILDDIR)/interface $(SRCDIR)/interface.c -lncurses -pthread

km:
	gcc -I include -o $(BUILDDIR)/key_manager $(SRCDIR)/key_manager.c -pthread
