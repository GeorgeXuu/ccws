# Makefile

CC = g++
FLAG = -g

CFLAGS = -g -Wall -Wextra -Wno-write-strings -Wno-unused-variable 
CON = -fconcepts-diagnostics-depth=3

# verbose warnings.
WARNINGS = -Wall -Wextra -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow -Woverloaded-virtual
           
OPT = -O3
VER = -std=c++23
CON = -fconcepts-diagnostics-depth=3

all: main 

main: main.cpp 
	$(CC) $(CFLAGS) $(VER) $(OPT) $(CON) $^ -o $@

clean:
	rm -rf main
