CXX = g++
LIB_INCLUDES = $(patsubst %,-I%,$(wildcard libs/*/include))
LIB_SRCS     = $(wildcard libs/*/src/*.cpp)
LIB_OBJS     = $(patsubst libs/%.cpp,build/libs/%.o,$(LIB_SRCS))
CXXFLAGS = -std=c++17 -Iinclude $(LIB_INCLUDES) -g -MMD -MP
TARGET = main
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=build/%.o)
DEPS = $(OBJS:.o=.d) $(LIB_OBJS:.o=.d)

all: build $(TARGET)

build:
	mkdir -p build
	mkdir -p $(sort $(dir $(LIB_OBJS)))

$(TARGET): $(OBJS) $(LIB_OBJS)
	$(CXX) $(OBJS) $(LIB_OBJS) -o $(TARGET) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/libs/%.o: libs/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build $(TARGET)

.PHONY: all clean build
