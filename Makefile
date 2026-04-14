# Makefile for Aircraft Simulator
CC = g++
CFLAGS = -Wall -O2

ifeq ($(OS),Windows_NT)
    # Windows Linker Flags
    # Use the local GL headers if they are missing from system, but here we try to fix includes
    CFLAGS = -Wall -O2
    LDFLAGS = -lfreeglut -lopengl32 -lglu32
    TARGET = voxel_flight.exe
    CLEAN_CMD = del /Q /F *.o $(TARGET) 2>NUL || exit 0
else
    # Linux Linker Flags
    LDFLAGS = -lGL -lGLU -lglut
    TARGET = flight_sim
    CLEAN_CMD = rm -f *.o $(TARGET)
endif

SRCS = main.cpp atmosphere.cpp camera.cpp globals.cpp hud.cpp init.cpp input.cpp jet.cpp menu.cpp physics.cpp sky.cpp terrain.cpp shader_loader.cpp shadow_system.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(CLEAN_CMD)

.PHONY: all clean
