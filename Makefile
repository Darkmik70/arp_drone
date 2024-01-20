# Source directory and build directory
SRCDIR = ./src
BUILDDIR = ./build
LOGDIR = ./log

# UTILOBJ
UTIL_OBJ = $(BUILDDIR)/util.o


# Default target
all: $(BUILDDIR) util logger wd server km drone interface main 

# create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# create build directory
$(LOGDIR):
	mkdir -p $(BUILDDIR)/$(LOGDIR)

# Run project
run:
	./$(BUILDDIR)/main

clean:
	rm -f $(BUILDDIR)/main
	rm -f $(BUILDDIR)/server
	rm -f $(BUILDDIR)/interface
	rm -f $(BUILDDIR)/key_manager
	rm -f $(BUILDDIR)/drone
	rm -f $(BUILDDIR)/watchdog
	rm -f $(BUILDDIR)/logger
	rm -f $(BUILDDIR)/util


drone:
	gcc -I include -o $(BUILDDIR)/drone $(UTIL_OBJ) $(SRCDIR)/drone.c -pthread -lm
main:
	gcc -I include -o $(BUILDDIR)/main $(UTIL_OBJ) $(SRCDIR)/main.c
server:
	gcc -I include -o $(BUILDDIR)/server $(UTIL_OBJ) $(SRCDIR)/server.c -pthread
interface:
	gcc -I include -o $(BUILDDIR)/interface $(UTIL_OBJ) $(SRCDIR)/interface.c -lncurses -pthread
km:
	gcc -I include -o $(BUILDDIR)/key_manager $(UTIL_OBJ) $(SRCDIR)/key_manager.c -pthread
targets:
	gcc -I include -o $(BUILDDIR)/targets $(UTIL_OBJ) $(SRCDIR)/targets.c
obstacles:
	gcc -I include -o $(BUILDDIR)/obstacles $(UTIL_OBJ) $(SRCDIR)/obstacles.c
wd:
	gcc -I include -o $(BUILDDIR)/watchdog $(UTIL_OBJ) $(SRCDIR)/watchdog.c
logger:
	gcc -I include -o $(BUILDDIR)/logger $(UTIL_OBJ) $(SRCDIR)/logger.c
util:
	gcc -I include -o $(BUILDDIR)/util.o  -c $(SRCDIR)/util.c
