# Makefile for Aircraft Simulator
CC = g++
CFLAGS = -Wall -O2

# Directories for inclusion
INCLUDES = -I./core -I./graphics -I./world -I./flight -I./ui

ifeq ($(OS),Windows_NT)
    # Windows Linker Flags
    FREEGLUT_PATH = C:\freeglut-3.8.0
    CFLAGS = -Wall -O2 $(INCLUDES) -I$(FREEGLUT_PATH)\include -Igraphics
    LDFLAGS = -L$(FREEGLUT_PATH)\build\lib -Lbin -lfreeglut -lopengl32 -lglu32 -lassimp -lxinput9_1_0
    TARGET = bin/voxel_flight.exe
    CLEAN_CMD = rm -f *.o bin/*.exe
else
    # Linux Linker Flags
    CFLAGS = -Wall -O2 $(INCLUDES) -Igraphics
    LDFLAGS = -lGL -lGLU -lglut -lassimp
    TARGET = bin/flight_sim
    CLEAN_CMD = rm -f *.o bin/$(TARGET)
endif

SRCS = main.cpp \
       world/atmosphere.cpp flight/camera.cpp core/globals.cpp \
       ui/hud.cpp core/init.cpp core/input.cpp flight/jet.cpp ui/menu.cpp \
       flight/physics.cpp world/sky.cpp world/terrain.cpp \
       graphics/shader_loader.cpp graphics/shadow_system.cpp graphics/model_loader.cpp \


# Note: sound_manager.cpp wasn't compiled in the old Makefile but it exists, I'm adding it just in case, or I'll leave it as is if there is no header. Actually, let's keep SRCS exactly as original but with paths.
SRCS = main.cpp \
       world/atmosphere.cpp flight/camera.cpp core/globals.cpp \
       ui/hud.cpp core/init.cpp core/input.cpp flight/jet.cpp ui/menu.cpp \
       flight/physics.cpp world/sky.cpp world/terrain.cpp \
       graphics/shader_loader.cpp graphics/shadow_system.cpp graphics/model_loader.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(CLEAN_CMD)

.PHONY: all clean
