# Makefile for Aircraft Simulator
CC = g++
CFLAGS = -Wall -O2

ifeq ($(OS),Windows_NT)
    # Windows Linker Flags
    FREEGLUT_PATH = C:\freeglut-3.8.0
    CFLAGS = -Wall -O2 -I$(FREEGLUT_PATH)\include -Istb_image
    LDFLAGS = -L$(FREEGLUT_PATH)\build\lib -lfreeglut -lopengl32 -lglu32 -lassimp
    TARGET = voxel_flight.exe
    CLEAN_CMD = rm -f *.o $(TARGET)
else
    # Linux Linker Flags
    CFLAGS = -Wall -O2 -Istb_image
    LDFLAGS = -lGL -lGLU -lglut -lassimp
    TARGET = flight_sim
    CLEAN_CMD = rm -f *.o $(TARGET)
endif

SRCS = main.cpp atmosphere.cpp camera.cpp globals.cpp hud.cpp init.cpp input.cpp jet.cpp menu.cpp physics.cpp sky.cpp terrain.cpp shader_loader.cpp shadow_system.cpp model_loader.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(CLEAN_CMD)

.PHONY: all clean
