# Akaka — Full Technical Documentation

A 2D local multiplayer party game built in C++17 with SDL2. Players fly around a scrolling arena with jetpacks, fight over score earned from falling pizza slices, and spend that score on hat-based special abilities to knock each other off the screen.

This document describes the codebase **as currently written**, including parts that are unfinished or broken. Where behavior differs from what the in-repo `README.md` TODO list claims, this is called out explicitly.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Controls](#2-controls)
3. [Build Instructions](#3-build-instructions)
4. [Project Layout](#4-project-layout)
5. [High-Level Architecture](#5-high-level-architecture)
6. [Core Utility Classes](#6-core-utility-classes)
7. [Configuration System](#7-configuration-system)
8. [Texture & Audio Systems](#8-texture--audio-systems)
9. [Scene System](#9-scene-system)
10. [Menu Scene — Player Join Flow](#10-menu-scene--player-join-flow)
11. [Game Scene & Match Loop](#11-game-scene--match-loop)
12. [Player System](#12-player-system)
13. [Projectile System](#13-projectile-system)
14. [Ability System](#14-ability-system)
15. [Explosion Engine (`libs/ExplosionEngine`)](#15-explosion-engine-libsexplosionengine)
16. [Particle System](#16-particle-system)
17. [Collectables](#17-collectables)
18. [Configuration Reference (`config.ini`)](#18-configuration-reference-configini)
19. [Assets Reference](#19-assets-reference)
20. [Known Issues & Technical Debt](#20-known-issues--technical-debt)
21. [Feature Status vs README TODO](#21-feature-status-vs-readme-todo)
22. [Roadmap](#22-roadmap)

---

## 1. Overview

- Up to 4 local players, mixing keyboard presets and joysticks freely
- Jetpack-based movement: gravity pulls down, jetpack thrust pushes up, horizontal acceleration/deceleration model
- Players bounce off the screen edges (configurable restitution) instead of just stopping
- **Player-vs-player collision is implemented** (elastic collision with position correction) — see [Section 12](#12-player-system)
- Score comes from picking up falling pizza slices; score is spent on hat-based abilities
- 4 abilities are wired and playable; a 5th (Christmas sleigh) exists in source but does not compile
- Particle effects (jetpack thrust trail) and a small explosion/screen-shake engine (`libs/ExplosionEngine`) shared across missiles, traffic cones, and the kamikaze ability
- Fully configurable via `assets/config.ini` (an ad-hoc `key=value` format, not real INI)

**Dependencies:** SDL2, SDL2_image, SDL2_ttf, SDL2_mixer

**Window title (hardcoded):** `"Encore un jeu random"` — this has not been changed to the game's actual name.

---

## 2. Controls

Three built-in keyboard presets (`include/KeyPreset.hpp`), plus joystick auto-detection:

| Preset | Left | Right | Jetpack | Ability |
|---|---|---|---|---|
| 1 | A | D | S | W |
| 2 | J | L | K | I |
| 3 | Numpad 4 | Numpad 6 | Numpad 5 | Numpad 8 |

Joystick mapping (fixed, not remappable):
- Axis 0 (left stick / D-pad X): move left/right, deadzone ±8000
- Button 0: jetpack
- Button 1: ability

Menu navigation reuses the same key presets: `left`/`right` move the hat/skin selector, `thrust` acts as "down", `missile` acts as "up", and moving the cursor to the "Ready?" row and pressing left/right toggles ready state. Joysticks navigate the menu with the D-pad or left stick, using the same up/down/left/right semantics.

---

## 3. Build Instructions

### Linux

```bash
sudo apt install g++ libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev   # Debian/Ubuntu
sudo pacman -S gcc sdl2 sdl2_ttf sdl2_image sdl2_mixer                                  # Arch
```

```bash
make
./main
```

### Windows (MSYS2)

1. Install [MSYS2](https://www.msys2.org/)
2. Open **MSYS2 MinGW x64** and install the toolchain + libraries:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-SDL2 \
             mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer
   ```
3. Add `C:\msys64\mingw64\bin` to your system `PATH`, restart your terminal.
4. Build:
   ```bash
   mingw32-make -f makefile.win
   ```
5. Run `main.exe`. If a DLL is missing, copy it from `C:\msys64\mingw64\bin\` — all runtime DLLs are already bundled in `/dll` for distribution builds.

### Cross-compiling from Linux/WSL

```bash
x86_64-w64-mingw32-g++ *.cpp -o main.exe \
  -I/usr/x86_64-w64-mingw32/include/SDL2 \
  -Dmain=SDL_main \
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer \
  -lsetupapi -lcfgmgr32 -lole32 -luuid -lversion \
  -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
```

`release.sh` packages a Windows build (binary + DLLs + assets) into a zip for distribution.

---

## 4. Project Layout

```
akaka/
├── src/                     # .cpp implementation files
├── include/                 # .hpp headers
├── libs/ExplosionEngine/    # small in-repo particle/explosion library
│   ├── include/
│   └── src/
├── assets/
│   ├── skins/               # player character skins (turtle, squirrel)
│   ├── hats/                # ability-granting hats
│   ├── collectables/        # pizza, poison sprites
│   ├── gifts/                # Christmas gift sprite (unused — see §20)
│   ├── buttons/              # UI button sprites
│   ├── sounds/sfx/, sounds/musics/
│   ├── config.ini
│   ├── playerThrustParticle.ini, missileParticle.ini  # particle presets
│   └── *.ttf                 # fonts (KiwiSoda, pixelfont, Hypermonosaturation)
├── dll/                      # bundled Windows runtime DLLs for releases
├── makefile / makefile.win / makefile.win.linux
├── release.sh
└── README.md
```

---

## 5. High-Level Architecture

### 5.1 Process entry point (`main.cpp`)

`main()` does raw SDL setup (video, joystick, audio subsystems, `IMG_Init`, `Mix_OpenAudio`, `TTF_Init`), creates a single fullscreen-desktop window at logical resolution `1024x576`, and initializes `TextureManager` with the renderer.

It then owns:
- a `SceneManager` seeded with a `MenuScene`
- a `std::map<int, SDL_Joystick*>` of currently-open joysticks, populated/cleared on `SDL_JOYDEVICEADDED` / `SDL_JOYDEVICEREMOVED`
- a single `LTimer` used to compute frame delta time

**Main loop**, once per iteration:
1. Compute `deltaTime` from the delta timer, **clamped to 0.05s** (20 FPS minimum simulated step, to avoid huge jumps after a stall)
2. Poll all pending SDL events, forward each to `manager.current()->handleEvent(e)`
3. If the scene stack is empty, break
4. `manager.current()->update(deltaTime)`
5. `manager.current()->render()`
6. `SDL_Delay(16)` — a **fixed** 16ms delay regardless of how long the frame actually took

Note: `Game.hpp` defines `FPS = 60` and `TICKS_PER_FRAME = 1000/60` constants, but nothing in `main.cpp` or `Game.cpp` actually uses a frame cap based on them — the real pacing comes entirely from the fixed `SDL_Delay(16)` in `main.cpp`. On a fast machine this yields something close to 60 FPS by coincidence, not by design; on a slow machine there is no compensation beyond the deltaTime clamp.

### 5.2 Scene stack

`SceneManager` (`SceneManager.hpp/.cpp`) holds a `std::stack<std::unique_ptr<Scene>>`:
- `push(scene)` — adds a new scene on top (used to go Menu → Game)
- `pop()` — removes the top scene (used to go back to the menu, or to quit if the menu itself is popped)
- `change(scene)` — pops the current scene (if any) and pushes a new one — **currently unused** anywhere in the codebase
- `current()` — returns the top scene, or `nullptr` if empty

`Scene` (`Scene.hpp`) is a 4-method interface: `handleEvent`, `update`, `render`, `isDone`. `isDone()` is defined by every scene but **never actually read** by `SceneManager` or `main.cpp` — scenes end themselves by calling `mManager.pop()` directly from `handleEvent`/`update`, not by returning `true` from `isDone()`.

### 5.3 Match loop (inside `Game`)

Once a `GameScene` is pushed, `Game::update(deltaTime)` drives, in this exact order:
1. `mDot.move()` — legacy leftover, see §20
2. Background scroll offset update
3. `projectileManager.update(deltaTime)`
4. `explosionManager.update(deltaTime)`
5. `playerManager.update(deltaTime)` (this internally also resolves player-vs-player collisions, see §12)
6. `particleManager.update(deltaTime)`
7. Remove dead pizza collectables, update remaining ones
8. Spawn a new pizza if the pizza timer has elapsed

`Game::render()` renders, per frame: background (two tiled copies for seamless scroll), explosions, players, particles, pizza, projectiles, then a **per-player HUD strip** at the bottom of the screen (name, score, health bar, ability cooldown pie, mini skin/hat portrait) — all drawn directly with `SDL_Render*` primitives, not through a UI library.

---

## 6. Core Utility Classes

### 6.1 `LTexture` (`LTexture.hpp/.cpp`)

Thin RAII wrapper around `SDL_Texture*`. Two ways to build a texture:
- `loadFromeFile(path)` (note: typo in the method name, kept as-is throughout the codebase) — loads an image via `IMG_Load`
- `loadFromRenderedText(text, color, font)` — renders text to a surface via `TTF_RenderText_Blended`, then converts it to a texture

`render(x, y, clip, angle, center, flip, width, height)` wraps `SDL_RenderCopyEx`; `width`/`height` default to `0`, which the implementation treats as "use the texture's native size".

### 6.2 `LTimer` (`LTimer.hpp/.cpp`)

Simple millisecond stopwatch built on `SDL_GetTicks()`. Supports `start`, `stop`, `pause`, `unpause`, `getTicks()`. Used everywhere for cooldowns (abilities), spawn intervals (particles, pizza), and the game's own delta-time computation. It is **not** frame-based — it's wall-clock based, hence the `deltaTime` clamp in `main.cpp` to avoid huge simulation steps after a freeze/breakpoint/alt-tab.

### 6.3 `Utils` namespace (`Utils.hpp/.cpp`)

Shared geometry/gameplay helpers used across players, projectiles, and abilities:

| Function | Purpose |
|---|---|
| `collide(a, b)` | AABB intersection test via `SDL_IntersectRect` |
| `isWithinDistance(a, b, threshold)` | True if center-to-center distance is below `threshold` (compares squared distances, no `sqrt`) |
| `distSq(a, b)` | Squared center-to-center distance between two rects |
| `theNearest(tested, targets)` | Linear scan returning the closest rect (center-to-center) to `tested` |
| `spawnOffScreen(screenWidth, screenHeight, margin)` | Picks a random point just outside one of the 4 screen edges (used by `MissileAbility`) |
| `drawRoundedRect(renderer, rect, radius, color)` | **Does not actually round the corners** — it just fills a plain rectangle; the `radius` parameter is accepted but unused. Used by the menu's player panels |
| `drawProgressPie(renderer, cx, cy, radius, progress)` | Draws a circle outline (360 points) plus a filled pie-slice wedge for `progress` in `[0,1]` (orange while charging, green at 100%) — used for the ability cooldown indicator in the HUD |

---

## 7. Configuration System

### 7.1 `GameConfig` (`Config.hpp/.cpp`)

A hand-rolled `key=value` file parser — **not** a real INI parser (no sections, no proper escaping). Behavior:
- Reads the whole file via `SDL_RWops` into one string
- Splits on `\n`, skips empty lines and lines starting with `#` or `;`
- Splits each remaining line on the first `=`, trims whitespace from both sides, stores into an `unordered_map<string,string>`
- Typed getters (`getInt`, `getFloat`, `getBool`, `getString`) parse on demand from the string map, falling back to a caller-supplied default if the key is missing
- `getBool` accepts (case-insensitively) `"true"`, `"1"`, `"yes"`, `"on"` as truthy
- `save()` writes the in-memory map back to `assets/config.ini` — **hardcoded path**, ignoring the filename the `GameConfig` instance was originally constructed with. It is also **never called anywhere** in the codebase; the config is effectively read-only at runtime today

A global `extern GameConfig g_config;` is declared in the header but there is **no corresponding definition** anywhere in the `.cpp` files — referencing `g_config` from any new code would fail to link. `Game` instead keeps its own private `GameConfig mConfig` and `mThrustParticleGameConfig` instances rather than using the global.

### 7.2 Particle presets (`.ini` files under `assets/`)

`ParticleConfig::load(GameConfig&)` reads a **different** small key set from whatever `GameConfig` it's given (`playerThrustParticle.ini`, `missileParticle.ini`): a color gradient (`particle_color_count` + `particle_color_N = r,g,b,a,timeMs` lines), plus `growRate`, `friction`, `riseForce`, `vxSpread`, `vyMin`/`vyMax`, `sizeMin`/`sizeMax`. See §16 for how these drive particle motion.

---

## 8. Texture & Audio Systems

### 8.1 `TextureManager` (singleton, `TextureManager.hpp/.cpp`)

Classic Meyers singleton (`getInstance()`), holds `unordered_map<string, unique_ptr<LTexture>>`.
- `loadTexture(id, path)` — loads once, no-ops (returns `true`) if the id already exists
- `loadDirectory(folder, prefix)` — loads every `.png` in a folder, id = `prefix + filename (no extension)`. This is how hats/skins are auto-discovered: every file in `assets/hats/` becomes `hat_<filename>`, every file in `assets/skins/` becomes `skin_<filename>` — **adding a new hat or skin PNG is enough to make it selectable in the menu**, no code change needed for the asset itself (only for wiring an ability to it, see §14)
- `getTexture(id)` — returns `nullptr` silently if not found (no error logged) — callers generally don't check for `nullptr` before calling `->render(...)` on the result, which would segfault if an id is ever wrong

### 8.2 `AudioManager` (`AudioManager.hpp/.cpp`)

Wraps SDL_mixer. `init()` opens the audio subsystem at 44.1kHz stereo and allocates 64 mixing channels. `loadMusic`/`loadSFX` cache by string id. `playSFX(id, loops, channel)` returns the channel it played on (or `-1` on failure/not-found) so callers can later call `stopChannel` on that specific sound (e.g. to cut off a looping jetpack SFX when the player lets go).

> **Bug:** `stopChannel(int channel)` ignores its `channel` parameter entirely and calls `Mix_HaltChannel(-1)`, which halts **every** currently-playing channel, not just the one requested. See §20.

Registered SFX ids (loaded in `Game::loadMedia()`): `jetpackThrust`, `missileLaunch`, `explosion`, `boing`. Registered music: `miniloop14`.

---

## 9. Scene System

Already summarized architecturally in §5.2. Two concrete scenes exist: `MenuScene` and `GameScene`.

---

## 10. Menu Scene — Player Join Flow

`MenuScene` (`MenuScene.hpp/.cpp`) implements a 4-column "press any key/button to join" lobby, one column per potential player slot (`PlayerSlot mSlots[4]`).

### 10.1 Joining

Any recognized input (a keyboard key matching one of the 3 presets, or any joystick hat/axis/button event) that doesn't already belong to a joined slot creates a new slot, provided fewer than 4 players have joined:
- Assigns `presetIndex` (if keyboard) or `joystickId` (if joystick)
- Defaults `skinIndex`/`hatIndex` to `0`, resolving to the first alphabetically-loaded skin/hat id
- Resets `menuCursorY` to `0` (top row)

Analog stick input below the 8000 deadzone is ignored for the "did someone just join" check.

### 10.2 Per-slot menu (once joined)

Each joined slot has 4 selectable rows, navigated with up/down:
0. **Hat** — left/right cycles through all discovered `hat_*` ids
1. **Skin** — left/right cycles through all discovered `skin_*` ids
2. **"Come later"** — a placeholder row with no behavior (literal untranslated string, meant to become a jetpack/trail cosmetic slot later)
3. **Ready?** — left/right/confirm toggles `ready`

Once a slot is `ready`, its row navigation locks (`if(slot.ready){ if(actCancel) slot.ready = false; }` — though note `actCancel` is declared but **never set to `true`** anywhere in `handleEvent`, meaning there is currently no way to un-ready via the documented code path once ready, short of `ESCAPE` leaving the whole menu).

### 10.3 Countdown & start

When every joined slot becomes ready simultaneously, `starting` is set to `true` and a 5000ms countdown (`ticksLeft`) begins, rendered as `"Game start in Ns..."` in the corner. `MenuScene::update()` decrements it by `deltaTime*1000` each frame; at `<= 0` it calls `startGame()`.

`startGame()` pushes a new `GameScene` (constructed from `mSlots`/`mJoinedCount`) onto the `SceneManager`, then resets every slot's `ready`/`menuCursorY` for next time (in case the user returns to the menu later, e.g. by pressing ESCAPE mid-match).

### 10.4 Rendering

Each column draws: a rounded(-ish, see §6.3 caveat) background panel, a scaled-up (5×) preview of the selected skin+hat, then the 4 menu row labels (current row highlighted cyan), and a `"[ READY ]"` banner if ready. Unjoined columns show a dark placeholder with `"Press a touch"` (untranslated French-ism for "press a button").

---

## 11. Game Scene & Match Loop

`GameScene` (`GameScene.hpp/.cpp`) is a thin adapter: on construction it `new Game()`s a `Game` instance, calls `init()`/`loadMedia()`/`start()` on it, and from then on forwards `handleEvent`/`update`/`render` straight through. `ESCAPE` pops the scene (ending the match) directly from `GameScene::handleEvent`. When `Game::isOver()` becomes true (currently only ever set by an `SDL_QUIT` event, there's no in-match "someone won" condition), `GameScene::update` halts all audio channels and pops itself.

> **Leak:** `GameScene` never deletes the `Game*` it allocates with `new` — there is no `~GameScene()` destructor. Every match played leaks one `Game` object (and everything it owns is heap/stack cleaned only for scoped members; the outer `Game` itself never runs its own `~Game()` → `close()` in this path). Not fatal in a short session, but real.

`Game` itself owns and wires together every match-level system: `ProjectileManager`, `ExplosionManager`, `PlayerManager`, `AudioManager`, `ParticleManager`, plus the pizza spawner state and score/health HUD rendering described in §5.3.

### 11.1 Pizza spawner

`Game::update()` maintains `mPizzaTimer` and `mPizzaTimeUntilNext` (a random value in `[0, 999]` ms, re-rolled every time a pizza spawns). When the timer exceeds that threshold, a new `ScoreCollectable` is created off the right edge of the screen at a random height, worth 100 points, moving left at `10x GLOBAL_SPEED` (background scroll speed).

---

## 12. Player System

### 12.1 `PlayerConfig` (`Player.hpp`)

A large plain struct assembled per-player in `Game::init()` from `config.ini` values, the chosen `PlayerSlot`, and shared manager pointers (audio, particle, ability). Notably it is **move-only** in this version (`Player(PlayerConfig&& config)`), and `PlayerManager::addPlayer` takes it by rvalue reference — a deliberate change from letting `Player` be copy-constructed, since it now owns a `unique_ptr<Ability>`.

### 12.2 Physics (`Player::update`)

Every frame, for each living player:

```
vx = (vx + acceleration * deltaTime * dir) * (1 - (1 - deceleration) * deltaTime)
vx clamped to [-maxVx, +maxVx]

vy -= jetpackThrust * deltaTime      // jetpack only applies for the frame it was held
vy -= gravityForce * deltaTime       // gravityForce is negative in config, so this adds downward accel

x += vx * deltaTime
y += vy * deltaTime
```

`dir` (-1/0/+1) and `jetpackThrust` are both **reset to 0/0.0 at the end of every `update()` call** — `move()` and `jetpack()` must be called again every single frame to keep moving/thrusting, which is exactly what `handleInput`/`handleJoystickInput` do each frame based on current key/button state.

**Screen-edge bouncing:** on hitting any of the 4 logical screen edges, the corresponding velocity component is either zeroed (if `player_bounce=false`) or reflected and scaled by `bounceRestitution` (if `true`). A `"boing"` SFX plays whenever the reflected speed exceeds 5 units and bounce is enabled.

**Death:** when `life <= 0`, `isAlive` flips to `false` (prints `"Player dead"` to stdout) and the player stops rendering/updating physics — but the `Player` object itself is never removed from `PlayerManager::players` (a `std::vector<Player>` sized once at match start), so its slot in the HUD strip and player-index-based systems (missile "nearest living player" search, etc.) stay stable for the whole match — dead players are just skipped by `isAlive` checks scattered through `Missile`, `TrafficCone`, `PlayerManager`, etc.

### 12.3 Score & health

- `score` starts at `0`, changed only via `updateScore(delta)` (abilities spend it, pizza pickups add it)
- `life` starts at `config.maxHealth` (default 100), changed via `updateLife(delta)`
- **Turtle passive:** `updateLife` multiplies any life change by `0.75` if `config.skinId == "skin_turtle"` — i.e. turtles take (and heal) 25% less. This is **not documented anywhere in the README** and is easy to miss since it lives inside `updateLife` rather than near the ability system.
- **Squirrel passive:** implemented in `ScoreCollectable::onHit` (§17), not in `Player` — doubles score from pizza pickups for `skin_squirell`. Also undocumented in the README, and the README's TODO list still lists this as *not yet done* (see §21).

### 12.4 Input handling

`handleInput(keys)` (keyboard) and `handleJoystickInput(joystick)` (gamepad) are called every frame from `PlayerManager::update` for every player, and both directly read the raw device state and call `move()`/`jetpack()`/`config.ability->use(this)` inline — there is **no intermediate input abstraction**. A player is hard-assigned to exactly one input source at construction (`config.joystickId != -1` picks the joystick path over the keyboard path unconditionally), so a player cannot use both a keyboard preset and a joystick simultaneously.

### 12.5 Player-vs-player collision (`resolveCollisionWith`)

**This exists and is wired in**, contrary to what the README's TODO list claims (see §21). Implemented as a simple elastic-collision-with-positional-correction pass, called from `PlayerManager::update` for every pair of currently-overlapping, currently-alive players:

1. Compute the overlap on both axes between the two AABBs
2. Resolve along whichever axis has the **smaller** overlap (standard "minimum translation vector" heuristic)
3. Push each player half the overlap distance apart along that axis
4. Apply a 1D elastic-collision velocity exchange along that same axis, using `bounceRestitution` as the restitution coefficient `e`:
   ```
   v1' = (v1 + v2 - e*(v1 - v2)) / 2
   v2' = (v1 + v2 + e*(v1 - v2)) / 2
   ```

This runs as an O(n^2) pairwise loop in `PlayerManager::update` — fine at 2-4 players, would need a broad-phase if the player count ever grew significantly.

### 12.6 Ability integration

Each `Player` owns `std::unique_ptr<Ability> ability` (constructed in `Game::init()` based on the chosen hat — see §14). `Player::update()` calls `ability->update(deltaTime)` every frame (used by abilities that need per-frame state, though none of the currently-compiling abilities actually override it meaningfully). `getAbilityProgress()` exposes `ability->getCooldownProgress()` for the HUD pie indicator.

> Note: `Player::render()` does **not** call anything on `ability` — only `Ability::use`/`update`/`getCooldownProgress` exist on the base class in this version, there is no `Ability::render()`. Any ability that needs to draw a persistent on-screen entity of its own (like a flying sleigh) has no hook to do so yet.

---

## 13. Projectile System

`Projectile` (`Projectile.hpp`) is a tiny abstract base: `update`, `render`, `isDead()` (`= !isAlive`), protected `x/y/vx/vy/isAlive/collider`. `ProjectileManager` (`ProjectileManager.hpp/.cpp`) owns a `std::vector<unique_ptr<Projectile>>`, offers one `spawn(x, y, cfg)` overload per concrete type, updates all of them, then erases the dead ones via `std::remove_if` every frame, then renders the survivors.

Two concrete projectile types currently compile:

### 13.1 `Missile`

A homing projectile (`Missile.hpp/.cpp`):
- On spawn: plays `"missileLaunch"` SFX (tracked by channel so it can be stopped later), starts pointed at angle `0`
- Every frame: finds the nearest living player that isn't its own thrower (falls back to the thrower itself if every other player is dead), computes the angle to that target, turns toward it at a max rate of `precision` rad/s (config, default `3.0`), and moves forward at `velocity` scaled by an "alignment ratio" — the missile moves at only 80% speed at worst (never slower) when badly misaligned, ramping to full speed once pointed roughly at the target. This produces a slightly wobbly, not-perfectly-locked-on homing feel rather than instant tracking.
- Spawns thrust particles from its tail every 5ms of elapsed time (via its own `LTimer`, checked every update — effectively "as often as possible" at normal frame rates)
- Triggers an explosion once within `explosionTriggerRange` of its target, after a further `explosionDelay` ms grace period (both configurable) — i.e. proximity arms it, then a short fuse actually detonates it
- `explode()`: spawns a visual explosion via `ExplosionManager`, applies falloff damage (`maxDamage * (1 - distSq/maxRadiusSq)`) and directional knockback (`maxForce` scaled the same way) to every living player within `maxRadius = explosionConfig.power * 50`, stops its own launch SFX channel, plays `"explosion"`, and triggers an 8-unit / 0.3s screen shake

### 13.2 `TrafficCone`

A ground-launched cone (`TrafficCone.hpp/.cpp`) with a two-phase behavior:
1. **Drifting phase:** moves left at the shared `globalSpeed` (background scroll speed pointer, so it drifts with the world) until any living, non-thrower player's **x-coordinate** comes within `triggerRange` of the cone's x-coordinate (only X is checked to arm it, not Y/distance)
2. **Launched phase:** once armed, it ignores horizontal drift and instead flies straight **up** at `speed` px/s, spawning thrust particles from its base every 5ms, until the nearest player is now *above* it (`nearest->y > y`), at which point it explodes with the same falloff damage/knockback/shake pattern as `Missile::explode` (using `TrafficConeConfig::maxDamage` instead)

`TrafficConeAbility::use()` always spawns the cone at the **same fixed spot** — bottom-right corner of the play area (`screenWidth + 10, screenHeight - collider.h`) — not near the caster, so its drift phase always starts from the right edge regardless of who casts it.

---

## 14. Ability System

`Ability` (`Ability.hpp`) is the abstract base: `use(Player*)` (pure virtual — the "press ability button" action), `update(deltaTime)` (default no-op, most abilities don't override it), `getCooldownProgress()` (shared implementation: `timeSinceLast.getTicks() / cooldown`, clamped to `[0,1]`). Protected state: `cooldown` (ms), `cost` (score), `timeSinceLast` timer.

Hat -> ability wiring happens once, in `Game::init()`, by string-comparing `playerSlot[i].hatId` against known ids:

| Hat id | Ability class | Cost | Cooldown | Behavior |
|---|---|---|---|---|
| *(anything unmatched — the `else` branch)* | `MissileAbility` | 200 (config `score_to_launch_missile`, though the ability's own `cost` field is hardcoded `200` in `Ability.cpp` and the config value isn't actually threaded into it) | 5000 ms | Spawns a `Missile` from a random point just off-screen, targeting the nearest other living player |
| `hat_trafficCone` | `TrafficConeAbility` | 200 | 2000 ms | Spawns a `TrafficCone` at the fixed bottom-right spawn point |
| `hat_witch` | `FreezeAbility` | 500 | 1000 ms | Zeroes the velocity of every other living player within `radius=300px` (constructed with `duration=0`, which is stored but **never used** — the "freeze" has no actual time-based lockout, it's a one-shot velocity zero, not a sustained freeze) |
| `hat_kamikaze` | `KamikazeAbility` | 100 | 2000 ms | Self-detonates at the caster's position: explosion visual, falloff damage/knockback to every *other* living player (does not hurt the caster) within `maxRadius = explosionConfig.power * 50` (here `power=3` -> 150px radius), 16-unit/0.3s screen shake |
| *(anything else, e.g. `hat_cook`, `hat_cowboy`, `hat_soldier`, `hat_grade`, or no hat at all)* | falls through to the same `else` -> `MissileAbility` | — | — | — |

### 14.1 `ChristmasAbility` — broken, does not compile

`include/Ability.hpp` also declares a `ChristmasAbility` class, but its declaration is **syntactically incomplete**:

```cpp
class ChristmasAbility : public Ability{
    ChristmasAbility(std::vector<player::Player>* players, explode::ExplosionManager& mgr, explode::ExplosionConfig cfg, AudioManager* audioManager)
}
```

— no `use()` override (required, since it's pure virtual on the base), no closing `;` after the class body, and the constructor is declared under an implicit `private:` (default access for `class`) with no matching definition in `Ability.cpp`. Attempting to build this codebase **fails immediately** on `Ability.cpp`/anything that includes `Ability.hpp` (i.e. everything gameplay-related) with a compiler error at that line. No hat currently maps to `hat_christmas` in `Game::init()` either, so even if the class compiled, nothing would construct it yet.

`ChristmasSleigh` (`Christmas.hpp/.cpp`) is a related, separately-broken piece: the header declares a 4-argument constructor (`players, config, explosionManager*, explosionConfig`), but `Christmas.cpp` only implements a 2-argument constructor (`players, config`) with an **empty body** — the two don't match, and nothing else in `Christmas.cpp` is implemented (`KidnapMe`, `update`, `render` are declared but have no definitions at all). `assets/christmasSleigh.png` and `assets/gifts/blue.png` exist on disk but are never loaded by `TextureManager` anywhere.

In short: the Christmas/Santa hat concept from the README's TODO is authored as headers/stubs only, none of it currently runs, and the project **will not compile** until `ChristmasAbility` is either finished or removed.

### 14.2 `include/MissileAbility.hpp`

An empty file (0 bytes). Likely a leftover from before `MissileAbility` was folded into `Ability.hpp`/`Ability.cpp` directly. Harmless (nothing includes it) but dead weight.

---

## 15. Explosion Engine (`libs/ExplosionEngine`)

A self-contained, reusable sub-library (own `include/`/`src/`, own `makefile`) providing the visual explosion + screen-shake effect shared by every damage source.

- `ExplosionConfig` — `power` (scales radius/particle count), `groundY` (unused by callers currently, always default `-1`), and boolean toggles `fireballs`/`debris`/`embers`/`smoke` for which particle sub-effects to include
- `Explosion` — owns a `vector<unique_ptr<BaseParticle>>`, spawned once at a position, updates/renders/reports `isDone()` once all its particles expire
- `ExplosionManager` — owns a `vector<Explosion>`, exposes `spawn(x, y, cfg)`, `update`, `render`, `count()`, and the **screen-shake** API: `triggerShake(intensity, duration)` plus `getShakeX()`/`getShakeY()` which `Game::render()` reads every frame to offset the render viewport (`SDL_RenderSetViewport`) for a camera-shake effect
- Particle sub-types: `BaseParticle` (abstract), `EmberParticle`, `FireballParticle` — every damage-dealing system in the game (`Missile`, `TrafficCone`, `KamikazeAbility`) constructs its own `ExplosionConfig` inline rather than sharing one, so tuning "what an explosion looks like" is currently duplicated per call site instead of centralized

This library is generic enough that it doesn't reference `player::Player` at all — it only deals in coordinates and configs, which is why it's split into its own `libs/` folder with its own build.

---

## 16. Particle System

Distinct from the Explosion Engine above — this is the simpler system used for jetpack thrust trails and cone/missile exhaust (`Particle.hpp/.cpp`, `ParticleManager.hpp/.cpp`).

- `ParticleConfig::load(GameConfig&)` reads a color gradient (`particle_color_count` keyframes, each `r,g,b,a,timeMs`) and precomputes a **per-millisecond lookup table** (`colorFrameList`, one `SDL_Color` per ms of the particle's lifetime) by linearly interpolating between consecutive keyframes — trading memory for a cheap `O(1)` color lookup by elapsed time during rendering (`getCurrentColor(intCurrentTime)` is just an array index)
- `ThrustParticle` — the only concrete particle type. On `init()`, rolls random `vx` (from `vxSpread`), `vy` (from `[vyMin,vyMax]`), and `size` (from `[sizeMin,sizeMax]`) from the config, all divided by 100 for `vx`/`vy` to keep per-frame increments small since velocity is added directly to position every frame without being multiplied by `deltaTime` (`fx += vx`, not `fx += vx*deltaTime`) — meaning particle speed is implicitly frame-rate dependent, unlike every other moving entity in the game
- `ParticleManager` keeps a **fixed pool** of `2000` `ThrustParticle`s (`MAX_THRUST_PARTICLES`), round-robin allocated (`spawnThrustParticle` overwrites whichever slot is next, whether or not it's still alive — a particle can be forcibly killed early if the pool wraps around fast enough, which is very unlikely in practice with 4 players but not structurally impossible under extreme thrust spam)
- Rendering iterates the pool in reverse-recency order (newest first) so newer particles draw on top

Two `.ini` presets exist: `playerThrustParticle.ini` (jetpack trail) and `missileParticle.ini` (missile/cone exhaust) — both loaded once into `Game::mThrustParticleConfig` in `Game::init()` and then copied into every `PlayerConfig`/`MissileConfig`/`TrafficConeConfig` that needs one (each copy is independent after that point; there is no shared/live-reloadable particle config).

---

## 17. Collectables

`Collectable` (`Collectable.hpp/.cpp`) is a small abstract base: position/velocity/collider/`isAlive`, a shared `render()` that just blits `cTexture` at `(x,y)`, and two pure virtuals (`update`, `onHit`) left to subclasses.

The only concrete subclass is `ScoreCollectable` (`ScoreCollectable.hpp/.cpp`), used for pizza:
- `init(scoreOnHit, textureId)` grabs the texture from `TextureManager` and remembers the score value
- `update(deltaTime, players)` moves it by `vx/vy * deltaTime`, then checks AABB collision against every living player; on the first hit, calls `onHit()` and stops (the early `return` inside the loop means only the first colliding player in iteration order can ever pick it up in the same frame, which is fine since only one player usually reaches it first anyway)
- `onHit(player)` — awards `cScore`, or **double** `cScore` if `player.getSkinId() == "skin_squirell"` (the squirrel passive, see §12.3), then marks itself dead for cleanup

There is currently only one collectable type wired up (pizza); `assets/collectables/poison.png` exists on disk but nothing in the codebase references a "poison" texture id or spawns a poison collectable — it's an asset waiting for a feature (matches the README TODO's "cook drops poisoned food" idea, not yet implemented).

---

## 18. Configuration Reference (`config.ini`)

All keys read via `GameConfig::get*` in `Game::init()`/`Game::Game()`, with the fallback shown as the default if the key is absent:

| Key | Type | Default | Used for |
|---|---|---|---|
| `SCREEN_WIDTH` | int | 800 | Fallback window width (actual runtime resolution is queried from the renderer via `SDL_RenderGetLogicalSize`, so this mostly matters before that call) |
| `SCREEN_HEIGHT` | int | 600 | Same, height |
| `PLAYER_NUMBER` | int | 2 | Fallback player count if `Game` is constructed without going through the menu's `joinedCount` |
| `player_jetpack_force` | float | 700.0 | Upward thrust while jetpack held |
| `player_max_vx` | float | 1000.0 | Horizontal speed cap |
| `player_acceleration` | float | 1000.0 | Horizontal acceleration while moving |
| `player_deceleration` | float | 0.8 | Horizontal drag factor per second (closer to 1.0 = less drag) |
| `player_health` | int | 100 | Starting/max HP |
| `player_bounce` | bool | true | Whether hitting a screen edge bounces (true) or just stops (false) |
| `bounce_restitution` | float | 0.4 | Bounce energy retained (edges) **and** the `e` coefficient in player-vs-player collision (§12.5) — one value serves both purposes |
| `score_to_launch_missile` | int | 200/100 (see note) | Intended as the missile ability's score cost — **see §20**, the value actually used by `MissileAbility` is a separate hardcoded `200` in `Ability.cpp`, not this config value |
| `show_player_collider` | bool | false | Debug: draw player hitbox outline |
| `gravity` | float | -500.0 | Downward acceleration (negative sign baked into the formula, see §12.2) |
| `music` | bool | true (code default) | Whether `Game::start()` plays background music — the shipped `config.ini` sets this to `false` |
| `missile_precision` | float | 3.0 | Missile max turn rate, rad/s |
| `missile_velocity` | float | 1000.0 | Missile forward speed at full alignment |
| `missile_explosion_trigger_range` | float | 70.0 | Distance to target that arms the detonation fuse |
| `show_missile_collider` | bool | false | Debug: draw missile hitbox outline |
| `missile_explosion_delay` | int | 70 | ms between arming and actually detonating |
| `missile_max_dmg` | float | 40.0 | Missile explosion peak damage |

Note on `score_to_launch_missile`: the shipped `assets/config.ini` sets it to `100`, `Player.hpp`'s `PlayerConfig` default is `200`, and `Game::init()` reads it with a fallback of `200` into `cfg.scoreToLaunchMissile` — but `cfg.scoreToLaunchMissile` is **never read anywhere** in `Missile.cpp`, `Ability.cpp`, or `Player.cpp`. The actual cost enforced at cast time is `MissileAbility`'s own `cost = 200;` line, hardcoded in its constructor. The config key and the `PlayerConfig` field are effectively dead as far as the missile ability's real cost is concerned.

`TrafficConeAbility`, `FreezeAbility`, and `KamikazeAbility` costs/cooldowns (200/2000, 500/1000, 100/2000 respectively) are **not** exposed via `config.ini` at all — they're hardcoded directly in each ability's constructor in `Ability.cpp`.

---

## 19. Assets Reference

```
assets/
├── skins/
│   ├── turtle.png      -> skin_turtle   (25% damage taken/healed, see §12.3)
│   └── squirell.png    -> skin_squirell (2x score from pizza, see §17)
├── hats/
│   ├── christmas.png     -> hat_christmas    (no ability wired; would-be ChristmasAbility, see §14.1)
│   ├── cook.png          -> hat_cook         (no ability wired, falls back to Missile)
│   ├── cowboy.png        -> hat_cowboy       (no ability wired, falls back to Missile)
│   ├── grade.png         -> hat_grade        (no ability wired, falls back to Missile — "grade" = graduate/diplome)
│   ├── kamikaze.png      -> hat_kamikaze     (KamikazeAbility)
│   ├── soldier.png       -> hat_soldier      (no ability wired, falls back to Missile)
│   ├── trafficCone.png   -> hat_trafficCone  (TrafficConeAbility)
│   └── witch.png         -> hat_witch        (FreezeAbility)
├── collectables/
│   ├── pizza.png   -> used (ScoreCollectable, +100 score)
│   └── poison.png  -> unused (no collectable spawns it yet)
├── gifts/blue.png            -> unused (would belong to the broken Christmas ability)
├── christmasSleigh.png       -> unused, same reason
├── buttons/playBtn.png       -> loaded by MenuScene, but MenuScene never renders it — the actual "start" mechanism is the all-ready countdown, not a clickable button. `playBtnHitbox` is computed but nothing in `handleEvent` checks mouse clicks against it, and there is no `SDL_MOUSEBUTTONDOWN` handling in `MenuScene::handleEvent` at all.
├── missile00.png, trafficCone.png, dot.bmp, abstract_seamless_bg_01.png  -> in active use (missile sprite, cone sprite, legacy Dot texture, scrolling background)
├── config.ini, missileConfig.ini*, missileParticle.ini, playerThrustParticle.ini
│   (* `missileConfig.ini` exists on disk but nothing in the code loads a `GameConfig` from that
│      specific filename — only `config.ini` and `playerThrustParticle.ini` are constructed by `Game`;
│      `missileParticle.ini` is present but not referenced by `Game::Game()` either — check before assuming
│      missile particle tuning comes from that file)
├── KiwiSoda.ttf       -> menu UI font
├── pixelfont.ttf      -> in-match HUD font (score/player name)
└── Hypermonosaturation-zrMo0.ttf  -> present on disk, not loaded anywhere in the code
```

---

## 20. Known Issues & Technical Debt

Concrete, verified-in-source issues, roughly ordered by how much they'd bite you:

1. **Build is currently broken.** `ChristmasAbility` in `Ability.hpp` is an incomplete class declaration (missing `use()` override, missing closing `;`). Nothing compiles until this is fixed or removed. See §14.1.
2. **`AudioManager::stopChannel(channel)` halts every channel, not the one passed in** (`Mix_HaltChannel(-1)` instead of `Mix_HaltChannel(channel)`). In practice this means, e.g., landing from a jetpack burst can cut off an in-progress missile-launch or explosion sound early, since "stop my jetpack loop" actually stops everything.
3. **`GameScene` leaks its `Game*`.** `new Game()` in the constructor, no destructor to `delete` it. Every match played leaks one `Game` and everything transitively owned by value inside it that isn't otherwise cleaned (most subsystems clean themselves via their own destructors when the leaked `Game` object's members are torn down at process exit, but the leak itself is real and would show up in a leak-checker).
4. **Global `extern GameConfig g_config;`** is declared in `Config.hpp` with no definition anywhere — dead declaration, would fail to link if anything tried to use it.
5. **`util::drawRoundedRect` doesn't round corners** — it silently degrades to a plain filled rect, `radius` is accepted but ignored. Cosmetic only (menu panels), but the name overpromises.
6. **Two skin/hat passives exist but aren't documented or discoverable in-game**: turtle's 25% damage reduction (`Player::updateLife`) and squirrel's 2x pizza score (`ScoreCollectable::onHit`). A new player has no way to learn these exist short of reading the source — there's no tooltip/description shown in the menu for hats or skins.
7. **`score_to_launch_missile` config key is effectively dead** — the real missile cost is hardcoded to `200` in `MissileAbility`'s constructor in `Ability.cpp`. Changing the config value in `config.ini` has zero effect on gameplay. Same story for the other 3 abilities' cost/cooldown, which have no config exposure at all.
8. **`FreezeAbility`'s `duration` constructor parameter is stored but never used.** It's constructed with `duration=0` and the ability just zeroes velocity once — there's no timed "frozen" state, despite the name and the stored field implying one.
9. **Particle motion is frame-rate dependent.** `ThrustParticle::update` does `fx += vx` / `fy += vy` without multiplying by `deltaTime`, unlike every other moving entity in the codebase (players, missiles, cones all multiply by `deltaTime`). Thrust trail speed/behavior will visibly change if the frame rate changes.
10. **`playBtn.png` / `playBtnHitbox` are dead code in `MenuScene`.** The texture loads, the hitbox rect is computed, but nothing renders the button or checks mouse input against it — the countdown-on-all-ready flow is the only way to start a match today.
11. **`FreezeAbility`'s un-ready path is unreachable.** `actCancel` in `MenuScene::handleEvent` is declared but never set `true`, so the `if(slot.ready){ if(actCancel) slot.ready = false; }` branch in the ready-lock logic can never trigger from the documented inputs.
12. **`Dot`/`mDot` is dead legacy code** (an SDL "Lazy Foo" tutorial leftover — a single bouncing square). `Game::update()` still calls `mDot.move()` every frame, but `Game::render()` never renders it, so it silently ticks with zero visible effect and zero purpose.
13. **`Hypermonosaturation-zrMo0.ttf`, `missileConfig.ini`, `assets/gifts/blue.png`, `assets/christmasSleigh.png`, `assets/collectables/poison.png` are all unreferenced assets** sitting on disk — either leftover from removed features or prepared ahead of unfinished ones (poison, Christmas sleigh).
14. **No win condition / game-over state.** A match only ends via `ESCAPE` (pop back to menu) or the whole app quitting; players dying doesn't end the match or declare a winner, they just stop rendering/updating and sit frozen on screen for the rest of the round.
15. **`SceneManager::change()` and `Scene::isDone()` are unused dead API surface** — every real scene transition in the codebase goes through `push`/`pop` called directly from `handleEvent`/`update`, not through `change` or by returning `true` from `isDone`.
16. **`include/MissileAbility.hpp` and `include/EffectManager.hpp` are empty files** — no content, nothing includes them, safe to delete but currently just clutter.
17. **Dead player objects are never removed from the players vector**, and there's no upper bound check anywhere assuming a shrinking list — every system that iterates `playerManager.players` (missile targeting, cone targeting, collision resolution, HUD rendering) has to individually check `isAlive` rather than relying on the container only holding live players.

---

## 21. Feature Status vs README TODO

The `README.md` TODO list is stale in both directions — some items are already done, one whole feature area is broken rather than "not started":

| README TODO item | Actual status |
|---|---|
| Collision entre joueur *(player collision)* | **Already implemented and wired** (`Player::resolveCollisionWith`, called from `PlayerManager::update`) — the TODO checkbox is simply out of date |
| Passif de l'écureuil : 200% points sur pizza | **Already implemented** (`ScoreCollectable::onHit`) — again, stale checkbox |
| La capa du père Noël (cadeaux-bombes) | **Not implemented — actively broken.** Header/stub only, does not compile (§14.1) |
| Refaire toutes les textures | Still placeholder-level art, matches TODO status |
| Missiles qui spawn aléatoirement autour de l'écran | Partially true: `MissileAbility` already uses `util::spawnOffScreen` for random edge spawning. What's *not* done is the separate "random missile event" item further down the list (an unprompted, timed missile targeting everyone, independent of any player casting an ability) |
| Explosions avec souffle qui décale le joueur, screen shake | **Already implemented** for every damage source that goes through `ExplosionManager` (knockback + `triggerShake` calls exist in `Missile`, `TrafficCone`, `KamikazeAbility`) — TODO appears to predate this work |
| Gore / tête ramassable pour 1000 points | Not implemented — no gore asset, no such collectable class |
| Plot (traffic cone) qui spawn au sol et explose | The traffic cone ability that exists today is a *thrown* projectile with a drift->launch->explode behavior (§13.2), not a passive "spawns on the ground automatically" mechanic — closer to done than not, but not exactly matching the TODO's original description |
| Passif du cuistot (bouffe empoisonnée) | Not implemented — `hat_cook` falls through to the default `MissileAbility`, `poison.png` asset exists but unused |
| Soldat : appel stratégique | Not implemented — `hat_soldier` falls through to `MissileAbility` |
| Cowboy : ralentit le temps (dead eye) | Not implemented — `hat_cowboy` falls through to `MissileAbility` |
| Diplômé : PowerPoint qui arrache | Not implemented — `hat_grade` falls through to `MissileAbility` |
| Jetpack nucléaire | Not implemented |
| Event missile aléatoire pour éteindre tout le monde | Not implemented — every missile currently only exists because a player cast the ability |

---

## 22. Roadmap

In rough priority order given the current state:

1. **Fix the build.** Finish or remove `ChristmasAbility`/`ChristmasSleigh` so the project compiles again — this blocks everything else.
2. Fix the `AudioManager::stopChannel` bug (one-line fix, but affects every ability that plays a stoppable looping/tracked sound).
3. Wire the still-unmapped hats (`cook`, `cowboy`, `soldier`, `grade`) to real abilities per the README TODO list, following the existing `Ability` subclass pattern.
4. Decide whether `score_to_launch_missile` and the other hardcoded ability costs/cooldowns should move into `config.ini` for real, or whether the config key should just be removed to stop implying it does something.
5. Add a win condition / match-end state (currently a match only ever ends by leaving it).
6. Clean up dead code identified in §20 (empty headers, unused `Dot`, unused `g_config`, unreferenced assets) — low risk, immediate clarity win.
7. Multiplayer-over-network is a much larger, separate effort (decoupling input collection from `Player`, moving to an authoritative-host model, etc.) and should come after the above stabilization work, not before it.

---

## 23. Frame-by-Frame Data Flow

A single iteration of the main loop, spelled out call-by-call, once a `GameScene` is on top of the stack. Arrows show who calls whom; indentation shows nesting.

```
main.cpp loop
├── SDL_PollEvent loop
│   └── GameScene::handleEvent(e)
│       ├── ESCAPE -> SceneManager::pop()            (ends the match immediately)
│       └── else -> Game::handleEvents(e)
│           └── SDL_QUIT -> mQuit = true
│
├── GameScene::update(deltaTime)
│   ├── Game::isOver() check -> if true: halt audio, SceneManager::pop(), return
│   └── Game::update(deltaTime)
│       ├── mDot.move()                              (dead code, §20.12)
│       ├── scroll offset -= GLOBAL_SPEED * dt
│       ├── ProjectileManager::update(dt)
│       │   ├── for each projectile: Missile::update / TrafficCone::update
│       │   │   (each internally may call ->explode(), setting isAlive=false)
│       │   └── erase all isDead() projectiles
│       ├── ExplosionManager::update(dt)
│       │   ├── advance every Explosion's particles, drop finished ones
│       │   └── decay active screen-shake toward 0
│       ├── PlayerManager::update(dt)
│       │   ├── for each player:
│       │   │   ├── Player::update(dt)          (physics integration, bounce, ability->update)
│       │   │   └── Player::handleInput / handleJoystickInput
│       │   │       └── may call Ability::use(player) if the ability button is held
│       │   └── O(n^2) pass: util::collide + Player::resolveCollisionWith for overlapping pairs
│       ├── ParticleManager::update(dt)
│       │   └── every alive ThrustParticle::update(dt)
│       ├── erase dead pizza (ScoreCollectable), update remaining ones
│       │   └── ScoreCollectable::update -> may call onHit() -> Player::updateScore
│       └── pizza spawn timer check -> maybe emplace_back a new ScoreCollectable
│
└── GameScene::render()
    └── Game::render()
        ├── apply shake offset to render viewport (ExplosionManager::getShakeX/Y)
        ├── draw two tiled copies of the scrolling background
        ├── ExplosionManager::render         (explosions drawn under players)
        ├── PlayerManager::render            (skin + hat + optional collider outline)
        ├── ParticleManager::render          (thrust trails, drawn over players)
        ├── pizza render loop
        ├── ProjectileManager::render        (missiles / cones)
        ├── reset viewport to full screen (shake no longer applies to HUD)
        └── per-player HUD strip loop (see §24 for the exact geometry)
```

Everything above happens within a single `deltaTime`-scaled step; there is no fixed-timestep sub-stepping or interpolation between renders — one simulation step always corresponds to exactly one render.

---

## 24. HUD Layout Geometry

The per-player HUD strip at the bottom of the screen (`Game::render()`, last loop) is computed entirely from `mScreenWidth`, `mPlayerNumber`, and a fixed `indicatorRect.h = 50`:

```
indicatorRect.w = mScreenWidth / mPlayerNumber
indicatorRect.h = 50
indicatorRect.y = mScreenHeight - 50
indicatorRect.x = playerIndex * indicatorRect.w          // one column per player, left to right

backLifeRect (the grey health bar background):
  x = indicatorRect.x + indicatorRect.w * 0.15
  y = indicatorRect.y + indicatorRect.h * 0.50
  w = indicatorRect.w * 0.5
  h = indicatorRect.h * 0.25

lifeRect (the green foreground, shrinks as health drops):
  same x/y/h as backLifeRect
  w = backLifeRect.w * (life / maxLife), or 0 if life <= 0

ability cooldown pie:
  centerX = backLifeRect.x + backLifeRect.w + indicatorRect.w * 0.15
  centerY = indicatorRect.y + indicatorRect.h * 0.5
  radius  = 16   (fixed)
```

Draw order within each column: grey column background -> red full-width life-bar backing -> green life-bar foreground (scaled by current/max life) -> white cooldown pie outline + colored progress wedge (orange while on cooldown, green once ready) -> a small skin+hat portrait anchored to the column's right edge (`(i+1)*indicatorRect.w - 42`) -> "Player N" label -> score label underneath it.

Every one of these — `LTexture scoreTexture; LTexture playerNumberTexture;` — is a **stack-local `LTexture` re-created and re-rendered-to-text from scratch every single frame, for every player**, inside the render loop. There's no caching of "Player 2"'s label texture between frames even though it never changes, and the score texture is rebuilt from scratch every frame purely because the underlying number can change — but so is the player-number label, which never does. This is a real (if currently invisible-at-4-players) performance cost: `TTF_RenderText_Blended` + `SDL_CreateTextureFromSurface` + `SDL_DestroyTexture` run up to 8 times per frame (2 labels x 4 players) just for static or slowly-changing text.

---

## 25. Appendix A — Full Struct & Class Field Reference

This appendix lists every config struct and every class's member data in one place, for quick lookup without opening the header.

### `player::PlayerConfig` (`Player.hpp`)

| Field | Type | Default | Notes |
|---|---|---|---|
| `jetpackForce` | float | 700.0 | |
| `maxVy` | float | 600.0 | Declared but **not referenced anywhere** in `Player.cpp` — vertical speed is never actually clamped to this |
| `maxVx` | float | 1000.0 | |
| `acceleration` | float | 1000.0 | |
| `deceleration` | float | 0.5 (struct default; `Game::init()` overrides with config `player_deceleration`, default 0.8) | |
| `gravityForce` | float | -500.0 | |
| `bounceRestitution` | float | 0.4 | |
| `maxHealth` | int | 100 | |
| `scoreToLaunchMissile` | int | 200 | Dead field, see §18/§20 |
| `bounce` | bool | false (struct default; overridden to config `player_bounce`, default true) | |
| `screenWidth` / `screenHeight` | int | 800 / 600 | |
| `showCollider` | bool | false | |
| `collider` | `SDL_Rect` | `{0,0,32,32}` | Local-space offset+size, added to world x/y each frame |
| `colliderColor` | `SDL_Color` | magenta `{255,0,255,255}` | |
| `keyPreset` | `KeyPreset` | — | |
| `joystickId` | int | -1 | -1 means "use keyboard" |
| `skin` / `hat` | `LTexture*` | — | Raw non-owning pointers into `TextureManager` |
| `hatId` / `skinId` | `std::string` | `""` | Used for passive checks (`skin_turtle`, `skin_squirell`) |
| `audioManager` | `AudioManager*` | — | |
| `players` | `std::vector<Player>*` | — | Back-pointer to the shared player list, used by abilities/missiles for targeting |
| `thrustParticleConfig` | `ParticleConfig` | — | Copied in, not shared live |
| `particleManager` | `ParticleManager*` | — | |
| `ability` | `std::unique_ptr<Ability>` | `nullptr` | Set once in `Game::init()`, never reassigned afterward |

### `projectile::MissileConfig` (`Missile.hpp`)

| Field | Type | Default |
|---|---|---|
| `players` | `std::vector<player::Player>*` | `nullptr` |
| `throwerIndex` | int | -1 |
| `precision` | float | 3.0 |
| `velocity` | float | 1000.0 |
| `explosionTriggerRange` | float | 70.0 |
| `launchSFX` | `Mix_Chunk*` | `NULL` — declared but unused; the launch sound is actually played by id (`"missileLaunch"`) through `AudioManager`, not via this raw pointer |
| `textureId` | `std::string` | `"missile"` |
| `collider` | `SDL_Rect` | `{0,0,32,32}` |
| `showCollider` | bool | false |
| `particleConfig` | `ParticleConfig` | — |
| `particleManager` | `ParticleManager*` | — |
| `explosionManager` | `explode::ExplosionManager*` | — |
| `explosionConfig` | `explode::ExplosionConfig` | — |
| `maxDamage` | float | 50.0 |
| `explosionDelay` | int | 70 |
| `audioManager` | `AudioManager*` | — |

### `projectile::TrafficConeConfig` (`TrafficCone.hpp`)

| Field | Type | Default |
|---|---|---|
| `players` | `std::vector<player::Player>*` | `nullptr` |
| `throwerIndex` | int | -1 |
| `speed` | float | 2000.0 (upward launch speed) |
| `explosionTriggerRange` | float | 30.0 — declared, but the actual arming check in `TrafficCone::update` uses `triggerRange`, not this field; the two are easy to confuse by name |
| `globalSpeed` | `float*` | `nullptr` — pointer into `Game::GLOBAL_SPEED`, used during the drift phase |
| `triggerRange` | float | *(no default — uninitialized unless set by the caller)* — `Game::init()` always sets it explicitly to `20.f`, so this is not hit in practice today, but the struct itself provides no safe default |
| `textureId` | `std::string` | `"trafficCone"` |
| `collider` | `SDL_Rect` | `{0,0,16,16}` |
| `showCollider` | bool | false |
| `particleConfig` | `ParticleConfig` | — |
| `particleManager` | `ParticleManager*` | — |
| `explosionManager` | `explode::ExplosionManager*` | — |
| `explosionConfig` | `explode::ExplosionConfig` | — |
| `maxDamage` | float | 50.0 |
| `audioManager` | `AudioManager*` | — |

### `explode::ExplosionConfig` (`Explosion.hpp`, ExplosionEngine)

| Field | Type | Default |
|---|---|---|
| `power` | float | 1.0 — drives both particle count/spread inside the engine and, by convention at every call site in this game (not enforced by the engine itself), `maxRadius = power * 50` and `maxForce = power * 500` for damage/knockback falloff |
| `groundY` | float | -1 (unused by any current caller) |
| `fireballs` / `debris` / `embers` / `smoke` | bool | all `true` |

### `ChristmasSleighConfig` (`Christmas.hpp`) — belongs to the non-compiling feature, listed for completeness

| Field | Type | Default |
|---|---|---|
| `maxDamage` | float | 50.0 |
| `maxRadius` | float | 10.0 |
| `sleighSpeed` | float | 100.0 |
| `dropPercent` | float | 0.1 |
| `giftMinDetonation` / `giftMaxDetonation` | int | 500 / 10000 (ms) |
| `giftMinSpeed` / `giftMaxSpeed` | float | 3.0 / 10.0 |
| `screenWidth` / `screenHeight` | int | 666 / 666 *("come later..." — literal comment in the source next to `screenHeight`)* |

### `PlayerSlot` (`PlayerSlot.hpp`) — one per menu column

| Field | Type | Default | Notes |
|---|---|---|---|
| `presetIndex` | int | -1 | Which of the 3 keyboard presets, or -1 if this slot is joystick-controlled |
| `ready` | bool | false | |
| `skinId` / `hatId` / `jetpackId` | `std::string` | `""` | `jetpackId` is declared but **never assigned or read anywhere** — a cosmetic slot planned but not built (matches the menu's "Come later" placeholder row) |
| `skinIndex` / `hatIndex` / `jetpackIndex` | int | 0 | Index into the loaded id lists; `jetpackIndex` unused for the same reason as `jetpackId` |
| `joystickId` | int | -1 | -1 means keyboard |
| `menuCursorY` | int | 0 | Which of the 4 menu rows is selected |
| `lastAxisX` / `lastAxisY` | int | 0 | Edge-detection state for analog stick menu navigation (so holding the stick doesn't repeat-navigate every frame) |

### `player::Player` private fields not already covered above (`Player.hpp`)

| Field | Type | Purpose |
|---|---|---|
| `x, y` | float | World position (top-left of the sprite) |
| `vx, vy` | float | Current velocity |
| `score` | int | |
| `dir` | int | -1/0/1, reset every frame after `update()` consumes it |
| `DEAD_ZONE` | `static constexpr int` = 8000 | Joystick axis deadzone |
| `skin`, `hat` | `LTexture*` | Cached copies of `config.skin`/`config.hat` (duplicated pointers — `getSkin()`/`getHat()` actually return `config.skin`/`config.hat` directly, so these two private members appear to be unused leftovers shadowing the config's copies) |
| `jetpackThrust` | float | Set by `jetpack()`, consumed and reset to 0 every `update()` |
| `thrustParticlesTimer` | `LTimer` | Gates thrust-particle spawn rate to every 20ms |
| `missileTableSize` | int | Declared, **never used** |
| `players` | `Player*` | Declared, **never used** (the real cross-player access goes through `config.players`, a `vector<Player>*`, not this raw pointer) |
| `playerTableSize` | int | Declared, **never used** |
| `life` | int | |
| `jetpackChannel` | int | Audio channel currently playing the jetpack loop, or -1 |
| `mJetpackActive` | bool | Tracks whether the jetpack SFX loop is currently playing, to avoid re-triggering it every frame the button is held |
| `config` | `PlayerConfig` | Owns the whole configuration by value after the move-construction |

---

## 26. Appendix B — File Size Reference

For orientation when opening the codebase for the first time, every source file over 20 lines, largest first:

| File | Lines |
|---|---|
| `src/Game.cpp` | 325 |
| `src/MenuScene.cpp` | 295 |
| `src/Player.cpp` | 279 |
| `src/Missile.cpp` | 153 |
| `src/Ability.cpp` | 152 |
| `src/Particle.cpp` | 147 |
| `include/Player.hpp` | 130 |
| `src/TrafficCone.cpp` | 112 |
| `src/LTexture.cpp` | 102 |
| `src/main.cpp` | 101 |
| `src/Utils.cpp` | 101 |
| `src/AudioManager.cpp` | 99 |
| `include/Game.hpp` | 99 |
| `src/Config.cpp` | 85 |
| `include/Ability.hpp` | 69 |
| `include/Particle.hpp` | 67 |
| `src/LTimer.cpp` | 65 |
| `include/Missile.hpp` | 57 |
| `include/TrafficCone.hpp` | 53 |
| `src/TextureManager.cpp` | 50 |
| `include/MenuScene.hpp` | 46 |
| `include/LTexture.hpp` | 40 |
| `include/Christmas.hpp` | 40 |
| `src/GameScene.cpp` | 39 |
| `src/ProjectileManager.cpp` | 35 |
| `include/TextureManager.hpp` | 35 |
| `src/PlayerManager.cpp` | 32 |
| `src/ParticleManager.cpp` | 32 |

`Game.cpp`, `MenuScene.cpp`, and `Player.cpp` together are roughly a quarter of the entire `src/` line count — these three are the highest-value files to read first when getting oriented, since almost every other class exists to be configured and driven by them.

---

## 27. Glossary

| Term | Meaning in this codebase |
|---|---|
| **Hat** | A cosmetic layer drawn over the player skin that also determines which `Ability` the player gets (§14). Selected in the menu, fixed for the whole match. |
| **Skin** | The player's base character sprite. Two exist (`turtle`, `squirell`); each carries a hidden passive (§12.3, §17) unrelated to the ability system. |
| **Capa** *(the French dev's own shorthand, kept in the README TODO)* | Short for "capacité" — an ability. |
| **Thrower** | The player who cast an ability that spawned a projectile; stored as `throwerIndex` so the projectile can exclude them from its own targeting/damage where relevant (e.g. a missile won't target its own caster while other players are alive). |
| **Trigger range vs. explosion range** | Two *different* config fields exist with similar names across `Missile`/`TrafficCone` (`explosionTriggerRange` vs `triggerRange`) — see the caveat in the `TrafficConeConfig` table in §25. |
| **GLOBAL_SPEED** | `Game`'s background scroll speed (px/s), also reused as the traffic cone's drift speed and the pizza's horizontal speed multiplier — one knob currently controls three unrelated-looking things. |
| **Alignment ratio** | The missile-homing term for how well-aimed the missile currently is at its target; scales its forward speed between 80% and 100% (§13.1). |

---

## 28. Appendix C — Class Hierarchy Diagrams

### Abstract base classes and their implementations

```
Scene (Scene.hpp)
├── MenuScene
└── GameScene

Ability (Ability.hpp)
├── MissileAbility        — compiles, wired to any unmatched hat
├── TrafficConeAbility     — compiles, wired to hat_trafficCone
├── FreezeAbility          — compiles, wired to hat_witch
├── KamikazeAbility        — compiles, wired to hat_kamikaze
└── ChristmasAbility       — DOES NOT COMPILE (§14.1), not wired to any hat

Projectile (Projectile.hpp)
├── Missile
└── TrafficCone
    (Gift/ChristmasSleigh do NOT inherit Projectile — they aren't
     Projectile subclasses at all in this version, they're free-standing,
     unfinished classes with their own ad-hoc update/render methods)

Collectable (Collectable.hpp)
└── ScoreCollectable       — the only concrete collectable (pizza)

Particle (Particle.hpp)
└── ThrustParticle         — the only concrete particle (jetpack/missile/cone trails)

explode::BaseParticle (ExplosionEngine, separate hierarchy)
├── EmberParticle
└── FireballParticle
```

### Ownership / ""who holds a pointer to whom""

```
main.cpp
└── SceneManager (stack of Scene)
    └── GameScene
        └── Game* (leaked, see §20.3)
            ├── PlayerManager
            │   └── vector<Player>              <- pointed to by every *Config struct's `players` field
            ├── ProjectileManager
            │   └── vector<unique_ptr<Projectile>>
            ├── ExplosionManager
            │   └── vector<Explosion>
            ├── ParticleManager
            │   └── ThrustParticle[2000] (fixed pool, not heap-allocated per-particle)
            ├── AudioManager                     <- pointed to by every *Config struct's `audioManager` field
            └── each Player
                └── unique_ptr<Ability>          <- itself holds a raw pointer back to ProjectileManager/
                                                     ExplosionManager/AudioManager to spawn effects
```

Almost every config struct in the game (`PlayerConfig`, `MissileConfig`, `TrafficConeConfig`) carries the same trio of raw back-pointers — `players`, `audioManager`, `particleManager`/`explosionManager` — because there is no service locator or dependency-injection container; every object that needs to reach a shared system is handed a raw pointer to it explicitly at construction time, one field at a time. This is consistent throughout the codebase but means adding a new shared system requires threading a new pointer field through every config struct that will need it.

---

## 29. Appendix D — Suggested Reading Order

If you're getting reacquainted with this codebase after time away, reading in this order tracks how data actually flows through the game rather than alphabetically:

1. `main.cpp` — the whole process lifecycle in ~100 lines
2. `Scene.hpp` / `SceneManager.hpp/.cpp` — the two-scene skeleton everything else hangs off
3. `MenuScene.hpp/.cpp` — how players join and configure themselves before a match
4. `Game.hpp/.cpp` — the actual match: init wiring (hat -> ability), the frame loop, the HUD
5. `Player.hpp/.cpp` + `PlayerManager.hpp/.cpp` — physics, input, collision, the two hidden passives
6. `Ability.hpp/.cpp` — read this right after `Player`, since abilities only make sense in terms of what they do *to* a `Player`. Stop and note the broken `ChristmasAbility` at the bottom.
7. `Projectile.hpp`, then `Missile.hpp/.cpp` and `TrafficCone.hpp/.cpp` — what abilities actually spawn
8. `Collectable.hpp/.cpp` + `ScoreCollectable.hpp/.cpp` — the pizza loop, and where the squirrel passive actually lives
9. `Particle.hpp/.cpp` + `ParticleManager.hpp/.cpp`, then `libs/ExplosionEngine/` — the two separate "things fly around and fade out" systems, and why there are two of them instead of one
10. `Config.hpp/.cpp` + `TextureManager.hpp/.cpp` + `AudioManager.hpp/.cpp` — the three plumbing/loader systems everything above depends on, saved for last because they're the least game-specific and easiest to understand once you already know what they're loading data *for*

Everything in `include/`/`src/` not listed above (`Dot`, `EffectManager.hpp`, `MissileAbility.hpp`, `Christmas.hpp/.cpp`) is either dead code or an unfinished stub — see §20 before spending time trying to understand what they're "supposed" to do; in most cases the answer is "nothing yet."

---

## 30. If You Only Read One Section

Given everything above, the single highest-leverage fact for picking this project back up is: **the codebase does not currently compile**, because of the incomplete `ChristmasAbility` declaration in `include/Ability.hpp` (§14.1). Every other observation in this document — the stale README TODO items, the dead fields, the `stopChannel` bug — is secondary until that one class either gets a proper `use()` override and a closing `;`, or gets deleted outright.