# akaka
x86_64-w64-mingw32-g++ *.cpp -o main.exe \
  -I/usr/x86_64-w64-mingw32/include/SDL2 \
  -Dmain=SDL_main \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image \
  -lsetupapi -lcfgmgr32 -lole32 -luuid -lversion \
  -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
