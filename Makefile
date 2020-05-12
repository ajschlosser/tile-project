TARGET  = bin/game

SRC_FILES = $(wildcard src/*.cc src/engine/*.cc src/engine/graphics/*.cc src/map/chunk/*.cc)

CXX  = g++
CC  =  $(CXX)

DEBUG_LEVEL     = -g
EXTRA_CCFLAGS   = -m64 -O3 -Wall -stdlib=libc++ -std=gnu++2a
CXXFLAGS        = $(DEBUG_LEVEL) $(EXTRA_CCFLAGS)
CCFLAGS         = $(CXXFLAGS)
INC_DIR          = ./include
CPPFLAGS        = -I$(INC_DIR)
LDLIBS          = -lSDL2 -lSDL2_image -lSDL2_ttf -ljsoncpp

O_FILES         = $(SRC_FILES:%.cc=%.o)

all: $(TARGET)
$(TARGET): $(O_FILES)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDLIBS) -o $(TARGET) $(O_FILES)

clean:
	$(RM) $(TARGET) $(O_FILES)