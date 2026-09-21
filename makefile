CXX = g++
LIB_INCLUDES = $(patsubst %,-I%,$(wildcard libs/*/include))
LIB_SRCS     = $(wildcard libs/*/src/*.cpp)
LIB_OBJS     = $(patsubst libs/%.cpp,build/libs/%.o,$(LIB_SRCS))

# Lua 5.4 systeme (paquet Arch `lua54`). PAS `lua` : c'est la 5.5, que sol2 3.5 ne gere pas.
LUA_CFLAGS = $(shell pkg-config --cflags lua54)
LUA_LIBS   = $(shell pkg-config --libs lua54)

CXXFLAGS = -std=c++17 -Iinclude $(LIB_INCLUDES) $(LUA_CFLAGS) -DSOL_ALL_SAFETIES_ON=1 -g -MMD -MP
TARGET = main
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=build/%.o)
DEPS = $(OBJS:.o=.d) $(LIB_OBJS:.o=.d)

all: build $(TARGET)

build:
	mkdir -p build
	mkdir -p $(sort $(dir $(LIB_OBJS)))

$(TARGET): $(OBJS) $(LIB_OBJS)
	$(CXX) $(OBJS) $(LIB_OBJS) -o $(TARGET) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer $(LUA_LIBS) -lm -ldl

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/libs/%.o: libs/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build $(TARGET)

.PHONY: all clean build
