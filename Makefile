# Makefile for Aircraft Simulator

CC = g++
CFLAGS = -Wall -O2
LDFLAGS = -lGL -lGLU -lglut

SRCS = main.cpp atmosphere.cpp camera.cpp globals.cpp hud.cpp init.cpp input.cpp jet.cpp menu.cpp physics.cpp sky.cpp terrain.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = flight_sim

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
