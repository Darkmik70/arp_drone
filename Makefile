# Source directory and build directory
SRCDIR = src
BUILDDIR = build

# Default target
all: km drone server interface main

# Run project
run:
	./$(BUILDDIR)/main

clean:
	rm -f $(BUILDDIR)/main
	rm -f $(BUILDDIR)/server
	rm -f $(BUILDDIR)/interface
	rm -f $(BUILDDIR)/km
	rm -f $(BUILDDIR)/drone


drone:
	gcc -o $(BUILDDIR)/drone $(SRCDIR)/drone.c

main:
	gcc -o $(BUILDDIR)/main $(SRCDIR)/main.c

server:
	gcc -o $(BUILDDIR)/server $(SRCDIR)/server.c -pthread

interface:
	gcc -o $(BUILDDIR)/interface $(SRCDIR)/interface.c -lncurses -pthread

km:
	gcc -o $(BUILDDIR)/km $(SRCDIR)/km.c -pthread
