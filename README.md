# akaka

A 2D multiplayer game built in C++ with SDL2.

## Features

- Up to INFINITE players if you build at your own and have enough infinte USB port to branch controller (3 in last release)
- Jetpack-based movement with gravity and bounce physics
- Missile system with score penalties
- Particle effects (thrust, explosions)
- Fully configurable via `config.ini`

## Dependencies

- SDL2
- SDL2_ttf
- SDL2_image

---

## Build

### Linux

Install dependencies:
```bash
sudo apt install g++ libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev   # Debian/Ubuntu
sudo pacman -S gcc sdl2 sdl2_ttf sdl2_image                          # Arch
```

Then build:
```bash
make
./main
```

---

### Windows

#### Step 1 — Install MSYS2

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 MinGW x64** from the Start menu
3. Install the required tools and libraries:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image
```

#### Step 2 — Add MSYS2 to your PATH

1. Open **Windows Settings** → **System** → **About** → **Advanced system settings**
2. Click **Environment Variables**
3. Under **System variables**, find **Path** and click **Edit**
4. Add: `C:\msys64\mingw64\bin`
5. Click OK and **restart any open terminals**

#### Step 3 — Build

Open a terminal (CMD or PowerShell) in the project folder and run:

```bash
mingw32-make -f Makefile.win
```

Or use the full path if `mingw32-make` is not found:

```bash
C:\msys64\mingw64\bin\mingw32-make.exe -f Makefile.win
```

#### Step 4 — Run

```bash
main.exe
```

> **Note:** If the game fails to launch with a missing DLL error, copy the following files from `C:\msys64\mingw64\bin\` into the project folder:
> - `SDL2.dll`
> - `SDL2_ttf.dll`
> - `SDL2_image.dll`

---

#### Alternative — WSL cross-compilation

If you have WSL installed with a Linux distro, you can cross-compile a Windows `.exe` directly from Linux:

```bash
# Install cross-compiler and SDL2 Windows libs (Arch)
sudo pacman -S mingw-w64-gcc mingw-w64-sdl2 mingw-w64-sdl2_ttf mingw-w64-sdl2_image

# Compile
x86_64-w64-mingw32-g++ *.cpp -o main.exe \
  -I/usr/x86_64-w64-mingw32/include/SDL2 \
  -Dmain=SDL_main \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image \
  -lsetupapi -lcfgmgr32 -lole32 -luuid -lversion \
  -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
```

The generated `main.exe` runs natively on Windows.

---

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

---

## Project Structure

```
akaka/
├── src/            # Source files (.cpp)
├── include/        # Header files (.hpp)
├── assets/         # Images, fonts, config files
├── build/          # Compiled object files
├── Makefile        # Linux
├── Makefile.win    # Windows (MSYS2)
└── README.md
```

---

## TODO

- [  ] Refaire toute les textures
- [  ] Faire en sorte que les missiles spawn aléatoirement autour de l'ecran
- [  ] Ajouter des explosions pour les missiles avec souffle qui decale le joueur, screen shaking et bien d'autres
- [  ] Ajouter une capa qui fait un truc explosif qui sort du jetpack avec grosse poussé et qui a l'impact (d'un joueur ou du sol explose)
