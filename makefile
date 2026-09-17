CXX = g++
CC  = gcc
LIB_INCLUDES = $(patsubst %,-I%,$(wildcard libs/*/include))
LIB_SRCS     = $(wildcard libs/*/src/*.cpp)
LIB_OBJS     = $(patsubst libs/%.cpp,build/libs/%.o,$(LIB_SRCS))

# Lua 5.4 (C), sans l'interpreteur/compilateur standalone (lua.c, luac.c)
LUA_DIR    = libs/lua/src
LUA_SRCS   = $(filter-out $(LUA_DIR)/lua.c $(LUA_DIR)/luac.c,$(wildcard $(LUA_DIR)/*.c))
LUA_OBJS   = $(patsubst libs/%.c,build/libs/%.o,$(LUA_SRCS))
LUA_CFLAGS = -std=gnu99 -O2 -DLUA_USE_LINUX -MMD -MP

CXXFLAGS = -std=c++17 -Iinclude $(LIB_INCLUDES) -I$(LUA_DIR) -DSOL_ALL_SAFETIES_ON=1 -g -MMD -MP
TARGET = main
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=build/%.o)
DEPS = $(OBJS:.o=.d) $(LIB_OBJS:.o=.d) $(LUA_OBJS:.o=.d)

all: build $(TARGET)

build:
	mkdir -p build
	mkdir -p $(sort $(dir $(LIB_OBJS) $(LUA_OBJS)))

$(TARGET): $(OBJS) $(LIB_OBJS) $(LUA_OBJS)
	$(CXX) $(OBJS) $(LIB_OBJS) $(LUA_OBJS) -o $(TARGET) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm -ldl

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/libs/%.o: libs/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/libs/%.o: libs/%.c
	$(CC) $(LUA_CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build $(TARGET)

.PHONY: all clean build
