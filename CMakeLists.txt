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

# Lua 5.4.9 and sol2 3.5.0 are vendored in libs/ (shared with the makefiles).
# Lua is built as a static library, without the standalone
# interpreter/compiler (lua.c, luac.c)
set(LUA_DIR ${CMAKE_SOURCE_DIR}/libs/lua/src)
file(GLOB LUA_SRC_FILES ${LUA_DIR}/*.c)
list(REMOVE_ITEM LUA_SRC_FILES ${LUA_DIR}/lua.c ${LUA_DIR}/luac.c)

add_library(lua STATIC ${LUA_SRC_FILES})
target_include_directories(lua PUBLIC ${LUA_DIR})
target_compile_definitions(lua PRIVATE $<$<PLATFORM_ID:Linux>:LUA_USE_LINUX>)
target_link_libraries(lua PUBLIC ${CMAKE_DL_LIBS} $<$<NOT:$<PLATFORM_ID:Windows>>:m>)

file(GLOB SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB EXPLOSION_SRC_FILES ${CMAKE_SOURCE_DIR}/libs/ExplosionEngine/src/*.cpp)

add_executable(main ${SRC_FILES} ${EXPLOSION_SRC_FILES})

target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/ExplosionEngine/include
    ${CMAKE_SOURCE_DIR}/libs/sol2/include
)

target_compile_definitions(main PRIVATE SOL_ALL_SAFETIES_ON=1)

target_link_libraries(main PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
    lua
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