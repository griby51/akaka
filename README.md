# akaka

A 2D multiplayer game built in C++ with SDL2.

## Features

- Up to 3 players
- Jetpack-based movement with gravity and bounce physics
- Missile system with score penalties
- Particle effects (thrust, explosions)
- Fully configurable via `config.ini`

## Dependencies

- SDL2
- SDL2_ttf
- SDL2_image

## Build

### Linux

```bash
make
./main
```

### Windows (via WSL cross-compilation)

```bash
x86_64-w64-mingw32-g++ *.cpp -o main.exe \
  -I/usr/x86_64-w64-mingw32/include/SDL2 \
  -Dmain=SDL_main \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image \
  -lsetupapi -lcfgmgr32 -lole32 -luuid -lversion \
  -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
```

### Windows (via MSYS2)

```bash
C:\msys64\mingw64\bin\g++.exe *.cpp -o main.exe \
  -IC:/msys64/mingw64/include/SDL2 \
  -Dmain=SDL_main \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image \
  -lsetupapi -lcfgmgr32 -lole32 -luuid -lversion \
  -static-libgcc -static-libstdc++ -Wl,--subsystem,windows \
  -LC:/msys64/mingw64/lib
```

## Configuration

Edit `config.ini` to tweak gameplay settings:

| Key | Description |
|-----|-------------|
| `SCREEN_WIDTH` / `SCREEN_HEIGHT` | Window resolution |
| `PLAYER_NUMBER` | Number of players (1-3) |
| `GRAVITY_FORCE` | Gravity strength |
| `JETPACK_FORCE` | Jetpack thrust force |
| `ACCELERATION` / `DECELERATION` | Player movement feel |
| `MAX_VX` | Max horizontal speed |
| `BOUNCE` | Enable/disable bouncing |
| `BOUNCE_RESTITUTION` | Bounce energy factor (0-1) |
| `scoreToLaunchMissile` | Score threshold to trigger missiles |
| `missileScorePenality` | Score penalty on missile hit |

## Project Structure

```
akaka/
├── src/            # Source files (.cpp)
├── include/        # Header files (.hpp)
├── assets/         # Images, fonts, config files
├── build/          # Compiled object files
└── Makefile
```
