EXE = registry
CFLAGS = -Wall
CXXFLAGS = -Wall
LDLIBS =
CC = gcc
CXX = g++

.PHONY: all
all: $(EXE)

# Implicit rules defined by Make, but you can redefine if needed
#
#registry: registry.c
#	$(CC) $(CFLAGS) registry.c $(LDLIBS) -o registry
#
# OR
#
#registry: registry.cc
#	$(CXX) $(CXXFLAGS) registry.cc $(LDLIBS) -o registry

.PHONY: clean
clean:
	rm -f $(EXE)
