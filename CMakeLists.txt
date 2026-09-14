cmake_minimum_required(VERSION 3.16)
project(akaka C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_image REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)
find_package(SDL2_mixer REQUIRED CONFIG)

# Lua 5.4 + sol2, downloaded on first configure
include(FetchContent)

# Give extracted files the extraction time, so a version bump triggers a rebuild
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(lua
    URL https://www.lua.org/ftp/lua-5.4.9.tar.gz
    URL_HASH SHA256=2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6
)
FetchContent_Declare(sol2
    URL https://github.com/ThePhD/sol2/archive/refs/tags/v3.5.0.tar.gz
    URL_HASH SHA256=86c0f6d2836b184a250fc2907091c076bf53c9603dd291eaebade36cc342e13c
)
FetchContent_MakeAvailable(lua sol2)

# The Lua tarball has no CMakeLists: build it as a static library,
# without the standalone interpreter/compiler (lua.c, luac.c)
file(GLOB LUA_SRC_FILES ${lua_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM LUA_SRC_FILES ${lua_SOURCE_DIR}/src/lua.c ${lua_SOURCE_DIR}/src/luac.c)

add_library(lua STATIC ${LUA_SRC_FILES})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
target_compile_definitions(lua PRIVATE $<$<PLATFORM_ID:Linux>:LUA_USE_LINUX>)
target_link_libraries(lua PUBLIC ${CMAKE_DL_LIBS} $<$<NOT:$<PLATFORM_ID:Windows>>:m>)

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB EXPLOSION_SRC_FILES ${CMAKE_SOURCE_DIR}/libs/ExplosionEngine/src/*.cpp)

add_executable(main ${SRC_FILES} ${EXPLOSION_SRC_FILES})

target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/ExplosionEngine/include
)

target_link_libraries(main PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
    lua
    sol2::sol2
)

# Copy assets and (on Windows) required DLLs next to the built binary,
# so `main`/`main.exe` can run directly from the build directory.
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:main>/assets
)

if(WIN32)
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/dll
            $<TARGET_FILE_DIR:main>
    )
endif()