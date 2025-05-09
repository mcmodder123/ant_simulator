CC = g++
CFLAGS = -O2 -std=c++17 -lSDL2

ant_simulator: main.cpp
	$(CC) $(CFLAGS) -o ant_simulator main.cpp

.PHONY: install
install:
	cp ant_simulator /usr/local/bin/ant_simulator
