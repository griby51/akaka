CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -g
TARGET = main
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=build/%.o)

all: build $(TARGET)

build:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

build/%.o: src/%.cpp
	$(CXX) -Iinclude -c $< -o $@

clean:
	rm -rf build $(TARGET)

.PHONY: all clean build
