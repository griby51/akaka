# Documentation du code — akaka

Documentation interne du C++. Elle couvre chaque classe et chaque fonction, fichier par fichier,
avec les pieges et le code mort identifie. Pour l'API de modding Lua, voir `DocLua.md`.

## Vue d'ensemble de l'architecture

Le jeu est en cours de passage en multijoueur en ligne, avec une architecture client-serveur
autoritaire. La separation qui en decoule structure tout le code :

| Couche | Fichiers | Regle |
|---|---|---|
| Simulation | `World`, `Player`, `Projectile`, `Collectable`, `ScriptEngine` | ne lit aucune entree SDL, ne joue aucun son, ne dessine rien |
| Serialisation | `Snapshot`, `Protocol`, `ByteBuffer`, `AssetIds` | traduit l'etat en octets, sans connaitre le transport |
| Transport | `Transport`, `LoopbackTransport`, `EnetClient`, `EnetServer` | deplace des octets, sans connaitre leur sens |
| Orchestration | `Server`, `Client` | le serveur simule et diffuse, le client envoie ses entrees et affiche |
| Presentation | `Game`, `AudioManager`, `EffectManager`, `ParticleManager`, `LTexture` | consomme un `Snapshot` et sa liste d'evenements |

Les six regles a ne jamais casser :

1. la simulation empile des `GameEvent` au lieu d'appeler l'audio ou le rendu
2. une fonction `write*` et sa `read*` se modifient toujours ensemble
3. tout ce qui vient du reseau est borne et plafonne avant allocation
4. ce qui dure se deduit de l'etat, ce qui arrive une fois est un evenement
5. ce qui ne change jamais ne va pas dans un `Snapshot` (voir `PlayerInfo`, table d'assets)
6. les handles de son et les `netId` ne sont jamais recycles

Le mode solo passe par `LoopbackTransport` : aucune socket n'est ouverte, mais le protocole est
exerce a chaque partie.

## Sommaire


**Moteur, simulation et reseau**

- [include/Game.hpp + src/Game.cpp](#includegamehpp-srcgamecpp)
- [include/World.hpp + src/World.cpp](#includeworldhpp-srcworldcpp)
- [include/Server.hpp + src/Server.cpp](#includeserverhpp-srcservercpp)
- [include/Client.hpp + src/Client.cpp](#includeclienthpp-srcclientcpp)
- [include/Transport.hpp](#includetransporthpp)
- [include/LoopbackTransport.hpp + src/LoopbackTransport.cpp](#includeloopbacktransporthpp-srcloopbacktransportcpp)
- [include/Protocol.hpp + src/Protocol.cpp](#includeprotocolhpp-srcprotocolcpp)
- [include/Snapshot.hpp + src/Snapshot.cpp](#includesnapshothpp-srcsnapshotcpp)
- [include/ByteBuffer.hpp](#includebytebufferhpp)
- [include/AssetIds.hpp + src/AssetIds.cpp](#includeassetidshpp-srcassetidscpp)
- [include/GameEvent.hpp + src/GameEvent.cpp](#includegameeventhpp-srcgameeventcpp)
- [include/PlayerInput.hpp](#includeplayerinputhpp)
- [include/InputSampler.hpp + src/InputSampler.cpp](#includeinputsamplerhpp-srcinputsamplercpp)
- [include/PlayerInfo.hpp](#includeplayerinfohpp)
- [include/GameContext.hpp](#includegamecontexthpp)
- [src/main.cpp](#srcmaincpp)

**Gameplay et scripting**

- [include/Player.hpp + src/Player.cpp](#includeplayerhpp-srcplayercpp)
- [include/PlayerManager.hpp + src/PlayerManager.cpp](#includeplayermanagerhpp-srcplayermanagercpp)
- [include/Projectile.hpp + src/Projectile.cpp](#includeprojectilehpp-srcprojectilecpp)
- [include/ProjectileManager.hpp + src/ProjectileManager.cpp](#includeprojectilemanagerhpp-srcprojectilemanagercpp)
- [include/Collectable.hpp + src/Collectable.cpp](#includecollectablehpp-srccollectablecpp)
- [include/ScoreCollectable.hpp + src/ScoreCollectable.cpp](#includescorecollectablehpp-srcscorecollectablecpp)
- [include/Ability.hpp + src/Ability.cpp](#includeabilityhpp-srcabilitycpp)
- [include/LuaAbility.hpp + src/LuaAbility.cpp](#includeluaabilityhpp-srcluaabilitycpp)
- [include/ScriptEngine.hpp + src/ScriptEngine.cpp](#includescriptenginehpp-srcscriptenginecpp)
- [include/Utils.hpp + src/Utils.cpp](#includeutilshpp-srcutilscpp)
- [include/KeyPreset.hpp](#includekeypresethpp)
- [include/PlayerSlot.hpp](#includeplayerslothpp)

**Presentation, ressources et scenes**

- [include/LTexture.hpp + src/LTexture.cpp](#includeltexturehpp-srcltexturecpp)
- [include/LTimer.hpp + src/LTimer.cpp](#includeltimerhpp-srcltimercpp)
- [include/TextureManager.hpp + src/TextureManager.cpp](#includetexturemanagerhpp-srctexturemanagercpp)
- [include/Animation.hpp + src/Animation.cpp](#includeanimationhpp-srcanimationcpp)
- [include/AnimationManager.hpp + src/AnimationManager.cpp](#includeanimationmanagerhpp-srcanimationmanagercpp)
- [include/AudioManager.hpp + src/AudioManager.cpp](#includeaudiomanagerhpp-srcaudiomanagercpp)
- [include/EffectManager.hpp + src/EffectManager.cpp](#includeeffectmanagerhpp-srceffectmanagercpp)
- [include/Particle.hpp + src/Particle.cpp](#includeparticlehpp-srcparticlecpp)
- [include/ParticleManager.hpp + src/ParticleManager.cpp](#includeparticlemanagerhpp-srcparticlemanagercpp)
- [include/Config.hpp + src/Config.cpp](#includeconfighpp-srcconfigcpp)
- [include/Scene.hpp](#includescenehpp)
- [include/SceneManager.hpp + src/SceneManager.cpp](#includescenemanagerhpp-srcscenemanagercpp)
- [include/MenuScene.hpp + src/MenuScene.cpp](#includemenuscenehpp-srcmenuscenecpp)
- [include/GameScene.hpp + src/GameScene.cpp](#includegamescenehpp-srcgamescenecpp)
- [assets/config.ini](#assetsconfigini)
- [assets/playerThrustParticle.ini](#assetsplayerthrustparticleini)

---


# Moteur, simulation et reseau

## include/Game.hpp + src/Game.cpp

### `class Game`

Couche de **présentation** du jeu : elle possède le `Server` (simulation autoritaire), le `Client` (consommateur de snapshots) et le `LoopbackLink` qui les relie en solo sans aucune socket. Elle ne lit jamais directement l'état de simulation pour dessiner : elle dessine le dernier `Snapshot` reçu par le client, et joue les sons / effets issus de la liste d'événements embarquée dans ce snapshot. Elle reste toutefois couplée au monde serveur pour la configuration (dimensions d'écran) et pour l'échantillonnage des entrées (elle lit les `KeyPreset` / `joystickId` stockés dans les `Player` de la simulation).

| Membre | Type | Rôle |
|---|---|---|
| `mWindow` | `SDL_Window*` | Fenêtre SDL reçue par `init`. **Écrit, jamais relu** (voir code mort). |
| `mRenderer` | `SDL_Renderer*` | Renderer SDL non possédé, utilisé par tout le rendu et par les `LTexture` du HUD. |
| `mController` | `SDL_Joystick*` | Toujours `nullptr` : jamais ouvert, seulement fermé dans `close()`. Vestige de l'ancien système d'input. |
| `mConfig` | `GameConfig` | `assets/config.ini` : dimensions, constantes de gameplay joueur, activation musique. |
| `mThrustParticleGameConfig` | `GameConfig` | `assets/playerThrustParticle.ini`, source du `ParticleConfig` de la poussée jetpack. |
| `mPlayerNumber` | `int` | Nombre de joueurs. Initialisé depuis l'ini (`PLAYER_NUMBER`) puis **écrasé** par `joinedCount` dans `init`. Sert seulement à découper la largeur du HUD. |
| `mQuit` | `bool` | Drapeau de sortie, levé sur `SDL_QUIT`, lu par `isOver()`. |
| `audioManager` | `AudioManager` | Mixer SDL_mixer : musique + SFX. Purement présentation. |
| `effectManager` | `EffectManager` | Animations one-shot (explosions) et screen-shake. Purement présentation. |
| `mLink` | `LoopbackLink` | Les deux files d'octets en mémoire (vers serveur / vers client) du mode solo. |
| `mLoopServer` | `LoopbackServer` | Implémentation `ServerTransport` adossée à `mLink`, `clientId = 0`. |
| `mLoopClient` | `LoopbackClient` | Implémentation `ClientTransport` adossée à `mLink`, `clientId = 0`. |
| `mServer` | `Server` | Simulation autoritaire + boucle à pas fixe + émission des snapshots. |
| `mClient` | `Client` | Envoi des inputs, réception/désérialisation du snapshot courant. |
| `mPlayerInfos` | `std::vector<PlayerInfo>` | Données **statiques** par joueur (skin, chapeau, vie max, collider) que le snapshot ne transporte pas ; indexé par `PlayerState::index`. |
| `mOwnedPlayers` | `std::vector<uint8_t>` | Indices des joueurs contrôlés localement, transmis au `Client` dans `start()` ; en solo c'est `0..joinedCount-1`. |
| `particleManager` | `ParticleManager` | Particules. Vit côté présentation mais son adresse est injectée dans `World::context` → les abilities Lua peuvent spawner des particules directement (couplage restant à casser pour le vrai réseau). |
| `mScoreFont` | `TTF_Font*` | Police 14 px du HUD, possédée (fermée dans `close()`). |
| `mThrustParticleConfig` | `ParticleConfig` | Config de particule copiée dans chaque `PlayerConfig`. |
| `mWhite` / `mRed` / `mGreen` | `SDL_Color` | Couleurs de texte. `mRed` **jamais utilisé**. |
| `mSfxChannels` | `std::unordered_map<uint32_t,int>` | Table `handle d'événement SFX → canal SDL_mixer`, indispensable pour honorer un `EventType::StopSfx` dont seul le handle est transmis sur le réseau. |

#### `Game();`
- **Rôle** : construit les deux `GameConfig` depuis leurs fichiers ini, construit `mLoopServer` / `mLoopClient` sur `&mLink` avec `clientId = 0`, puis pré-remplit les dimensions d'écran du monde serveur et `mPlayerNumber` depuis l'ini.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : lit deux fichiers sur disque ; écrit `mServer.world().screenWidth/screenHeight` (pixels) et `mPlayerNumber`.
- **Pièges** : l'ordre des initialiseurs suit l'ordre de déclaration dans le header, pas celui de la liste ; `mLoopServer`/`mLoopClient` capturent `&mLink`, donc `Game` n'est **ni copiable ni déplaçable** en sécurité (pointeurs internes). Les valeurs `SCREEN_WIDTH`/`SCREEN_HEIGHT` lues ici sont immédiatement écrasées par `SDL_RenderGetLogicalSize` dans `init`.

#### `~Game();`
- **Rôle** : appelle `close()`.
- **Effets de bord** : libère la police et (théoriquement) le joystick.
- **Pièges** : n'appelle pas `audioManager.clean()` explicitement — c'est le destructeur de `AudioManager` qui s'en charge.

#### `bool init(SDL_Renderer* renderer, SDL_Window* window, PlayerSlot* playerSlots, int joinedCount);`
*(défini sous le nom de paramètre `playerSlot`)*
- **Rôle** : point de montage de toute la partie. Initialise l'audio, initialise le `World` (branchement du `GameContext`), **câble les transports** (`mServer.setTransport(&mLoopServer)`, `mClient.setTransport(&mLoopClient)`), récupère la taille logique réelle du renderer, puis crée un `player::Player` par slot ainsi que son `PlayerInfo` miroir.
- **Paramètres** :
  - `renderer` : renderer SDL non possédé, stocké dans `mRenderer`.
  - `window` : fenêtre SDL, stockée dans `mWindow` (et jamais relue).
  - `playerSlots` : tableau brut de `PlayerSlot` venant du menu (skin, chapeau, preset clavier, `joystickId`). **Pas de vérification de nullité.**
  - `joinedCount` : nombre d'éléments valides du tableau ; devient `mPlayerNumber` et la taille de `mPlayerInfos` / `mOwnedPlayers`.
- **Retour** : toujours `true` (aucun échec signalé, même si une texture ou une ability manque).
- **Effets de bord** : `srand(time(NULL))` ; `audioManager.init()` ; `World::init()` ; `context.particleManager = &particleManager` ; écrit `screenWidth`, `screenHeight`, `effectiveHeight = screenHeight - 50` (pixels, la bande de 50 px du bas est réservée au HUD) ; `players.reserve(joinedCount)` ; charge `mThrustParticleConfig` ; pour chaque slot appelle `ScriptEngine::createAbilityForHat` (exécution de Lua) et `playerManager.addPlayer(std::move(cfg))`.
- **Pièges** :
  - Doit être appelé **avant** `loadMedia()` et `start()` (c'est ce que fait `GameScene`), mais il consomme déjà `TextureManager::getTexture(skinId/hatId)` : ça ne marche que parce que `MenuScene` a déjà fait les `loadDirectory("assets/hats/")` / `("assets/skins/")`. `Game::loadMedia()` les refait après coup.
  - `cfg.keyPreset` n'est renseigné **que si** `playerSlot[i].presetIndex >= 0`. Sinon `KeyPreset` (aucun initialiseur de membre) reste **indéterminé** ; si en plus `joystickId == -1`, `input::sample` indexera `keys[scancode indéterminé]` → lecture hors limites / UB.
  - `info.colliderW/H` sont lus dans `cfg.collider` **avant** toute personnalisation → toujours la valeur par défaut `{0,0,32,32}`.
  - `reserve(joinedCount)` est essentiel : `PlayerConfig::players` pointe sur le vecteur, et plusieurs systèmes gardent des pointeurs/références vers les `Player` ; une réallocation ultérieure les invaliderait.
  - `createAbilityForHat` peut renvoyer `nullptr` (chapeau sans `ability` déclarée dans Lua) — voir le piège de `captureSnapshot`.

#### `bool loadMedia();`
- **Rôle** : charge les assets de présentation : textures de chapeaux/skins, l'animation `explosion_missile`, la police du HUD, 4 SFX et une musique.
- **Paramètres** : aucun.
- **Retour** : `true` si tout va bien ; `false` **uniquement** si `TTF_OpenFont` échoue. Les échecs de `loadSFX`/`loadMusic`/`loadDirectory` sont ignorés.
- **Effets de bord** : remplit `TextureManager` et `AnimationManager` (singletons globaux), alloue `mScoreFont`, remplit les maps de `audioManager`.
- **Pièges** : la variable locale est déclarée `int success = true;` (typage incohérent avec le `bool` de retour, sans conséquence). L'`Animation` est codée en dur ici (`64×64`, 6 colonnes, 30 frames, 0.03 s/frame, pas de boucle) et non pilotée par les mods.

#### `void start();`
- **Rôle** : démarre la partie : blend mode, musique, mesure de la largeur du fond (nécessaire au défilement calculé dans `World::step`), démarrage du monde, puis déclaration au client des joueurs possédés localement.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : `SDL_SetRenderDrawBlendMode(BLEND)` ; joue `miniloop14` si `music=true` dans l'ini, volume 32/128 ; second `srand(time(0))` ; écrit `mServer.world().backgroundWidth` (pixels) ; `World::start()` (armement du timer de pizzas) ; `mClient.setOwnedPlayers(mOwnedPlayers)`.
- **Pièges** : doit être appelé après `init()` **et** après `loadMedia()`/le chargement des mods, sinon `getTexture("bg")` renvoie `nullptr` et `backgroundWidth` reste à 0 → `scrollingOffset` est remis à 0 à chaque frame (fond figé). La texture `"bg"` n'est pas chargée par le C++ mais par `mods/base/textures.lua`.

#### `void handleEvents(const SDL_Event& e);`
- **Rôle** : traite les événements SDL bruts qui concernent la présentation : quitter, et F1 comme raccourci de debug qui spawne une explosion au centre.
- **Paramètres** : `e` — événement SDL déjà dépilé par `main`.
- **Retour** : —
- **Effets de bord** : met `mQuit = true` sur `SDL_QUIT` ; sur F1 appelle `effectManager.spawn("explosion_missile", w/2, effectiveHeight/2)`.
- **Pièges** : aucune entrée de gameplay ne passe plus par ici — les entrées sont **échantillonnées par polling** dans `update()` via `InputSampler`. F1 spawne l'effet **localement**, en court-circuitant l'`EventQueue` : en réseau seul ce client le verrait.

#### `void update(float realDeltaTime);`
- **Rôle** : une itération complète de la boucle client-serveur : échantillonner les entrées → les envoyer au serveur → faire avancer le serveur → relever les snapshots → faire avancer les systèmes purement visuels → consommer les événements du snapshot.
- **Paramètres** : `realDeltaTime` — temps écoulé **en secondes** depuis la frame précédente (déjà bridé à 0.05 s par `main`).
- **Retour** : —
- **Effets de bord** : construit un `std::vector<PlayerInput>` (une allocation par frame) ; `mClient.sendInputs` (sérialise et pousse dans `mLink`) ; `mServer.update` (fait tourner la simulation et diffuse éventuellement un snapshot) ; `mClient.poll` (désérialise) ; `particleManager.update` / `effectManager.update` ; `drainEvents` sur les événements du snapshot courant.
- **Pièges** :
  - L'ordre est **obligatoire** : envoyer les inputs avant `mServer.update()` donne une latence nulle en loopback ; inverser l'ordre ajouterait une frame de retard.
  - Les entrées sont échantillonnées en itérant sur `mServer.world().playerManager.players` pour y lire `getKeyPreset()` / `getJoystickId()`. C'est le dernier couplage présentation→simulation : un vrai client distant n'a pas accès à ces `Player`.
  - `inputs[i]` est positionnel : il correspond à l'indice `i` dans le vecteur de joueurs, et `mOwnedPlayers` (qui est `0..n-1`) sert d'étiquettes dans le message. Si ces deux listes divergeaient, les entrées seraient attribuées au mauvais joueur.
  - `drainEvents` n'est appelé que si un snapshot existe ; tant qu'aucun snapshot n'est arrivé, aucun son n'est joué.

#### `void drainEvents(const std::vector<GameEvent>& events);`
- **Rôle** : traduit les `GameEvent` du snapshot en appels concrets audio/visuels, puis fait le ménage dans la table des canaux SFX.
- **Paramètres** : `events` — liste d'événements du snapshot courant (référence non possédée, valide le temps de l'appel).
- **Retour** : —
- **Effets de bord** :
  - `Sfx` : `audioManager.playSFX(AssetIds::name(e.id))` ; si le canal retourné est `>= 0`, enregistre `mSfxChannels[e.handle] = channel`.
  - `StopSfx` : cherche `e.handle`, coupe le canal, retire l'entrée.
  - `Effect` : `effectManager.spawn(nom, e.x, e.y, e.a)` — `a` porte l'échelle.
  - `Shake` : `effectManager.triggerShake(e.a, e.b)` — `a` = intensité (pixels), `b` = durée (secondes).
  - En fin de fonction, parcourt `mSfxChannels` et supprime toute entrée dont le canal ne joue plus (sinon la map grossirait indéfiniment, et un handle pourrait pointer vers un canal recyclé par un autre son).
- **Pièges** : la résolution `e.id → nom` passe par le singleton `AssetIds`, dont la table est construite **à la volée** côté serveur ; un vrai client distant qui n'aurait pas enregistré les mêmes noms dans le même ordre résoudrait les ids vers les mauvais assets (voir `AssetIds`). Le `switch` n'a pas de `default` : un `EventType` corrompu venu du réseau est silencieusement ignoré (comportement souhaitable ici). La boucle de nettoyage utilise correctement `it = erase(it)`.

#### `void render();`
- **Rôle** : garde-fou devant le rendu réel.
- **Retour** : —
- **Effets de bord** : délègue à `renderSnapshot(mClient.snapshot())`.
- **Pièges** : si aucun snapshot n'est encore arrivé, retour immédiat **sans** `SDL_RenderPresent` → la ou les premières frames restent sur le contenu précédent du back buffer.

#### `void renderSnapshot(const Snapshot& snap);`
- **Rôle** : dessine une frame entière à partir du seul snapshot : viewport décalé par le screen-shake, fond défilant, joueurs (skin + chapeau + collider de debug), particules, effets, collectables, projectiles, puis le HUD hors viewport (barre de vie, score, camembert de cooldown d'ability, avatar).
- **Paramètres** : `snap` — snapshot désérialisé ; toutes les positions sont en pixels, `angle` en degrés (passé tel quel à `LTexture::render`).
- **Retour** : —
- **Effets de bord** : appels SDL de dessin ; **crée deux `LTexture` par joueur et par frame** (`loadFromRenderedText`) pour le numéro et le score ; `SDL_RenderPresent` en fin de fonction.
- **Pièges** :
  - `bg->render(...)` est appelé **sans test de nullité** (contrairement aux skins/chapeaux) → segfault si la texture `"bg"` n'a pas été chargée par les mods.
  - Lit encore `mServer.world().screenWidth/screenHeight` au lieu de les prendre dans le snapshot : non transposable à un client distant.
  - Le fond est dessiné deux fois (`scrollingOffset` et `scrollingOffset + largeur`) pour le défilement sans couture ; ça suppose `backgroundWidth >= screenWidth`.
  - `mPlayerInfos[p.index]` est protégé par `if(p.index >= mPlayerInfos.size()) continue;` (indispensable, `index` vient du réseau).
  - Le HUD est indexé par la **position `i` dans le vecteur** (`"Player i+1"`, teinte de gris), mais les données statiques par `p.index` : les deux divergeraient dès qu'un joueur disparaîtrait du snapshot.
  - `info.maxLife > 0` est bien testé avant la division. `progress = p.abilityProgress / 255.f` reconstitue le flottant quantifié à l'émission.
  - La création de `LTexture` par frame est un coût d'allocation/upload GPU récurrent (candidat à une mise en cache).
  - `SDL_RenderSetViewport(mRenderer, NULL)` avant le HUD est obligatoire, sinon le HUD tremblerait avec le screen-shake.

#### `void close();`
- **Rôle** : libère les ressources SDL possédées.
- **Retour** : —
- **Effets de bord** : `TTF_CloseFont(mScoreFont)` puis `mScoreFont = nullptr` ; `SDL_JoystickClose(mController)` puis `nullptr`.
- **Pièges** : idempotent (tests de nullité). La branche joystick est morte : `mController` n'est jamais affecté. Ne détruit ni le renderer ni la fenêtre (non possédés — c'est `main` qui le fait).

#### `bool isOver();`
- **Rôle** : indique à `GameScene` qu'il faut dépiler la scène.
- **Retour** : `mQuit`.
- **Pièges** : non `const` alors qu'elle pourrait l'être.

**Code mort / membres inutilisés de ce fichier :**
- `mWindow` : écrit dans `init`, jamais relu.
- `mController` : jamais ouvert ; la branche correspondante de `close()` est inatteignable.
- `mRed` : jamais utilisé.
- `mPlayerNumber` : la valeur lue depuis `config.ini` (`PLAYER_NUMBER`) est toujours écrasée par `joinedCount`.
- Les valeurs `SCREEN_WIDTH`/`SCREEN_HEIGHT` du constructeur sont toujours écrasées par `SDL_RenderGetLogicalSize`.
- `tm.loadDirectory("assets/hats/"...)` / `("assets/skins/"...)` dans `loadMedia` sont redondants : `MenuScene` les a déjà chargés et `init` les a déjà consommés.
- `PlayerState::vx`, `vy`, `isControlled`, `thrusting` et `EntityState::netId`, `w`, `h` sont transmis dans le snapshot mais **jamais lus** par le rendu.

---

## include/World.hpp + src/World.cpp

### `class World`

Simulation **pure** et autoritaire : aucun appel de rendu SDL, aucun appel audio. Tout ce qui doit être vu ou entendu est poussé dans l'`EventQueue events`, consommée plus tard par la présentation via le snapshot. Elle agrège les managers de joueurs, de projectiles et la liste des collectables, et expose un `GameContext` de pointeurs que les scripts Lua utilisent pour agir sur le monde.

| Membre | Type | Rôle |
|---|---|---|
| `playerManager` | `player::PlayerManager` | Possède le `std::vector<Player>` ; mis à jour avec le vecteur d'inputs. |
| `projectileManager` | `projectile::ProjectileManager` | Possède les projectiles (`shared_ptr`) et attribue leurs `netId`. |
| `pizzas` | `std::vector<ScoreCollectable>` | Collectables de score, spawnés par `step` et purgés quand `isAlive == false`. |
| `events` | `EventQueue` | File d'événements de présentation produits durant le tick ; sérialisée dans le snapshot puis vidée par le `Server`. |
| `context` | `GameContext` | Agrégat de pointeurs vers les systèmes du monde, câblé par `init()` et passé aux abilities Lua. |
| `globalSpeed` | `float` | Vitesse de défilement du décor, en pixels/seconde (50 par défaut) ; sert aussi de base à la vitesse des pizzas. |
| `scrollingOffset` | `float` | Décalage horizontal courant du fond, en pixels, toujours ≤ 0. |
| `tick` | `uint32_t` | Compteur de pas de simulation, incrémenté en fin de `step`, recopié dans le snapshot. |
| `screenWidth` | `int` | Largeur logique de jeu, en pixels (défaut 800, écrasé par `Game::init`). |
| `screenHeight` | `int` | Hauteur logique totale, en pixels (défaut 600). |
| `effectiveHeight` | `int` | Hauteur jouable = `screenHeight - 50`, la bande du bas étant le HUD. Sert de sol/plafond. |
| `backgroundWidth` | `float` | Largeur de la texture de fond, en pixels ; seuil de rebouclage du défilement. Renseigné par `Game::start`. |
| `mPizzaTimer` | `LTimer` | Chronomètre (SDL_GetTicks, **millisecondes de temps réel**) du prochain spawn de pizza. |
| `mPizzaTimeUntilNext` | `int` | Délai tiré au hasard avant le prochain spawn, en millisecondes (0–999). |

#### `void init();`
- **Rôle** : câble le `GameContext` sur les membres du `World` (une seule fois, après construction).
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : écrit les 7 pointeurs `context.events`, `players`, `projectiles`, `globalSpeed`, `screenWidth`, `screenHeight`, `effectiveHeight`.
- **Pièges** : **ne renseigne pas** `context.particleManager` — c'est `Game::init` qui le fait juste après, parce que le `ParticleManager` vit côté présentation. À appeler avant toute création de joueur ou d'ability (les abilities capturent le `GameContext*`). Comme `context` contient des pointeurs vers des membres de `*this`, déplacer ou copier un `World` laisserait des pointeurs pendants.

#### `void start();`
- **Rôle** : arme le premier spawn de pizza.
- **Retour** : —
- **Effets de bord** : `mPizzaTimeUntilNext = rand() % 1000` (ms) ; `mPizzaTimer.start()`.
- **Pièges** : à appeler après `init()` et après `srand()`. Utilise `rand()` global et un timer basé sur le temps **réel**, pas sur le compteur de ticks : la simulation n'est donc **pas déterministe** ni reproductible à partir des seules entrées — bloquant si tu veux un jour du lockstep ou de la prédiction client avec rollback.

#### `void step(float deltaTime, const std::vector<PlayerInput>& inputs);`
- **Rôle** : un pas de simulation complet : défilement, projectiles, joueurs, purge et mise à jour des collectables, spawn éventuel d'une pizza, incrément du tick.
- **Paramètres** :
  - `deltaTime` — pas de temps **en secondes** ; appelé par le `Server` avec la constante `FIXED_DT` (1/60 s).
  - `inputs` — un `PlayerInput` par joueur, indexé positionnellement comme `playerManager.players`. Le `Server` garantit `inputs.size() == players.size()`.
- **Retour** : —
- **Effets de bord** : décrémente `scrollingOffset` de `globalSpeed * deltaTime` et le remet à 0 une fois passé `-backgroundWidth` ; `projectileManager.update` ; `playerManager.update` (c'est là que les inputs sont appliqués et que les abilities, donc le Lua, s'exécutent et poussent des événements) ; `erase/remove_if` sur `pizzas` ; `update` de chaque pizza (détection de collecte, donc score et événements) ; éventuellement `emplace_back` d'une pizza initialisée à 100 points, texture `"pizza"`, position `x = screenWidth`, `y` aléatoire dans `[0, effectiveHeight-16)`, `vx = -globalSpeed * 10`, collider 16×16 ; incrémente `tick`.
- **Pièges** :
  - Si `backgroundWidth` vaut 0, `scrollingOffset` est remis à 0 à chaque pas → fond figé.
  - La purge (`remove_if`) a lieu **avant** la mise à jour : une pizza tuée pendant ce pas survit dans le vecteur jusqu'au pas suivant. Sans conséquence visible car `captureSnapshot` saute les `!isAlive`.
  - `pizzas.emplace_back()` peut réallouer et invalider toute référence/pointeur gardé vers une pizza existante.
  - `rand() % (effectiveHeight - 16)` : division par zéro / modulo négatif si `effectiveHeight <= 16`.
  - Le timer de pizza est en temps réel alors que `step` est appelé à pas fixe : en cas de rattrapage multi-pas, plusieurs pas consécutifs peuvent voir le même dépassement résolu une seule fois, ou aucun — le rythme de spawn dérive du nombre de pas.
  - Le vecteur `inputs` n'est pas vérifié en taille ici ; c'est `PlayerManager::update` qui doit le faire.

**Code mort / membres inutilisés de ce fichier :** aucun. Tous les membres sont écrits et lus. À noter en revanche que `screenHeight` est exposé via le `GameContext` alors que la simulation utilise surtout `effectiveHeight`.

---

## include/Server.hpp + src/Server.cpp

### Constantes de fichier

#### `static constexpr float FIXED_DT = 1.f / 60.f;`
Pas de simulation fixe, en secondes (16,67 ms). Déclaré au niveau global du header avec `static` → une copie à liaison interne par unité de traduction qui inclut `Server.hpp` (inoffensif, mais `constexpr` seul suffirait, et `inline constexpr` serait plus propre).

#### `static constexpr int MAX_STEPS_PER_FRAME = 5;`
Plafond de pas de simulation par appel à `update`, garde-fou contre la spirale de la mort (une frame lente qui en provoque d'autres).

### `class Server`

Côté **autoritaire** : il possède le `World`, reçoit les messages d'input via un `ServerTransport` abstrait, fait avancer la simulation à pas fixe et diffuse des snapshots sérialisés. Il ignore totalement SDL, le rendu et le type concret de transport — c'est ce qui permet au mode solo de passer par le `LoopbackTransport` et à un futur mode en ligne de brancher des sockets sans toucher cette classe.

| Membre | Type | Rôle |
|---|---|---|
| `mWorld` | `World` | La simulation possédée. |
| `mTransport` | `ServerTransport*` | Transport non possédé ; si `nullptr`, `update` ne fait **rien du tout**. |
| `mInputs` | `std::vector<PlayerInput>` | Dernières entrées connues par indice de joueur ; **persistantes** entre frames (un input non renvoyé est réutilisé tel quel). |
| `mSnapshotCounter` | `uint32_t` | Nombre de pas depuis le dernier snapshot émis. |
| `mAccumulator` | `float` | Reste de temps réel non consommé, en secondes. |
| `mSnapshotInterval` | `uint32_t` | Nombre de pas entre deux snapshots. Vaut 1 : un snapshot par pas de simulation. |

#### `void init(PlayerSlot* slots, int count, int screenW, int screenH);`
- **Déclarée dans le header mais jamais définie et jamais appelée.** Vestige d'une architecture où le serveur aurait créé lui-même les joueurs à partir des slots du menu ; aujourd'hui c'est `Game::init` qui le fait en passant par `Server::world()`. Toute tentative d'appel donnerait une erreur de lien. **Code mort à supprimer ou à implémenter.**

#### `void setTransport(ServerTransport* transport);`
- **Rôle** : injecte le transport de sortie/entrée du serveur.
- **Paramètres** : `transport` — pointeur non possédé, dont la durée de vie doit couvrir celle du `Server`.
- **Retour** : —
- **Effets de bord** : écrit `mTransport`.
- **Pièges** : **doit** être appelé avant le premier `update`, sinon le monde ne tourne pas du tout (et non pas « tourne sans réseau »). Dans `Game`, le `LoopbackServer` est un membre déclaré avant le `Server`, donc détruit après — l'ordre est correct.

#### `void setSnapshotInterval(uint32_t ticks){mSnapshotInterval = ticks;}`
- **Rôle** : règle la fréquence d'émission des snapshots, en pas de simulation.
- **Paramètres** : `ticks` — 1 = un snapshot par pas (60 Hz), 2 = 30 Hz, etc.
- **Retour** : —
- **Effets de bord** : écrit `mSnapshotInterval`.
- **Pièges** : **jamais appelée** (voir code mort). Attention : `mWorld.events.clear()` n'a lieu qu'à l'émission d'un snapshot, donc avec un intervalle > 1 les événements s'accumulent sur plusieurs pas et partent groupés — c'est le comportement voulu (aucun événement perdu), mais les `Shake`/`Sfx` arrivent alors en rafale. Une valeur de 0 émettrait un snapshot à chaque pas (le test est `>=`).

#### `void update(float realDeltaTime);`
- **Rôle** : cœur du serveur. (1) Redimensionne `mInputs` au nombre de joueurs. (2) Vide la file de réception du transport et applique tous les messages `Input`. (3) Accumule le temps réel et exécute autant de pas `FIXED_DT` que possible. (4) Après chaque pas, si le compteur atteint l'intervalle, capture un snapshot, le sérialise, le diffuse et vide l'`EventQueue`.
- **Paramètres** : `realDeltaTime` — temps réel écoulé, **en secondes**.
- **Retour** : —
- **Effets de bord** : `mTransport->receive()` (consomme et vide la file entrante) ; écrit `mInputs` ; fait avancer tout le `World` (donc exécute du Lua) ; alloue un `ByteWriter` et un `Snapshot` par émission ; `mTransport->broadcast(..., false)` → non fiable ; `mWorld.events.clear()`.
- **Pièges** :
  - **Retour immédiat si `mTransport == nullptr`** : la simulation est totalement gelée, ce qui est un piège de câblage facile à diagnostiquer de travers.
  - `mInputs` est redimensionné mais jamais remis à zéro : si le nombre de joueurs diminuait, les entrées résiduelles seraient réutilisées ; et un joueur qui cesse d'envoyer ses inputs continue d'appliquer les derniers reçus (bourrage de touche). Pour du vrai réseau il faudra un horodatage/numéro de séquence et une expiration.
  - Les indices hors bornes venant du réseau sont correctement rejetés (`if(index >= mInputs.size()) continue;`), mais `inputs[i]` est indexé en parallèle de `indices[i]` sans revérifier que les deux vecteurs ont la même taille — c'est garanti par `readInputMessage`, qui les remplit par paires.
  - Les messages mal formés ou de type inconnu sont silencieusement ignorés (pas de `Join`/`Welcome` traités).
  - En cas de retard, au 5e pas `mAccumulator` est **remis à 0** : on jette le temps restant plutôt que de le reporter, donc le jeu ralentit visiblement au lieu de s'emballer.
  - Le snapshot est capturé **à l'intérieur** de la boucle de pas : si une frame exécute 3 pas avec un intervalle de 1, trois snapshots sont diffusés dans la même frame. Côté client, `poll()` les traite tous et ne garde que le dernier — les snapshots intermédiaires sont donc **perdus, avec leurs événements** (sons/effets manqués lors des rattrapages).
  - `broadcast` est appelé avec `reliable = false` pour les snapshots : correct pour un flux d'état, mais les événements audio/visuels qui voyagent dedans ne survivront pas à une perte de paquet UDP.

#### `const World& Server::world() const;` / `World& Server::world();`
- **Rôle** : accès direct au monde simulé, en lecture seule ou en écriture.
- **Paramètres** : aucun.
- **Retour** : référence sur `mWorld` ; jamais invalide tant que le `Server` vit.
- **Effets de bord** : la surcharge non-const expose tout l'état interne en écriture.
- **Pièges** : c'est par cette porte que `Game` configure et peuple le monde (dimensions, joueurs, `backgroundWidth`) et lit `screenWidth`/`screenHeight` au rendu. C'est pratique en loopback mais ce couplage devra disparaître pour un client distant — `Server::init` était probablement censé le remplacer.

**Code mort de ce fichier :**
- `Server::init(PlayerSlot*, int, int, int)` : déclarée, **jamais définie**, jamais appelée.
- `Server::setSnapshotInterval` : définie inline, jamais appelée ; `mSnapshotInterval` reste à 1.
- L'inclusion de `PlayerSlot.hpp` ne sert plus qu'à la signature de `init`.
- `Server.cpp` inclut `ByteBuffer.hpp`, `Snapshot.hpp`, `Transport.hpp`, `World.hpp` alors que `Server.hpp` les fournit déjà transitivement (inoffensif).

---

## include/Client.hpp + src/Client.cpp

### `class Client`

Côté **non autoritaire** : il envoie les entrées des joueurs qu'il contrôle et conserve le dernier `Snapshot` désérialisé, que la présentation consultera pour dessiner. Aucune simulation, aucune prédiction, aucune interpolation pour l'instant : le client affiche brut le dernier état reçu.

| Membre | Type | Rôle |
|---|---|---|
| `mTransport` | `ClientTransport*` | Transport non possédé ; si `nullptr`, `sendInputs` et `poll` ne font rien. |
| `mSnapshot` | `Snapshot` | Dernier snapshot valide reçu. |
| `mHasSnapshot` | `bool` | Faux jusqu'au premier snapshot correctement décodé ; une fois vrai, jamais remis à faux. |
| `mOwned` | `std::vector<uint8_t>` | Indices des joueurs dont ce client envoie les entrées ; sert d'étiquettes dans le message `Input`. |

#### `void setTransport(ClientTransport* transport);`
- **Rôle** : injecte le transport.
- **Paramètres** : `transport` — pointeur non possédé devant survivre au `Client`.
- **Retour** : —
- **Effets de bord** : écrit `mTransport`.
- **Pièges** : à appeler avant le premier `sendInputs`/`poll`, sinon ils sont des no-op silencieux (pas de log, pas d'erreur).

#### `void setOwnedPlayers(std::vector<uint8_t> indices);`
- **Rôle** : déclare quels joueurs sont pilotés localement.
- **Paramètres** : `indices` — pris **par valeur** puis déplacé (idiome sink) ; indices dans le vecteur de joueurs du monde.
- **Retour** : —
- **Effets de bord** : remplace `mOwned`.
- **Pièges** : la taille de `mOwned` doit correspondre à celle du vecteur passé à `sendInputs`, sinon `writeInputMessage` tronque au plus petit des deux. Appelée une seule fois, depuis `Game::start`.

#### `void sendInputs(const std::vector<PlayerInput>& inputs);`
- **Rôle** : sérialise un message `Input` appariant `mOwned[i]` à `inputs[i]` et l'envoie au serveur.
- **Paramètres** : `inputs` — un `PlayerInput` par joueur possédé, dans le même ordre que `mOwned`.
- **Retour** : —
- **Effets de bord** : alloue un `ByteWriter` par appel (donc un `std::vector<uint8_t>` par frame) ; `mTransport->send(w.data(), false)` → en loopback, **copie** du buffer dans la file du `LoopbackLink`.
- **Pièges** : no-op si pas de transport. `reliable = false` : les entrées sont un flux, une perte est acceptable, mais sans numéro de séquence le serveur ne peut ni détecter un trou ni rejeter un paquet arrivé en retard — à ajouter avant de passer sur UDP. Le message est envoyé même si `mOwned` est vide (2 octets : type + compte 0).

#### `void poll();`
- **Rôle** : vide la file entrante du transport, décode tous les messages et retient le dernier `Snapshot` valide.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : `mTransport->receive()` (consomme la file) ; sur succès, `mSnapshot = std::move(snap)` et `mHasSnapshot = true`.
- **Pièges** :
  - Les messages vides, de type inconnu, ou dont `readSnapshot` échoue sont ignorés — et dans ce dernier cas `mSnapshot` conserve l'état précédent (dégradation propre, pas de crash).
  - Si plusieurs snapshots arrivent dans le même appel, **seul le dernier est conservé** : les `GameEvent` des snapshots intermédiaires sont définitivement perdus. C'est exactement le cas qui se produit quand le serveur rattrape plusieurs pas en une frame. Correctif futur : concaténer les événements au lieu d'écraser le snapshot.
  - Aucun contrôle sur `snap.tick` : un snapshot plus **ancien** arrivé après un plus récent (réordonnancement UDP) écraserait l'état récent. À corriger avant le vrai réseau.

#### `const Snapshot& Client::snapshot() const;`
- **Rôle** : accès en lecture au dernier snapshot.
- **Retour** : référence sur `mSnapshot` ; **valide même avant le premier snapshot** (c'est alors un `Snapshot` par défaut : tick 0, tous les vecteurs vides).
- **Effets de bord** : aucun.
- **Pièges** : la référence est invalidée par le prochain `poll()` qui réussit — ne pas la conserver au-delà d'une frame.

#### `bool Client::hasSnapshot() const;`
- **Rôle** : indique si au moins un snapshot a été décodé.
- **Retour** : `mHasSnapshot`.
- **Effets de bord** : aucun.
- **Pièges** : ne dit rien de la **fraîcheur** du snapshot ; reste `true` même si le serveur ne répond plus depuis longtemps. Il faudra un suivi de `tick`/horodatage pour détecter une déconnexion.

**Code mort de ce fichier :** aucun. `mOwned`, `mHasSnapshot` et `mSnapshot` sont tous lus. En revanche le client n'émet jamais `MsgType::Join` et ne traite jamais `MsgType::Welcome`.

---

## include/Transport.hpp

Fichier d'interfaces pures : la couture qui permet d'échanger le loopback contre de vraies sockets sans toucher ni `Server` ni `Client`.

### `struct NetMessage`
Message entrant **côté serveur**, qui doit savoir de quel client il provient.

| Membre | Type | Rôle |
|---|---|---|
| `clientId` | `int` | Identifiant de l'expéditeur ; `-1` = invalide/inconnu. |
| `data` | `std::vector<uint8_t>` | Charge utile brute, possédée par le message. |

*Asymétrie à noter : le client reçoit de simples `std::vector<uint8_t>` (il n'a qu'un seul interlocuteur), le serveur reçoit des `NetMessage`.*

### `class ClientTransport`
Interface abstraite du canal vu depuis un client.

#### `virtual ~ClientTransport() = default;`
- **Rôle** : destructeur virtuel, obligatoire pour pouvoir détruire une implémentation via un `ClientTransport*`.
- **Pièges** : le déclarer supprime les opérations de déplacement implicites de la classe de base — sans importance ici.

#### `virtual void send(const std::vector<uint8_t>& data, bool reliable) = 0;`
- **Rôle** : envoie un message au serveur.
- **Paramètres** : `data` — octets à envoyer (l'implémentation doit copier si elle diffère l'envoi) ; `reliable` — demande une livraison garantie et ordonnée (pour un futur transport UDP fiable).
- **Retour** : rien — **aucun moyen de signaler un échec d'envoi**.

#### `virtual std::vector<std::vector<uint8_t>> receive() = 0;`
- **Rôle** : récupère et **consomme** tous les messages reçus depuis le dernier appel.
- **Retour** : un vecteur de buffers, vide s'il n'y a rien.
- **Effets de bord** : vide la file interne ; alloue le vecteur de retour.
- **Pièges** : sémantique destructive — appeler deux fois dans la même frame fait perdre les messages du premier appel si on ne les a pas gardés.

#### `virtual bool isConnected() const = 0;`
- **Rôle** : état de la connexion.
- **Retour** : `true` si le canal est utilisable.
- **Pièges** : **jamais appelée nulle part** dans le projet.

### `class ServerTransport`
Interface abstraite du canal vu depuis le serveur.

#### `virtual ~ServerTransport() = default;`
- **Rôle** : destructeur virtuel.

#### `virtual void sendTo(int clientId, const std::vector<uint8_t>& data, bool reliable) = 0;`
- **Rôle** : envoi ciblé à un client (prévu pour `Welcome`, les messages d'erreur, etc.).
- **Paramètres** : `clientId` — destinataire ; `data` — octets ; `reliable` — livraison garantie.
- **Pièges** : **jamais appelée** par `Server`.

#### `virtual void broadcast(const std::vector<uint8_t>& data, bool reliable) = 0;`
- **Rôle** : envoi à tous les clients connectés. C'est la seule voie de sortie réellement utilisée (snapshots).
- **Paramètres** : `data` — octets ; `reliable` — `false` pour les snapshots.

#### `virtual std::vector<NetMessage> receive() = 0;`
- **Rôle** : récupère et consomme tous les messages de tous les clients.
- **Retour** : vecteur de `NetMessage` étiquetés par `clientId`.
- **Effets de bord** : vide la file entrante.

**Code mort de ce fichier :**
- `ClientTransport::isConnected` : implémentée dans `LoopbackClient`, jamais appelée.
- `ServerTransport::sendTo` : implémentée dans `LoopbackServer`, jamais appelée.
- Le paramètre `reliable` est accepté partout mais **ignoré** par la seule implémentation existante.

---

## include/LoopbackTransport.hpp + src/LoopbackTransport.cpp

### `class LoopbackLink`

Le « câble » du mode solo : deux files de buffers en mémoire, une par sens. Zéro socket, zéro sérialisation réseau en plus, zéro latence — mais les messages passent bien par le même encodage binaire que le futur mode en ligne, ce qui fait du solo un test permanent du protocole.

| Membre | Type | Rôle |
|---|---|---|
| `mToServer` | `std::vector<NetMessage>` | Messages client→serveur en attente. |
| `mToClient` | `std::vector<std::vector<uint8_t>>` | Messages serveur→client en attente. |

#### `void LoopbackLink::pushToServer(int clientId, const std::vector<uint8_t>& data)`
- **Rôle** : met un message en attente dans la file montante.
- **Paramètres** : `clientId` — expéditeur ; `data` — octets, **copiés** dans le `NetMessage`.
- **Retour** : —
- **Effets de bord** : `push_back` (allocation + copie du buffer).
- **Pièges** : copie intégrale à chaque input envoyé (une fois par frame) ; un `std::move` ou un `emplace_back` éviterait l'allocation.

#### `void LoopbackLink::pushToClient(const std::vector<uint8_t>& data)`
- **Rôle** : met un message en attente dans la file descendante.
- **Paramètres** : `data` — octets, **copiés**.
- **Retour** : —
- **Effets de bord** : `push_back` (allocation + copie). C'est la copie du snapshot complet à chaque émission.

#### `std::vector<NetMessage> LoopbackLink::takeServerMessages()`
- **Rôle** : transfère la propriété de toute la file montante à l'appelant et la vide.
- **Retour** : la file (vide si rien).
- **Effets de bord** : `mToServer` est déplacé puis explicitement `clear()`.
- **Pièges** : le `clear()` après `std::move` n'est pas superflu par pure forme — un conteneur déplacé est dans un état valide mais **non spécifié**, donc le `clear()` garantit qu'il est bien vide et que la capacité n'est pas réutilisée de façon surprenante.

#### `std::vector<std::vector<uint8_t>> LoopbackLink::takeClientMessages()`
- **Rôle** : idem pour la file descendante.
- **Retour** : la file des buffers reçus par le client.
- **Effets de bord** : vide `mToClient`.

### `class LoopbackClient : public ClientTransport`

Adaptateur `ClientTransport` sur un `LoopbackLink`.

| Membre | Type | Rôle |
|---|---|---|
| `mLink` | `LoopbackLink*` | Lien partagé, non possédé. |
| `mClientId` | `int` | Identité annoncée au serveur (0 en solo). |

#### `LoopbackClient::LoopbackClient(LoopbackLink* link, int clientId)`
- **Rôle** : mémorise le lien et l'identifiant.
- **Paramètres** : `link` — peut être `nullptr` (toutes les méthodes deviennent des no-op) ; `clientId` — identité de ce client.
- **Effets de bord** : initialise les deux membres.
- **Pièges** : le lien doit survivre au transport. Dans `Game`, `mLink` est déclaré **avant** `mLoopServer`/`mLoopClient`, donc construit avant et détruit après : l'ordre est correct.

#### `void LoopbackClient::send(const std::vector<uint8_t>& data, bool)`
- **Rôle** : pousse le message dans la file montante.
- **Paramètres** : `data` — octets ; le paramètre `reliable` est **non nommé donc volontairement ignoré** (pas de notion de fiabilité en mémoire).
- **Retour** : —
- **Effets de bord** : `mLink->pushToServer(mClientId, data)`.
- **Pièges** : no-op silencieux si `mLink == nullptr`.

#### `std::vector<std::vector<uint8_t>> LoopbackClient::receive()`
- **Rôle** : récupère tous les messages descendants.
- **Retour** : la file, ou un vecteur vide si pas de lien.
- **Effets de bord** : vide `mToClient`.
- **Pièges** : en solo, le `Client` reçoit **aussi** ce que `broadcast` a poussé, sans filtrage par `clientId` — normal, il n'y a qu'un client.

#### `bool LoopbackClient::isConnected() const`
- **Rôle** : considère la connexion établie dès qu'un lien existe.
- **Retour** : `mLink != nullptr`.
- **Effets de bord** : aucun.
- **Pièges** : jamais appelée.

### `class LoopbackServer : public ServerTransport`

Adaptateur `ServerTransport` sur le même `LoopbackLink`.

| Membre | Type | Rôle |
|---|---|---|
| `mLink` | `LoopbackLink*` | Lien partagé, non possédé. |
| `mClientId` | `int` | **Unique** client connu ; sert de filtre dans `sendTo`. |

#### `LoopbackServer::LoopbackServer(LoopbackLink* link, int clientId)`
- **Rôle** : mémorise le lien et l'identifiant du seul client.
- **Paramètres** : `link` — peut être `nullptr` ; `clientId` — doit **correspondre** à celui du `LoopbackClient` (0 et 0 dans `Game`).
- **Effets de bord** : initialise les deux membres.

#### `void LoopbackServer::sendTo(int clientId, const std::vector<uint8_t>& data, bool)`
- **Rôle** : envoi ciblé, qui n'aboutit que si la cible est le client unique de ce loopback.
- **Paramètres** : `clientId` — destinataire ; si `!= mClientId`, le message est **jeté silencieusement** ; `data` — octets ; `reliable` ignoré.
- **Retour** : —
- **Effets de bord** : `mLink->pushToClient(data)` dans le cas favorable.
- **Pièges** : jamais appelée. Un mauvais `clientId` ne produit aucun diagnostic.

#### `void LoopbackServer::broadcast(const std::vector<uint8_t>& data, bool)`
- **Rôle** : diffuse à « tous » les clients, c'est-à-dire au seul client du lien.
- **Paramètres** : `data` — octets ; `reliable` ignoré.
- **Retour** : —
- **Effets de bord** : `mLink->pushToClient(data)` (copie).
- **Pièges** : contrairement à `sendTo`, **ne filtre pas** sur `mClientId` — cohérent avec la sémantique de diffusion.

#### `std::vector<NetMessage> LoopbackServer::receive()`
- **Rôle** : récupère tous les messages montants.
- **Retour** : la file, ou vide si pas de lien.
- **Effets de bord** : vide `mToServer`.

**Code mort de ce fichier :**
- `LoopbackServer::sendTo` : définie, jamais appelée.
- `LoopbackClient::isConnected` : définie, jamais appelée.
- `LoopbackServer::mClientId` : lu **uniquement** par `sendTo`, donc en pratique jamais lu.
- Les paramètres `reliable` sont tous ignorés.
- `LoopbackTransport.cpp` réinclut `Transport.hpp` déjà fourni par son header.

---

## include/Protocol.hpp + src/Protocol.cpp

Couche de (dé)sérialisation : toute la connaissance du format binaire est concentrée ici. Format **plat, non compressé, non delta-encodé, little-endian**, posé sur `ByteWriter`/`ByteReader`.

### `enum class MsgType : uint8_t`
Premier octet de tout message : `Join = 1`, `Welcome = 2`, `Input = 3`, `Snapshot = 4`. Seuls `Input` et `Snapshot` sont réellement écrits et lus ; `Join` et `Welcome` sont réservés pour la phase de connexion (et donc **du code mort pour l'instant**).

### Constantes de garde
| Constante | Valeur | Rôle |
|---|---|---|
| `MAX_NET_PLAYERS` | `8` | Plafond de joueurs dans un message `Input` ou un snapshot ; au-delà, le décodage **échoue**. |
| `MAX_NET_PROJECTILES` | `1024` | Plafond du nombre d'entités dans un bloc `writeEntities`/`readEntities` ; utilisé pour les projectiles **et** les collectables. |
| `MAX_NET_EVENTS` | `256` | Plafond du nombre de `GameEvent` dans un snapshot. |
| `PROTOCOL_VERSION` | `1` | Numéro de version — **jamais écrit ni vérifié** (code mort). |

Ces trois plafonds sont la défense principale contre un paquet hostile ou corrompu qui annoncerait un compte énorme : sans eux, le `reserve(count)` déclencherait une allocation massive.

#### `void writeInput(ByteWriter& w, const PlayerInput& in)`
- **Rôle** : encode un `PlayerInput` en **un seul octet** de bits (bit 0 = `left`, 1 = `right`, 2 = `thrust`, 3 = `ability`).
- **Paramètres** : `w` — writer à compléter ; `in` — entrée à encoder.
- **Retour** : —
- **Effets de bord** : ajoute 1 octet à `w`.
- **Pièges** : les bits 4 à 7 sont libres pour de futures actions. L'ordre des bits est le contrat implicite avec `readInput` : les deux doivent être modifiés ensemble.

#### `bool readInput(ByteReader& r, PlayerInput& out)`
- **Rôle** : décode l'octet de bits.
- **Paramètres** : `r` — reader positionné sur l'octet ; `out` — **écrit uniquement en cas de succès**.
- **Retour** : `false` si le reader est à sec (`!r.ok()`), `true` sinon.
- **Effets de bord** : avance `r` de 1 octet ; écrit les 4 booléens de `out`.
- **Pièges** : `out` n'est pas touché en cas d'échec, donc il conserve sa valeur précédente.

#### `static void writeEntities(ByteWriter& w, const std::vector<EntityState>& entities)` *(statique de fichier)*
- **Rôle** : écrit un bloc d'entités : un `u16` de compte, puis pour chacune `netId`(u32), `texture`(u16), `x`,`y`,`angle`(f32), `w`,`h`(u16) → 22 octets par entité.
- **Paramètres** : `w` — writer ; `entities` — entités à écrire.
- **Retour** : —
- **Effets de bord** : ajoute `2 + 22 * n` octets à `w`.
- **Pièges** : le cast `(uint16_t)entities.size()` **tronque silencieusement** au-delà de 65535 entités, et n'est pas borné à `MAX_NET_PROJECTILES` à l'écriture — un monde avec plus de 1024 projectiles produirait un message que le lecteur rejetterait **intégralement** (plus de snapshot du tout). Garde-fou à ajouter côté écriture.

#### `static bool readEntities(ByteReader& r, std::vector<EntityState>& out)` *(statique de fichier)*
- **Rôle** : lit un bloc d'entités écrit par `writeEntities`.
- **Paramètres** : `r` — reader ; `out` — entités **ajoutées** (`push_back`) à la fin du vecteur.
- **Retour** : `false` si le compte est illisible ou `> MAX_NET_PROJECTILES`, ou si le reader est à sec en fin de lecture ; `true` sinon.
- **Effets de bord** : `out.reserve(count)` puis `push_back` → allocation proportionnelle au compte annoncé (d'où l'importance du plafond) ; avance `r`.
- **Pièges** : **ne vide pas `out`** — c'est sûr uniquement parce que `readSnapshot` fait `out = Snapshot()` au début. Appelée sur un vecteur non vide, elle concaténerait. En cas d'échec en cours de route, `out` contient des entités partiellement lues avec des champs à 0 (le `ByteReader` renvoie 0 une fois à sec), mais l'appelant abandonne le snapshot entier.

#### `static void writeEvent(ByteWriter& w, const GameEvent& e)` *(statique de fichier)*
- **Rôle** : écrit un événement : `type`(u8), `id`(u16), `x`,`y`,`a`,`b`(f32), `handle`(u32) → 23 octets fixes.
- **Paramètres** : `w` — writer ; `e` — événement.
- **Retour** : —
- **Effets de bord** : ajoute 23 octets.
- **Pièges** : format fixe quel que soit le type d'événement : un `Shake` gaspille `id`, `x`, `y` et `handle`, un `StopSfx` n'utilise que `handle`. Optimisable par un encodage variable selon le type, au prix de la simplicité.

#### `static void readEvent(ByteReader& r, GameEvent& e)` *(statique de fichier)*
- **Rôle** : lit un événement.
- **Paramètres** : `r` — reader ; `e` — événement écrit **inconditionnellement**.
- **Retour** : `void` — **aucun signalement d'échec**, c'est à l'appelant de consulter `r.ok()` après coup.
- **Effets de bord** : avance `r` de 23 octets ; écrit tous les champs de `e`.
- **Pièges** : `e.type = (EventType)r.u8()` **sans validation** : une valeur hors de `{0,1,2,3}` produit un `EventType` invalide. Ce n'est pas un crash car `Game::drainEvents` utilise un `switch` sans `default` (l'événement est ignoré), mais c'est techniquement une valeur hors du domaine de l'énumération — à valider explicitement avant d'accepter du trafic non fiable.

#### `void writeSnapshot(ByteWriter& w, const Snapshot& snap)`
- **Rôle** : écrit le **corps** du snapshot (sans l'octet de type) : `tick`(u32), `scrollingOffset`(f32), puis un `u16` de compte de joueurs suivi de chaque `PlayerState` (`index` u8, `x`,`y`,`vx`,`vy` f32, `life` i16, `score` i32, `abilityProgress` u8, un octet de flags : bit 0 `isAlive`, bit 1 `isControlled`, bit 2 `thrusting`) → 24 octets par joueur ; puis le bloc projectiles, le bloc collectables, et enfin un `u16` de compte d'événements suivi des événements.
- **Paramètres** : `w` — writer ; `snap` — snapshot à encoder.
- **Retour** : —
- **Effets de bord** : ajoute les octets à `w`.
- **Pièges** : aucun des trois comptes n'est borné à l'écriture (`MAX_NET_PLAYERS`, `MAX_NET_PROJECTILES`, `MAX_NET_EVENTS`), alors que la lecture les impose : un monde dépassant un plafond devient **indécodable** et le client reste gelé sur son dernier snapshot valide, sans aucun message d'erreur. C'est le piège le plus sérieux du protocole actuel. Les blocs projectiles et collectables partagent le même plafond de 1024.

#### `bool readSnapshot(ByteReader& r, Snapshot& out)`
- **Rôle** : décode le corps d'un snapshot.
- **Paramètres** : `r` — reader positionné après l'octet de type ; `out` — **réinitialisé à un `Snapshot()` neuf** dès l'entrée, puis rempli.
- **Retour** : `false` si le compte de joueurs est illisible ou `> MAX_NET_PLAYERS`, si un des deux blocs d'entités échoue, si le compte d'événements est illisible ou `> MAX_NET_EVENTS`, ou si le reader est à sec à la fin ; `true` sinon.
- **Effets de bord** : écrase `out` ; `reserve` + `push_back` sur les quatre vecteurs.
- **Pièges** :
  - `out = Snapshot()` en tête : un échec **détruit** le contenu précédent de `out`. C'est pour cela que `Client::poll` décode dans un `Snapshot snap` local et ne l'affecte à `mSnapshot` qu'en cas de succès — inverser cet ordre introduirait un bug visible (écran qui se vide sur paquet corrompu).
  - `p.index` n'est pas validé ici ; c'est le consommateur (`renderSnapshot`) qui borne.
  - `abilityProgress` revient en `uint8_t` : la précision du flottant d'origine est définitivement perdue (pas de 1/255 ≈ 0,4 %).
  - Les champs non lus par le rendu (`vx`, `vy`, `netId`, `w`, `h`, `isControlled`, `thrusting`) coûtent quand même leur place dans chaque paquet.

#### `void writeSnapshotMessage(ByteWriter& w, const Snapshot& snap)`
- **Rôle** : écrit l'octet `MsgType::Snapshot` puis délègue à `writeSnapshot`.
- **Paramètres** : `w` — writer ; `snap` — snapshot.
- **Retour** : —
- **Effets de bord** : ajoute `1 + taille du corps` octets.
- **Pièges** : il n'existe **pas** de `readSnapshotMessage` symétrique — c'est `Client::poll` qui lit l'octet de type lui-même avant d'appeler `readSnapshot`. Asymétrie à garder en tête.

#### `void writeInputMessage(ByteWriter& w, const std::vector<uint8_t>& indices, const std::vector<PlayerInput>& inputs)`
- **Rôle** : écrit l'octet `MsgType::Input`, un `u8` de compte, puis `count` paires (`index` u8, bits d'input u8) → `2 + 2 * count` octets.
- **Paramètres** : `w` — writer ; `indices` — étiquettes de joueurs ; `inputs` — entrées correspondantes, appariées positionnellement.
- **Retour** : —
- **Effets de bord** : ajoute les octets à `w`.
- **Pièges** : `count = min(indices.size(), inputs.size())` puis bridé à `MAX_NET_PLAYERS` — la troncature est **silencieuse** dans les deux cas. Si les deux vecteurs ont des tailles différentes, les entrées excédentaires sont perdues sans avertissement.

#### `bool readInputMessage(ByteReader& r, std::vector<uint8_t>& outIndices, std::vector<PlayerInput>& outInputs)`
- **Rôle** : décode un message `Input`.
- **Paramètres** : `r` — reader positionné après l'octet de type ; `outIndices` / `outInputs` — **vidés puis remplis par paires**, donc toujours de même taille à la sortie.
- **Retour** : `false` si le compte est illisible ou `> MAX_NET_PLAYERS`, si une lecture d'input échoue, ou si le reader est à sec à la fin ; `true` sinon.
- **Effets de bord** : `clear()` puis `push_back` sur les deux vecteurs.
- **Pièges** : les deux vecteurs sont vidés **après** la validation du compte, donc un compte invalide les laisse intacts ; mais un échec en cours de lecture les laisse partiellement remplis — `Server::update` abandonne alors le message par `continue`, sans toucher `mInputs`. Les indices ne sont pas validés ici, c'est `Server::update` qui le fait (`index >= mInputs.size()`).

**Code mort de ce fichier :**
- `PROTOCOL_VERSION` : défini, jamais écrit, jamais vérifié. Un client et un serveur de versions différentes se parleraient sans le savoir.
- `MsgType::Join` et `MsgType::Welcome` : jamais écrits, jamais lus.
- `writeInput` est publique mais n'est appelée que par `writeInputMessage` ; `writeSnapshot` de même, uniquement par `writeSnapshotMessage`. Elles pourraient devenir internes.
- Absence de `readSnapshotMessage` / `readInputMessage` complet (lecture de l'octet de type) : ce travail est dupliqué dans `Client::poll` et `Server::update`.

---

## include/Snapshot.hpp + src/Snapshot.cpp

### `struct PlayerState`
Vue réseau d'un joueur : tout ce qui change à chaque tick. Les données statiques (skin, chapeau, vie max, taille de collider) **ne sont pas** ici — elles sont dans `PlayerInfo`, côté client.

| Membre | Type | Rôle |
|---|---|---|
| `index` | `uint8_t` | Indice du joueur dans le vecteur du monde ; clé de jointure avec `Game::mPlayerInfos`. |
| `x`, `y` | `float` | Position en pixels. |
| `vx`, `vy` | `float` | Vitesse en pixels/seconde. **Transmise mais jamais lue** par le client actuel (prévue pour l'interpolation/extrapolation). |
| `life` | `int16_t` | Points de vie courants ; dénominateur = `PlayerInfo::maxLife`. |
| `score` | `int32_t` | Score courant. |
| `isAlive` | `bool` | Si faux, le joueur n'est pas dessiné (mais reste dans le HUD). |
| `isControlled` | `bool` | Joueur sous contrôle externe (ability `christmas` qui le fait chevaucher un projectile). **Transmis mais jamais lu** par le rendu. |
| `thrusting` | `bool` | Jetpack actif. **Transmis mais jamais lu** (les particules de poussée sont encore spawnées directement depuis la simulation via `GameContext::particleManager`). |
| `abilityProgress` | `uint8_t` | Progression du cooldown d'ability **quantifiée** : `progress ∈ [0,1]` × 255. Reconvertie en flottant pour le camembert du HUD. |

### `struct EntityState`
Vue réseau générique d'une entité visuelle sans logique côté client (projectiles, collectables).

| Membre | Type | Rôle |
|---|---|---|
| `netId` | `uint32_t` | Identité stable attribuée par `ProjectileManager`. **Transmise mais jamais lue** ; indispensable le jour où il faudra interpoler ou associer des effets persistants. |
| `texture` | `uint16_t` | Id numérique d'asset (table `AssetIds`), résolu en nom puis en `LTexture*` au rendu. |
| `x`, `y` | `float` | Position en pixels. |
| `angle` | `float` | Rotation en degrés, utilisée pour les projectiles. |
| `w`, `h` | `uint16_t` | Dimensions du collider en pixels. **Transmises mais jamais lues.** |

### `struct Snapshot`
État complet du monde à un tick donné, **plus** la liste des événements de présentation produits depuis le snapshot précédent. C'est l'unique contrat entre simulation et présentation.

| Membre | Type | Rôle |
|---|---|---|
| `tick` | `uint32_t` | Numéro de pas de simulation. Transmis et décodé mais **jamais exploité** (ni ordonnancement, ni détection de doublon). |
| `scrollingOffset` | `float` | Décalage du fond en pixels ; permet au client de dessiner le défilement sans le simuler. |
| `players` | `std::vector<PlayerState>` | Tous les joueurs, vivants ou non. |
| `projectiles` | `std::vector<EntityState>` | Projectiles vivants uniquement. |
| `collectables` | `std::vector<EntityState>` | Pizzas vivantes uniquement. |
| `events` | `std::vector<GameEvent>` | Sons, effets et screen-shake à déclencher. **Copie** de l'`EventQueue` du monde. |

#### `Snapshot captureSnapshot(const World& world);`
- **Rôle** : projette l'état de simulation vers la structure sérialisable. C'est la frontière exacte entre les types riches du monde (`player::Player`, `shared_ptr<Projectile>`, `ScoreCollectable`) et les POD réseau.
- **Paramètres** : `world` — monde en lecture seule ; la signature `const World&` garantit que la capture ne modifie pas la simulation.
- **Retour** : un `Snapshot` par valeur (élidé / déplacé). Jamais d'échec signalé.
- **Effets de bord** : quatre allocations de vecteur avec `reserve` sur les tailles sources ; copie de toute la liste d'événements (`world.events.events()`).
- **Pièges** :
  - **`state.abilityProgress = (uint8_t)(p.getAbilityProgress() * 255.f)`** : pas de clamp. Si `getCooldownProgress()` renvoyait une valeur hors de `[0,1]`, la conversion flottant→`uint8_t` hors domaine est un **comportement indéfini**. Un `std::clamp` avant le cast est le correctif.
  - **`Player::getAbilityProgress()` fait `config.ability->getCooldownProgress()` sans test de nullité**, et `ScriptEngine::createAbilityForHat` renvoie `nullptr` pour un chapeau sans `ability` déclarée en Lua. Comme `captureSnapshot` appelle ce getter pour **chaque joueur à chaque tick**, choisir un tel chapeau fait **planter le serveur au premier tick**. C'est le bug le plus concret de cette zone.
  - `state.index = (uint8_t)i` : les joueurs sont identifiés par leur **position** dans le vecteur, pas par un id stable. Tant que personne ne quitte en cours de partie c'est équivalent, mais ça ne tiendra pas pour une vraie session en ligne.
  - Les projectiles sont filtrés sur `!p || p->isDead()`, les pizzas sur `!c.isAlive` : les vecteurs du snapshot sont donc plus courts que les vecteurs source, et les `reserve` surévaluent légèrement.
  - Pour les collectables, **`netId` et `angle` ne sont pas renseignés** et restent à 0 : ils occupent 8 octets par pizza et par snapshot pour rien.
  - Les troncatures `(int16_t)getLife()`, `(uint16_t)pw/ph/collider.w/h` sont silencieuses — sans risque aux valeurs actuelles (vie ≤ quelques centaines, tailles en dizaines de pixels).
  - La copie de `world.events.events()` est nécessaire car le `Server` appelle `events.clear()` juste après : la liste doit survivre dans le snapshot.

**Code mort de ce fichier :** la déclaration anticipée `class World;` dans le header est correcte et volontaire (évite d'inclure `World.hpp`). Champs transmis mais jamais consommés : `Snapshot::tick`, `PlayerState::vx`, `vy`, `isControlled`, `thrusting`, `EntityState::netId`, `w`, `h` (plus `angle` pour les collectables, jamais écrit).

---

## include/ByteBuffer.hpp

Deux petites classes entièrement inline, sans dépendance, qui définissent l'encodage bas niveau : **little-endian explicite**, donc indépendant de l'endianness de la machine.

### `class ByteWriter`

Accumule des octets dans un `std::vector<uint8_t>` qui grandit à la demande.

| Membre | Type | Rôle |
|---|---|---|
| `mData` | `std::vector<uint8_t>` | Tampon de sortie possédé. |

#### `void u8(uint8_t v){ mData.push_back(v); }`
- **Rôle** : primitive d'écriture ; **toutes** les autres s'y ramènent.
- **Paramètres** : `v` — l'octet.
- **Retour** : —
- **Effets de bord** : `push_back`, peut réallouer (donc invalider tout pointeur obtenu de `data()`).

#### `void u16(uint16_t v)`
- **Rôle** : écrit 2 octets, **octet de poids faible d'abord** (little-endian).
- **Paramètres** : `v` — valeur.
- **Effets de bord** : 2 octets ajoutés.

#### `void u32(uint32_t v)`
- **Rôle** : écrit 4 octets en little-endian, via deux `u16`.
- **Paramètres** : `v` — valeur.
- **Effets de bord** : 4 octets ajoutés.

#### `void i16(int16_t v){ u16((uint16_t)v); }`
- **Rôle** : écrit un entier signé 16 bits par réinterprétation en non signé.
- **Pièges** : la conversion signé→non signé est bien définie (modulo 2¹⁶) et le retour par `(int16_t)` à la lecture donne la bonne valeur sur toute implémentation en complément à deux (garanti depuis C++20, universel en pratique).

#### `void i32(int32_t v){ u32((uint32_t)v); }`
- **Rôle** : idem en 32 bits.

#### `void f32(float v)`
- **Rôle** : écrit un flottant en recopiant ses 4 octets de représentation via `std::memcpy`, puis en les écrivant comme un `u32`.
- **Paramètres** : `v` — valeur.
- **Effets de bord** : 4 octets ajoutés.
- **Pièges** : le `memcpy` est la **bonne** façon de faire (un `reinterpret_cast` violerait les règles d'aliasing strict). En revanche cela suppose `sizeof(float) == 4` et le **même format IEEE-754 des deux côtés** — vrai sur toutes les plateformes visées, mais c'est une hypothèse non vérifiée. Les `NaN` et infinis traversent tels quels : un bug de simulation produisant un `NaN` se propagera jusqu'au rendu.

#### `const std::vector<uint8_t>& data() const { return mData; }`
- **Rôle** : accès en lecture au tampon, pour le passer à `send`/`broadcast`.
- **Retour** : référence constante sur `mData`.
- **Pièges** : invalidée par toute écriture ultérieure — ne pas conserver au-delà de l'appel d'envoi.

#### `void clear(){ mData.clear(); }`
- **Rôle** : vide le tampon en conservant la capacité (réutilisation sans réallocation).
- **Effets de bord** : taille à 0.
- **Pièges** : **jamais appelée** : `Server::update` et `Client::sendInputs` construisent un `ByteWriter` neuf à chaque message. Réutiliser un writer membre + `clear()` supprimerait une allocation par frame.

### `class ByteReader`

Curseur en lecture seule sur un tampon **non possédé**, avec un drapeau d'erreur collant.

| Membre | Type | Rôle |
|---|---|---|
| `mData` | `const uint8_t*` | Tampon source, **non possédé**. |
| `mSize` | `size_t` | Taille du tampon en octets. |
| `mPos` | `size_t` | Position de lecture courante. |
| `mOk` | `bool` | Vrai jusqu'à la première tentative de lecture au-delà de la fin ; **jamais remis à vrai**. |

#### `ByteReader(const uint8_t* data, size_t size) : mData(data), mSize(size) {}`
- **Rôle** : construit un curseur sur un tampon existant.
- **Paramètres** : `data` — début des octets ; `size` — nombre d'octets lisibles.
- **Effets de bord** : aucun (ne copie rien).
- **Pièges** : **ne possède pas** le tampon : si le `std::vector` source est détruit, modifié ou réalloué pendant la lecture, le reader est pendant → UB. Dans `Server::update` et `Client::poll`, le vecteur source vit bien dans la boucle `for`, donc c'est correct. Aucune vérification de `data != nullptr` (mais avec `size == 0`, `u8` échoue proprement avant tout déréférencement).

#### `uint8_t u8()`
- **Rôle** : primitive de lecture ; **toutes** les autres s'y ramènent.
- **Retour** : l'octet lu, ou **0 si le tampon est épuisé**.
- **Effets de bord** : incrémente `mPos` en cas de succès ; met `mOk = false` en cas de dépassement (et n'avance pas).
- **Pièges** : le test `mPos + 1 > mSize` est correct et ne peut pas déborder en pratique. Comme l'échec renvoie 0 au lieu de lever, **il faut consulter `ok()`** : tout ce code dépend de cette discipline.

#### `uint16_t u16()`
- **Rôle** : lit 2 octets little-endian.
- **Retour** : la valeur, ou une valeur partielle/0 si le tampon est épuisé (avec `mOk` à faux).
- **Effets de bord** : avance de 0 à 2 octets.
- **Pièges** : l'ordre d'évaluation est **garanti** parce que `low` et `high` sont deux déclarations séparées ; une version en une seule expression (`u8() | (u8() << 8)`) serait un bug d'ordre d'évaluation non spécifié. À ne pas « simplifier ».

#### `uint32_t u32()`
- **Rôle** : lit 4 octets little-endian via deux `u16` (même précaution d'ordre).
- **Retour** : la valeur, ou partielle avec `mOk` faux.

#### `int16_t i16(){ return (int16_t)u16(); }` / `int32_t i32(){ return (int32_t)u32(); }`
- **Rôle** : lecture d'entiers signés par réinterprétation.
- **Retour** : la valeur signée.
- **Pièges** : conversion hors domaine techniquement définie par l'implémentation avant C++20, bien définie depuis ; sans risque pratique.

#### `float f32()`
- **Rôle** : lit 4 octets et les réinterprète en `float` par `memcpy`.
- **Retour** : le flottant ; **0.0f** si le tampon était vide dès le départ (les quatre octets lus valent 0, dont la représentation IEEE-754 est bien +0.0).
- **Pièges** : une lecture partielle peut produire un `NaN` ou une valeur absurde ; seul `ok()` permet de s'en apercevoir.

#### `bool ok() const { return mOk; }`
- **Rôle** : indique si toutes les lectures effectuées jusqu'ici ont tenu dans le tampon.
- **Retour** : `mOk`.
- **Effets de bord** : aucun.
- **Pièges** : drapeau **collant** — c'est voulu : on peut enchaîner des dizaines de lectures et ne vérifier qu'une fois à la fin (ce que font `readSnapshot` et `readEntities`). Il n'existe pas de méthode pour savoir combien d'octets restent ni si le message a été **entièrement** consommé : un message plus long que prévu passe sans déclencher d'erreur.

**Code mort de ce fichier :** `ByteWriter::clear()` n'est jamais appelée.

---

## include/AssetIds.hpp + src/AssetIds.cpp

### `class AssetIds`

Singleton qui attribue un `uint16_t` à chaque nom d'asset (texture, son, animation) afin que les snapshots et les événements transportent **2 octets au lieu d'une chaîne**. L'id 0 est réservé à la chaîne vide, ce qui en fait une valeur « aucun asset » utilisable comme valeur par défaut de `EntityState::texture` et `GameEvent::id`.

| Membre | Type | Rôle |
|---|---|---|
| `mIds` | `std::unordered_map<std::string, uint16_t>` | Index nom → id, pour l'attribution. |
| `mNames` | `std::vector<std::string>` | Table id → nom ; l'id **est** l'indice. `mNames[0]` est la chaîne vide. |

#### `AssetIds(const AssetIds&) = delete;` / `AssetIds& operator = (const AssetIds&) = delete;`
- **Rôle** : interdisent copie et affectation, comme il se doit pour un singleton.
- **Pièges** : les opérations de déplacement sont supprimées par ricochet — voulu.

#### `AssetIds::AssetIds()` *(privé)*
- **Rôle** : réserve l'id 0 en poussant la chaîne vide dans `mNames`.
- **Effets de bord** : `mNames.size() == 1` à la sortie ; `mIds` reste vide (la chaîne vide **n'est pas** dans l'index, donc un appel à `id("")` créerait un **second** id pour la chaîne vide — incohérence mineure).
- **Pièges** : constructeur privé → instanciation uniquement via `getInstance`.

#### `uint16_t AssetIds::id(const std::string& name)`
- **Rôle** : renvoie l'id du nom, en l'enregistrant à la première demande (« intern » paresseux).
- **Paramètres** : `name` — nom d'asset.
- **Retour** : l'id existant, ou un nouvel id égal à `mNames.size()` avant insertion.
- **Effets de bord** : **mutant** : `push_back` dans `mNames` et insertion dans `mIds` pour un nom inconnu.
- **Pièges** :
  - **Le point le plus important pour le passage en ligne** : les ids sont attribués **dans l'ordre de première utilisation à l'exécution**. Ils ne sont donc stables ni entre deux parties, ni entre deux machines, ni entre deux jeux de mods. Tant que serveur et client sont le même processus (loopback), aucun problème ; dès qu'ils sont séparés, un `id` de 7 pourra désigner `"explosion"` chez l'un et `"boing"` chez l'autre. Il faudra soit une table d'assets transmise dans le `Welcome`, soit une attribution déterministe (tri alphabétique des assets chargés, hachage du nom).
  - Au-delà de 65535 assets, le cast `(uint16_t)mNames.size()` **réutiliserait silencieusement des ids** (irréaliste ici).
  - Non protégée contre les accès concurrents : un futur serveur threadé devra verrouiller.
  - `name` est copié deux fois (une dans `mNames`, une comme clé de `mIds`).

#### `const std::string& AssetIds::name(uint16_t id) const`
- **Rôle** : résolution inverse id → nom, utilisée par `Game::drainEvents` et `renderSnapshot`.
- **Paramètres** : `id` — id reçu, potentiellement arbitraire puisqu'il vient du réseau.
- **Retour** : référence sur le nom ; une référence sur une **chaîne vide statique** si `id` est hors bornes.
- **Effets de bord** : aucun (`const`).
- **Pièges** : le garde `id >= mNames.size()` est indispensable. Le repli sur la chaîne vide fait que `getTexture("")` / `playSFX("")` échouent proprement, donc un id invalide donne une absence d'asset plutôt qu'un crash. La référence renvoyée est invalidée par un `push_back` ultérieur de `id()` (réallocation de `mNames`) : **ne pas conserver** le résultat au-delà de l'usage immédiat — c'est respecté partout dans le code actuel, où il est passé directement en argument.

#### `uint16_t AssetIds::count() const`
- **Rôle** : nombre d'entrées, chaîne vide incluse.
- **Retour** : `mNames.size()` tronqué en `uint16_t`.
- **Pièges** : **jamais appelée** (code mort). Elle serait utile pour transmettre la table d'assets lors du `Welcome`.

#### `AssetIds& AssetIds::getInstance()`
- **Rôle** : accès au singleton.
- **Retour** : référence sur une variable `static` locale de fonction.
- **Effets de bord** : construit l'instance au premier appel.
- **Pièges** : idiome du « Meyers singleton » — initialisation garantie **thread-safe** depuis C++11, et pas de problème d'ordre d'initialisation statique. En revanche l'ordre de **destruction** en fin de programme reste non contrôlé par rapport aux autres statiques.

**Code mort de ce fichier :** `AssetIds::count()` n'est jamais appelée. Le constructeur enregistre la chaîne vide dans `mNames` mais pas dans `mIds`, laissant une incohérence latente si `id("")` était un jour appelé.

---

## include/GameEvent.hpp + src/GameEvent.cpp

### `enum class EventType`
`Sfx`, `StopSfx`, `Effect`, `Shake` — valeurs implicites 0 à 3. L'ordre compte : il est sérialisé sur un octet, donc **insérer une valeur au milieu casse la compatibilité** du protocole.

### `struct GameEvent`
Événement de présentation produit par la simulation, à charge utile générique : un seul format pour les quatre types, les champs changeant de sens selon `type`.

| Membre | Type | Rôle |
|---|---|---|
| `type` | `EventType` | Nature de l'événement. **Sans initialiseur** → indéterminé pour un `GameEvent` déclaré sans l'affecter. |
| `id` | `uint16_t` | Id `AssetIds` du son (`Sfx`) ou de l'animation (`Effect`) ; inutilisé pour `StopSfx` et `Shake`. |
| `x`, `y` | `float` | Position en pixels, utilisée par `Effect` seulement. |
| `a` | `float` | Champ générique : échelle pour `Effect`, intensité en pixels pour `Shake`. |
| `b` | `float` | Champ générique : durée en secondes pour `Shake`. |
| `handle` | `uint32_t` | Identifiant du son joué, renvoyé par `sfx()` et nécessaire à `stopSfx()` ; 0 pour `Effect` et `Shake`. |

Les noms `a`/`b` sont volontairement neutres pour rester réutilisables, au prix de la lisibilité côté consommateur : le sens exact n'existe que dans l'accord entre `EventQueue::effect`/`shake` et le `switch` de `Game::drainEvents`.

### `class EventQueue`

Le canal unique par lequel la simulation demande des sons, des animations et du screen-shake **sans jamais toucher SDL**. Elle est embarquée dans le snapshot, donc les effets traversent le réseau de la même façon que l'état.

| Membre | Type | Rôle |
|---|---|---|
| `mEvents` | `std::vector<GameEvent>` | Événements accumulés depuis le dernier `clear()`. |
| `mNextHandle` | `uint32_t` | Prochain handle à distribuer ; part de **1**, donc 0 signifie « pas de handle ». |

#### `uint32_t EventQueue::sfx(const std::string& id)`
- **Rôle** : demande le déclenchement d'un son et renvoie un handle permettant de l'arrêter plus tard.
- **Paramètres** : `id` — nom du SFX tel qu'enregistré dans `AudioManager` (`"jetpackThrust"`, `"explosion"`, …).
- **Retour** : le handle attribué, toujours `>= 1`.
- **Effets de bord** : `AssetIds::id(id)` — **enregistre le nom dans le singleton s'il est inconnu** ; `push_back` dans `mEvents` ; incrémente `mNextHandle`.
- **Pièges** : l'appelant **doit conserver** le handle s'il veut pouvoir couper le son (c'est ce que fait le `Player` pour la boucle de jetpack). `mNextHandle` n'est jamais remis à zéro et déborderait après 2³² sons — inatteignable. Un son dont le nom n'existe pas dans `AudioManager` est silencieusement ignoré côté présentation (`playSFX` renvoie un canal négatif, aucune entrée dans `mSfxChannels`).

#### `void EventQueue::stopSfx(uint32_t handle)`
- **Rôle** : demande l'arrêt du son identifié par `handle`.
- **Paramètres** : `handle` — valeur renvoyée par un `sfx()` antérieur.
- **Retour** : —
- **Effets de bord** : `push_back` d'un événement dont seuls `type` et `handle` sont significatifs.
- **Pièges** : aucune validation du handle ; un handle inconnu, périmé ou nul est simplement ignoré par `drainEvents`. Si le son est déjà terminé de lui-même, `Game` a déjà retiré l'entrée de `mSfxChannels` lors de sa passe de nettoyage — l'arrêt est alors un no-op, ce qui évite de couper un canal recyclé par un autre son.

#### `void EventQueue::effect(const std::string& id, float x, float y, float scale)`
- **Rôle** : demande une animation one-shot à une position donnée.
- **Paramètres** : `id` — nom d'animation enregistré dans `AnimationManager` (ex. `"explosion_missile"`) ; `x`, `y` — position en **pixels** ; `scale` — facteur d'échelle (1 = taille native), rangé dans `a`.
- **Retour** : —
- **Effets de bord** : `AssetIds::id(id)` (peut enregistrer un nouveau nom) ; `push_back`.
- **Pièges** : `handle` reste à 0 — un effet n'est pas annulable. Le paramètre `scale` n'a pas de valeur par défaut ici alors que `EffectManager::spawn` en a une (`1.f`) : les appelants Lua/C++ doivent toujours le fournir.

#### `void EventQueue::shake(float intensity, float duration)`
- **Rôle** : demande une secousse d'écran.
- **Paramètres** : `intensity` — amplitude en **pixels**, rangée dans `a` ; `duration` — durée en **secondes**, rangée dans `b`.
- **Retour** : —
- **Effets de bord** : `push_back` ; `id`, `x`, `y`, `handle` restent à 0.
- **Pièges** : plusieurs secousses dans le même tick produiront autant d'appels à `triggerShake`, qui les écrase l'une après l'autre (seule la dernière compte) plutôt que de les cumuler.

#### `const std::vector<GameEvent>& EventQueue::events() const`
- **Rôle** : accès en lecture à la liste accumulée ; utilisé par `captureSnapshot` pour la copier.
- **Retour** : référence constante sur `mEvents`.
- **Effets de bord** : aucun.
- **Pièges** : la référence est invalidée par le prochain `push_back` ou `clear()` — d'où la **copie** faite par `captureSnapshot`, qui est indispensable puisque `Server::update` appelle `clear()` juste après.

#### `void EventQueue::clear()`
- **Rôle** : vide la file après que les événements ont été capturés dans un snapshot.
- **Effets de bord** : `mEvents.clear()` (capacité conservée) ; `mNextHandle` **n'est pas** réinitialisé, ce qui est essentiel : les handles doivent rester uniques pour la durée de la partie afin qu'un `StopSfx` émis plusieurs ticks après le `Sfx` corresponde encore.
- **Pièges** : appelée **uniquement** par `Server::update`, et seulement après une émission de snapshot. Si le transport est absent, aucun snapshot n'est émis, donc la file grossirait indéfiniment — mais dans ce cas la simulation ne tourne pas non plus. En revanche, si un snapshot est émis et perdu (UDP, ou snapshots intermédiaires écrasés par `Client::poll`), les événements qu'il portait sont **définitivement perdus** : les sons et explosions sont intrinsèquement non fiables dans cette architecture.

**Code mort de ce fichier :** aucun ; les quatre producteurs et les deux accesseurs sont tous utilisés. À noter : `GameEvent::type` n'a pas d'initialiseur de membre par défaut contrairement à tous les autres champs (incohérence qui ne mord pas, car tous les producteurs l'affectent et `readEvent` l'écrase).

---

## include/PlayerInput.hpp

### `struct PlayerInput`
Intention de jeu d'un joueur pour un tick, réduite à quatre booléens. C'est le seul objet qui remonte du client vers le serveur, et il est sérialisé sur **un unique octet** par `writeInput`.

| Membre | Type | Rôle |
|---|---|---|
| `left` | `bool` | Demande de déplacement vers la gauche (bit 0 sur le réseau). |
| `right` | `bool` | Demande de déplacement vers la droite (bit 1). |
| `thrust` | `bool` | Jetpack actif (bit 2). |
| `ability` | `bool` | Déclenchement de l'ability du chapeau (bit 3). |

Aucune fonction. Tous les membres ont un initialiseur par défaut (`false`), donc un `PlayerInput` déclaré nu est un input neutre — propriété sur laquelle reposent `readInput` (qui ne touche pas `out` en cas d'échec) et `input::sample` (qui renvoie l'objet par défaut dans ses chemins d'échec).

**Remarques de conception** : l'absence de numéro de séquence et d'horodatage est le manque principal pour la suite du chantier réseau — sans eux, impossible de détecter une perte, de rejeter un paquet réordonné, ni de faire de la prédiction client avec réconciliation. L'input est purement **booléen** (pas d'axe analogique) : la zone morte du joystick est appliquée à l'échantillonnage, et la finesse de l'axe est définitivement perdue.

---

## include/InputSampler.hpp + src/InputSampler.cpp

### `namespace input`
Fonction libre d'échantillonnage, volontairement sortie de `Player` : c'est de la **lecture de périphérique**, donc du ressort du client, pas de la simulation. Seul point du code netcode qui touche encore SDL en dehors de `Game`.

#### `static constexpr int DEAD_ZONE = 8000;` *(constante de fichier)*
Seuil de zone morte sur l'axe horizontal du joystick, exprimé dans l'échelle `Sint16` de SDL (`-32768..32767`), soit environ **24 %** de la course. À liaison interne, invisible hors de `InputSampler.cpp`.

#### `PlayerInput input::sample(const KeyPreset& preset, int joystickId, const Uint8* keys)`
- **Rôle** : produit l'intention de jeu d'un joueur en interrogeant son périphérique : manette si le joueur en a une, sinon clavier via son `KeyPreset`.
- **Paramètres** :
  - `preset` — les quatre `SDL_Scancode` du joueur (`left`, `right`, `thrust`, `missile`). **Ignoré** si `joystickId != -1`.
  - `joystickId` — *instance id* SDL du joystick, ou `-1` pour « joueur au clavier ». Attention : c'est bien un **instance id** (`SDL_JoystickFromInstanceID`), pas un index de périphérique — les deux ne coïncident pas après branchements/débranchements.
  - `keys` — tableau d'état clavier obtenu par `SDL_GetKeyboardState`, indexé par scancode. Peut être `nullptr`.
- **Retour** : un `PlayerInput`. Renvoie un input **neutre** (tout à `false`) si le joystick demandé n'est pas ouvert, ou si `keys == nullptr` sur le chemin clavier.
- **Effets de bord** : aucun sur l'état du jeu ; lit l'état courant des périphériques SDL (`SDL_JoystickFromInstanceID`, `SDL_JoystickGetAxis`, `SDL_JoystickGetButton`).
- **Mapping manette** : axe 0 → `left`/`right` via la zone morte ; bouton 0 → `thrust` ; bouton 1 → `ability`. Les numéros sont codés en dur et non configurables.
- **Pièges** :
  - **Piège principal** : sur le chemin clavier, `preset` est indexé sans aucune validation. Comme `PlayerConfig::keyPreset` est un `KeyPreset` **sans initialiseurs** et que `Game::init` ne le remplit que si `presetIndex >= 0`, un joueur sans preset **et** sans manette fait lire `keys[valeur indéterminée]` → lecture hors du tableau de `SDL_NUM_SCANCODES` entrées, donc **comportement indéfini**. Correctif : donner des valeurs par défaut à `KeyPreset`, ou refuser d'échantillonner un joueur sans périphérique.
  - Le chemin manette **retourne tôt** : un joueur avec manette ne peut pas aussi jouer au clavier.
  - `SDL_JoystickGetButton` renvoie un `Uint8` converti implicitement en `bool` — correct.
  - `SDL_JoystickFromInstanceID` est appelée à **chaque joueur et chaque frame** : c'est une recherche dans la liste des joysticks ouverts, négligeable à 2–4 joueurs mais évitable en conservant le `SDL_Joystick*`.
  - Appelée depuis `Game::update`, donc **une fois par frame de rendu**, pas une fois par pas de simulation : si le serveur rattrape plusieurs pas, le même input est réappliqué à chaque pas (comportement attendu), et si le framerate dépasse 60 Hz, certains échantillons sont écrasés avant d'avoir servi.
  - Le débranchement d'une manette en cours de partie fait renvoyer un input neutre (le joystick n'est plus trouvable) : dégradation propre, le joueur devient simplement inerte.

**Code mort de ce fichier :** aucun. Le header inclut `<SDL2/SDL.h>` pour `Uint8`, et `KeyPreset.hpp` pour le type du premier paramètre.

---

## include/PlayerInfo.hpp

### `struct PlayerInfo`
Données **statiques** d'un joueur : tout ce que le rendu a besoin de savoir et qui ne change pas d'un tick à l'autre, donc tout ce qu'il serait inutile de répéter dans chaque snapshot. C'est le pendant « constant » de `PlayerState`. Dans l'architecture visée, cette structure sera transmise une seule fois à la connexion (message `Welcome`) ; aujourd'hui elle est construite localement par `Game::init` depuis les `PlayerSlot` du menu.

| Membre | Type | Rôle |
|---|---|---|
| `skinId` | `std::string` | Clé de texture du corps dans `TextureManager` (ex. `"skin_xxx"`). |
| `hatId` | `std::string` | Clé de texture du chapeau ; détermine aussi l'ability côté simulation. |
| `maxLife` | `int` | Vie maximale, dénominateur de la barre de vie du HUD. Défaut 100. |
| `colliderW` | `int` | Largeur du collider en pixels, pour le rectangle de debug. Défaut 32. |
| `colliderH` | `int` | Hauteur du collider en pixels. Défaut 32. |
| `showCollider` | `bool` | Active le tracé magenta du collider. Défaut `false`. |

Aucune fonction.

**Pièges** : les identifiants sont des `std::string`, donc **non sérialisables tels quels** dans le format binaire actuel ; il faudra les convertir en ids `AssetIds` (ou transmettre la table) pour le mode en ligne. `colliderW`/`colliderH` sont remplis dans `Game::init` depuis `cfg.collider` **avant** toute personnalisation, donc valent toujours 32×32 même si le `Player` utilise un autre collider — le rectangle de debug peut donc mentir.

---

## include/GameContext.hpp

### `struct GameContext`
Sac de **pointeurs non possédés** vers les systèmes du monde, câblé une fois par `World::init` (plus `particleManager` par `Game::init`) et passé aux abilities, notamment aux abilities Lua créées par `ScriptEngine`. C'est le point d'entrée unique par lequel un script peut agir sur la simulation.

| Membre | Type | Rôle |
|---|---|---|
| `players` | `std::vector<player::Player>*` | Le vecteur de joueurs du `PlayerManager` : permet de cibler, pousser, blesser. |
| `projectiles` | `projectile::ProjectileManager*` | Permet de créer des projectiles. |
| `events` | `EventQueue*` | Permet de demander sons, effets et secousses **sans toucher SDL** — c'est ce qui garde `World` pur. |
| `particleManager` | `ParticleManager*` | **Exception à cette pureté** : pointe sur un objet qui vit côté présentation (`Game::particleManager`), et n'est pas rempli par `World::init` mais par `Game::init`. Une ability qui spawne des particules par ce biais produit un effet purement local, invisible des autres clients. À faire passer par l'`EventQueue`. |
| `screenWidth` | `int*` | Largeur logique de jeu en pixels. |
| `effectiveHeight` | `int*` | Hauteur jouable en pixels (hors bande de HUD). |
| `screenHeight` | `int*` | Hauteur totale en pixels. |
| `globalSpeed` | `float*` | Vitesse de défilement en pixels/seconde, **modifiable** par un script. |

Aucune fonction. Le header utilise des déclarations anticipées (`namespace player{class Player;}`, `ProjectileManager`, `ParticleManager`, `EventQueue`) pour rester très léger à inclure — mais il inclut `<vector>`, nécessaire au type complet `std::vector<Player>*`... qui ne requiert en réalité qu'une déclaration de `std::vector`, l'inclusion restant la solution portable.

**Pièges** :
- Les pointeurs visent des **membres du `World`** : déplacer, copier ou détruire le `World` laisse un `GameContext` entièrement pendant. Aucune des abilities ne peut détecter ce cas.
- Tous les champs valent `nullptr` avant `World::init()` ; **aucune ability ne vérifie la nullité**, donc créer une ability avant `init()` mène à un déréférencement nul.
- `players` pointe sur le vecteur, pas sur ses éléments : un `push_back` qui réalloue invalide tout pointeur ou référence qu'une ability aurait mémorisé vers un `Player`. D'où le `reserve(joinedCount)` de `Game::init`.
- Les trois dimensions sont exposées **par pointeur mutable** : un script pourrait les modifier, ce qui désynchroniserait le rendu (qui lit directement `mServer.world()`).

---

## src/main.cpp

#### `int main(int argc, char* args[])`
- **Rôle** : point d'entrée. Initialise SDL et ses extensions, crée fenêtre et renderer, câble les singletons globaux, charge les mods Lua, empile la `MenuScene`, puis exécute la boucle principale (horloge, pompe d'événements SDL, gestion du branchement à chaud des manettes, `update`/`render` de la scène courante), et enfin nettoie tout.
- **Paramètres** : `argc` / `args` — **tous deux inutilisés** ; aucune option de ligne de commande n'est traitée (pas de `--server`, `--connect`, ce qu'il faudra ajouter pour le mode en ligne).
- **Retour** : toujours `0`, même en cas d'échec d'initialisation.
- **Effets de bord** :
  - `SDL_Init(VIDEO | JOYSTICK | AUDIO)`, `IMG_Init(PNG)`, `Mix_OpenAudio(48000 Hz, format par défaut, 2 canaux, tampon de 2048 échantillons)`, `TTF_Init()`.
  - Deux hints : `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS` (les manettes continuent de répondre fenêtre non focalisée) et `SDL_HINT_RENDER_SCALE_QUALITY = "0"` (mise à l'échelle au plus proche voisin — indispensable pour du pixel art).
  - Crée une fenêtre 1024×576 en `FULLSCREEN_DESKTOP`, un renderer accéléré, et fixe la **taille logique** à 1024×576 : c'est cette valeur que `Game::init` récupère par `SDL_RenderGetLogicalSize` et qui devient `World::screenWidth/screenHeight`.
  - `TextureManager::init(renderer)`, `ScriptEngine::init()`, `ScriptEngine::loadMods("mods")` — c'est ici que `mods/base/textures.lua` charge la texture `"bg"` et que chapeaux, projectiles et abilities sont enregistrés.
  - Possède `openJoysticks` (`std::map<instanceId, SDL_Joystick*>`), ouvre sur `SDL_JOYDEVICEADDED`, ferme sur `SDL_JOYDEVICEREMOVED`, et ferme tout le reste en sortie de boucle.
  - Nettoyage final : `TextureManager::clean()`, `SDL_DestroyRenderer`, `SDL_DestroyWindow`, `TTF_Quit`, `IMG_Quit`, `SDL_Quit`.
- **Boucle principale** : tant que la pile de scènes n'est pas vide → calcule `deltaTime = deltaTimer.getTicks() / 1000.0f` (**secondes**, bridé à **0,05 s**), redémarre le chrono, pompe tous les événements SDL (manettes, `SDL_QUIT`, puis transmission à `manager.current()->handleEvent`), sort si plus de scène courante, puis `update(deltaTime)`, `render()` et `SDL_Delay(16)`.
- **Pièges** :
  - **Aucune vérification d'erreur** sur `SDL_Init`, `IMG_Init`, `Mix_OpenAudio`, `TTF_Init`, `SDL_CreateWindow` ni `SDL_CreateRenderer`. Un renderer nul se propagerait jusqu'au premier appel de dessin.
  - Le bridage `deltaTime > 0.05f` est le pendant côté client du `MAX_STEPS_PER_FRAME` du serveur : sur une frame très longue, le temps est **jeté** plutôt que rattrapé — les deux mécanismes se cumulent, le jeu ralentit au lieu de téléporter les entités.
  - `deltaTimer.start()` est appelé **avant** la pompe d'événements, donc `deltaTime` mesure bien la frame précédente complète. Comme `LTimer::getTicks()` est en millisecondes entières, la résolution temporelle est de 1 ms : à 60 FPS, `deltaTime` oscille entre 16 et 17 ms, ce qui introduit une gigue permanente dans l'accumulateur du serveur.
  - `SDL_Delay(16)` est un plafonnement de framerate **grossier et non adaptatif** : il s'ajoute au temps de rendu au lieu de le compenser, et ne tient pas compte de la VSync (le renderer est créé sans `SDL_RENDERER_PRESENTVSYNC`). Le framerate réel plafonne donc en dessous de 60. Un vrai limiteur calculerait `16 - temps_de_frame`.
  - Sur `SDL_QUIT`, toutes les scènes sont dépilées **à l'intérieur** de la boucle d'événements, mais les événements restants sont quand même traités ; le garde `if(manager.current())` évite le déréférencement nul, et le `if(!manager.current()) break;` sort avant `update`/`render`.
  - **`Mix_CloseAudio()` et `Mix_Quit()` ne sont jamais appelés** — fuite à la terminaison, bénigne mais réelle, et asymétrique par rapport à `TTF_Quit`/`IMG_Quit`.
  - Les joysticks sont fermés après la boucle, mais le `Game` détruit plus tôt (via `GameScene`) a pu stocker des instance ids : aucun risque ici puisque `Game` ne possède aucun joystick.
  - `std::map` (ordonnée) suffit largement pour quelques manettes ; `unordered_map` serait marginalement plus adapté.
  - `#include <map>` est nécessaire ; `<SDL2/SDL_hints.h>`, `<SDL2/SDL_video.h>` etc. sont déjà tirés par `SDL.h` (inclusions explicites redondantes mais saines).

**Code mort de ce fichier :**
- Les paramètres `argc` et `args` ne sont jamais utilisés.
- `Mix_CloseAudio` / `Mix_Quit` manquants (symétrie de nettoyage incomplète plutôt que code mort à proprement parler).

---


# Gameplay et scripting

## include/Player.hpp + src/Player.cpp

### `struct player::PlayerConfig`

Sac de configuration *passé par rvalue* au constructeur de `Player` (`PlayerConfig&&`) et stocké par valeur dans le joueur. Il mélange trois natures de données : des réglages de physique lus depuis `Config` (voir `Game::init`), des pointeurs vers des services partagés (audio, particules, `EventQueue`) et la propriété exclusive de l'ability (`std::unique_ptr<Ability>`). C'est ce dernier point qui impose le déplacement : `PlayerConfig` n'est pas copiable.

| Membre | Type | Rôle |
|---|---|---|
| `jetpackForce` | `float` | Poussée du jetpack en px/s² (appliquée tant que la touche est tenue). Défaut 700. |
| `maxVy` | `float` | Vitesse verticale max en px/s. **Jamais lue** (voir code mort). |
| `maxVx` | `float` | Clamp de la vitesse horizontale en px/s. Défaut 1000. |
| `acceleration` | `float` | Accélération horizontale en px/s² appliquée via `dir`. Défaut 1000. |
| `deceleration` | `float` | Coefficient de frottement horizontal, sans unité, utilisé comme `1 - (1 - deceleration) * dt`. Défaut 0.50 dans le header, 0.8 dans `Game::init`. |
| `gravityForce` | `float` | Gravité en px/s², **convention négative** : `vy -= gravityForce * dt`, donc -500 fait tomber le joueur. |
| `bounceRestitution` | `float` | Coefficient de restitution (0 = amorti, 1 = élastique) pour les murs et les collisions joueur/joueur. |
| `maxHealth` | `int` | PV max, sert aussi de PV initiaux et de retour à `getMaxLife()`. |
| `bounce` | `bool` | Si faux, les murs annulent la vitesse ; si vrai, ils la renvoient avec `bounceRestitution`. |
| `screenWidth` | `int` | Largeur de la zone jouable en pixels (mur droit). |
| `screenHeight` | `int` | Hauteur de la zone jouable en pixels (mur bas) ; `Game::init` y met `effectiveHeight` (écran moins le bandeau HUD de 50 px). |
| `showCollider` | `bool` | Debug : affichage du collider. Jamais lu par `Player` ; recopié dans `PlayerInfo` côté client avant le move. |
| `collider` | `SDL_Rect` | Offset (`x`,`y`) et taille (`w`,`h`) initiaux du collider en pixels. Défaut 32x32. |
| `colliderColor` | `SDL_Color` | **Jamais lu** (la couleur est codée en dur dans `Game::renderSnapshot`). |
| `keyPreset` | `KeyPreset` | Touches clavier du joueur ; relu par `getKeyPreset()` pour l'échantillonnage d'input. |
| `joystickId` | `int` | *Instance id* SDL de la manette, -1 = clavier. |
| `skin` / `hat` | `LTexture*` | Textures du joueur. Renseignées mais **plus utilisées pour le rendu** (le client passe par `PlayerInfo.skinId`). |
| `hatId` / `skinId` | `std::string` | Identifiants d'assets. `skinId` est lu par la logique de gameplay (bonus tortue/écureuil), `hatId` sert uniquement à choisir l'ability dans `Game::init`. |
| `audioManager` | `AudioManager*` | Utilisé **directement** (hors `EventQueue`) pour la boucle sonore du jetpack. |
| `events` | `EventQueue*` | File d'événements de la simulation : les sons de rebond y sont empilés au lieu d'être joués. Peut être `nullptr` (toujours testé). |
| `players` | `std::vector<Player>*` | **Jamais lu** par `Player`. Renseigné par `Game::init`. |
| `thrustParticleConfig` | `ParticleConfig` | Réglages des particules de poussée (copie par valeur). |
| `particleManager` | `ParticleManager*` | Cible des particules de poussée. Déréférencé **sans test de nullité**. |
| `ability` | `std::unique_ptr<Ability>` | Ability du joueur, possédée. `nullptr` si le chapeau n'en déclare pas. |

### `class player::Player`

Entité simulée du joueur : position/vitesse en pixels, PV, score, collider, et une ability optionnelle. Elle est *simulation-only* : elle ne dessine rien, le rendu passe par `captureSnapshot()` puis `Game::renderSnapshot`. C'est le type exposé à Lua sous le nom `Player`.

| Membre de données | Type | Rôle |
|---|---|---|
| `isAlive` | `bool` (public) | Passe à `false` dès que `life <= 0` ; gèle toute la mise à jour. Exposé en lecture seule à Lua. |
| `collider` | `SDL_Rect` (public) | Boîte de collision en pixels, resynchronisée à la fin de `update()`. C'est **elle** (et pas `x`/`y`) que lisent les bindings `getPosition`/`getCenter`/`getSize`. |
| `isControlled` | `bool` (public) | Le joueur est piloté par un script (ex. traîneau de `christmas.lua`) : physique, dégâts et collisions suspendus. Lecture/écriture depuis Lua. |
| `x`, `y` | `float` | Position du coin haut-gauche en pixels. |
| `vx`, `vy` | `float` | Vitesse en px/s. `vy` positif = vers le bas. |
| `score` | `int` | Score, sert aussi de **monnaie** : `LuaAbility` en retire `cost`. |
| `dir` | `int` | Direction horizontale demandée ce tick (-1, 0, 1), remise à 0 à chaque `update()`. |
| `jetpackThrust` | `float` | Poussée en px/s² demandée ce tick, remise à 0 à chaque `update()`. |
| `thrustParticlesTimer` | `LTimer` | Limite le débit des particules de poussée (une toutes les 20 ms). |
| `life` | `int` | PV courants. |
| `jetpackChannel` | `int` | Canal SDL_mixer de la boucle jetpack, -1 si aucun. |
| `mJetpackActive` | `bool` | Mémorise si la boucle sonore tourne, pour ne la lancer/arrêter qu'une fois. |
| `mThrusting` | `bool` | Le joueur poussait pendant le dernier `update()` ; sérialisé dans le snapshot pour l'animation côté client. |
| `config` | `PlayerConfig` | Configuration possédée (voir ci-dessus). |

#### `Player::Player(PlayerConfig&& config)`
- **Rôle** : construit le joueur, le place au centre-haut de l'écran, initialise collider, PV et score.
- **Paramètres** : `config` — configuration déplacée dans le membre `config` via la liste d'initialisation.
- **Retour** : —
- **Effets de bord** : démarre `thrustParticlesTimer` ; `isAlive = true` ; position initiale `x = screenWidth / 2`, `y = screenHeight / 4` (pixels) ; `collider.x/y = config.collider.x/y + x/y`, puis `w`/`h` copiés ; `life = maxHealth` ; `score = 0`. Prend possession du `unique_ptr<Ability>`.
- **Pièges** : dans le corps du constructeur, le nom `config` désigne le **paramètre** (il masque le membre), et ce paramètre vient d'être déplacé. Ça fonctionne parce que seuls des champs scalaires y sont lus (`screenWidth`, `screenHeight`, `collider`, `maxHealth`), que le déplacement de `PlayerConfig` laisse intacts — mais c'est un piège classique : lire un objet *moved-from*. Écrire `this->config` serait plus sûr et plus lisible. Le `collider.x/y` initial inclut l'offset de config, alors que `update()` écrira ensuite `collider.x = x` **sans** offset : l'offset est donc perdu dès la première frame (il vaut 0 par défaut, donc invisible).

#### `void Player::move(int direction)`
- **Rôle** : enregistre la direction horizontale demandée pour le tick courant.
- **Paramètres** : `direction` — -1 (gauche), 1 (droite), 0 (rien) ; sans unité, multiplie `acceleration`.
- **Retour** : —
- **Effets de bord** : écrit `dir`.
- **Pièges** : `dir` est consommé puis remis à 0 par `update()`. Appeler `move()` sans appeler `update()` fait persister la direction. Aucune validation : passer 5 accélère cinq fois plus.

#### `void Player::jetpack()`
- **Rôle** : demande la poussée du jetpack pour ce tick et émet une particule de poussée si le débit le permet.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : `jetpackThrust = config.jetpackForce` ; si `thrustParticlesTimer.getTicks() >= 20` (ms), redémarre le timer et appelle `config.particleManager->spawnThrustParticle(x + 5, y + 25, config.thrustParticleConfig)` (offset en pixels sous le joueur).
- **Pièges** : `config.particleManager` est déréférencé **sans test de nullité** → crash si un `Player` est construit sans particule manager. Les particules sont un effet **visuel** créé depuis la simulation : contrairement aux sons et aux effets d'animation (qui passent par `EventQueue`), elles court-circuitent la file d'événements et ne seront donc pas reproductibles sur un client distant.

#### `void Player::teleportTo(float x, float y)`
- **Rôle** : déplace instantanément le joueur (coin haut-gauche).
- **Paramètres** : `x`, `y` — pixels, repère écran.
- **Retour** : —
- **Effets de bord** : écrit `this->x`, `this->y`. **Ne touche pas au collider** ni aux vitesses.
- **Pièges** : le collider ne sera resynchronisé qu'à la fin du prochain `update()` — et jamais si `isControlled` est vrai (sortie anticipée). Les scripts qui téléportent un joueur contrôlé (traîneau) laissent donc un collider figé à l'ancienne position, ce que voient `playersInRadius`, `explode` et `Player:getCenter()` côté Lua. Exposé à Lua sous le nom `teleport`.

#### `void Player::updateScore(int toAdd)`
- **Rôle** : ajoute (ou retire si négatif) des points.
- **Paramètres** : `toAdd` — points, signé.
- **Retour** : —
- **Effets de bord** : `score += toAdd`.
- **Pièges** : aucun plancher : le score peut devenir négatif (par exemple si plusieurs coûts d'ability se cumulent). Exposé à Lua sous `addScore`.

#### `int Player::getScore() const`
- **Rôle** : lit le score.
- **Paramètres** : aucun. **Retour** : points. **Effets de bord** : aucun.
- **Pièges** : `LuaAbility::use` s'en sert comme test de solvabilité (`score < cost`).

#### `void Player::applyInput(const PlayerInput& in)`
- **Rôle** : traduit un état d'input (déjà échantillonné par `input::sample`) en actions de gameplay. C'est le point d'entrée « autoritaire » : en client-serveur, c'est ici qu'arrive l'input réseau.
- **Paramètres** : `in` — booléens `left`, `right`, `thrust`, `ability`.
- **Retour** : —
- **Effets de bord** : `move(-1)` ou `move(1)` (gauche prioritaire sur droite) ; `jetpack()` si `thrust` ; gère la boucle sonore du jetpack (`playSFX("jetpackThrust", -1)` → `jetpackChannel`, `mJetpackActive`) ; `config.ability->use(this)` si `ability` et si une ability existe.
- **Pièges** : sort immédiatement si `!isAlive`, donc un mort ne relâche jamais sa boucle sonore de jetpack (elle ne sera coupée qu'au destructeur). **Le son du jetpack est joué directement via `config.audioManager`**, alors que tout le reste du jeu empile ses sons dans `EventQueue` ; ça ne marche que parce que serveur et client vivent dans le même process (`LoopbackTransport`). C'est le dernier son non migré vers la file d'événements. `in.ability` est un état, pas un front : maintenir la touche appelle `use()` chaque tick (le cooldown dans `LuaAbility` sert de garde-fou).

#### `void Player::update(float deltaTime)`
- **Rôle** : intègre la physique du joueur pour un tick : frottement + accélération horizontale, jetpack, gravité, murs, synchronisation du collider, détection de mort.
- **Paramètres** : `deltaTime` — secondes (plafonné à 0.05 s dans `main`).
- **Retour** : —
- **Effets de bord** : met à jour l'ability (`config.ability->update(dt)`) ; modifie `vx`, `vy`, `x`, `y`, `collider.x/y` ; empile jusqu'à un événement `sfx("boing")` par mur touché dans `config.events` ; écrit `mThrusting` et remet `jetpackThrust` et `dir` à 0 ; peut passer `isAlive` à `false` et imprime `"Player dead"`.
- **Pièges** :
  - Sortie immédiate si `!isAlive`.
  - Si `isControlled`, sortie **après** la mise à jour de l'ability mais **avant** toute physique : ni position, ni vitesse, ni collider ne bougent (`mThrusting` et `jetpackThrust` sont forcés à 0). Le script qui prend le contrôle doit donc gérer lui-même la position *et* assumer un collider figé.
  - Le frottement est appliqué **après** l'accélération et sous forme `(vx + a*dt*dir) * (1 - (1 - deceleration) * dt)` : le résultat dépend du framerate et devient instable si `(1 - deceleration) * dt > 1`.
  - `vy` n'est **jamais** clampé (`config.maxVy` n'est pas utilisé) : pas de vitesse terminale.
  - L'intégration est de type Euler explicite : la vitesse du tick courant est appliquée après avoir été modifiée.
  - Pour les rebonds, le test du son `abs(vx) > 5` est fait **après** la réflexion, donc sur la vitesse déjà réduite par la restitution, pas sur la vitesse d'impact. De plus `abs` n'est pas qualifié (`std::abs`) sur un `float` : selon les en-têtes tirés par SDL, la surcharge entière peut être choisie et tronquer l'argument. Utiliser `std::abs` lèverait le doute.
  - Le mur bas est à `screenHeight - collider.h`, donc la zone jouable dépend de la taille du collider.
  - `collider.x = x` tronque le `float` vers `int` (perte du sous-pixel pour toute la détection de collision).
  - La mort n'est détectée qu'à la fin du tick, donc un joueur à 0 PV agit encore pendant la frame où il meurt.

#### `void Player::applyKnockBack(float forceX, float forceY)`
- **Rôle** : ajoute une impulsion instantanée à la vitesse (explosions, coups).
- **Paramètres** : `forceX`, `forceY` — en px/s (c'est une vitesse ajoutée, pas une force : aucune masse ni `dt`).
- **Retour** : —
- **Effets de bord** : `vx += forceX`, `vy += forceY`.
- **Pièges** : pas de clamp — un gros knockback peut dépasser `maxVx` pendant un tick avant que `update()` ne le ramène. Ignore `isControlled` (contrairement à `updateLife`), donc un joueur contrôlé accumule de la vitesse qui s'appliquera d'un coup à la libération.

#### `void Player::updateLife(int toAdd)`
- **Rôle** : applique des dégâts (`toAdd` négatif) ou un soin (`toAdd` positif).
- **Paramètres** : `toAdd` — points de vie, signé.
- **Retour** : —
- **Effets de bord** : `life += toAdd`.
- **Pièges** : no-op si `isControlled` (invulnérabilité pendant un scénario scripté). La résistance de la skin `skin_turtle` fait `toAdd *= 0.75` **sur un `int`** : la multiplication passe par un `double` puis est tronquée vers 0 (ex. -40 → -30, mais -1 → 0, donc immunité aux dégâts de 1). Cette réduction s'applique aussi aux **soins** (un soin de 40 ne rend que 30 PV), ce qui est probablement involontaire. Aucun plafonnement à `maxHealth` : on peut dépasser les PV max. `life` peut descendre très en dessous de 0 ; c'est `update()` qui convertit ça en mort.

#### `int Player::getLife() const`
- **Rôle** : PV courants. **Paramètres** : aucun. **Retour** : PV (peut être négatif). **Effets de bord** : aucun.

#### `int Player::getMaxLife() const`
- **Rôle** : PV max, lus depuis la config. **Retour** : `config.maxHealth`. **Effets de bord** : aucun.

#### `float Player::getX() const` / `float Player::getY() const` (inline, header)
- **Rôle** : position flottante réelle du coin haut-gauche, en pixels. **Retour** : `x` / `y`. **Effets de bord** : aucun.
- **Pièges** : utilisés par `captureSnapshot` pour le rendu, alors que le gameplay Lua lit le collider (entier). Les deux peuvent donc diverger d'un sous-pixel — et complètement si `isControlled`.

#### `float Player::getVx() const` / `float Player::getVy() const` (inline, header)
- **Rôle** : vitesse courante en px/s. **Retour** : `vx` / `vy`. **Effets de bord** : aucun. Lus uniquement par `captureSnapshot`.

#### `bool Player::isThrusting() const` (inline, header)
- **Rôle** : indique si le jetpack poussait au dernier tick simulé. **Retour** : `mThrusting`. **Effets de bord** : aucun.
- **Pièges** : c'est un état *du tick précédent* (il est calculé en fin d'`update()`), et il est forcé à `false` quand `isControlled`.

#### `void Player::setVelocity(float vx, float vy)`
- **Rôle** : remplace la vitesse (contrairement à `applyKnockBack` qui l'ajoute).
- **Paramètres** : `vx`, `vy` — px/s.
- **Retour** : — **Effets de bord** : écrit les membres `vx`/`vy` via `this->`.
- **Pièges** : les paramètres masquent les membres, d'où le `this->` obligatoire. Ignore `isControlled`.

#### `void Player::resolveCollisionWith(Player& other)`
- **Rôle** : sépare deux joueurs qui se chevauchent et échange leurs vitesses sur l'axe de moindre pénétration (collision 1D avec restitution).
- **Paramètres** : `other` — l'autre joueur, **modifié**.
- **Retour** : —
- **Effets de bord** : modifie `x`/`y`/`vx`/`vy` et `collider` des **deux** joueurs. Chacun est repoussé de la moitié du chevauchement.
- **Pièges** : no-op si l'un des deux est `isControlled`. L'axe choisi est celui du plus petit chevauchement (`overlapX < overlapY`), ce qui provoque des changements d'axe brusques quand les deux sont proches. Le coefficient de restitution utilisé est celui de `*this` (`config.bounceRestitution`) : la collision est **asymétrique** si les deux joueurs ont des configs différentes. La formule suppose des masses égales. Les positions sont écrites en `float` puis le collider est resynchronisé par troncature. Appelé une seule fois par paire et par tick par `PlayerManager::update`, sans itération de convergence : un empilement à trois joueurs peut rester en interpénétration.

#### `float Player::getAbilityProgress() const`
- **Rôle** : avancement du cooldown de l'ability, pour l'indicateur circulaire du HUD.
- **Paramètres** : aucun. **Retour** : `[0, 1]`, 1 = prête. **Effets de bord** : aucun.
- **Pièges** : **déréférence `config.ability` sans test de nullité**. Or `ScriptEngine::createAbilityForHat` renvoie `nullptr` pour un chapeau inconnu ou sans champ `ability`, et `captureSnapshot` appelle cette fonction pour **chaque joueur à chaque tick** : c'est un crash garanti avec un tel chapeau. Un `return config.ability ? ... : 1.f;` corrigerait. À noter aussi : la méthode est `const` mais appelle `Ability::getCooldownProgress()` qui ne l'est pas — ça compile parce que `std::unique_ptr::operator->() const` renvoie un `T*` non const (la constance du `unique_ptr` ne se propage pas au pointé).

#### `std::string Player::getSkinId() const`
- **Rôle** : identifiant de la skin, utilisé par le gameplay pour les bonus liés au personnage.
- **Retour** : copie de `config.skinId` (chaîne vide possible). **Effets de bord** : aucun (mais une allocation/copie de `std::string` à chaque appel — `const std::string&` suffirait).
- **Pièges** : la logique de gameplay compare des **chaînes littérales** (`"skin_turtle"`, `"skin_squirell"` — avec sa faute de frappe) dispersées dans `Player::updateLife` et `ScoreCollectable::onHit`. C'est exactement le genre de règle qui gagnerait à passer en Lua comme les abilities.

#### `LTexture* Player::getSkin()` / `LTexture* Player::getHat()`
- **Rôle** : accès aux textures du joueur. **Retour** : pointeur possédé par `TextureManager`, potentiellement `nullptr`. **Effets de bord** : aucun.
- **Pièges** : **aucun appelant** depuis la séparation simulation/rendu : le client résout les textures via `PlayerInfo.skinId`/`hatId` et `TextureManager`.

#### `int Player::getJoystickId() const` (inline, header)
- **Rôle** : *instance id* SDL de la manette du joueur. **Retour** : `config.joystickId`, -1 pour le clavier. **Effets de bord** : aucun. Lu par `Game::update` pour alimenter `input::sample`.

#### `const KeyPreset& Player::getKeyPreset() const` (inline, header)
- **Rôle** : touches clavier du joueur. **Retour** : référence vers `config.keyPreset`. **Effets de bord** : aucun.
- **Pièges** : la référence pointe dans le `Player` ; elle est invalidée si le vecteur de joueurs se réalloue. Conceptuellement, un mapping de touches est une donnée **client** qui n'a rien à faire dans l'entité simulée : c'est à nettoyer quand le serveur sera distant.

#### `Player::~Player()`
- **Rôle** : coupe la boucle sonore du jetpack.
- **Effets de bord** : `config.audioManager->stopChannel(jetpackChannel)`.
- **Pièges** : déréférencement **sans test de nullité**. Surtout, les constructeur/affectation par déplacement sont `= default` : un `Player` *moved-from* conserve la **copie** du pointeur `audioManager` et de `jetpackChannel`, donc son destructeur coupe le canal du joueur vivant. C'est invisible aujourd'hui seulement parce que `Game::init` fait `players.reserve(joinedCount)` avant les `emplace_back`, évitant toute réallocation. Un `players.push_back` de trop et le son du jetpack se coupe tout seul.

#### `Player(Player&&) noexcept = default` / `Player& operator=(Player&&) noexcept = default` (header)
- **Rôle** : rendre `Player` déplaçable pour qu'il tienne dans un `std::vector` (la copie est implicitement supprimée par le `unique_ptr<Ability>`).
- **Pièges** : voir le destructeur — le déplacement par défaut ne « neutralise » pas `jetpackChannel`/`audioManager`.

**Code mort / non utilisé dans ce fichier**
- `PlayerConfig::maxVy` : jamais lu, aucune vitesse terminale verticale n'est appliquée.
- `PlayerConfig::colliderColor` : jamais lu (couleur du collider codée en dur dans `Game::renderSnapshot`).
- `PlayerConfig::players` : renseigné par `Game::init`, jamais lu par `Player`.
- `PlayerConfig::skin` / `hat` (et donc `Player::getSkin()` / `getHat()`) : renseignés mais sans aucun appelant depuis le passage au rendu par snapshot.
- `PlayerConfig::showCollider` : jamais lu par `Player`, sert uniquement de relais vers `PlayerInfo`.
- `printf("Player dead\n")` : trace de debug résiduelle dans la boucle de simulation.

## include/PlayerManager.hpp + src/PlayerManager.cpp

### `class player::PlayerManager`

Conteneur propriétaire des joueurs et ordonnanceur de leur tick : il applique les inputs, intègre la physique puis résout les collisions joueur/joueur. Il est possédé par `World` et ne connaît ni rendu ni réseau.

| Membre de données | Type | Rôle |
|---|---|---|
| `players` | `std::vector<Player>` (public) | Les joueurs, stockés **par valeur** ; l'indice dans ce vecteur est l'identité du joueur (utilisé par `PlayerInput`, `PlayerState::index`, `PlayerInfo`). |

#### `void PlayerManager::addPlayer(PlayerConfig&& config)`
- **Rôle** : construit un joueur en place à partir d'une config déplacée.
- **Paramètres** : `config` — configuration transférée (contient le `unique_ptr<Ability>`).
- **Retour** : —
- **Effets de bord** : `players.emplace_back(std::move(config))` ; peut réallouer le vecteur.
- **Pièges** : la réallocation déplace les `Player` existants, ce qui déclenche le destructeur des objets *moved-from* et donc un `stopChannel` parasite (voir `~Player`). Toujours `reserve()` avant, comme le fait `Game::init`. Aucune limite de nombre de joueurs, aucune validation de la config.

#### `void PlayerManager::update(float deltaTime, const std::vector<PlayerInput>& inputs)`
- **Rôle** : un tick complet pour tous les joueurs.
- **Paramètres** : `deltaTime` — secondes ; `inputs` — un `PlayerInput` par joueur, **apparié par indice**.
- **Retour** : —
- **Effets de bord** : pour chaque joueur, `applyInput` (si un input existe à cet indice) puis `update` ; ensuite double boucle sur toutes les paires `(i, j<i)` : si les deux sont vivants et que `util::collide` est vrai, `players[i].resolveCollisionWith(players[j])`.
- **Pièges** : ordre imposé — input **puis** physique, sinon les commandes seraient consommées avant d'être lues (`dir`/`jetpackThrust` sont remis à zéro par `update`). Si `inputs.size() < players.size()`, les joueurs surnuméraires sont simplement inertes (pas d'erreur) ; si `inputs` est plus grand, le surplus est ignoré. Les collisions sont résolues **après** que tous les joueurs ont bougé, et en une seule passe O(n²) sans itération : un sandwich à trois joueurs peut rester en interpénétration. Les collisions joueur/projectile et joueur/collectable sont ailleurs (côté Lua et dans `ScoreCollectable`).

**Code mort / non utilisé dans ce fichier**
- `#include "Utils.hpp"` est nécessaire (`util::collide`) ; rien à signaler côté code mort. À noter cependant que `players` est public : l'encapsulation de la classe est purement nominale (`World`, `Game`, `Snapshot` et `GameContext` y accèdent directement).

## include/Projectile.hpp + src/Projectile.cpp

### `class projectile::Projectile`

Coquille C++ d'un projectile dont **tout le comportement vit en Lua** : la classe ne garde que l'état minimal nécessaire au moteur (position, vitesse, taille, texture, angle, id réseau) et délègue chaque tick à la fonction `onUpdate` fournie dans la table Lua de création. Elle hérite de `std::enable_shared_from_this` pour pouvoir se passer elle-même au script.

| Membre de données | Type | Rôle |
|---|---|---|
| `x`, `y` | `float` | Position du coin haut-gauche en pixels. |
| `vx`, `vy` | `float` | Vitesse en px/s, intégrée par le C++ après l'appel au script. |
| `isAlive` | `bool` | Faux = à supprimer au prochain balayage de `ProjectileManager`. |
| `netId` | `uint32_t` | Identifiant réseau attribué par `ProjectileManager::spawn`, sérialisé dans le snapshot. |
| `textureId` | `std::string` | Nom de la texture tel que fourni par Lua. Conservé mais **plus relu** après le constructeur. |
| `textureAssetId` | `uint16_t` | Id numérique compact (via `AssetIds`) envoyé dans le snapshot à la place de la chaîne. |
| `angle` | `float` | Angle de rendu en degrés, piloté uniquement par Lua. |
| `w`, `h` | `int` | Taille en pixels, utilisée pour le collider, le test hors-écran et le rendu. |
| `ctx` | `GameContext*` | Contexte passé au script à chaque tick ; sert aussi à lire les dimensions de l'écran. |
| `onUpdate` | `sol::protected_function` | Hook Lua appelé chaque tick. Peut être invalide (projectile purement balistique). |
| `data` | `sol::table` | Table Lua **vide à la création**, servant de stockage libre pour le script (ex. `self.data.owner`, `self.data.channel`). |

#### `Projectile::Projectile(sol::table params, GameContext* ctx)`
- **Rôle** : construit un projectile à partir d'une table Lua descriptive.
- **Paramètres** :
  - `params` — table Lua ; clés lues : `texture` (string, défaut `""`), `x`/`y` (pixels, défaut 0), `width`/`height` (pixels, défaut = taille de la texture, sinon 0), `vx`/`vy` (px/s, défaut 0), `onUpdate` (fonction).
  - `ctx` — contexte de jeu, stocké tel quel.
- **Retour** : —
- **Effets de bord** : **enregistre l'id d'asset de la texture** (`AssetIds::id()` crée l'entrée si elle n'existe pas) ; interroge `TextureManager` pour la taille par défaut ; crée une **nouvelle table Lua vide** pour `data` ; `isAlive = true`.
- **Pièges** : `data` est créée vide, elle **ne reprend pas** les champs de `params` — un script qui veut conserver des données doit les écrire après coup dans `projectile.data` (c'est ce que font les mods pour `owner`). Si la texture est inconnue, `tex` est `nullptr` et la taille tombe à 0x0 : le projectile existe, bouge, mais est invisible et son collider est vide. `AssetIds::id()` est appelé même pour une texture vide/inexistante, ce qui pollue la table d'ids. `onUpdate = params["onUpdate"]` ne valide rien : une valeur non appelable sera détectée seulement à l'appel. Le projectile **doit** être créé via `std::make_shared` (voir `update`).

#### `void Projectile::update(float dt)`
- **Rôle** : un tick : appelle le hook Lua, puis intègre la position et tue le projectile s'il est très loin de l'écran.
- **Paramètres** : `dt` — secondes.
- **Retour** : —
- **Effets de bord** : appelle `onUpdate(shared_from_this(), ctx, dt)` ; en cas d'erreur Lua, imprime `"Projectile error : ..."` et **tue le projectile** ; sinon `x += vx*dt`, `y += vy*dt` ; `kill()` si `isOffScreen(5000)`.
- **Pièges** :
  - Sort immédiatement si `!isAlive`, et re-teste `isAlive` **après** le hook : un script qui appelle `self:kill()` empêche l'intégration de la frame.
  - `shared_from_this()` lève `std::bad_weak_ptr` si l'objet n'est pas détenu par un `shared_ptr` : la construction via `make_shared` (ce que fait le binding `spawnProjectile`) est une **précondition**.
  - Le script est appelé **avant** l'intégration : les positions lues côté Lua sont celles du tick précédent.
  - Une erreur Lua détruit silencieusement le projectile (fail-fast volontaire, mais aucun événement n'est émis pour le signaler).
  - La marge de despawn 5000 est codée en dur, en pixels ; c'est très large, donc un projectile « perdu » survit longtemps. Les scripts utilisent leur propre `isOffScreen(50)`.
  - `ctx` est déréférencé par `isOffScreen` sans test de nullité.

#### `bool Projectile::isOffScreen(float margin) const`
- **Rôle** : teste si la boîte du projectile est entièrement sortie de l'écran élargi d'une marge.
- **Paramètres** : `margin` — pixels (défaut 0 dans la déclaration).
- **Retour** : `true` si hors champ.
- **Effets de bord** : aucun.
- **Pièges** : déréférence `ctx->screenWidth` et `ctx->screenHeight`, deux `int*` : crash si `ctx` est nul ou si `World::init()` n'a pas encore câblé le contexte. Le test du haut/gauche utilise la taille (`w + x < -margin`), celui du bas/droite non (`x > screenWidth + margin`) : l'asymétrie est volontaire (coin haut-gauche) mais rend la marge effective différente selon le bord. Utilise `screenHeight` et non `effectiveHeight` : un projectile peut « vivre » dans la bande du HUD.

#### `bool Projectile::isDead() const` (inline, header)
- **Rôle** : prédicat de suppression. **Retour** : `!isAlive`. **Effets de bord** : aucun.
- **Pièges** : doublon exact (inversé) de `isValid()` ; `isDead` est utilisé par le C++, `isValid` par Lua.

#### `void Projectile::kill()` (inline, header)
- **Rôle** : marque le projectile pour suppression. **Effets de bord** : `isAlive = false`. La suppression effective a lieu dans `ProjectileManager::update`, donc le projectile survit jusqu'à la fin du tick courant.

#### `bool Projectile::isValid() const` (inline, header)
- **Rôle** : version « positive » du prédicat, exposée à Lua sous `isValid`. **Retour** : `isAlive`.

#### `uint32_t Projectile::getNetId() const` / `void Projectile::setNetId(uint32_t id)` (inline, header)
- **Rôle** : identifiant réseau stable du projectile. **Retour** : `netId` (0 = non assigné). **Effets de bord** : `setNetId` écrit `netId`.
- **Pièges** : `setNetId` est appelé par `ProjectileManager::spawn` ; l'appeler soi-même casserait l'unicité. Non exposé à Lua (volontairement).

#### `void Projectile::setVelocity(float nvx, float nvy)`
- **Rôle** : remplace la vitesse. **Paramètres** : px/s. **Effets de bord** : écrit `vx`, `vy`. Les noms `nvx`/`nvy` évitent le masquage des membres.

#### `void Projectile::setAngle(float degrees)`
- **Rôle** : définit l'angle de rendu. **Paramètres** : degrés. **Effets de bord** : écrit `angle`.
- **Pièges** : purement cosmétique — l'angle n'influence ni la vitesse ni le collider (qui reste un AABB non tourné). C'est au script de garder angle et vitesse cohérents.

#### `void Projectile::setPosition(float nx, float ny)`
- **Rôle** : téléporte le projectile (coin haut-gauche). **Paramètres** : pixels. **Effets de bord** : écrit `x`, `y`.

#### `std::tuple<float, float> Projectile::getPosition() const` (inline, header)
- **Rôle** : position courante. **Retour** : `{x, y}` en pixels — côté Lua, un tuple devient deux valeurs de retour (`local x, y = p:getPosition()`).

#### `std::tuple<float, float> Projectile::getVelocity() const` (inline, header)
- **Rôle** : vitesse courante. **Retour** : `{vx, vy}` en px/s (deux valeurs côté Lua).

#### `std::tuple<int, int> Projectile::getSize() const` (inline, header)
- **Rôle** : taille. **Retour** : `{w, h}` en pixels.

#### `sol::table Projectile::getData() const` (inline, header)
- **Rôle** : expose la table de stockage libre du script. **Retour** : la table `data` (référence partagée vers l'objet Lua, pas une copie). **Effets de bord** : aucun côté C++, mais le script peut tout y écrire.
- **Pièges** : exposée à Lua en **propriété** (`p.data`, sans parenthèses) via `sol::property`. La table maintient en vie tout ce qu'on y met, y compris des `Player*` (`data.owner`) : un pointeur brut dont la durée de vie n'est **pas** garantie par la table — si le vecteur de joueurs se réalloue, `data.owner` devient pendouillant.

#### `uint16_t Projectile::getTextureAssetId() const` (inline, header)
- **Rôle** : id compact de la texture pour la sérialisation. **Retour** : `textureAssetId` (0 possible). Lu uniquement par `captureSnapshot`.

#### `float Projectile::getAngle() const` (inline, header)
- **Rôle** : angle de rendu en degrés. **Retour** : `angle`. Lu uniquement par `captureSnapshot`.

#### `SDL_Rect Projectile::getCollider() const` (inline, header)
- **Rôle** : AABB du projectile en pixels entiers. **Retour** : `{(int)x, (int)y, w, h}`.
- **Pièges** : **aucun appelant** — les collisions des projectiles sont entièrement faites en Lua (distances au centre, `playersInRadius`, `explode`), pas avec un rectangle.

**Code mort / non utilisé dans ce fichier**
- `Projectile::getCollider()` : jamais appelé (C++ comme Lua).
- `textureId` : écrit dans le constructeur, jamais relu ensuite (seul `textureAssetId` circule).
- `isValid()` / `isDead()` : redondants ; un seul des deux serait suffisant si le binding Lua inversait le résultat.
- `#include "Projectile.hpp"` est inclus deux fois dans `ScriptEngine.cpp` (voir plus bas) ; ici le `#include <memory>` du header ne sert qu'à `enable_shared_from_this`.

## include/ProjectileManager.hpp + src/ProjectileManager.cpp

### `class projectile::ProjectileManager`

Propriétaire des projectiles vivants. Il garantit trois choses : les projectiles créés pendant un tick n'entrent en jeu qu'au tick suivant (file `pending`), les morts sont retirés en une passe (`remove_if`), et chaque projectile reçoit un id réseau unique croissant.

| Membre de données | Type | Rôle |
|---|---|---|
| `projectiles` | `std::vector<std::shared_ptr<Projectile>>` | Projectiles actifs, mis à jour et sérialisés. |
| `pending` | `std::vector<std::shared_ptr<Projectile>>` | Tampon des projectiles créés pendant le tick courant. |
| `mNextNetId` | `uint32_t` | Prochain id réseau à attribuer, commence à 1 (0 = « pas d'id »). |

#### `void ProjectileManager::spawn(std::shared_ptr<Projectile> projectile)`
- **Rôle** : enregistre un projectile qui entrera en jeu au prochain tick.
- **Paramètres** : `projectile` — possession partagée, prise par valeur puis déplacée.
- **Retour** : —
- **Effets de bord** : si non nul, attribue `mNextNetId++` via `setNetId` ; empile dans `pending`.
- **Pièges** : le test de nullité ne protège que l'attribution de l'id — **un `shared_ptr` nul est quand même empilé** et provoquera un déréférencement nul dans `update()`. Aujourd'hui le seul appelant (le binding `spawnProjectile`) renvoie tôt en cas d'erreur, donc le cas ne se produit pas, mais le garde est incomplet. `mNextNetId` ne gère pas le débordement de `uint32_t` (théorique : ~4 milliards de projectiles) et n'est jamais réinitialisé entre deux parties — ce qui est plutôt une bonne chose pour un protocole réseau.

#### `void ProjectileManager::update(float deltaTime)`
- **Rôle** : tick de tous les projectiles, nettoyage des morts, intégration des nouveaux.
- **Paramètres** : `deltaTime` — secondes.
- **Retour** : —
- **Effets de bord** : appelle `p->update(deltaTime)` pour chaque projectile actif (ce qui exécute du code Lua et peut empiler des événements, infliger des dégâts, ou créer d'autres projectiles) ; supprime de `projectiles` tous ceux dont `isDead()` est vrai (libération des `shared_ptr`, donc destruction si plus personne ne les référence) ; déplace `pending` dans `projectiles` et vide `pending`.
- **Pièges** : **l'ordre est critique**. Boucler sur `projectiles` pendant que du Lua peut spawner → c'est précisément pourquoi `pending` existe : sans lui, un `push_back` pendant le `for(auto& p : projectiles)` invaliderait les itérateurs. Corollaire : un projectile créé ce tick ne reçoit son premier `update` qu'au tick suivant, et un projectile qui s'auto-réplique chaque tick fait grossir la liste sans limite (aucun plafond). Le nettoyage est fait **avant** l'intégration des `pending`, donc un projectile créé déjà mort survit un tick de plus. `World::step` appelle `projectileManager.update` **avant** `playerManager.update` : les projectiles voient donc les positions joueurs du tick précédent.

#### `const std::vector<std::shared_ptr<Projectile>>& ProjectileManager::all() const` (inline, header)
- **Rôle** : accès en lecture à la liste des projectiles actifs, pour la sérialisation.
- **Retour** : référence constante sur `projectiles` (ne contient **pas** les `pending`).
- **Effets de bord** : aucun.
- **Pièges** : `captureSnapshot` re-teste quand même `!p || p->isDead()`, car un projectile tué pendant le tick peut encore être dans la liste au moment de la capture.

**Code mort / non utilisé dans ce fichier** — aucun : les trois membres et les trois fonctions sont utilisés.

## include/Collectable.hpp + src/Collectable.cpp

### `class Collectable`

Classe de base abstraite des objets ramassables : elle fixe l'état commun (position, vitesse, collider, texture, vivant ou non) et deux points d'extension (`update`, `onHit`). Tous ses champs d'état sont publics parce que `World` et `captureSnapshot` les manipulent directement.

| Membre de données | Type | Rôle |
|---|---|---|
| `x`, `y` | `float` (public) | Position en pixels. **Non initialisés**. |
| `vx`, `vy` | `float` (public) | Vitesse en px/s. **Non initialisés**. |
| `textureAssetId` | `uint16_t` (public) | Id compact de la texture pour le snapshot. Défaut 0. |
| `collider` | `SDL_Rect` (public) | AABB en pixels. `x`/`y` synchronisés par `setPos`/`update` ; `w`/`h` renseignés de l'extérieur (`World::step`). |
| `isAlive` | `bool` (public) | Faux = à retirer du monde (balayé par `World::step`). |
| `cTexture` | `LTexture*` (protégé) | Texture résolue par `ScoreCollectable::init`. **Jamais relue.** |

#### `void Collectable::setPos(float posX, float posY)`
- **Rôle** : positionne l'objet et synchronise immédiatement son collider.
- **Paramètres** : `posX`, `posY` — pixels.
- **Retour** : —
- **Effets de bord** : écrit `x`, `y`, `collider.x`, `collider.y`.
- **Pièges** : ne touche pas à `collider.w`/`h`, qui doivent être réglés séparément (`World::step` le fait **après** `setPos`). Troncature `float` → `int` sur le collider.

#### `virtual void Collectable::update(float deltaTime, std::vector<player::Player>* players) = 0`
- **Rôle** : point d'extension du tick d'un ramassable (déplacement + détection de ramassage).
- **Paramètres** : `deltaTime` — secondes ; `players` — pointeur vers **tous** les joueurs, pour tester les collisions.
- **Retour** : — (fonction virtuelle pure, pas de définition).
- **Pièges** : signature très couplée (le collectable itère lui-même sur les joueurs) ; `players` n'est jamais vérifié non nul par les implémentations.

#### `virtual void Collectable::onHit(player::Player& player) = 0`
- **Rôle** : point d'extension de l'effet appliqué quand un joueur touche l'objet.
- **Paramètres** : `player` — le joueur touché, modifiable.
- **Retour** : — (virtuelle pure).

**Code mort / non utilisé dans ce fichier**
- `cTexture` : affecté par `ScoreCollectable::init` mais plus jamais lu — le rendu passe par `textureAssetId` et `AssetIds`. Avec lui, l'`#include "LTexture.hpp"` devient inutile.
- `vy` : **jamais écrit** nulle part (ni ici, ni dans `ScoreCollectable`, ni dans `World`), mais **lu** à chaque tick par `ScoreCollectable::update`. Comme les membres `float` n'ont pas d'initialiseur, c'est une **lecture de valeur indéterminée** : la pizza dérive verticalement de façon imprévisible (en pratique souvent 0 car la mémoire du vecteur est fraîche, mais ce n'est pas garanti). Corriger en mettant `float x = 0.f, y = 0.f; float vx = 0.f, vy = 0.f;`.
- Pas de destructeur virtuel : détruire un `Collectable*` polymorphe serait un comportement indéfini. Inoffensif aujourd'hui car `World` stocke des `std::vector<ScoreCollectable>` par valeur, mais c'est un piège si un jour les ramassables deviennent polymorphes (ce qui est l'intérêt de la hiérarchie).
- `#include <vector>` et `#include "Utils.hpp"` sont tirés par le header sans y être nécessaires (`Utils` l'est pour `ScoreCollectable.cpp`).

## include/ScoreCollectable.hpp + src/ScoreCollectable.cpp

### `class ScoreCollectable : public Collectable`

Seule implémentation concrète de `Collectable` : un objet qui défile horizontalement et donne des points au joueur qui le touche (la « pizza » de `World`).

| Membre de données | Type | Rôle |
|---|---|---|
| `cScore` | `int` (privé) | Points accordés au ramassage. Défaut 0. |

#### `void ScoreCollectable::init(int scoreOnHit, std::string textureId)`
- **Rôle** : initialise la valeur en points et la texture.
- **Paramètres** : `scoreOnHit` — points ; `textureId` — identifiant d'asset (passé **par valeur**, copie inutile).
- **Retour** : —
- **Effets de bord** : résout `cTexture` via `TextureManager` ; **enregistre/récupère** l'id compact via `AssetIds::id()` ; écrit `cScore`.
- **Pièges** : pseudo-constructeur à appeler impérativement après `emplace_back` (ce que fait `World::step`) ; rien n'empêche d'oublier l'appel, auquel cas l'objet vaut 0 point avec une texture nulle. Ne renvoie aucune erreur si la texture n'existe pas.

#### `void ScoreCollectable::update(float deltaTime, std::vector<player::Player>* players) override`
- **Rôle** : déplace l'objet et détecte le ramassage.
- **Paramètres** : `deltaTime` — secondes ; `players` — tous les joueurs de la partie.
- **Retour** : —
- **Effets de bord** : `x += vx*dt`, `y += vy*dt` ; resynchronise `collider.x/y` ; pour chaque joueur en collision, appelle `onHit` (qui modifie le score du joueur et met `isAlive` à faux).
- **Pièges** :
  - Lit `vy`, qui n'est jamais initialisé (voir code mort de `Collectable`).
  - Déréférence `players` sans test de nullité.
  - **L'ordre des tests est inversé** : `util::collide(...)` est évalué **avant** `if(!players[i].isAlive) continue;`. Le `continue` arrive donc après le test de collision — fonctionnellement correct (un mort n'encaisse pas le `onHit`) mais trompeur à la lecture.
  - **Pas de `break` après `onHit`** et `isAlive` n'est pas retesté dans la boucle : si deux joueurs touchent la pizza le même tick, **les deux** marquent les points. Comportement probablement non voulu ; ajouter un `break` (ou tester `isAlive`) le corrigerait.
  - `util::collide` prend des `SDL_Rect&` non const, d'où les `(*players)[i].collider` et `collider` passés en lvalue.

#### `void ScoreCollectable::onHit(player::Player& player) override`
- **Rôle** : accorde les points et consomme l'objet.
- **Paramètres** : `player` — le joueur qui ramasse.
- **Retour** : —
- **Effets de bord** : `player.updateScore(...)` ; `isAlive = false` (la suppression effective est faite par le `remove_if` de `World::step`).
- **Pièges** : la skin `"skin_squirell"` (orthographe du code) double les points — règle de gameplay codée en dur en C++ avec comparaison de chaîne, alors que tout le reste du contenu est passé en Lua. Candidat évident à la migration vers un hook de mod. `isAlive = false` n'empêche pas la fin de la boucle de l'appelant (voir ci-dessus).

**Code mort / non utilisé dans ce fichier**
- `cTexture`, écrit par `init`, n'est lu par personne (hérité de `Collectable`).
- Le paramètre `textureId` pourrait être `const std::string&` ; la copie est inutile.
- Aucune fonction orpheline : `init`, `update` et `onHit` sont tous appelés depuis `World`.

## include/Ability.hpp + src/Ability.cpp

### `class Ability`

Interface des capacités du joueur : coût en points, cooldown, et un chronomètre pour mesurer l'écoulement du cooldown. L'unique implémentation est `LuaAbility` — la classe existe pour que `Player` ne dépende pas de sol2.

| Membre de données | Type | Rôle |
|---|---|---|
| `cooldown` | `int` (protégé) | Délai de recharge en **millisecondes**. **Non initialisé** par la classe de base. |
| `cost` | `int` (protégé) | Coût en points de score. **Non initialisé** par la classe de base. |
| `timeSinceLast` | `LTimer` (protégé) | Chronomètre redémarré à chaque usage réussi. |

#### `virtual Ability::~Ability() = default`
- **Rôle** : destructeur virtuel, indispensable puisque `Player` détient un `std::unique_ptr<Ability>` qui détruit en réalité un `LuaAbility`.

#### `virtual void Ability::use(player::Player* player) = 0`
- **Rôle** : déclenche la capacité.
- **Paramètres** : `player` — le lanceur, pointeur brut non possédé.
- **Retour** : — (virtuelle pure ; rien n'indique à l'appelant si l'usage a été accepté).
- **Pièges** : appelé **à chaque tick** tant que la touche est tenue (`Player::applyInput`), c'est donc à l'implémentation de gérer cooldown et coût. Le pointeur n'est jamais vérifié non nul.

#### `virtual void Ability::update(float deltaTime) {}`
- **Rôle** : hook de mise à jour par tick, pour les capacités à effet continu.
- **Paramètres** : `deltaTime` — secondes.
- **Retour** : —
- **Effets de bord** : aucun dans l'implémentation par défaut (corps vide).
- **Pièges** : appelé par `Player::update` **avant** la sortie anticipée `isControlled`, donc une ability continue tourne même pendant qu'un script contrôle le joueur. `LuaAbility` ne le redéfinit pas : il n'existe aujourd'hui aucun hook `onUpdate` pour les abilities (seulement pour les projectiles).

#### `float Ability::getCooldownProgress()`
- **Rôle** : fraction de cooldown écoulée, pour le camembert de progression du HUD.
- **Paramètres** : aucun.
- **Retour** : `[0, 1]`, 1 = prête. Pas de cas d'échec signalé.
- **Effets de bord** : aucun (mais `LTimer::getTicks()` n'est pas `const`, d'où une méthode non `const`).
- **Pièges** : si `cooldown == 0`, la division flottante par zéro donne `+inf` (ou `NaN` si `getTicks()` vaut 0), clampé ensuite à 1 pour `+inf` mais **pas** pour `NaN` — `NaN > 1.f` est faux, donc `NaN` est renvoyé tel quel et finit converti en `uint8_t` dans le snapshot (comportement indéfini). Une ability Lua sans champ `cooldown` déclenche exactement ce cas. Par ailleurs `cooldown` et `cost` n'ont pas d'initialiseur dans la classe de base : une future sous-classe qui oublie de les régler lira des valeurs indéterminées. Corriger en `int cooldown = 0; int cost = 0;` et en traitant `cooldown <= 0` comme « toujours prête ».

**Code mort / non utilisé dans ce fichier**
- `#include "AudioManager.hpp"` : plus aucun usage depuis que les sons passent par `EventQueue` et Lua.
- `Ability::update` : redéfini par personne (`LuaAbility` ne l'implémente pas) ; le corps par défaut vide est donc le seul comportement existant, et `Player::update` appelle une fonction qui ne fait rien à chaque tick.

## include/LuaAbility.hpp + src/LuaAbility.cpp

### `class LuaAbility : public Ability`

Pont entre une ability déclarée en Lua (`registerAbility{...}`) et l'interface C++ `Ability`. Elle détient la table d'**instance** du joueur (dont le `__index` pointe vers la définition partagée), applique les règles de cooldown et de coût, puis délègue l'effet au hook `onUse`.

| Membre de données | Type | Rôle |
|---|---|---|
| `instance` | `sol::table` | Table Lua propre à ce joueur ; sert de `self` au hook et de stockage d'état persistant entre deux usages. |
| `onUse` | `sol::protected_function` | Hook Lua de déclenchement. |
| `ctx` | `GameContext*` | Contexte passé en 3ᵉ argument du hook. |

#### `LuaAbility::LuaAbility(sol::table instance, GameContext* ctx)`
- **Rôle** : construit le pont, lit le coût et le cooldown depuis la table, et arme le chronomètre.
- **Paramètres** :
  - `instance` — table d'instance créée par `ScriptEngine::createAbility` (son `__index` est la définition enregistrée).
  - `ctx` — contexte de jeu, stocké tel quel (peut être nul, jamais vérifié).
- **Retour** : —
- **Effets de bord** : `onUse = instance["onUse"]` (résolu via le `__index`, donc trouvé dans la définition) ; `cost = instance.get_or("cost", 0)` (points) ; `cooldown = (int)(instance.get_or("cooldown", 0.0) * 1000.0)` — **conversion secondes → millisecondes** ; démarre `timeSinceLast`.
- **Pièges** : le cooldown est exprimé en **secondes** côté Lua et en millisecondes côté C++ ; un `cooldown = 500` dans un mod (pensé en ms) donnerait 500 secondes. `timeSinceLast.start()` au moment de la construction signifie que l'ability est utilisable dès que `cooldown` ms se sont écoulées **depuis la création du joueur**, pas dès le début du match (ce qui est pratiquement équivalent ici). Aucune validation que `onUse` est appelable (c'est `registerAbility` qui l'a vérifié en amont).

#### `void LuaAbility::use(player::Player* player) override`
- **Rôle** : applique les gardes (cooldown, solvabilité), appelle le hook Lua, et ne débite le coût que si le script confirme l'usage.
- **Paramètres** : `player` — le lanceur, transmis à Lua sous forme d'usertype `Player`.
- **Retour** : — (aucun retour : l'appelant ne sait pas si l'usage a été accepté).
- **Effets de bord** : appelle `onUse(instance, player, ctx)` ; en cas d'erreur Lua, imprime `"Ability error : ..."` et sort sans rien débiter ; si le hook renvoie une valeur vraie, `player->updateScore(-cost)` puis `timeSinceLast.start()`. Les effets de gameplay (dégâts, projectiles, sons, secousses) sont faits **par le script**, via les bindings de `GameContext`.
- **Pièges** :
  - Le test de cooldown est `timeSinceLast.getTicks() <= cooldown` : il faut **dépasser** strictement le cooldown.
  - La solvabilité est `getScore() < cost`, donc un score exactement égal au coût passe et tombe à 0.
  - Le **protocole de retour** est essentiel : `onUse` doit renvoyer `true` pour que le cooldown soit armé et le score débité. Un script qui oublie son `return true` reste utilisable à chaque frame **et gratuit** ; c'est le mécanisme qui permet à une ability d'annuler son propre déclenchement (pas de cible, etc.).
  - `sol::optional<bool> used = result;` ne convertit que si le premier retour est un **booléen** : renvoyer `1` (nombre) côté Lua est considéré comme un échec. Il faut `return true` littéralement.
  - `player` n'est pas vérifié non nul.
  - L'erreur Lua ne désarme rien : un script cassé sera réappelé à chaque frame, inondant la console de `printf`.

**Code mort / non utilisé dans ce fichier**
- `Ability::update` n'est pas redéfini : il n'y a pas de hook `onUpdate` pour les abilities (contrairement aux projectiles).
- `#include <sol/optional_implementation.hpp>` et `#include <sol/error.hpp>` sont nécessaires ; rien d'orphelin.

## include/ScriptEngine.hpp + src/ScriptEngine.cpp

### `class ScriptEngine`

Singleton qui possède l'état Lua, installe l'API exposée aux mods (`registerBindings`), charge les mods depuis le disque, et fabrique les `Ability` à partir des définitions Lua. C'est la frontière unique entre le C++ et sol2 : aucun autre fichier ne manipule `sol::state`.

| Membre de données | Type | Rôle |
|---|---|---|
| `lua` | `std::unique_ptr<sol::state>` (privé) | L'état Lua, créé par `init()`. Nul avant. Le `unique_ptr` permet de garder `sol/forward.hpp` dans le header au lieu de tout sol2. |

Deux tables globales Lua servent de registres : `_abilities` (map `id` → définition) et `_hats` (tableau séquentiel de définitions de chapeaux).

#### `static ScriptEngine& ScriptEngine::getInstance()` (inline, header)
- **Rôle** : accès au singleton.
- **Retour** : référence sur l'instance locale statique (construite au premier appel, thread-safe depuis C++11).
- **Effets de bord** : construit l'instance au premier appel (le constructeur est `= default`, il **n'appelle pas** `init()`).
- **Pièges** : la destruction a lieu en fin de programme, dans un ordre non garanti par rapport aux autres singletons (`TextureManager`, `AssetIds`) ; détruire l'état Lua après `TextureManager::clean()` est sans danger ici car Lua ne possède aucune texture.

#### `ScriptEngine(const ScriptEngine&) = delete` / `ScriptEngine& operator=(const ScriptEngine&) = delete` (header)
- **Rôle** : interdire la copie du singleton.

#### `ScriptEngine::ScriptEngine() = default` / `ScriptEngine::~ScriptEngine() = default`
- **Rôle** : constructeur et destructeur privés, définis dans le .cpp.
- **Pièges** : ils **doivent** être définis dans le .cpp (et non `= default` dans le header) parce que le header ne connaît `sol::state` que par déclaration anticipée : `unique_ptr` a besoin du type complet pour générer son destructeur. C'est le motif « pimpl ».

#### `void ScriptEngine::init()`
- **Rôle** : crée l'état Lua, ouvre un sous-ensemble contrôlé de la bibliothèque standard, prépare les registres et installe les bindings.
- **Paramètres** : aucun. **Retour** : —
- **Effets de bord** : alloue `sol::state` ; ouvre `base`, `math`, `string`, `table`, `package` ; crée les tables globales `_abilities` et `_hats` ; met `package.cpath` à `""` et `package.loadlib` à `nil` ; appelle `registerBindings()`.
- **Pièges** : **bac à sable partiel** : `io`, `os`, `debug` et `coroutine` ne sont pas ouverts, et le chargement de bibliothèques natives est neutralisé (`cpath` vidé, `loadlib` retiré) — un mod ne peut donc pas toucher au système de fichiers ni charger de `.so`. En revanche `base` laisse passer `load`/`loadstring` et `dofile`, donc l'isolement n'est pas total. Appeler `init()` deux fois **détruit** l'état précédent et donc tous les mods chargés. `init()` doit être appelé avant tout le reste (`main` le fait juste après `TextureManager::init`, ce qui est nécessaire puisque `loadTexture` a besoin du renderer).

#### `static std::vector<player::Player*> findPlayersInRadius(GameContext& ctx, float x, float y, float radius, const std::vector<player::Player*>& ignore)`
- **Rôle** : fonction d'aide **statique au fichier** (invisible ailleurs) qui collecte les joueurs vivants dont le **centre du collider** est dans un disque. Base commune de `playersInRadius` et `explode`.
- **Paramètres** : `ctx` — contexte (pour accéder à la liste des joueurs) ; `x`, `y` — centre du disque en pixels ; `radius` — rayon en pixels ; `ignore` — joueurs à exclure (comparaison par adresse).
- **Retour** : vecteur de pointeurs vers les joueurs touchés ; **vide** si `ctx.players` est nul.
- **Effets de bord** : aucun (allocation du vecteur résultat).
- **Pièges** : compare les distances au carré (`dSquare <= radius²`) pour éviter la racine. Filtre les morts (`!p.isAlive`). Le test porte sur le **centre** du collider, pas sur un recouvrement de boîtes : un gros joueur dont seul le bord est dans le rayon n'est pas touché. Les pointeurs renvoyés sont invalidés par toute réallocation du vecteur de joueurs. Le filtrage `ignore` est un `std::find` linéaire — négligeable à 4 joueurs.

#### `void ScriptEngine::registerBindings()`
- **Rôle** : installe toute l'API Lua : deux fonctions globales de contenu, une fonction globale de ressources, et trois usertypes (`Player`, `GameContext`, `Projectile`).
- **Paramètres** : aucun. **Retour** : —
- **Effets de bord** : écrit dans les globales Lua ; les lambdas capturant `this` gardent une référence sur le singleton.
- **Pièges** : appelée uniquement par `init()`, après l'ouverture des bibliothèques. Deux lambdas capturent `this` pour accéder à `(*lua)` : elles deviendraient pendouillantes si l'état Lua était recréé par un second `init()`.

---

#### Bindings Lua — fonctions globales

##### `loadTexture(id, path) -> boolean`
- **Côté Lua** : `loadTexture("pizza", "assets/collectables/pizza.png")`.
- **Côté C++** : `set_function("loadTexture", ...)` → `TextureManager::getInstance().loadTexture(id, path)`.
- **Validations** : aucune dans le binding ; c'est `TextureManager` qui vérifie le chargement.
- **Valeur de repli** : renvoie `false` si le chargement échoue ; aucune exception, aucun message depuis le binding.
- **Pièges** : nécessite que `TextureManager::init(renderer)` ait eu lieu (c'est le cas dans `main`). Le chemin est relatif au répertoire de travail, pas à `MOD_DIR` : les mods doivent composer eux-mêmes leurs chemins.

##### `registerHat(def)`
- **Côté Lua** : `registerHat{ id = "hat_witch", texture = "assets/hats/witch.png", ability = "freeze" }`. Pas de retour.
- **Côté C++** : lit `def.id` et `def.texture`, charge la texture **sous l'identifiant du chapeau** (`loadTexture(*id, *texture)`), puis ajoute la table à la fin du tableau global `_hats` (`hats.add(def)`).
- **Validations** : `id` absent → `"No id, hat ignored"` et abandon ; `texture` absent → `"registerHat <id> : no texture, ignored"` et abandon ; échec du chargement de la texture → `"registerHat <id> : texture cant be loaded : <path>"` et abandon. Le champ `ability` n'est **pas** validé ici (une ability inexistante ne sera détectée qu'à la création du joueur).
- **Valeur de repli** : rien n'est enregistré en cas d'échec ; le chapeau est simplement absent du jeu.
- **Effets de bord** : charge une texture ; empile dans `_hats` ; imprime `"[lua] hat saved : <id>"`.
- **Pièges** : contrairement à `registerAbility`, **aucune détection de doublon** : deux chapeaux de même `id` coexistent dans `_hats` et c'est le premier trouvé qui gagne dans `createAbilityForHat`. L'identifiant du chapeau sert aussi d'identifiant de texture, ce qui le fait collisionner avec les textures chargées en masse par `TextureManager::loadDirectory("assets/hats/", "hat_")` dans `Game::loadMedia` — convention implicite à connaître. `_hats` étant un tableau et non une map, la recherche est linéaire.

##### `registerAbility(def)`
- **Côté Lua** : `registerAbility{ id = "missile", cost = 50, cooldown = 1.5, onUse = function(self, player, ctx) ... return true end }`. Pas de retour.
- **Côté C++** : lit `def.id` et `def.onUse`, puis stocke la table dans `_abilities[id]`.
- **Validations** : `id` absent → `"registerAbility : no id, ability ignored"` ; `onUse` absent ou non convertible en fonction → `"regiterAbility : <id> : onUse not found, ability ignored"` (coquille dans le message) ; si `_abilities[id]` existe déjà → `"registerAbility : <id> was already declared, replaced"` puis **écrasement** (c'est le mécanisme qui permet à un mod de surcharger une ability de `base`).
- **Valeur de repli** : l'ability n'est pas enregistrée ; `createAbility` échouera plus tard avec `"createAbility : unknown ability"`.
- **Effets de bord** : écrit dans `_abilities` ; imprime `"[lua] ability saved : <id>"`.
- **Pièges** : `cost` et `cooldown` ne sont **pas** validés ici — ils sont lus par `LuaAbility` avec des défauts 0, et un `cooldown` absent mène au cas division-par-zéro de `getCooldownProgress`. L'ordre de chargement des mods détermine qui écrase qui (voir `loadMods`).

---

#### Bindings Lua — usertype `Player` (`sol::no_constructor`)

Non constructible depuis Lua : les scripts ne reçoivent que des joueurs existants, via `ctx:players()`, `ctx:playersInRadius()`, `ctx:explode()` ou l'argument `player` de `onUse`. Les instances sont des **pointeurs bruts vers des éléments du `std::vector<Player>`** : les conserver d'un tick sur l'autre dans `projectile.data` n'est sûr que parce que le vecteur ne se réalloue pas en cours de partie.

| Nom Lua | Signature Lua | C++ | Notes |
|---|---|---|---|
| `setVelocity` | `p:setVelocity(vx, vy)` | `&Player::setVelocity` | Remplace la vitesse (px/s). Ignore `isControlled`. |
| `teleport` | `p:teleport(x, y)` | `&Player::teleportTo` | Déplace le coin haut-gauche (pixels) **sans** mettre à jour le collider. |
| `applyKnockBack` | `p:applyKnockBack(fx, fy)` | `&Player::applyKnockBack` | Ajoute une vitesse (px/s), pas une force. |
| `getLife` | `p:getLife() -> integer` | `&Player::getLife` | PV courants, négatif possible. |
| `getMaxLife` | `p:getMaxLife() -> integer` | `&Player::getMaxLife` | PV max de la config. |
| `getScore` | `p:getScore() -> integer` | `&Player::getScore` | Score = monnaie des abilities. |
| `addScore` | `p:addScore(amount)` | `&Player::updateScore` | Signé, aucun plancher. |
| `isAlive` | `p.isAlive -> boolean` | `sol::readonly(&Player::isAlive)` | **Propriété en lecture seule** : une tentative d'écriture lève une erreur Lua. |
| `isControlled` | `p.isControlled` (lecture **et** écriture) | `&Player::isControlled` | Propriété modifiable : un script peut prendre et rendre le contrôle (`christmas.lua`). Suspend physique, dégâts et collisions ; à remettre impérativement à `false`, sinon le joueur reste figé et invulnérable à vie. |
| `damage` | `p:damage(amount)` | lambda `self.updateLife(-amount)` | `amount` positif = dégâts. No-op si `isControlled`. Le paramètre `source` documenté dans le meta n'existe **pas** côté C++. |
| `heal` | `p:heal(amount)` | lambda `self.updateLife(amount)` | Subit aussi la réduction de 25 % de `skin_turtle` (bug de `updateLife`). |
| `getPosition` | `p:getPosition() -> x, y` | lambda | Renvoie **le collider** converti en `float`, pas `x`/`y` : valeur du tick précédent, et figée si `isControlled`. |
| `getCenter` | `p:getCenter() -> cx, cy` | lambda | Centre du collider en pixels. C'est ce que visent les projectiles guidés. |
| `getSize` | `p:getSize() -> w, h` | lambda | Taille du collider en pixels (entiers). |

- **Non exposé** : `getX`/`getY`/`getVx`/`getVy` (le meta Lua documente un `Player:getVelocity()` qui **n'est pas bindé**), `getSkinId`, `isThrusting`, `getAbilityProgress`, `resolveCollisionWith`, `getKeyPreset`, `getJoystickId`.

---

#### Bindings Lua — usertype `GameContext` (`sol::no_constructor`)

Toutes les entrées sont des lambdas, donc des **méthodes** à appeler avec `:` (`ctx:explode{...}`). Le `GameContext` n'est valide que pendant un match ; chaque accès teste la nullité du pointeur correspondant et retombe sur une valeur neutre.

##### `ctx:players() -> Player[]`
- **C++** : construit un `std::vector<Player*>` à partir de `*self.players` et le renvoie en `sol::as_table` (table séquentielle Lua).
- **Validations** : `self.players` nul → table **vide**.
- **Pièges** : inclut **les morts** ; les scripts doivent filtrer sur `p.isAlive` (ce que fait `missile.lua`). Reconstruit un vecteur et une table Lua à chaque appel — à ne pas mettre dans une boucle serrée.

##### `ctx:playersInRadius(x, y, radius, ignore?) -> Player[]`
- **C++** : `findPlayersInRadius(self, x, y, radius, ignore.value_or({}))`, renvoyé en `sol::as_table`.
- **Paramètres** : `x`, `y`, `radius` en pixels ; `ignore` est une **table de `Player`** (optionnelle).
- **Validations** : `ctx.players` nul → table vide ; joueurs morts exclus ; le disque est testé sur le **centre** du collider.
- **Valeur de repli** : `ignore` absent → vecteur vide. **Piège majeur** : passer un `Player` seul au lieu d'une table (`ignore = player` au lieu de `{player}`) échoue la conversion `sol::optional<std::vector<Player*>>` et est **silencieusement** traité comme « rien à ignorer » — le lanceur se touche lui-même. C'est documenté dans `mods/base/meta/akaka.lua`.

##### `ctx:explode(params) -> Player[]`
- **Côté Lua** : `ctx:explode{ x = ..., y = ..., radius = ..., damage = 40, force = 1000, ignore = { player } }`.
- **C++** : trouve les joueurs dans le rayon, puis pour chacun calcule une atténuation `factor = 1 - distSq/radius²`, applique `updateLife(-(int)(damage*factor))` et `applyKnockBack(dirX*force*factor, dirY*force*factor)` où `(dirX, dirY)` est le vecteur unitaire du centre de l'explosion vers le centre du joueur.
- **Paramètres** : `x`, `y` (pixels, **requis**), `radius` (pixels, **requis**), `damage` (PV, défaut 0), `force` (px/s, défaut 0), `ignore` (table de `Player`, optionnel).
- **Retour** : table des joueurs touchés (y compris si `damage` et `force` valent 0, ce qui permet de s'en servir comme simple requête).
- **Validations** : si `x`, `y` ou `radius` manque → `"[lua] explode : x, y and radius are required"` et retour d'une table **vide** sans rien appliquer.
- **Effets de bord** : modifie PV et vitesses des joueurs touchés. **N'émet aucun son ni effet visuel** : le script doit appeler lui-même `playSFX`, `spawnEffect` et `shakeScreen`.
- **Pièges** : l'atténuation est **quadratique** (`1 - d²/R²`), pas linéaire — les dégâts chutent très vite en s'éloignant du centre. `damage` est un `float` côté params mais tronqué vers l'entier après multiplication : un petit `damage*factor` (< 1) donne 0 PV. Un joueur pile au centre (`dist == 0`) reçoit un knockback vers le **haut** (`dirX = 0, dirY = -1`) : fallback arbitraire mais non documenté. `updateLife` est no-op sur un joueur `isControlled`, mais `applyKnockBack` s'y applique quand même. Les joueurs morts sont exclus par `findPlayersInRadius`.

##### `ctx:playSFX(id) -> integer`
- **C++** : `self.events->sfx(id)` — **empile** un `GameEvent{Sfx}` dans l'`EventQueue` au lieu de jouer le son. L'id texte est converti en `uint16_t` par `AssetIds`.
- **Retour** : un *handle* (≥ 1) à conserver pour pouvoir arrêter le son.
- **Validations / repli** : si `self.events` est nul, renvoie **0** — valeur sentinelle qui n'est jamais un handle valide (`mNextHandle` commence à 1).
- **Pièges** : le son n'est joué qu'au drain côté client (`Game::drainEvents`), donc un tick plus tard. C'est **le** mécanisme qui rend les effets rejouables sur un client distant. L'`id` doit correspondre à un son chargé par `AudioManager::loadSFX` (fait en dur dans `Game::loadMedia`) : un id inconnu crée quand même une entrée dans `AssetIds` et un événement, mais ne produira aucun son.

##### `ctx:stopSFX(handle)`
- **C++** : `self.events->stopSfx(handle)` → empile un `GameEvent{StopSfx}`. Pas de retour.
- **Validations / repli** : no-op silencieux si `self.events` est nul. Aucune validation du handle.
- **Pièges** : le client retrouve le canal via sa map `mSfxChannels` ; un handle inconnu (0, ou déjà terminé) est ignoré sans erreur. Utilisé par les mods pour couper une boucle (ex. le bruit de propulsion d'un missile) à l'explosion.

##### `ctx:shakeScreen(intensity, duration)`
- **C++** : `self.events->shake(intensity, duration)` → empile un `GameEvent{Shake}` (`a = intensity`, `b = duration`).
- **Paramètres** : `intensity` — amplitude en pixels ; `duration` — secondes.
- **Validations / repli** : no-op si `self.events` est nul. Aucune borne sur l'intensité.
- **Pièges** : effet purement client (`EffectManager::triggerShake`) ; plusieurs secousses dans le même tick ne se cumulent pas forcément — c'est l'`EffectManager` qui tranche.

##### `ctx:spawnEffect(animId, x, y, scale?)`
- **C++** : `self.events->effect(animId, x, y, scale.value_or(1.f))` → empile un `GameEvent{Effect}`.
- **Paramètres** : `animId` — nom d'animation enregistrée dans `AnimationManager` ; `x`, `y` — pixels ; `scale` — facteur d'échelle, **défaut 1.0**.
- **Validations / repli** : no-op si `self.events` est nul ; l'existence de l'animation n'est **pas** vérifiée (le client ignorera un nom inconnu).
- **Pièges** : la seule animation enregistrée à ce jour est `"explosion_missile"` (en dur dans `Game::loadMedia`) — il n'existe pas encore de `registerAnimation` côté Lua, c'est l'asymétrie qui reste dans le pipeline de contenu.

##### `ctx:spawnProjectile(params) -> Projectile|nil`
- **Côté Lua** : `local p = ctx:spawnProjectile{ texture = "missile", x = ..., y = ..., vx = ..., vy = ..., width = 32, height = 32, onUpdate = function(self, ctx, dt) ... end }`.
- **C++** : construit `std::make_shared<projectile::Projectile>(params, &self)` et le confie à `self.projectiles->spawn(p)` ; renvoie le projectile à Lua pour que le script puisse finir de le configurer (typiquement `p.data.owner = player`).
- **Paramètres requis** : `texture`, `x`, `y`. Autres clés lues par le constructeur de `Projectile` : `width`, `height` (défaut = taille de la texture), `vx`, `vy` (défaut 0), `onUpdate`.
- **Validations** : `self.projectiles` nul → renvoie `nil` sans message ; `texture`, `x` ou `y` manquant → `"[lua] spawnProjectile : texture, x and y are required"` et `nil` ; texture inconnue de `TextureManager` → `"[lua] spawnProjectile : unknown texture <nom>"` **mais la création continue** (avertissement seulement, projectile invisible de taille 0).
- **Valeur de repli** : `nil` — le script **doit** le tester s'il veut écrire dans `p.data`, sinon erreur Lua « index a nil value ».
- **Effets de bord** : allocation `shared_ptr` ; attribution d'un `netId` ; entrée dans la file `pending` du manager (le projectile n'est donc actif qu'au tick suivant) ; création d'une table Lua `data` vide.
- **Pièges** : le pointeur `&self` (le `GameContext`) est stocké dans le projectile pour toute sa vie : il doit survivre au projectile (c'est le cas, il appartient à `World`). Renvoyer le `shared_ptr` à Lua signifie que **Lua co-détient** le projectile : garder la référence dans une variable Lua empêche sa destruction même après `kill()` et retrait du manager.

##### `ctx:worldSpeed() -> number`
- **C++** : `*self.globalSpeed` — vitesse de défilement du monde en px/s.
- **Validations / repli** : `0.f` si le pointeur est nul.

##### `ctx:screenWidth() -> integer`
- **C++** : `*self.screenWidth` — largeur logique du rendu en pixels (1024 d'après `main`).
- **Validations / repli** : `0` si nul.

##### `ctx:screenHeight() -> integer`
- **C++** : `*self.screenHeight` — hauteur logique totale en pixels (576).
- **Validations / repli** : `0` si nul.

##### `ctx:effectiveHeight() -> integer`
- **C++** : `*self.effectiveHeight` — hauteur jouable, soit `screenHeight - 50` (le bandeau de HUD en bas).
- **Validations / repli** : `0` si nul.
- **Pièges** : c'est `effectiveHeight` que la physique du joueur utilise comme mur bas (`cfg.screenHeight = effectiveHeight` dans `Game::init`), alors que `Projectile::isOffScreen` utilise `screenHeight`. Les scripts doivent choisir le bon selon qu'ils raisonnent en zone de jeu ou en zone d'écran.

---

#### Bindings Lua — usertype `Projectile` (`sol::no_constructor`)

Non constructible depuis Lua : on passe toujours par `ctx:spawnProjectile`. L'instance arrive aux scripts comme premier argument (`self`) du hook `onUpdate`.

| Nom Lua | Signature Lua | C++ | Notes |
|---|---|---|---|
| `isValid` | `p:isValid() -> boolean` | `&Projectile::isValid` | Faux dès que `kill()` a été appelé, même si l'objet existe encore ce tick. À tester avant d'agir sur un projectile mémorisé. |
| `setVelocity` | `p:setVelocity(vx, vy)` | `&Projectile::setVelocity` | px/s. |
| `setAngle` | `p:setAngle(degrees)` | `&Projectile::setAngle` | Degrés, purement visuel. |
| `setPosition` | `p:setPosition(x, y)` | `&Projectile::setPosition` | Pixels, coin haut-gauche. |
| `getPosition` | `p:getPosition() -> x, y` | `&Projectile::getPosition` | Tuple → deux valeurs de retour. |
| `getVelocity` | `p:getVelocity() -> vx, vy` | `&Projectile::getVelocity` | px/s. |
| `getSize` | `p:getSize() -> w, h` | `&Projectile::getSize` | Pixels entiers. |
| `kill` | `p:kill()` | `&Projectile::kill` | Marque pour suppression ; interrompt l'intégration du tick courant (`update` re-teste `isAlive`). |
| `data` | `p.data` (**propriété**, sans parenthèses) | `sol::property(&Projectile::getData)` | Table Lua libre, vide à la création. Lecture seule au sens où on ne peut pas **remplacer** la table (`p.data = {}` échoue faute de *setter*), mais on peut écrire dedans (`p.data.owner = player`). |
| `isOffScreen` | `p:isOffScreen(margin?) -> boolean` | lambda → `self.isOffScreen(margin.value_or(0.f))` | `margin` en pixels, **défaut 0**. Utilise `screenWidth`/`screenHeight`. |

- **Non exposé** : `getNetId`/`setNetId` (réservé au réseau), `getTextureAssetId`, `getAngle`, `getCollider`, `isDead` (doublon de `isValid`), `update`.

---

#### `bool ScriptEngine::runFile(const std::string& path)`
- **Rôle** : exécute un fichier Lua en mode protégé.
- **Paramètres** : `path` — chemin du script, relatif au répertoire de travail.
- **Retour** : `true` si le script s'est exécuté sans erreur ; `false` si `init()` n'a pas été appelé ou si le script a échoué (erreur de syntaxe comme d'exécution).
- **Effets de bord** : tout ce que fait le script (typiquement `loadTexture`, `registerHat`, `registerAbility`) ; imprime `"Error, init was not called..."` ou `"Error Script Engine : <path> : <message>"`.
- **Pièges** : `sol::script_pass_on_error` évite la levée d'exception — les erreurs sont transformées en valeur de retour, ce qui permet à `loadMods` de continuer avec les autres mods. Un fichier manquant est un échec comme un autre (pas de distinction entre « absent » et « cassé »).

#### `void ScriptEngine::loadMods(const std::string& modsDir)`
- **Rôle** : découvre les mods (un sous-répertoire = un mod), les trie en garantissant que `base` passe en premier, et exécute le `init.lua` de chacun.
- **Paramètres** : `modsDir` — répertoire racine des mods (`"mods"` depuis `main`).
- **Retour** : —
- **Effets de bord** : écrit `package.path` = `"<modsDir>/?.lua;<modsDir>/?/init.lua"` (ce qui permet aux mods de faire `require("base.projectiles.missile")`) ; pour chaque mod, définit les globales **`MOD_DIR`** (chemin avec `/` final) et **`MOD_NAME`** avant d'exécuter son `init.lua` ; imprime `"Mods dir doesn't exist"`, `"[lua] mod <nom> : no init.lua, ignored"` ou `"[lua] mod loaded : <nom>"`.
- **Pièges** :
  - **Déréférence `lua` sans test de nullité** (`(*lua)["package"]...`) : appeler `loadMods` avant `init()` est un crash immédiat, alors que `runFile` protège ce cas. Incohérence à corriger.
  - `package.path` est écrit **avant** le test d'existence du répertoire : effet de bord conservé même si le chargement est abandonné.
  - Le comparateur de tri force `base` en tête et range le reste par ordre alphabétique : c'est l'**ordre de surcharge** (un mod dont le nom suit `base` peut écraser une ability de `base` via `registerAbility`). Il n'y a donc pas de système de dépendances explicite, seulement cette convention de nommage.
  - Les fichiers (non-répertoires) de `mods/` sont ignorés, de même que les sous-répertoires sans `init.lua`.
  - `MOD_DIR`/`MOD_NAME` sont des **globales mutables** écrasées à chaque mod : un script qui les lit en différé (dans un `onUse` par exemple) verra la valeur du dernier mod chargé, pas la sienne. Il faut les capturer dans une locale au moment du chargement.
  - Aucune erreur fatale : un mod cassé est signalé et sauté.

#### `std::unique_ptr<Ability> ScriptEngine::createAbility(const std::string& id, GameContext* ctx)`
- **Rôle** : instancie une ability Lua enregistrée, pour un joueur donné.
- **Paramètres** : `id` — identifiant passé à `registerAbility` ; `ctx` — contexte du match, transmis au hook.
- **Retour** : un `LuaAbility` enveloppé dans un `unique_ptr`, ou **`nullptr`** si `init()` n'a pas été appelé ou si l'id est inconnu.
- **Effets de bord** : crée **deux tables Lua** : une table d'instance vide, et une table-métatable `{__index = def}` posée sur l'instance ; imprime `"createAbility : unknown ability : <id>"` en cas d'échec.
- **Pièges** : le motif `__index` est le cœur du design : la **définition** est partagée (fonctions, `cost`, `cooldown`), l'**instance** est propre au joueur, et toute écriture depuis le script (`self.foo = ...` dans `onUse`) va dans l'instance sans polluer la définition ni les autres joueurs. Les lectures, elles, remontent à la définition. Conséquence : modifier `self.cost` dans un script ne change rien au `cost` déjà lu en C++ par le constructeur de `LuaAbility` (il est capturé une fois pour toutes). Le `nullptr` renvoyé n'est pas géré par `Player::getAbilityProgress`, qui plantera (voir plus haut).

#### `std::unique_ptr<Ability> ScriptEngine::createAbilityForHat(const std::string& hatId, GameContext* ctx)`
- **Rôle** : résout la chaîne chapeau → ability, puis délègue à `createAbility`. C'est le seul point d'entrée utilisé par le jeu (`Game::init`).
- **Paramètres** : `hatId` — identifiant du chapeau choisi dans le menu ; `ctx` — contexte du match.
- **Retour** : l'ability du chapeau, ou **`nullptr`** si : `init()` n'a pas été appelé, le chapeau est introuvable dans `_hats`, le chapeau n'a pas de champ `ability`, ou l'ability référencée est inconnue.
- **Effets de bord** : ceux de `createAbility`.
- **Pièges** : parcours **linéaire** de `_hats` de l'indice 1 à `hats.size()` (convention Lua, base 1) avec comparaison de chaînes — acceptable à quelques dizaines de chapeaux. Le premier chapeau au bon id gagne (pas de détection de doublon à l'enregistrement). Les trois modes d'échec sont **silencieux** sauf le dernier (`createAbility` imprime) : un chapeau sans `ability` renvoie `nullptr` sans un mot, et c'est précisément le cas qui fait planter `Player::getAbilityProgress`. Ajouter un message et un garde côté `Player` est la correction minimale.

**Code mort / non utilisé dans ce fichier**
- `#include "Projectile.hpp"` apparaît **deux fois** (lignes 6 et 9).
- `#include "AudioManager.hpp"` et `#include "EffectManager.hpp"` : aucun symbole de ces en-têtes n'est utilisé depuis que les sons et les effets passent par `EventQueue`.
- `ScriptEngine::createAbility` n'a **aucun appelant externe** : seul `createAbilityForHat` l'utilise. Elle reste publique, utile si les abilities sont un jour attribuées autrement que par le chapeau.
- `GameContext::particleManager` est renseigné par `Game::init` mais **n'est exposé par aucun binding** : les scripts ne peuvent pas créer de particules (seules les animations via `spawnEffect`).
- Le meta Lua `mods/base/meta/akaka.lua` déclare plusieurs fonctions **non bindées** (marquées `[TODO]`) : `Player:getVelocity`, `Player:getIndex`, `Player:getHatId`, `Player:addStatus`, `Player:hasStatus`, `GameContext:nearestPlayer`, et un 2ᵉ paramètre `source` à `Player:damage`. Les appeler depuis un mod produit une erreur Lua à l'exécution malgré l'autocomplétion.

## include/Utils.hpp + src/Utils.cpp

### `namespace util`

Fonctions libres de géométrie 2D (collisions AABB, distances) et deux primitives de dessin. Pas d'état, pas de classe.

Aucun membre de données.

#### `bool util::collide(SDL_Rect& a, SDL_Rect& b)`
- **Rôle** : test de recouvrement entre deux rectangles alignés sur les axes.
- **Paramètres** : `a`, `b` — rectangles en pixels. Pris par **référence non const** uniquement parce que `SDL_IntersectRect` attend des pointeurs non const ; ils ne sont pas modifiés.
- **Retour** : `true` si les rectangles se recouvrent.
- **Effets de bord** : aucun (le rectangle `intersection` est local et jeté).
- **Pièges** : la référence non const empêche de passer un temporaire ou un rectangle `const` — d'où les variables locales chez les appelants. Un rectangle de largeur ou hauteur 0 ou négative ne collide jamais (`SDL_IntersectRect` renvoie `SDL_FALSE`) : un collectable dont `collider.w/h` n'a pas été renseigné est intouchable. Calculer l'intersection pour la jeter est un petit gaspillage ; `SDL_HasIntersection` serait plus direct.

#### `bool util::isWithinDistance(SDL_Rect& a, SDL_Rect& b, float threshold)`
- **Rôle** : teste si les **centres** de deux rectangles sont à moins de `threshold` l'un de l'autre.
- **Paramètres** : `a`, `b` — rectangles en pixels ; `threshold` — distance en pixels.
- **Retour** : `true` si `distSq(a,b) < threshold²`.
- **Effets de bord** : aucun.
- **Pièges** : comparaison stricte (`<`), donc la distance exactement égale au seuil est hors de portée. Un `threshold` négatif renvoie faux quoi qu'il arrive (le carré le rend positif mais `distSq ≥ 0`). **Aucun appelant** (voir code mort) : la logique de portée est aujourd'hui faite en Lua.

#### `float util::distSq(SDL_Rect& a, SDL_Rect& b)`
- **Rôle** : distance au carré entre les centres de deux rectangles (évite la racine carrée).
- **Paramètres** : `a`, `b` — rectangles en pixels.
- **Retour** : pixels².
- **Effets de bord** : aucun.
- **Pièges** : renvoie un **carré** — ne jamais le comparer directement à une distance. Appelé uniquement par `isWithinDistance`, elle-même sans appelant. À ne pas confondre avec la variable locale `distSq` de `ScriptEngine.cpp::explode`, qui n'a aucun rapport.

#### `SDL_Rect* util::theNearest(SDL_Rect& tested, std::vector<SDL_Rect*> targets)`
- **Rôle** : renvoie la cible dont le centre est le plus proche du centre de `tested`.
- **Paramètres** : `tested` — référence en pixels ; `targets` — vecteur de pointeurs, pris **par valeur** (copie du vecteur à chaque appel).
- **Retour** : pointeur vers la cible la plus proche, `NULL` si `targets` est vide.
- **Effets de bord** : aucun.
- **Pièges** : la distance initiale est la constante magique `1e12` — une cible plus éloignée que ça (impossible en pratique) ne serait pas retenue. Déréférence chaque `target` sans test de nullité. Copie inutile du vecteur (`const std::vector<SDL_Rect*>&` conviendrait). **Aucun appelant** : remplacée par la recherche de cible en Lua (`findTarget` dans `missile.lua`).

#### `SDL_Point util::spawnOffScreen(int screenWidth, int screenHeight, int margin)`
- **Rôle** : tire un point aléatoire juste à l'extérieur de l'écran, sur un des quatre bords.
- **Paramètres** : `screenWidth`, `screenHeight` — pixels ; `margin` — distance hors écran en pixels.
- **Retour** : `SDL_Point` sur le bord tiré.
- **Effets de bord** : consomme `rand()` (2 appels) ; dépend donc de l'état global du générateur (`srand` est fait dans `Game::start`).
- **Pièges** : `rand() % 4` puis un `switch` **sans `default`** — si `side` sortait de `[0,3]` (impossible ici), `p` serait renvoyé **non initialisé**. Le modulo introduit un léger biais de distribution. `margin` négatif placerait le point à l'intérieur de l'écran. **Aucun appelant** : la logique équivalente a été réécrite en Lua (`spawnPoint` dans `missile.lua`), avec `math.random`.

#### `void util::drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color)`
- **Rôle** : censée dessiner un rectangle à coins arrondis ; **dessine en réalité un rectangle plein**.
- **Paramètres** : `renderer` — cible SDL ; `rect` — pixels ; `radius` — **totalement ignoré** ; `color` — RGBA.
- **Retour** : —
- **Effets de bord** : change la couleur de dessin du renderer (et ne la restaure pas) ; remplit le rectangle.
- **Pièges** : le `radius` est un mensonge d'interface — le nom promet un arrondi que le corps n'implémente pas. Soit l'implémenter, soit renommer en `drawFilledRect` et retirer le paramètre. Modifier l'état du renderer sans le restaurer oblige tous les appels suivants à repositionner leur couleur. Appelée par `MenuScene::drawPanel`.

#### `void util::drawProgressPie(SDL_Renderer* renderer, int centerX, int centerY, int radius, float progress)`
- **Rôle** : dessine l'indicateur circulaire de cooldown : un cercle de contour, puis un secteur rempli proportionnel à la progression.
- **Paramètres** : `renderer` — cible SDL ; `centerX`, `centerY` — centre en pixels ; `radius` — rayon en pixels ; `progress` — fraction `[0, 1]`.
- **Retour** : —
- **Effets de bord** : dessine 360 points pour le contour puis jusqu'à 720 lignes pour le secteur ; **change la couleur de dessin** (vert si `progress >= 1`, orange sinon) et ne la restaure pas.
- **Pièges** : le contour est tracé **avec la couleur courante du renderer** (héritée de l'appelant), avant que la couleur du secteur ne soit fixée — le contour n'a donc pas de couleur propre. Le secteur part de -90° (midi) et tourne dans le sens horaire. Dessiner un disque avec 720 lignes par joueur et par frame est coûteux et laisse des trous à grand rayon ; une texture ou un masque serait plus propre. `progress` n'est pas clampé ici : `getCooldownProgress` le fait en amont, mais un `NaN` (cas `cooldown == 0`) tombe dans la branche `progress > 0.f` fausse, donc rien n'est dessiné — l'indicateur disparaît silencieusement. Utilise `M_PI` et `std::cos`/`std::sin` sans inclure `<cmath>` : ça ne compile que grâce aux inclusions transitives de SDL, fragile.

**Code mort / non utilisé dans ce fichier**
- `theNearest` : aucun appelant.
- `isWithinDistance` : aucun appelant.
- `spawnOffScreen` : aucun appelant.
- `distSq` : appelé uniquement par `isWithinDistance` (elle-même morte), donc mort transitivement.
- Autrement dit, **quatre des sept fonctions de `util` sont mortes** : ce sont les reliquats de la logique de ciblage et de spawn qui est passée en Lua. `include/Utils.hpp` inclut `<vector>` uniquement pour `theNearest`.
- Le paramètre `radius` de `drawRoundedRect` n'est jamais lu.
- `#include <SDL2/SDL_render.h>` dans le .cpp est redondant avec `<SDL2/SDL.h>` du header.

## include/KeyPreset.hpp

### `struct KeyPreset`

Mapping clavier d'un joueur : quatre scancodes SDL. Pas de méthode, pas de constructeur — agrégat pur.

| Membre de données | Type | Rôle |
|---|---|---|
| `left` | `SDL_Scancode` | Touche « aller à gauche ». |
| `right` | `SDL_Scancode` | Touche « aller à droite ». |
| `thrust` | `SDL_Scancode` | Touche du jetpack. |
| `missile` | `SDL_Scancode` | Touche d'**ability** — le nom date de l'époque où la seule capacité était le missile. |

**Pièges** : les quatre membres n'ont **pas d'initialiseur**. Un `KeyPreset` déclaré sans initialisation (c'est le cas du membre `PlayerConfig::keyPreset` quand `playerSlot[i].presetIndex < 0`, par exemple pour un joueur à la manette) contient des scancodes **indéterminés**, que `input::sample` utiliserait comme index dans le tableau `keys` de SDL — hors limites possible. En pratique `input::sample` sort avant d'y toucher quand `joystickId != -1`, donc le bug ne se déclenche pas aujourd'hui ; mettre `= SDL_SCANCODE_UNKNOWN` (0) partout supprimerait le risque. Le nom `missile` devrait devenir `ability` pour coller à `PlayerInput::ability`.

#### `inline KeyPreset presets[3]` (variable globale, header)
- **Rôle** : les trois configurations clavier prédéfinies — ZQSD/AWSD (`A D S W`), main droite (`J L K I`), pavé numérique (`KP_4 KP_6 KP_5 KP_8`). L'ordre des champs est `left, right, thrust, missile`.
- **Effets de bord** : `inline` garantit une **définition unique** malgré l'inclusion du header dans plusieurs unités de traduction (fonctionnalité C++17 ; sans elle il faudrait `extern` + une définition dans un .cpp).
- **Pièges** : variable globale **non const et mutable** : n'importe qui peut réécrire le mapping d'un joueur. `MenuScene` l'indexe de 0 à 2 pour détecter quelle touche a été pressée, et `Game::init` l'indexe par `playerSlot[i].presetIndex` après avoir vérifié `>= 0` mais **pas** `< 3` — un `presetIndex` à 3 ou plus serait une lecture hors limites. Le nombre de joueurs clavier est donc plafonné à 3 par cette constante, ce qui est implicite nulle part ailleurs.

## include/PlayerSlot.hpp

### `struct PlayerSlot`

État d'un emplacement joueur **dans le menu** : qui l'a rejoint, avec quel périphérique, quelle apparence, et où en est son curseur. C'est le format d'échange entre `MenuScene` et `Game::init` (passé en `PlayerSlot*` + nombre). Purement client : rien de tout ça ne doit remonter au serveur.

| Membre de données | Type | Rôle |
|---|---|---|
| `presetIndex` | `int` | Index dans `presets[]` (0..2), **-1 = pas de clavier** (joueur manette ou slot libre). C'est aussi le marqueur « slot occupé » côté clavier. |
| `ready` | `bool` | Le joueur a validé sa configuration ; la partie démarre quand tous les slots occupés sont prêts. |
| `skinId` | `std::string` | Identifiant d'asset de la skin choisie (ex. `"skin_turtle"`), vide par défaut. Recopié dans `PlayerConfig::skinId` et `PlayerInfo::skinId`. |
| `hatId` | `std::string` | Identifiant du chapeau choisi ; détermine **l'ability** via `createAbilityForHat`. |
| `jetpackId` | `std::string` | Identifiant de jetpack. **Jamais utilisé** (fonctionnalité non implémentée). |
| `skinIndex` | `int` | Position du curseur dans la liste `mSkinIds` du menu ; source de `skinId`. |
| `hatIndex` | `int` | Position du curseur dans `mHatIds` ; source de `hatId`. |
| `jetpackIndex` | `int` | **Jamais utilisé.** |
| `joystickId` | `int` | *Instance id* SDL de la manette, -1 = clavier. Recopié dans `PlayerConfig::joystickId`. |
| `menuCursorY` | `int` | Ligne sélectionnée dans le panneau du joueur : 0 = chapeau, 1 = skin, 2 = « Come later », 3 = « Ready ? ». Bornée à `[0, 3]` par `MenuScene`. |
| `lastAxisX` | `int` | Dernière valeur discrétisée (-1/0/1) de l'axe horizontal de la manette, pour transformer un axe analogique maintenu en **front** de navigation (une pression = un déplacement). |
| `lastAxisY` | `int` | Idem pour l'axe vertical. |

**Pièges généraux** : agrégat avec tous ses membres initialisés par défaut — contrairement à `KeyPreset`, il n'y a pas de piège de valeur indéterminée. En revanche `skinIndex`/`hatIndex` et `skinId`/`hatId` sont deux représentations **redondantes** de la même chose, qu'il faut maintenir synchronisées à la main (`MenuScene` le fait à trois endroits : entrée dans le slot, navigation gauche, navigation droite) ; une désynchronisation donnerait un chapeau affiché différent du chapeau joué. Les indices sont ramenés dans les bornes par un modulo sur `mSkinIds.size()`/`mHatIds.size()`, mais `Game::init` n'applique aucun modulo sur `presetIndex` avant d'indexer `presets[]`. La signature `Game::init(..., PlayerSlot* playerSlots, int joinedCount)` est un tableau C nu : la cohérence entre pointeur et taille n'est pas vérifiée.

**Code mort / non utilisé dans ce fichier**
- `jetpackId` : déclaré, jamais lu ni écrit (ni dans `MenuScene`, ni dans `Game`).
- `jetpackIndex` : idem — la sélection de jetpack est une fonctionnalité prévue mais absente.

---


# Presentation, ressources et scenes

## include/LTexture.hpp + src/LTexture.cpp

### `class LTexture`

Enveloppe RAII (partielle) autour d'un `SDL_Texture*`. Elle retient le renderer qui lui a été donné, ses dimensions natives en pixels, et sait se charger depuis un fichier image ou depuis du texte rendu par SDL_ttf. C'est la brique de dessin la plus basse du projet : tout ce qui s'affiche (sauf les particules et les rectangles de debug) passe par elle.

| Membre | Type | Rôle |
|---|---|---|
| `mTexture` | `SDL_Texture*` | La texture GPU possédée. `NULL` quand vide. Détruite par `free()`. |
| `gRenderer` | `SDL_Renderer*` | Renderer utilisé pour créer et dessiner la texture. **Non initialisé par le constructeur** (voir pièges). |
| `mWidth` | `int` | Largeur native en pixels de l'image/du texte chargé. 0 si vide. |
| `mHeight` | `int` | Hauteur native en pixels. 0 si vide. |

#### `LTexture();`

- **Rôle** : construit une texture vide.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : met `mTexture = NULL`, `mWidth = 0`, `mHeight = 0`.
- **Pièges** : `gRenderer` n'est **pas** mis à `nullptr`. Un `LTexture` sur lequel on oublie `setRenderer()` contient un pointeur indéterminé, et `loadFromeFile()` le passera tel quel à `SDL_CreateTextureFromSurface` (crash ou erreur SDL silencieuse). La classe n'a pas de constructeur de copie supprimé : copier un `LTexture` duplique le pointeur `mTexture` et provoque un double `SDL_DestroyTexture` quand les deux copies meurent.

#### `~LTexture();`

- **Rôle** : destructeur, libère la texture GPU.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : appelle `free()` → `SDL_DestroyTexture`.
- **Pièges** : doit s'exécuter **avant** `SDL_DestroyRenderer`/`SDL_Quit`. Les `LTexture` stockés dans le singleton `TextureManager` sont justement vidés à la main dans `main()` pour cette raison.

#### `void setRenderer(SDL_Renderer* renderer);`

- **Rôle** : injecte le renderer cible.
- **Paramètres** : `renderer` — renderer SDL déjà créé ; aucune validation.
- **Retour** : `void`.
- **Effets de bord** : écrit `gRenderer`. Ne prend pas possession du renderer.
- **Pièges** : à appeler impérativement avant tout `loadFromeFile` / `loadFromRenderedText` / `render`.

#### `bool loadFromeFile(std::string path);`

(le nom contient une faute de frappe d'origine — « From**e**File » — conservée partout dans le code)

- **Rôle** : charge une image disque (PNG, BMP… tout ce que gère SDL_image) en texture GPU.
- **Paramètres** : `path` — chemin relatif au répertoire de travail du binaire (le jeu est lancé depuis la racine du projet, d'où les chemins `assets/...`).
- **Retour** : `true` si `mTexture != NULL` à la fin, `false` si `IMG_Load` ou `SDL_CreateTextureFromSurface` a échoué. Dans les deux cas d'échec un message est écrit sur `stdout` via `printf`.
- **Effets de bord** : appelle `free()` d'abord (donc détruit l'éventuelle texture précédente) ; alloue une `SDL_Surface` puis une `SDL_Texture`, et libère toujours la surface avec `SDL_FreeSurface` ; met à jour `mWidth`/`mHeight` avec la taille de la surface. Applique un **color key magenta** `#FF00FF` (`SDL_MapRGB(..., 0xFF, 0, 0xFF)`) : tout pixel exactement magenta devient transparent.
- **Pièges** : nécessite `IMG_Init(IMG_INIT_PNG)` (fait dans `main`). Sur échec, `mTexture` reste `NULL` **et** `mWidth/mHeight` restent à 0 — un `render()` ultérieur dessinera un quad 0×0 sans prévenir. Le color key est appliqué même aux PNG qui ont déjà un canal alpha (inoffensif sauf si l'art utilise volontairement du magenta pur).

#### `bool loadFromRenderedText(std::string textureText, SDL_Color textColor, TTF_Font* gFont);`

- **Rôle** : rasterise une chaîne de texte en texture (utilisé par `Game` pour les scores et numéros de joueur).
- **Paramètres** :
  - `textureText` — texte ASCII/Latin-1 ; `TTF_RenderText_Solid` ne gère pas l'UTF-8 multi-octets.
  - `textColor` — couleur RGBA ; avec `_Solid` l'alpha de la structure est ignoré (rendu 8-bit palettisé, pixels opaques ou totalement transparents, bords crénelés).
  - `gFont` — police déjà ouverte par `TTF_OpenFont` ; la fonction ne prend pas possession de la police.
- **Retour** : `true` si la texture a été créée, `false` sinon.
- **Effets de bord** : `free()` préalable ; crée puis libère une `SDL_Surface` ; met à jour `mWidth`/`mHeight` (taille du texte rendu en pixels).
- **Pièges** :
  - **Bug réel** ligne 88 : `printf("... %s", TTF_GetError)` passe l'**adresse de la fonction** `TTF_GetError` au lieu d'appeler `TTF_GetError()`. Le `%s` déréférence un pointeur de code → sortie illisible ou segfault sur le chemin d'erreur. Correctif : `TTF_GetError()`.
  - `gFont == nullptr` (police non trouvée) n'est pas testé : `TTF_RenderText_Solid` renvoie `NULL`, on tombe dans le `printf` buggé ci-dessus.
  - Appeler cette fonction chaque frame pour un texte dynamique recrée une texture par frame (ce que fait `Game::renderSnapshot`).

#### `void free();`

- **Rôle** : détruit la texture et remet l'objet à vide.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `SDL_DestroyTexture` si `mTexture != NULL`, puis `mTexture = NULL`, `mWidth = 0`, `mHeight = 0`. Idempotent.
- **Pièges** : `gRenderer` n'est pas touché (volontaire : on peut recharger derrière).

#### `void setColor(Uint8 red, Uint8 green, Uint8 blue);`

- **Rôle** : applique une modulation de couleur (chaque pixel est multiplié par `(r,g,b)/255`) — sert à teinter un sprite.
- **Paramètres** : `red`, `green`, `blue` — 0-255, 255 = pas de modification sur ce canal.
- **Retour** : `void`. Écrit un `printf` si `SDL_SetTextureColorMod` renvoie < 0.
- **Effets de bord** : modifie l'état de la texture SDL, **persistant** jusqu'au prochain appel (ce n'est pas un paramètre de rendu ponctuel).
- **Pièges** : si `mTexture == NULL`, SDL met une erreur et la fonction log ; pas de crash. Aucun appelant dans le projet (code mort).

#### `void setBlendMode(SDL_BlendMode blending);`

- **Rôle** : choisit le mode de mélange de la texture (`SDL_BLENDMODE_NONE/BLEND/ADD/MOD`).
- **Paramètres** : `blending` — enum SDL.
- **Retour** : `void` ; le code de retour SDL est ignoré.
- **Effets de bord** : état persistant sur la texture.
- **Pièges** : nécessaire pour que `setAlpha` ait un effet (il faut `SDL_BLENDMODE_BLEND`). Aucun appelant (code mort).

#### `void setAlpha(Uint8);`

Définition : `void LTexture::setAlpha(Uint8 alpha)`. Le paramètre est **anonyme dans le header**.

- **Rôle** : règle la transparence globale de la texture.
- **Paramètres** : `alpha` — 0 (invisible) à 255 (opaque).
- **Retour** : `void` ; code de retour SDL ignoré.
- **Effets de bord** : état persistant sur la texture.
- **Pièges** : sans effet si le blend mode est `NONE`. Aucun appelant (code mort).

#### `void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE, int width = 0, int height = 0);`

- **Rôle** : dessine la texture dans le renderer courant, avec découpe de sprite sheet, rotation, miroir et redimensionnement optionnels.
- **Paramètres** :
  - `x`, `y` — coin **haut-gauche** de destination, en pixels de l'espace logique (1024×576, cf. `SDL_RenderSetLogicalSize` dans `main`).
  - `clip` — rectangle source dans la texture en pixels ; `NULL` = texture entière. S'il est fourni, la taille de destination prend par défaut `clip->w`/`clip->h` (donc pas de mise à l'échelle implicite).
  - `angle` — rotation en **degrés**, sens horaire.
  - `center` — pivot de rotation, en pixels **relatifs au rectangle de destination** ; `NULL` = centre du rectangle.
  - `flip` — miroir horizontal/vertical.
  - `width`, `height` — override de la taille de destination en pixels ; **0 signifie « ne pas forcer »** (c'est la convention maison, d'où l'impossibilité de demander une taille 0).
- **Retour** : `void` ; le code de retour de `SDL_RenderCopyEx` est ignoré.
- **Effets de bord** : écrit dans le back buffer du renderer. N'alloue rien.
- **Pièges** : si `mTexture == NULL`, `SDL_RenderCopyEx` échoue silencieusement. Les paramètres `x`/`y` sont `int` alors que `AnimationPlayer::render` lui passe des `float` (troncature vers zéro ⇒ léger tremblement sous-pixel).

#### `int getWidth();`

- **Rôle** : largeur native en pixels.
- **Paramètres** : aucun.
- **Retour** : `mWidth` ; 0 si la texture est vide ou le chargement a échoué.
- **Effets de bord** : aucun.
- **Pièges** : non `const` (empêche l'appel sur un `const LTexture&`).

#### `int getHeight();`

- **Rôle** : hauteur native en pixels.
- **Paramètres** : aucun.
- **Retour** : `mHeight` ; 0 si vide.
- **Effets de bord** : aucun.
- **Pièges** : non `const`.

**Code mort / non utilisé dans ce fichier**
- `setColor`, `setBlendMode`, `setAlpha` : définis, aucun appelant dans `src/`, `include/` ni dans les mods Lua.
- `free()` est public mais n'est appelé que depuis l'intérieur de la classe (destructeur et les deux `loadFrom*`).
- Pas de constructeur/opérateur de copie supprimé alors que la classe possède une ressource (risque latent de double destruction).

---

## include/LTimer.hpp + src/LTimer.cpp

### `class LTimer`

Chronomètre en millisecondes basé sur `SDL_GetTicks()`, avec pause. Le jeu l'utilise comme horloge de delta time (`main.cpp`), comme minuteur de cooldown (`Ability`, `LuaAbility`), de spawn (`World::mPizzaTimer`) et de cadence de particules (`Player::thrustParticlesTimer`).

| Membre | Type | Rôle |
|---|---|---|
| `mStartTicks` | `Uint32` | Valeur de `SDL_GetTicks()` au démarrage (ms depuis `SDL_Init`). 0 quand arrêté ou en pause. |
| `mPausedTicks` | `Uint32` | Durée écoulée figée au moment de la pause, en ms. |
| `mPaused` | `bool` | Vrai si en pause. |
| `mStarted` | `bool` | Vrai entre `start()` et `stop()`. |

#### `LTimer();`

- **Rôle** : crée un chronomètre arrêté à zéro.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : met les quatre membres à 0/false.
- **Pièges** : aucun ; `getTicks()` renvoie 0 tant qu'on n'a pas appelé `start()`.

#### `void start();`

- **Rôle** : démarre **ou redémarre** le chronomètre à zéro.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `mStarted = true`, `mPaused = false`, `mStartTicks = SDL_GetTicks()`, `mPausedTicks = 0`.
- **Pièges** : il n'y a pas de `restart()` : appeler `start()` sur un timer déjà lancé le remet à zéro, et c'est précisément l'usage fait dans `main.cpp` (`deltaTimer.start()` à chaque frame) et dans `Player.cpp`. Une pause en cours est annulée. `SDL_GetTicks()` déborde après ~49,7 jours (`Uint32` en ms) : un timer qui chevauche le wrap renvoie une durée gigantesque.

#### `void stop();`

- **Rôle** : arrête et remet tout à zéro.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : les quatre membres à 0/false. `getTicks()` renverra 0 ensuite.
- **Pièges** : aucun appelant dans le projet (code mort).

#### `void pause();`

- **Rôle** : gèle le temps écoulé.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : ne fait rien si le timer n'est pas démarré ou est déjà en pause. Sinon : `mPausedTicks = SDL_GetTicks() - mStartTicks` (ms écoulées), `mStartTicks = 0`, `mPaused = true`.
- **Pièges** : aucun appelant (code mort).

#### `void unpause();`

- **Rôle** : reprend le décompte là où il s'était arrêté.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : ne fait rien si non démarré ou non en pause. Sinon recule le point de départ : `mStartTicks = SDL_GetTicks() - mPausedTicks`, `mPausedTicks = 0`, `mPaused = false`.
- **Pièges** : aucun appelant (code mort).

#### `Uint32 getTicks();`

- **Rôle** : temps écoulé depuis `start()`, en millisecondes.
- **Paramètres** : aucun.
- **Retour** : 0 si le timer n'est pas démarré ; `mPausedTicks` si en pause ; sinon `SDL_GetTicks() - mStartTicks`.
- **Effets de bord** : aucun.
- **Pièges** : non `const`. Résolution d'environ 1 ms, mais la boucle principale fait `SDL_Delay(16)` : les deltas mesurés sont quantifiés à ~16-17 ms. `main.cpp` convertit en secondes (`/1000.0f`) et plafonne à 0,05 s.

#### `bool isStarted();`

- **Rôle** : indique si le chronomètre tourne ou est en pause (c.-à-d. « pas stoppé »).
- **Paramètres** : aucun.
- **Retour** : `mStarted`.
- **Effets de bord** : aucun.
- **Pièges** : renvoie `true` même en pause. Aucun appelant (code mort).

#### `bool isPaused();`

- **Rôle** : indique si le chronomètre est en pause **et** démarré.
- **Paramètres** : aucun.
- **Retour** : `mPaused && mStarted`.
- **Effets de bord** : aucun.
- **Pièges** : aucun appelant (code mort).

**Code mort / non utilisé dans ce fichier**
- `stop()`, `pause()`, `unpause()`, `isStarted()`, `isPaused()` : aucun appelant dans tout le projet. Seuls `start()` et `getTicks()` servent.
- `#include <SDL2/SDL_ttf.h>` dans le header est inutile (aucun type TTF employé).

---

## include/TextureManager.hpp + src/TextureManager.cpp

### `class TextureManager`

Singleton (Meyers singleton via variable statique locale) qui possède toutes les textures du jeu, indexées par identifiant chaîne. Il centralise la durée de vie des `LTexture` pour que le reste du code manipule des `LTexture*` empruntés sans se soucier de la libération.

| Membre | Type | Rôle |
|---|---|---|
| `mTextureMap` | `std::unordered_map<std::string, std::unique_ptr<LTexture>>` | Propriétaire des textures, clé = identifiant logique (`"skin_bleu"`, `"bg"`, …). |
| `mRenderer` | `SDL_Renderer*` | Renderer transmis à chaque `LTexture` créée. `nullptr` avant `init()`. |

#### `static TextureManager& getInstance(){ static TextureManager instance; return instance; }`

- **Rôle** : accès à l'instance unique (définie inline dans le header).
- **Paramètres** : aucun.
- **Retour** : référence sur l'instance statique ; jamais nulle.
- **Effets de bord** : construit l'instance au premier appel (initialisation thread-safe garantie depuis C++11).
- **Pièges** : la destruction a lieu à la fin du programme, **après** `SDL_Quit()` appelé dans `main`. C'est pourquoi `main.cpp` appelle `TextureManager::getInstance().clean()` explicitement avant `SDL_DestroyRenderer` : sans cela, `SDL_DestroyTexture` serait appelé sur un renderer déjà détruit.

#### `TextureManager(const TextureManager&) = delete;` / `TextureManager& operator=(const TextureManager&) = delete;`

- **Rôle** : interdire la copie du singleton.
- **Paramètres** / **Retour** : —
- **Effets de bord** : aucun (fonctions supprimées, erreur à la compilation si utilisées).

#### `TextureManager() {}` *(privé)*

- **Rôle** : constructeur par défaut vide ; `mRenderer` est initialisé à `nullptr` par l'initialiseur de membre du header.
- **Pièges** : privé, donc seul `getInstance()` peut construire.

#### `~TextureManager() { clean(); }` *(privé)*

- **Rôle** : vide la map à la fin du programme.
- **Effets de bord** : détruit tous les `LTexture` (donc tous les `SDL_Texture`).
- **Pièges** : voir `getInstance()` — s'exécute après `SDL_Quit()`. En pratique inoffensif car `main` a déjà appelé `clean()` et la map est vide.

#### `void init(SDL_Renderer* renderer);`

- **Rôle** : mémorise le renderer à utiliser pour toutes les textures.
- **Paramètres** : `renderer` — renderer valide ; non validé.
- **Retour** : `void`.
- **Effets de bord** : écrit `mRenderer`.
- **Pièges** : **précondition d'ordre** — doit être appelé après `SDL_CreateRenderer` et avant tout `loadTexture`. `main.cpp` le fait ligne 42, avant `ScriptEngine::loadMods` (qui charge les textures des mods). Ré-appeler `init()` avec un autre renderer ne met pas à jour les textures déjà chargées, qui resteraient liées à l'ancien.

#### `bool loadTexture(const std::string& id, const std::string& path);`

- **Rôle** : charge une image et l'enregistre sous `id`.
- **Paramètres** :
  - `id` — identifiant logique choisi par l'appelant (exposé à Lua via la fonction globale `loadTexture`).
  - `path` — chemin du fichier image.
- **Retour** : `true` si la texture est disponible à la fin — **y compris quand `id` existait déjà**, auquel cas la fonction retourne immédiatement sans rien charger. `false` uniquement si le chargement échoue (un `printf` « Error TextureManager : can't load » est émis).
- **Effets de bord** : alloue un `LTexture` (`make_unique`), lui passe `mRenderer`, insère dans `mTextureMap` en cas de succès. En cas d'échec, le `unique_ptr` local est détruit (pas de fuite).
- **Pièges** : la déduplication se fait sur l'`id` seul, donc re-déclarer le même `id` avec un chemin différent est silencieusement ignoré (le premier gagne). Si `init()` n'a pas été appelé, `mRenderer == nullptr` et le chargement échoue en loggant une erreur SDL.

#### `std::vector<std::string> TextureManager::loadDirectory(const std::string& folderPath, const std::string& prefixId);`

- **Rôle** : charge en masse tous les PNG d'un dossier, en dérivant l'identifiant du nom de fichier. Sert à peupler les listes de skins et de chapeaux du menu.
- **Paramètres** :
  - `folderPath` — dossier à parcourir (non récursif), ex. `"assets/hats/"`.
  - `prefixId` — préfixe ajouté devant le nom de fichier sans extension, ex. `"hat_"` → `assets/hats/cowboy.png` devient l'id `hat_cowboy`.
- **Retour** : la liste des identifiants effectivement chargés (ceux dont `loadTexture` a renvoyé `true`). Vecteur vide si le dossier ne contient aucun `.png`.
- **Effets de bord** : insère N textures dans la map.
- **Pièges** :
  - Extension testée en **sensible à la casse** : un `.PNG` ou un `.jpg` est ignoré sans avertissement.
  - `std::filesystem::directory_iterator` **lève** `std::filesystem::filesystem_error` si le dossier n'existe pas, et personne n'attrape l'exception → terminaison du programme.
  - **L'ordre d'itération n'est pas spécifié** par le standard (dépend du système de fichiers). Les indices `skinIndex`/`hatIndex` du menu référencent donc des positions non déterministes d'une machine à l'autre : un même index ne donne pas forcément le même chapeau. Pour un jeu en ligne c'est une source de désynchronisation ; mieux vaudrait trier le vecteur.
  - Appelé deux fois sur les mêmes dossiers (`MenuScene` puis `Game::loadMedia`) : le second appel ne recharge rien grâce à la déduplication par `id`, mais reparcourt le disque.

#### `LTexture* getTexture(const std::string& id);`

- **Rôle** : récupère une texture par identifiant.
- **Paramètres** : `id` — identifiant utilisé au chargement.
- **Retour** : pointeur **emprunté** (le manager reste propriétaire) ou `nullptr` si l'id est inconnu.
- **Effets de bord** : aucun (pas d'insertion accidentelle, la fonction utilise `find` et non `operator[]`).
- **Pièges** : plusieurs appelants ne testent pas le `nullptr` (ex. `MenuScene::drawPlayer`) → déréférencement nul si un asset manque. Le pointeur est invalidé par `clean()` ; les `LTexture` étant possédés par `unique_ptr`, un rehash de la map ne les déplace pas, donc le pointeur reste valide tant que l'entrée existe.

#### `void clean();`

- **Rôle** : détruit toutes les textures.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `mTextureMap.clear()` → destruction en cascade des `LTexture` → `SDL_DestroyTexture` pour chacune. Tous les `LTexture*` distribués deviennent pendants.
- **Pièges** : **à appeler avant `SDL_DestroyRenderer`** (fait dans `main`). Après un `clean()`, tous les pointeurs conservés ailleurs (par exemple `player::PlayerConfig::skin`/`hat`, `Projectile`) sont invalides ; rien dans le code ne les réinvalide.

**Code mort / non utilisé dans ce fichier**
- Aucune fonction morte : `init`, `loadTexture`, `loadDirectory`, `getTexture`, `clean` ont tous des appelants.
- Includes superflus dans le header : `<memory>` et `<vector>` sont utilisés, mais `<filesystem>` est inclus à la fois dans le `.hpp` et le `.cpp` alors qu'il n'est nécessaire que dans le `.cpp` ; `<vector>` est aussi doublement inclus.

---

## include/Animation.hpp + src/Animation.cpp

### `struct Animation`

Description **immuable et partagée** d'une animation sur sprite sheet : quelle texture, la taille d'une frame, la disposition en grille, la durée par frame et le bouclage. Les valeurs sont des agrégats initialisables (`Animation missileAnim{"explosion_missile_sheet", 64, 64, 6, 30, 0.03, false};`).

| Membre | Type | Rôle |
|---|---|---|
| `textureId` | `std::string` | Id de la sprite sheet dans `TextureManager`. Pas de défaut (chaîne vide). |
| `frameW` | `int` = 64 | Largeur d'une frame en pixels. |
| `frameH` | `int` = 64 | Hauteur d'une frame en pixels. |
| `columns` | `int` = 5 | Nombre de colonnes de la grille ; sert à convertir un index de frame en (colonne, ligne). |
| `frameCount` | `int` = 5 | Nombre total de frames jouées. |
| `frameDuration` | `float` = 10 | Durée d'une frame, **dans la même unité que le `deltaTime` passé à `update()`**, donc en **secondes** en pratique (`EffectManager` est nourri avec `realDeltaTime` en secondes ; le mod base utilise 0.03 s). La valeur par défaut de 10 est donc absurde (10 s/frame) et sert de garde-fou involontaire. |
| `loop` | `bool` = false | Si vrai, l'animation recommence indéfiniment ; sinon elle se marque terminée. |

### `class AnimationPlayer`

Instance de lecture d'une `Animation` : elle ne stocke que le temps écoulé et un drapeau de fin, et pointe sur la description partagée. Elle est copiable à bas coût (c'est ce qui permet de la mettre dans un `std::vector<Effect>`).

| Membre | Type | Rôle |
|---|---|---|
| `animation` | `const Animation*` = nullptr | Description empruntée, détenue par `AnimationManager`. |
| `elapsed` | `float` = 0 | Temps écoulé depuis le début, même unité que `frameDuration` (secondes). |
| `finished` | `bool` = false | Vrai une fois l'animation non bouclée terminée. |

#### `AnimationPlayer(const Animation* animation);`

- **Rôle** : construit un lecteur sur une description donnée.
- **Paramètres** : `animation` — pointeur vers une `Animation` qui doit survivre au lecteur ; `nullptr` accepté (le lecteur devient inerte).
- **Retour** : —
- **Effets de bord** : liste d'initialisation uniquement (`elapsed = 0`, `finished = false` via les initialiseurs du header).
- **Pièges** : le paramètre masque le membre du même nom (`: animation(animation)` fonctionne mais c'est fragile). La durée de vie est critique : `AnimationManager::clean()` rendrait ce pointeur pendant. Il n'y a pas de constructeur par défaut, donc `struct Effect` ne peut être construit qu'en fournissant un `AnimationPlayer`.

#### `void update(float deltaTime);`

- **Rôle** : avance l'horloge de lecture et gère la fin / le bouclage.
- **Paramètres** : `deltaTime` — temps écoulé en secondes (même unité que `frameDuration`).
- **Retour** : `void`.
- **Effets de bord** : incrémente `elapsed` ; si `elapsed >= frameCount * frameDuration`, met `finished = true` (animation simple) ou replie `elapsed` avec `std::fmod` (animation bouclée).
- **Pièges** : retourne immédiatement si `finished` ou si `animation == nullptr`. Un `deltaTime` très grand (lag) saute des frames ; une animation bouclée ne « rattrape » pas, elle modulo. Rien ne protège contre `frameDuration <= 0` ici — c'est `AnimationManager::registerAnimation` qui filtre en amont.

#### `void render(float x, float y, float scale = 1.f, double angle = 0.0) const;`

- **Rôle** : dessine la frame courante.
- **Paramètres** :
  - `x`, `y` — coin haut-gauche de destination en pixels (l'appelant, `EffectManager::spawn`, a déjà décalé de la moitié de la frame pour centrer).
  - `scale` — facteur multiplicatif appliqué à `frameW`/`frameH` ; 1 = taille native.
  - `angle` — rotation en degrés.
- **Retour** : `void`.
- **Effets de bord** : dessine dans le renderer détenu par la `LTexture` de la sprite sheet.
- **Pièges** : sort sans rien faire si `animation == nullptr`, si `finished`, ou si la texture est introuvable dans `TextureManager` (`getTexture` → `nullptr`, testé ici, contrairement à `MenuScene`). Le découpage suppose une grille régulière sans marge ni espacement. `x`/`y` `float` sont tronqués en `int` par `LTexture::render`. Le pivot de rotation est `NULL` donc le centre du quad de destination.

#### `bool isFinished() const;`

- **Rôle** : savoir si l'animation est terminée (critère de suppression dans `EffectManager::update`).
- **Paramètres** : aucun.
- **Retour** : `finished`. Toujours `false` pour une animation bouclée (donc un effet bouclé ne serait **jamais** retiré de la liste d'effets — fuite logique à garder en tête).
- **Effets de bord** : aucun.

#### `int getCurrentFrame() const;`

- **Rôle** : index de la frame à afficher.
- **Paramètres** : aucun.
- **Retour** : `0` si `animation == nullptr` ; sinon `elapsed / frameDuration` tronqué, **plafonné à `frameCount - 1`**.
- **Effets de bord** : aucun.
- **Pièges** : public mais seul `render()` l'appelle. Division par `frameDuration` sans garde (voir `registerAnimation`).

**Code mort / non utilisé dans ce fichier**
- `getCurrentFrame()` est public sans appelant externe.
- L'argument `angle` de `render()` n'est jamais utilisé : `EffectManager::render` appelle `e.player.render(e.x, e.y, e.scale)` seulement.
- La fonction Lua globale `registerAnimation(id, def)` est **documentée dans `mods/base/meta/akaka.lua`** mais n'est **pas exposée** par `ScriptEngine` : aucune animation n'est enregistrable depuis un mod, seule `Game::loadMedia` en enregistre une en dur (`explosion_missile`). Stub de documentation sans implémentation.
- `#include <SDL2/SDL_render.h>` et `<cmath>` : `cmath` est bien utilisé (`fmod`), `SDL_render.h` arrive déjà via `LTexture.hpp`.

---

## include/AnimationManager.hpp + src/AnimationManager.cpp

### `class AnimationManager`

Singleton registre des descriptions d'animation, clé = identifiant chaîne. Les `Animation` sont stockées **par valeur** dans une `unordered_map` et distribuées par pointeur const ; les `AnimationPlayer` s'appuient sur la stabilité d'adresse des nœuds de l'`unordered_map`.

| Membre | Type | Rôle |
|---|---|---|
| `animationMap` | `std::unordered_map<std::string, Animation>` | Les descriptions, possédées par valeur. |

#### `static AnimationManager& getInstance(){ static AnimationManager instance; return instance; }`

- **Rôle** : accès à l'instance unique (inline dans le header).
- **Paramètres** : aucun.
- **Retour** : référence non nulle.
- **Effets de bord** : construction paresseuse au premier appel.
- **Pièges** : destruction en fin de programme, dans un ordre non garanti par rapport aux autres statiques ; sans importance ici car `Animation` ne détient aucune ressource SDL.

#### `AnimationManager(const AnimationManager&) = delete;` / `AnimationManager& operator=(const AnimationManager&) = delete;`

- **Rôle** : interdire la copie.
- **Retour / effets** : —

#### `AnimationManager() {}` *(privé)* et `~AnimationManager() { clean(); }` *(privé)*

- **Rôle** : construction/destruction réservées au singleton ; le destructeur vide la map.
- **Effets de bord** : `clean()` → libère les chaînes `textureId`. Aucune ressource SDL impliquée.

#### `bool registerAnimation(const std::string& id, const Animation& animation);`

- **Rôle** : valide puis enregistre une description d'animation.
- **Paramètres** :
  - `id` — identifiant logique (ex. `"explosion_missile"`), utilisé ensuite par `EffectManager::spawn` et par les ids d'assets réseau (`AssetIds`).
  - `animation` — description copiée dans la map.
- **Retour** : `false` si la description est invalide (`frameCount <= 0`, `frameDuration <= 0`, `frameW <= 0`, `frameH <= 0` ou `columns <= 0`), avec un `printf` « Animation : X is invalid ». `true` si l'enregistrement a réussi **ou si l'id existait déjà** (retour anticipé sans écrasement).
- **Effets de bord** : insertion dans `animationMap` (copie de la `Animation`, donc de la chaîne `textureId`).
- **Pièges** : la validation ne vérifie **pas** que `textureId` correspond à une texture chargée — l'erreur n'apparaîtra qu'au `render()` (qui sort silencieusement). Elle ne vérifie pas non plus `frameCount <= columns * lignes disponibles`. La sémantique « id existant → `true` sans modification » signifie qu'on ne peut pas redéfinir une animation en cours de partie.

#### `const Animation* getAnimation(const std::string& id);`

- **Rôle** : retrouve une description par id.
- **Paramètres** : `id` — identifiant enregistré.
- **Retour** : pointeur const **emprunté** sur l'entrée de la map, ou `nullptr` si inconnu.
- **Effets de bord** : aucun (`find`, pas `operator[]`).
- **Pièges** : le pointeur reste valide tant que l'entrée n'est pas effacée (les nœuds d'une `unordered_map` ne bougent pas au rehash), mais `clean()` le rend pendant alors que des `AnimationPlayer` peuvent encore le détenir.

#### `void clean();`

- **Rôle** : vide le registre.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `animationMap.clear()`.
- **Pièges** : invalide tous les `const Animation*` déjà distribués (notamment ceux capturés dans les `Effect` vivants d'un `EffectManager`). Aucun appelant explicite dans le projet : seul le destructeur du singleton l'appelle.

**Code mort / non utilisé dans ce fichier**
- `clean()` n'a aucun appelant hors du destructeur privé.
- Une seule animation est enregistrée dans tout le projet (`explosion_missile`, depuis `Game::loadMedia`) : le registre est largement sous-utilisé, et le point d'entrée Lua correspondant n'existe pas (voir `Animation.cpp`).

---

## include/AudioManager.hpp + src/AudioManager.cpp

### `class AudioManager`

Gestionnaire SDL_mixer **non singleton** : `Game` en possède une instance membre. Il détient deux tables — musiques (`Mix_Music`, une seule joue à la fois) et effets (`Mix_Chunk`, joués sur des canaux) — et expose le contrôle de canaux dont les capacités Lua se servent (`ctx:playSFX`, `ctx:stopSFX`).

| Membre | Type | Rôle |
|---|---|---|
| `musicMap` | `std::unordered_map<std::string, Mix_Music*>` | Musiques possédées (libérées par `Mix_FreeMusic`). |
| `sfxMap` | `std::unordered_map<std::string, Mix_Chunk*>` | Échantillons possédés (libérés par `Mix_FreeChunk`). |

#### `AudioManager();`

- **Rôle** : constructeur vide ; n'initialise rien côté SDL.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : aucun (corps `{}`).
- **Pièges** : il faut appeler `init()` séparément (fait par `Game::init`).

#### `~AudioManager();`

- **Rôle** : destructeur.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : appelle `clean()` → libère toutes les musiques/chunks **et ferme le périphérique audio** (`Mix_CloseAudio`).
- **Pièges** : dans l'état actuel du code, ce destructeur **ne s'exécute jamais** : `GameScene` fait `mGame = new Game()` sans jamais le `delete` (voir `GameScene.cpp`). Toutes les musiques et SFX chargées fuient à chaque partie. Et si le destructeur venait à s'exécuter, il fermerait le périphérique audio global que `main.cpp` a ouvert — une seconde partie n'aurait plus de son.

#### `bool init();`

- **Rôle** : initialise le sous-système audio et ouvre le périphérique.
- **Paramètres** : aucun.
- **Retour** : `false` si `SDL_InitSubSystem(SDL_INIT_AUDIO)` ou `Mix_OpenAudio` échoue (avec `printf` de l'erreur), `true` sinon.
- **Effets de bord** :
  - `SDL_InitSubSystem(SDL_INIT_AUDIO)` (comptage de références : sans `SDL_QuitSubSystem` correspondant, le compteur ne redescend pas).
  - `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)` : 44100 Hz, format par défaut (S16 natif), 2 canaux (stéréo), buffer de 2048 échantillons (≈46 ms de latence).
  - `Mix_AllocateChannels(64)` : 64 canaux de mixage simultanés pour les SFX.
- **Pièges** : **double ouverture** — `main.cpp` appelle déjà `Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048)` ligne 21. SDL_mixer compte les ouvertures : le second appel ne reconfigure rien, le périphérique reste à 48000 Hz, et les paramètres demandés ici sont donc **ignorés**. Le code de retour de `init()` n'est pas testé par `Game::init`. Aucun `Mix_Init(MIX_INIT_OGG)` n'est appelé nulle part : le chargement du `.ogg` repose sur l'auto-initialisation de SDL_mixer au premier `Mix_LoadMUS`.

#### `void clean();`

- **Rôle** : libère toutes les ressources audio et ferme le périphérique.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `Mix_FreeMusic` sur chaque musique, `musicMap.clear()`, `Mix_FreeChunk` sur chaque chunk, `sfxMap.clear()`, puis `Mix_CloseAudio()`.
- **Pièges** :
  - `Mix_FreeChunk` sur un chunk encore en lecture est un comportement indéfini : il faudrait `Mix_HaltChannel(-1)` d'abord (la fonction `stopAllChannel()` existe pour ça mais n'est pas appelée ici).
  - `Mix_CloseAudio()` est inconditionnel, même si `init()` a échoué, et ferme un périphérique partagé avec `main.cpp`.
  - Appelé uniquement par le destructeur → mort en pratique (voir plus haut).

#### `bool loadMusic(const std::string& id, const std::string& path);`

- **Rôle** : charge un fichier musique (OGG dans ce projet) sous un id.
- **Paramètres** : `id` — identifiant logique (`"miniloop14"`) ; `path` — chemin du fichier.
- **Retour** : `true` si chargé **ou déjà présent** ; `false` si `Mix_LoadMUS` échoue (avec `printf` de `Mix_GetError`).
- **Effets de bord** : alloue un `Mix_Music` et l'insère dans `musicMap`.
- **Pièges** : déduplication par `id` seul (un second chemin pour le même id est ignoré silencieusement). Le `;` superflu après l'accolade fermante ligne 39 est inoffensif.

#### `bool loadSFX(const std::string& id, const std::string& path);`

- **Rôle** : charge un échantillon court entièrement en mémoire (`Mix_LoadWAV`, qui lit aussi OGG/FLAC selon la compilation).
- **Paramètres** : `id` — identifiant logique (`"explosion"`, `"jetpackThrust"`, …, référencés depuis Lua par nom) ; `path` — chemin.
- **Retour** : `true` si chargé ou déjà présent, `false` sur échec (avec log).
- **Effets de bord** : alloue un `Mix_Chunk` dans `sfxMap`.
- **Pièges** : mêmes remarques que `loadMusic`. Les ids doivent correspondre à ceux utilisés dans les mods Lua et dans `AssetIds` — aucune vérification croisée.

#### `void playMusic(const std::string& id, int loops=-1);`

- **Rôle** : lance une musique (remplace celle en cours).
- **Paramètres** :
  - `id` — identifiant chargé.
  - `loops` — nombre de répétitions ; **-1 = boucle infinie** (défaut), 0 = joue une fois.
- **Retour** : `void` ; le code de retour de `Mix_PlayMusic` est ignoré.
- **Effets de bord** : démarre la lecture sur le canal musique unique de SDL_mixer. Si l'id est inconnu : `printf("Music %s not found")` et rien d'autre.
- **Pièges** : `musicMap[id]` est utilisé après le `find` — ici sans risque car la clé existe, mais c'est une double recherche.

#### `int playSFX(const std::string& id, int loops=0, int channel = -1);`

- **Rôle** : joue un effet sonore et renvoie le canal utilisé, ce qui permet de l'arrêter plus tard (utilisé pour le son continu du jetpack et pour les sons de projectiles côté Lua).
- **Paramètres** :
  - `id` — identifiant chargé.
  - `loops` — répétitions supplémentaires ; **0 = joue une fois** (défaut), **-1 = boucle infinie** (ce qu'utilise `Player` pour le jetpack).
  - `channel` — canal imposé ; **-1 = premier canal libre** (défaut), parmi les 64 alloués.
- **Retour** : index du canal (≥ 0) utilisé, ou **-1** si l'id est inconnu (avec `printf("SFX %s not found")`) **ou** si `Mix_PlayChannel` échoue (tous les canaux occupés). Les appelants doivent tester `>= 0` avant de mémoriser le canal — `Game::drainEvents` le fait.
- **Effets de bord** : occupe un canal de mixage.
- **Pièges** : un canal rendu peut être réattribué à un autre son dès que le précédent se termine ; garder un index de canal longtemps (comme `Player::jetpackChannel`) risque d'arrêter le son de quelqu'un d'autre. `Game` nettoie sa table `mSfxChannels` chaque frame avec `isPlaying()` pour limiter le problème.

#### `void stopMusic();`

- **Rôle** : arrête immédiatement la musique.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `Mix_HaltMusic()`.
- **Pièges** : aucun appelant dans le projet (code mort) ; quitter une partie laisse donc la musique tourner.

#### `void setMusicVolume(int volume);`

- **Rôle** : règle le volume de la musique.
- **Paramètres** : `volume` — **0 à 128** (`MIX_MAX_VOLUME` = 128) ; SDL_mixer écrête hors bornes. `Game::start` met 32, soit 25 %.
- **Retour** : `void` (le volume précédent renvoyé par `Mix_VolumeMusic` est ignoré).
- **Effets de bord** : état global du canal musique.
- **Pièges** : agit globalement, pas par instance ; aucun équivalent n'existe pour le volume des SFX.

#### `void stopChannel(int channel);`

- **Rôle** : coupe un canal SFX précis.
- **Paramètres** : `channel` — index renvoyé par `playSFX`.
- **Retour** : `void`.
- **Effets de bord** : `Mix_HaltChannel(channel)`.
- **Pièges** : retourne sans rien faire si `channel < 0` — garde volontaire qui permet d'appeler la fonction avec un handle « invalide » sans précaution. Attention : `Mix_HaltChannel(-1)` arrêterait **tous** les canaux, et c'est précisément ce que la garde évite.

#### `bool isPlaying(int channel) const;`

- **Rôle** : savoir si un canal joue encore.
- **Paramètres** : `channel` — index de canal.
- **Retour** : `false` si `channel < 0` ; sinon `Mix_Playing(channel) != 0`.
- **Effets de bord** : aucun.
- **Pièges** : la garde `channel < 0` est là encore essentielle, car `Mix_Playing(-1)` renvoie le **nombre** de canaux actifs, ce qui aurait donné un `true` trompeur.

#### `void stopAllChannel();`

- **Rôle** : couper tous les effets sonores.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `Mix_HaltChannel(-1)` → arrête les 64 canaux (pas la musique).
- **Pièges** : **code mort** — `GameScene::update` appelle `Mix_HaltChannel(-1)` en direct au lieu de passer par cette méthode (fuite d'abstraction : `GameScene` doit alors inclure SDL_mixer).

**Code mort / non utilisé dans ce fichier**
- `stopMusic()` : aucun appelant.
- `stopAllChannel()` : aucun appelant (contourné par `Mix_HaltChannel(-1)` dans `GameScene.cpp`).
- `clean()` et `~AudioManager()` : jamais exécutés en pratique puisque l'objet `Game` qui contient l'`AudioManager` n'est jamais détruit → **fuite de toutes les ressources audio à chaque partie**.
- Le code de retour de `init()`, `loadMusic()`, `loadSFX()` n'est testé par aucun appelant.

---

## include/EffectManager.hpp + src/EffectManager.cpp

### `struct Effect`

Une instance d'effet visuel en cours : un lecteur d'animation plus sa position et son échelle. Agrégat sans constructeur explicite, créé par `Effect{AnimationPlayer(animation), x, y, scale}`.

| Membre | Type | Rôle |
|---|---|---|
| `player` | `AnimationPlayer` | Lecteur (temps écoulé + pointeur sur la description). |
| `x`, `y` | `float` | Coin haut-gauche de dessin en pixels, déjà décalé pour centrer l'effet. |
| `scale` | `float` | Facteur d'échelle appliqué à la frame. |

### `class EffectManager`

Gère deux choses sans rapport direct : la liste des animations ponctuelles à l'écran (explosions) et le **tremblement de caméra** (screen shake) global. `Game` en possède une instance et lui transmet les événements réseau `Effect` et `Shake` reçus dans le snapshot.

| Membre | Type | Rôle |
|---|---|---|
| `shakeIntensity` | `float` = 0 | Amplitude courante du tremblement en pixels (décroît dans le temps). |
| `shakeDuration` | `float` = 0 | Temps restant de tremblement, en secondes. |
| `shakeX` | `int` = 0 | Décalage horizontal à appliquer au rendu cette frame, en pixels. |
| `shakeY` | `int` = 0 | Décalage vertical, en pixels. |
| `effects` | `std::vector<Effect>` | Effets actifs ; compactés chaque frame. |

#### `void spawn(const std::string& id, float x, float y, float scale = 1.f);`

- **Rôle** : fait apparaître une animation **centrée** sur un point.
- **Paramètres** :
  - `id` — identifiant d'animation enregistré dans `AnimationManager`.
  - `x`, `y` — centre voulu de l'effet, en pixels.
  - `scale` — échelle ; 1 = taille native de la frame.
- **Retour** : `void`.
- **Effets de bord** : ajoute un `Effect` au vecteur. Le recentrage est fait ici : `x - (frameW * scale) / 2`, `y - (frameH * scale) / 2`.
- **Pièges** : si l'animation est inconnue, la fonction **retourne silencieusement** (aucun log) — un id d'asset mal orthographié dans un mod ne produit rien de visible et rien de diagnostiquable. Le `push_back` peut réallouer le vecteur, mais les `Effect` sont copiables sans problème (le `const Animation*` interne reste valable). La variable locale `const Animation* animation;` est déclarée non initialisée puis assignée à la ligne suivante — inoffensif mais inutilement risqué.

#### `void triggerShake(float intensity, float duration);`

- **Rôle** : déclencher (ou renforcer) un tremblement d'écran.
- **Paramètres** :
  - `intensity` — amplitude en pixels (le décalage tiré sera dans `[-intensity, +intensity]`).
  - `duration` — durée en secondes.
- **Retour** : `void`.
- **Effets de bord** : prend le **maximum** entre la valeur demandée et la valeur en cours, pour chacun des deux champs indépendamment. Deux explosions rapprochées ne s'additionnent donc pas, la plus forte gagne.
- **Pièges** : intensité et durée étant maximisées séparément, un petit tremblement long suivi d'un gros tremblement court donne « gros et long ». Aucune borne supérieure : une valeur Lua aberrante secoue l'écran de façon arbitraire.

#### `int getShakeX() const {return shakeX;}`

- **Rôle** : décalage horizontal de caméra à appliquer au rendu de la frame courante.
- **Paramètres** : aucun.
- **Retour** : `shakeX`, en pixels ; 0 quand il n'y a pas de tremblement.
- **Effets de bord** : aucun (inline, `const`).

#### `int getShakeY() const {return shakeY;}`

- **Rôle** : décalage vertical de caméra.
- **Paramètres** : aucun.
- **Retour** : `shakeY`, en pixels ; 0 au repos.
- **Effets de bord** : aucun.

#### `void update(float dt);`

- **Rôle** : avancer toutes les animations, retirer celles terminées, et recalculer le décalage de tremblement.
- **Paramètres** : `dt` — temps écoulé en secondes (`Game::update` passe `realDeltaTime`).
- **Retour** : `void`.
- **Effets de bord** :
  - `update(dt)` sur chaque `Effect::player`.
  - `erase`/`remove_if` sur les effets dont `isFinished()` est vrai (idiome *erase-remove*, invalide tous les itérateurs/pointeurs sur le vecteur).
  - Si `shakeDuration > 0` : décrémente la durée de `dt`, fait décroître l'intensité de **20 unités par seconde** (`shakeIntensity -= 20.f * dt`, plancher 0), puis tire `shakeX`/`shakeY` dans `[-intensity, +intensity]` via `rand()`. Si l'intensité est tombée sous 1, les décalages sont remis à 0.
  - Sinon, remet `shakeX`, `shakeY` et `shakeIntensity` à 0.
- **Pièges** :
  - Une animation avec `loop = true` n'est **jamais** retirée (`isFinished()` reste faux) : le vecteur grossirait indéfiniment.
  - La décroissance de 20/s est codée en dur : une intensité de 10 px s'éteint en 0,5 s quelle que soit la `duration` demandée.
  - `rand()` sans `srand` dédié (le seed est posé par `Game::init`/`Game::start` via `srand(time(...))`) ; le tremblement n'est donc pas reproductible — sans conséquence car c'est purement visuel et local au client.
  - `int range = (int)shakeIntensity * 2 + 1;` : la conversion s'applique à `shakeIntensity` seul (précédence correcte), `range` vaut au moins 3 dès qu'on entre dans la branche, donc pas de `% 0`.

#### `void render() const;`

- **Rôle** : dessiner tous les effets actifs.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : écrit dans le back buffer via `AnimationPlayer::render(x, y, scale)`.
- **Pièges** : les effets sont dessinés dans l'ordre d'insertion (le plus ancien derrière). L'argument `angle` n'est pas transmis. Le décalage de tremblement n'est **pas** appliqué aux effets : c'est `Game::renderSnapshot` qui l'applique aux autres éléments, les effets restent donc fixes pendant que la scène tremble.

**Code mort / non utilisé dans ce fichier**
- Aucune méthode morte : `spawn`, `triggerShake`, `getShakeX`, `getShakeY`, `update`, `render` sont tous appelés par `Game`.
- `#include <cstdint>` et `<algorithm>` : `algorithm` sert (`remove_if`), `cstdint` est inutile. `rand()` est utilisé sans inclure `<cstdlib>` (il arrive indirectement).
- Le header déclare `std::string` via `Animation.hpp` sans inclure `<string>` directement.

---

## include/Particle.hpp + src/Particle.cpp

### `struct ColorKeyframe`

Point de contrôle d'un dégradé de couleur dans le temps, lu depuis un fichier `.ini`.

| Membre | Type | Rôle |
|---|---|---|
| `r`, `g`, `b`, `a` | `Uint8` | Composantes de couleur et opacité, 0-255. |
| `time` | `int` | Instant de ce point de contrôle, en **millisecondes** depuis la naissance de la particule. |

### `class ParticleConfig`

Jeu de paramètres d'un type de particule, chargé depuis un `GameConfig`. Précalcule une **table de couleurs milliseconde par milliseconde** (`colorFrameList`) à partir des keyframes, pour que le rendu soit une simple indexation.

| Membre | Type | Rôle |
|---|---|---|
| `growRate` | `float` (public) | Pixels ajoutés à la taille **par appel d'`update`** (pas par seconde). |
| `friction` | `float` (public) | Multiplicateur appliqué à `vx` à chaque `update` (0,98 = -2 %/frame). |
| `riseForce` | `float` (public) | Valeur retirée de `vy` à chaque `update` (accélération vers le haut, en px/frame²). |
| `vxSpread` | `int` (public) | Dispersion horizontale initiale ; `vx` est tiré dans `[-vxSpread, vxSpread] / 100` px/frame. |
| `vyMin` | `int` (public) | Borne basse de la vitesse verticale initiale (centièmes de pixel/frame). |
| `vyMax` | `int` (public) | Borne haute (idem). |
| `sizeMin` | `int` (public) | Taille initiale minimale, en pixels (côté du carré). |
| `sizeMax` | `int` (public) | Taille initiale maximale, en pixels. |
| `keyframes` | `std::vector<ColorKeyframe>` (privé) | Points de contrôle lus du fichier, dans l'ordre du fichier. |
| `colorFrameList` | `std::vector<SDL_Color>` (privé) | Table précalculée : une couleur interpolée par milliseconde, de 0 à `keyframes.back().time - 1`. |

#### `void load(GameConfig& config);`

- **Rôle** : remplir toute la configuration depuis un fichier `.ini` déjà parsé.
- **Paramètres** : `config` — `GameConfig` ouvert sur `assets/playerThrustParticle.ini` (passé par référence non const car les accesseurs de `GameConfig` ne sont pas `const`).
- **Retour** : `void` ; aucune indication d'échec.
- **Effets de bord** :
  - Lit `particle_color_count` (défaut 0) puis `particle_color_0` … `particle_color_{n-1}`, chacune au format `r,g,b,a,time` (défaut `"255,255,255,255,500"`), parsées par `sscanf("%hhu,%hhu,%hhu,%hhu,%d")`.
  - `keyframes.clear()` avant remplissage.
  - Lit `growRate`, `friction`, `riseForce` (float, défaut 0), `vxSpread`, `vyMin`, `vyMax` (int, défaut 0), `sizeMin`, `sizeMax` (int, défaut 10).
  - Appelle `setColorFrameList()` puis `printf("Color frame list size : %i\n", ...)`.
- **Pièges** :
  - **Si `particle_color_count` vaut 0 ou est absent**, `keyframes` reste vide et `setColorFrameList()` appelle `keyframes.back()` sur un vecteur vide → **comportement indéfini** (lecture hors limites, crash probable). Aucune garde.
  - La valeur de retour de `sscanf` n'est pas vérifiée : une ligne malformée laisse des champs de la keyframe non initialisés.
  - `printf("%i", colorFrameList.size())` passe un `size_t` pour un `%i` → format incorrect (fonctionne par chance sur x86-64 en pratique, mais c'est de l'UB ; `%zu` serait correct).
  - **`colorFrameList` n'est pas vidée** (voir `setColorFrameList`) : rappeler `load()` sur le même objet accumule les couleurs.

#### `void setColorFrameList();` *(privé)*

- **Rôle** : interpoler linéairement les keyframes pour produire une couleur par milliseconde.
- **Paramètres** : aucun (travaille sur `keyframes`).
- **Retour** : `void`.
- **Effets de bord** : `push_back` dans `colorFrameList` pour chaque `t` de 0 à `keyframes.back().time` inclus, en cherchant l'intervalle `[a.time, b.time)` qui contient `t` et en interpolant `r,g,b,a` avec `pct = (t - a.time) / (b.time - a.time)`. Affiche `printf("Table size: %i\n", ...)`.
- **Pièges** :
  - `keyframes.back()` sur vecteur vide → UB (voir `load`).
  - **Pas de `colorFrameList.clear()`** en entrée : non idempotent.
  - Le dernier `t` (`t == totalTime`) ne tombe dans aucun intervalle `[a, b)` : aucune couleur n'est poussée pour lui. La table contient donc exactement `totalTime` entrées (indices 0..`totalTime-1`), ce qui est cohérent avec `getMaxTime()` qui renvoie `totalTime - 1`. L'équilibre est **juste** : `intCurrentTime` peut valoir au maximum `totalTime - 1`, soit le dernier index valide. Toute modification de l'une des deux fonctions casse l'autre.
  - Suppose que les keyframes sont **triées par `time` croissant** ; rien ne le vérifie. Un fichier désordonné produit des trous (couleurs manquantes → table plus courte → indexation hors limites au rendu).
  - Coût mémoire : 4 octets × `totalTime` (800 ms → 3,2 Ko) par instance de `ParticleConfig`… et cette config est **copiée dans chaque particule** (voir `ThrustParticle`).
  - Boucle en O(totalTime × nbKeyframes) : négligeable au chargement.

#### `int getMaxTime() const;`

- **Rôle** : durée de vie maximale exploitable, en millisecondes.
- **Paramètres** : aucun.
- **Retour** : `keyframes.back().time - 1` (799 avec le fichier fourni), c'est-à-dire le dernier index valide de `colorFrameList`.
- **Effets de bord** : aucun.
- **Pièges** : UB si `keyframes` est vide. Le `-1` est l'ajustement qui garantit qu'`intCurrentTime` reste dans les bornes de la table.

#### `SDL_Color getCurrentColor(int currentTime);`

- **Rôle** : lire la couleur précalculée à un instant donné.
- **Paramètres** : `currentTime` — âge de la particule en millisecondes, entier.
- **Retour** : la `SDL_Color` à cet instant.
- **Effets de bord** : aucun.
- **Pièges** : `colorFrameList[currentTime]` **sans aucune vérification de bornes** (`operator[]`, pas `at`). La sécurité repose entièrement sur la cohérence entre `getMaxTime()` et la taille réelle de la table. Un fichier `.ini` aux keyframes non triées, ou un seul keyframe, fait sortir de la table. La méthode n'est pas `const` alors qu'elle ne modifie rien (ce qui oblige `ThrustParticle::render` à garder une copie non const).

### `class Particle`

Classe de base abstraite d'une particule : position flottante doublée d'un `SDL_Rect` entier pour le dessin, taille, âge. L'interface impose `update`/`render`.

| Membre | Type | Rôle |
|---|---|---|
| `isAlive` | `bool` = false (public) | Drapeau de vie ; lu par `ParticleManager` pour sauter les particules mortes. |
| `particleRect` | `SDL_Rect` (protégé) | Rectangle de dessin : position entière + côté. Non initialisé par la classe de base. |
| `size` | `float` = 10 (protégé) | Côté courant en pixels, en flottant pour permettre une croissance sous-pixel. |
| `fx`, `fy` | `float` (protégé) | Position exacte en pixels (non initialisées avant `setPos`). |
| `lifeTime` | `float` = 0 (protégé) | Âge en millisecondes (accumulé en flottant). |
| `maxLifeTime` | `float` (protégé) | Durée de vie en ms ; **non initialisée** par la classe de base. |
| `intCurrentTime` | `int` = 0 (protégé) | Âge arrondi en ms, sert d'index dans `colorFrameList`. |

#### `void setPos(int posX, int posY);`

- **Rôle** : placer la particule.
- **Paramètres** : `posX`, `posY` — coin haut-gauche en pixels.
- **Retour** : `void`.
- **Effets de bord** : écrit `particleRect.x/y` **et** synchronise `fx`/`fy` (indispensable, sinon l'`update` repartirait de l'ancienne position flottante).
- **Pièges** : doit être appelé après `init()` (ce que fait `ParticleManager::spawnThrustParticle`), car `init()` repositionne le rectangle à (10000, 10000).

#### `int getX();`

- **Rôle** : abscisse entière courante.
- **Paramètres** : aucun.
- **Retour** : `particleRect.x`, en pixels.
- **Effets de bord** : aucun.
- **Pièges** : non `const` ; **aucun appelant** (code mort — les `getX()` trouvés ailleurs appartiennent à `player::Player`).

#### `int getY();`

- **Rôle** : ordonnée entière courante.
- **Paramètres** : aucun.
- **Retour** : `particleRect.y`, en pixels.
- **Effets de bord** : aucun.
- **Pièges** : non `const` ; aucun appelant (code mort).

#### `virtual void render(SDL_Renderer* renderer) = 0;`

- **Rôle** : contrat de dessin, implémenté par les classes dérivées.
- **Paramètres** : `renderer` — renderer cible (passé explicitement, contrairement à `LTexture` qui le mémorise).
- **Retour** : `void`.
- **Pièges** : **la classe de base n'a pas de destructeur virtuel** alors qu'elle a des fonctions virtuelles. Aujourd'hui sans conséquence (le pool est un tableau de `ThrustParticle` concrets, jamais détruits via `Particle*`), mais toute allocation polymorphe future fuirait.

#### `virtual void update(float deltaTime) = 0;`

- **Rôle** : contrat de simulation.
- **Paramètres** : `deltaTime` — en **secondes** (converti en ms dans l'implémentation).
- **Retour** : `void`.

### `class ThrustParticle`

Particule de propulseur : un carré plein qui monte, ralentit horizontalement, grossit et change de couleur selon la table précalculée. Unique implémentation concrète de `Particle`.

| Membre | Type | Rôle |
|---|---|---|
| `vx` | `float` = 0 | Vitesse horizontale, en **pixels par appel d'`update`** (pas par seconde). |
| `vy` | `float` = 0 | Vitesse verticale, même unité ; positive = vers le bas (axe SDL). |
| `growRate` | `float` | Copie depuis la config : pixels ajoutés par `update`. |
| `friction` | `float` | Copie : multiplicateur de `vx` par `update`. |
| `riseForce` | `float` | Copie : soustrait à `vy` à chaque `update`. |
| `vyMin`, `vyMax` | `float` | Copies des bornes de vitesse initiale — **écrites dans `init()` et jamais relues** (code mort). |
| `config` | `ParticleConfig` | **Copie complète** de la configuration, y compris les deux vecteurs de keyframes et la table de 800 couleurs. Sert uniquement à `getCurrentColor` au rendu. |

#### `void init(const ParticleConfig& config);`

Définition : `void ThrustParticle::init(const ParticleConfig& particleConfig)`.

- **Rôle** : (ré)initialiser une particule du pool au moment du spawn : tirer ses paramètres aléatoires et la marquer vivante.
- **Paramètres** : `particleConfig` — configuration source, copiée dans le membre `config`.
- **Retour** : `void`.
- **Effets de bord** :
  - Place le rectangle en (10000, 10000) 10×10 px — hors écran, pour qu'une particule mal positionnée ne soit pas visible.
  - Copie `friction`, `growRate`, `riseForce`, `vyMin`, `vyMax`.
  - `maxLifeTime = getMaxTime()` (799 ms avec le fichier fourni), `isAlive = true`, `lifeTime = 0`, `intCurrentTime = 0`.
  - `vx` : si `vxSpread != 0`, tiré dans `[-vxSpread, vxSpread[ / 100` px/frame (avec `vxSpread = 100` → ±1 px/frame).
  - `vy` : si `vyMin != vyMax`, tiré dans `[vyMin, vyMax[ / 100` px/frame ; **sinon `vy = vyMax` sans division par 100** (incohérence d'unité : avec `vyMin == vyMax == 5` la particule partirait à 5 px/frame au lieu de 0,05).
  - `size` : tiré dans `[sizeMin, sizeMax[` px, ou `sizeMax` si les deux sont égaux ; `particleRect.w/h` suivent.
  - `config = particleConfig` **en dernier**.
- **Pièges** :
  - **Coût caché majeur** : `config = particleConfig` copie les deux `std::vector` (≈3,2 Ko + keyframes) → **une allocation tas à chaque spawn de particule**, et le pool de 2000 particules détient 2000 copies de la même table (plusieurs mégaoctets). C'est le premier candidat à l'optimisation : remplacer le membre `config` par un `const ParticleConfig*`.
  - Si `vx` n'est pas retiré (cas `vxSpread == 0`), il **garde la valeur de la vie précédente** de cette case du pool (le membre n'est pas remis à 0) — même remarque pour `vy` dans sa branche. Pool recyclé = état résiduel.
  - `rand() % (vyMax - vyMin)` suppose `vyMax > vyMin` ; l'inverse donne un modulo négatif → UB/valeurs absurdes. Idem pour `sizeMax`/`sizeMin`.
  - Les bornes supérieures sont exclusives (`rand() %`), donc `sizeMax` n'est jamais atteint.

#### `void update(float deltaTime) override;`

- **Rôle** : avancer la particule d'un pas, et la tuer en fin de vie.
- **Paramètres** : `deltaTime` — en **secondes** ; converti en ms par `deltaTime * 1000.0f`.
- **Retour** : `void`.
- **Effets de bord** :
  - Sort immédiatement si `!isAlive`.
  - Accumule `lifeTime += deltaTime * 1000` ; si l'âge dépasse `maxLifeTime`, met `isAlive = false` et sort.
  - `intCurrentTime = (int)lifeTime` (index de couleur).
  - `fx += vx; fy += vy;` puis recopie dans `particleRect.x/y` (troncature).
  - `vy -= riseForce` (poussée vers le haut), `vx *= friction`.
  - `size += growRate`, `particleRect.w/h = (int)size`.
- **Pièges** :
  - **La physique n'est pas proportionnelle à `deltaTime`** : seul l'âge l'est. Les déplacements, la friction et la croissance sont par appel, donc la forme du panache dépend du framerate. Comme `main.cpp` fait un `SDL_Delay(16)` fixe, c'est stable en pratique, mais ça casserait avec une autre cadence — et côté réseau ça ne peut pas être reproductible.
  - La condition d'arrêt `!((lifeTime += ...) <= maxLifeTime)` incrémente dans le test (effet de bord dans la condition) : correct mais peu lisible. Elle garantit `lifeTime <= maxLifeTime` quand on continue, donc `intCurrentTime <= 799` : c'est **la** garde qui empêche `getCurrentColor` de déborder.
  - `size` peut devenir négative si `growRate` était négatif → `SDL_Rect` avec `w < 0`, non dessiné par SDL.
  - Le `;` après l'accolade du `if` (ligne 124) est inoffensif.

#### `void render(SDL_Renderer* renderer) override;`

- **Rôle** : dessiner la particule comme un rectangle plein coloré.
- **Paramètres** : `renderer` — renderer cible.
- **Retour** : `void`.
- **Effets de bord** : sort si `!isAlive`. Sinon appelle `SDL_SetRenderDrawColor` (**modifie la couleur de dessin globale du renderer**, qui reste en place pour les appels suivants) puis `SDL_RenderFillRect`.
- **Pièges** :
  - L'alpha n'a d'effet que si le blend mode du renderer est `SDL_BLENDMODE_BLEND` — c'est `Game::start()` qui l'active (`SDL_SetRenderDrawBlendMode`). Sans cet appel, les particules seraient opaques.
  - Modifie l'état du renderer sans le restaurer : tout code qui dessine après et oublie de refixer sa couleur héritera de la dernière couleur de particule.
  - `config.getCurrentColor` n'étant pas `const`, `render` ne peut pas être `const` non plus.
  - Aucune vérification de `renderer != nullptr`.

**Code mort / non utilisé dans ce fichier**
- `Particle::getX()` et `Particle::getY()` : aucun appelant.
- `ThrustParticle::vyMin` et `vyMax` : écrits par `init()`, jamais relus (les bornes ne servent qu'au tirage, qui lit directement `particleConfig`).
- `ThrustParticle::growRate`, `friction`, `riseForce` sont dupliqués : ils existent déjà dans le membre `config` copié. Les trois copies locales sont donc redondantes (elles évitent une indirection, mais la config entière est de toute façon copiée).
- `Particle` n'a pas de destructeur virtuel malgré ses méthodes virtuelles.
- `#include <vector>` et `"Config.hpp"` dans le header sont bien nécessaires ; en revanche `Particle.cpp` utilise `rand`, `sscanf` et `printf` sans inclure `<cstdlib>`/`<cstdio>` (ils arrivent indirectement par `Config.hpp`/SDL).

---

## include/ParticleManager.hpp + src/ParticleManager.cpp

### `class ParticleManager`

Pool circulaire de taille fixe pour les particules de propulseur : aucune allocation dynamique à l'exécution, la particule la plus ancienne est écrasée quand le pool fait le tour. `Game` en possède une instance et la publie dans le `GameContext` pour que Lua et `Player` puissent y pousser des particules.

| Membre | Type | Rôle |
|---|---|---|
| `MAX_THRUST_PARTICLES` | `static constexpr int` = 2000 | Capacité du pool. |
| `thrustPool` | `ThrustParticle[2000]` | Le pool lui-même, stocké **par valeur** dans l'objet. |
| `currentThrustIdx` | `int` | Index d'écriture suivant (curseur circulaire). |

#### `ParticleManager();`

- **Rôle** : initialiser le curseur.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : `currentThrustIdx = 0`. Les 2000 `ThrustParticle` sont construites par défaut (donc `isAlive == false`, et leur membre `config` est un `ParticleConfig` vide).
- **Pièges** : l'objet est **très volumineux** : `sizeof(ThrustParticle)` inclut un `ParticleConfig` complet (deux `std::vector`, 48 octets d'en-têtes) plus les scalaires, soit ~2000 × ~130 octets de structure, auxquels s'ajoutent les tables de couleurs allouées sur le tas dès qu'une particule est initialisée (3,2 Ko chacune → **jusqu'à ~6,4 Mo de doublons**). `Game` le contient par valeur, et `Game` est alloué avec `new` dans `GameScene` : pas de dépassement de pile, mais à ne pas déclarer sur la pile.

#### `void update(float deltaTime);`

- **Rôle** : simuler toutes les particules vivantes.
- **Paramètres** : `deltaTime` — en secondes (`Game::update` passe `realDeltaTime`).
- **Retour** : `void`.
- **Effets de bord** : appelle `update` sur chaque case dont `isAlive` est vrai.
- **Pièges** : parcourt systématiquement les 2000 cases même si aucune n'est vivante (coût fixe négligeable, mais c'est un parcours de plusieurs centaines de Ko peu amical pour le cache). Le test `isAlive` est redondant avec celui à l'intérieur d'`update`.

#### `void render(SDL_Renderer* renderer);`

- **Rôle** : dessiner les particules vivantes.
- **Paramètres** : `renderer` — renderer cible.
- **Retour** : `void`.
- **Effets de bord** : dessins dans le back buffer ; laisse la couleur de dessin du renderer modifiée (cf. `ThrustParticle::render`).
- **Pièges** : l'index est calculé à l'envers — `orderedIndex = (currentThrustIdx - 1 - i + MAX) % MAX` — donc le parcours part de la **particule la plus récente** vers la plus ancienne. Conséquence : les plus anciennes sont dessinées **en dernier**, donc **par-dessus** les récentes. Si l'intention était « les récentes au-dessus », la boucle est inversée. Le `+ MAX_THRUST_PARTICLES` évite le modulo négatif, correctement.

#### `void spawnThrustParticle(int x, int y, const ParticleConfig& config);`

- **Rôle** : faire naître une particule à une position donnée.
- **Paramètres** :
  - `x`, `y` — position en pixels (coin haut-gauche ; `Player` passe `x + 5, y + 25` pour viser la sortie du jetpack).
  - `config` — paramètres du type de particule.
- **Retour** : `void` ; aucun handle rendu, donc aucun moyen de retrouver la particule ensuite.
- **Effets de bord** : écrase la case `currentThrustIdx` (`init()` puis `setPos()`), puis avance le curseur modulo 2000.
- **Pièges** : **aucun test de `isAlive`** — une particule encore vivante est écrasée sans ménagement quand le pool fait le tour. Avec un spawn toutes les 20 ms par joueur (`Player::thrustParticlesTimer`) et une durée de vie de 800 ms, on consomme ~40 cases par joueur, donc 2000 est largement suffisant ; le recyclage prématuré n'arriverait qu'avec une cadence beaucoup plus élevée. Chaque appel **copie la configuration entière** (allocation tas) via `init()`.

**Code mort / non utilisé dans ce fichier**
- Aucune méthode morte : `update`, `render` et `spawnThrustParticle` sont appelés (respectivement par `Game::update`, `Game::renderSnapshot` et `Player::update`).
- Il n'existe pas de `clear()`/`reset()` : au retour au menu, les particules du pool restent « vivantes » en mémoire, mais comme un nouveau `Game` (et donc un nouveau `ParticleManager`) est créé à chaque partie, ce n'est pas visible — au prix de la fuite du `Game` précédent.
- Le pool est codé en dur pour un seul type de particule : la hiérarchie `Particle`/`ThrustParticle` polymorphe n'est donc pas exploitée (pas de pool générique).

---

## include/Config.hpp + src/Config.cpp

### `class GameConfig`

Parseur de fichier `.ini` minimaliste : lit tout le fichier au constructeur via les I/O SDL (`SDL_RWops`, donc portable Android/Windows), le découpe en paires `clé=valeur` et les expose avec conversion typée et valeur par défaut. Deux instances existent dans `Game` : une pour `assets/config.ini`, une pour `assets/playerThrustParticle.ini`.

| Membre | Type | Rôle |
|---|---|---|
| `values` | `std::unordered_map<std::string, std::string>` | Toutes les paires du fichier, valeurs conservées **en texte brut** (converties à la demande). |

#### `GameConfig(const std::string& filename = "config.ini");`

- **Rôle** : charger et parser un fichier de configuration.
- **Paramètres** : `filename` — chemin du fichier, relatif au répertoire de travail. Le défaut `"config.ini"` (sans `assets/`) n'est utilisé par personne : les deux appelants passent le chemin complet.
- **Retour** : — (constructeur). **Aucun signalement d'échec** : si le fichier est absent, un `printf("Cannot load config file ...")` est émis et l'objet reste simplement vide, tous les accesseurs renverront donc les valeurs par défaut. C'est un comportement de repli acceptable mais silencieux à l'exécution.
- **Effets de bord** :
  - `SDL_RWFromFile(filename, "r")`, `SDL_RWsize`, `SDL_RWread`, `SDL_RWclose` — le handle est bien fermé sur le chemin normal.
  - Allocation `new char[size + 1]` puis `delete[]` : pas de fuite sur le chemin normal.
  - Remplit `values`.
- **Parsing** : boucle sur les `'\n'` ; ignore les lignes vides et celles commençant par `#` ou `;` ; coupe à la **première** `=` ; retire les espaces et tabulations en tête et en queue de la clé et de la valeur.
- **Pièges** :
  - **La dernière ligne est ignorée si le fichier ne finit pas par un retour à la ligne** (la condition de boucle exige un `'\n'`). Les deux fichiers du projet se terminent bien par `\n`, mais une édition malencontreuse ferait disparaître la dernière clé en silence.
  - **Les fins de ligne Windows (`\r\n`) ne sont pas gérées** : le `\r` reste collé à la valeur (il n'est pas dans l'ensemble `" \t"` du trim), et `std::stoi`/`std::stof` s'arrêteraient avant — mais `getBool` comparerait `"true\r"` à `"true"` et renverrait `false`. Piège réel si le fichier est édité sous Windows ou via Git avec `core.autocrlf`.
  - **Les sections `[xxx]` ne sont pas supportées** : une telle ligne ne contient pas de `=` et est donc ignorée, sans erreur — les clés de sections différentes se retrouveraient à plat et s'écraseraient.
  - Si `SDL_RWsize` renvoie une valeur négative (erreur), `new char[size + 1]` a un argument négatif converti en `size_t` géant → `std::bad_alloc` ou `std::length_error` non attrapé.
  - Le résultat de `SDL_RWread` n'est pas vérifié : une lecture partielle laisse des octets non initialisés avant le `'\0'` final.
  - Le parsing détruit le contenu en place par `content.erase(0, pos+1)` : O(n²) sur la taille du fichier. Sans importance pour 10 lignes.

#### `int getInt(const std::string& key, int defaultVal);`

- **Rôle** : lire une clé comme entier.
- **Paramètres** : `key` — nom de la clé ; `defaultVal` — valeur rendue si la clé est absente.
- **Retour** : `std::stoi(valeur)` si la clé existe, sinon `defaultVal`.
- **Effets de bord** : aucun.
- **Pièges** : `std::stoi` **lève** `std::invalid_argument` si la valeur ne commence pas par un nombre, et `std::out_of_range` si elle dépasse `int` — aucune des deux n'est attrapée, donc une faute de frappe dans le `.ini` fait planter le jeu au lancement. En revanche `std::stoi("1000.0f")` renvoie 1000 sans broncher (il s'arrête au `.`), ce qui explique que les suffixes `f` du fichier ne posent pas problème. La fonction n'est pas `const`, d'où le `GameConfig&` non const dans `ParticleConfig::load`.

#### `float getFloat(const std::string& key, float defaultVal);`

- **Rôle** : lire une clé comme flottant.
- **Paramètres** : `key` ; `defaultVal` — rendu si la clé est absente.
- **Retour** : `std::stof(valeur)` ou `defaultVal`.
- **Effets de bord** : aucun.
- **Pièges** : mêmes exceptions non attrapées que `getInt`. `std::stof("700.0f")` vaut 700.0 (le suffixe `f` est ignoré car `strtof` s'arrête là) — c'est pourquoi le style `700.0f` dans le `.ini` fonctionne, mais c'est fragile et non standard pour un fichier INI. Pas `const`.

#### `bool getBool(const std::string& key, bool defaultVal);`

- **Rôle** : lire une clé comme booléen tolérant.
- **Paramètres** : `key` ; `defaultVal` — rendu si la clé est absente.
- **Retour** : si la clé existe, `true` pour `"true"`, `"1"`, `"yes"` ou `"on"` (comparaison insensible à la casse grâce à un passage en minuscules), **`false` pour tout le reste** (y compris une valeur inconnue). Si la clé est absente, `defaultVal`.
- **Effets de bord** : aucun (la mise en minuscules se fait sur une copie locale).
- **Pièges** : une valeur mal orthographiée (`"vrai"`, `"True "` avec espace déjà trimé, `"true\r"`) donne `false` au lieu du défaut — pas d'erreur signalée. `tolower(char)` reçoit un `char` potentiellement signé : UB pour les caractères non ASCII (en pratique sans conséquence ici). Pas `const`.

#### `std::string getString(const std::string& key, const std::string& defaultVal);`

- **Rôle** : lire une clé comme texte brut.
- **Paramètres** : `key` ; `defaultVal` — chaîne rendue si la clé est absente.
- **Retour** : la valeur trimée, ou `defaultVal`.
- **Effets de bord** : aucun ; renvoie une copie.
- **Pièges** : pas `const`. Seul utilisateur : `ParticleConfig::load` pour les lignes `particle_color_N`.

#### `bool save();`

- **Rôle** : réécrire les paires clé/valeur en mémoire dans un fichier.
- **Paramètres** : aucun.
- **Retour** : `false` si `SDL_RWFromFile(..., "w")` échoue, `true` sinon (le résultat de `SDL_RWwrite` n'est pas vérifié).
- **Effets de bord** : affiche `printf("Saving %zu keys")` puis chaque paire sur `stdout` ; **écrit systématiquement dans `"assets/config.ini"`**, chemin codé en dur, en écrasant le fichier.
- **Pièges** :
  - **Danger réel** : le chemin de sortie ignore le `filename` passé au constructeur. Appeler `save()` sur `mThrustParticleGameConfig` écraserait `assets/config.ini` avec les paramètres de particules.
  - La réécriture **perd les commentaires, les lignes vides et l'ordre des clés** (`unordered_map` → ordre arbitraire).
  - Pas d'écriture atomique (pas de fichier temporaire + renommage) : une interruption en cours d'écriture laisse un `config.ini` tronqué.
  - Il n'existe aucun `set*()` : la map n'est jamais modifiée après le chargement, donc `save()` ne pourrait au mieux que réécrire à l'identique. **Aucun appelant** (code mort).

**Code mort / non utilisé dans ce fichier**
- `save()` : aucun appelant ; de plus inutilisable tel quel (pas de mutateur, chemin de sortie codé en dur).
- Aucun accesseur n'est `const`, ce qui contamine les signatures appelantes (`ParticleConfig::load(GameConfig&)`).
- Includes superflus dans le header : `<vector>` n'est pas utilisé ; `<cstdio>` sert pour `printf`.
- Les clés `SCREEN_WIDTH`, `SCREEN_HEIGHT` et `PLAYER_NUMBER` sont lues par `Game::Game()` mais **absentes du fichier** et de toute façon écrasées juste après par `SDL_RenderGetLogicalSize` et par `joinedCount` : lectures mortes.

---

## include/Scene.hpp

### `class Scene`

Interface abstraite d'un écran de jeu. Trois méthodes forment le cycle de vie appelé par `main` (événement → mise à jour → rendu), la quatrième sert à signaler une fin. Les scènes sont détenues par `SceneManager` via `std::unique_ptr<Scene>`.

Aucun membre de données.

#### `virtual ~Scene() = default;`

- **Rôle** : destructeur virtuel, indispensable pour que `std::unique_ptr<Scene>` détruise correctement la scène concrète.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : appelle le destructeur de la classe dérivée.
- **Pièges** : c'est le point qui fait fonctionner la propriété polymorphe du `SceneManager` ; `MenuScene` s'appuie dessus pour fermer sa police TTF. `GameScene`, lui, n'implémente aucun destructeur et fuit son `Game` (voir plus bas).

#### `virtual void handleEvent(const SDL_Event& e) = 0;`

- **Rôle** : traiter un événement SDL brut.
- **Paramètres** : `e` — événement dépilé par `SDL_PollEvent` dans `main` ; passé par référence const, donc la scène ne peut pas le consommer/modifier.
- **Retour** : `void`.
- **Pièges** : **tous** les événements sont transmis à la scène du **sommet** de la pile uniquement. Il n'y a pas de notion d'événement « consommé » : une scène ne peut pas empêcher le traitement par une autre (de toute façon une seule reçoit). Danger majeur : une scène qui se dépile elle-même (`mManager.pop()`) depuis `handleEvent` est **détruite pendant l'exécution de sa propre méthode** — voir les pièges de `MenuScene::handleEvent`.

#### `virtual void update(float deltaTime) = 0;`

- **Rôle** : avancer la logique de la scène.
- **Paramètres** : `deltaTime` — temps écoulé depuis la frame précédente, en **secondes**, plafonné à 0,05 s par `main`.
- **Retour** : `void`.
- **Pièges** : même problème de destruction depuis l'intérieur (`GameScene::update` dépile la scène).

#### `virtual void render() = 0;`

- **Rôle** : dessiner la scène.
- **Paramètres** : aucun — la scène doit avoir mémorisé son renderer.
- **Retour** : `void`.
- **Pièges** : chaque scène est responsable de son propre `SDL_RenderClear` **et** de son `SDL_RenderPresent` (`MenuScene` fait les deux, `Game::render` aussi). Une pile de deux scènes visibles n'est donc pas possible sans refonte : la scène du dessous ne dessine pas, et la scène du dessus présente immédiatement.

#### `virtual bool isDone() = 0;`

- **Rôle** : signaler que la scène souhaite être retirée.
- **Paramètres** : aucun.
- **Retour** : `bool`.
- **Pièges** : **aucun appelant dans tout le projet** — ni `main`, ni `SceneManager`. Les deux scènes implémentent la méthode et maintiennent un drapeau `mDone`, mais personne ne le consulte ; la sortie se fait en réalité par des appels directs à `mManager.pop()`. Méthode d'interface morte : soit `main` devrait la consulter après `update`, soit elle devrait disparaître avec les `mDone`.

**Code mort / non utilisé dans ce fichier**
- `isDone()` : contrat jamais interrogé (voir ci-dessus).
- Pas de `virtual void onEnter()/onExit()` : il n'y a aucun hook pour qu'une scène sache qu'elle redevient active quand la scène du dessus est dépilée. C'est pour cela que `GameScene::update` doit couper les sons à la main avant de se dépiler.

---

## include/SceneManager.hpp + src/SceneManager.cpp

### `class SceneManager`

Pile de scènes propriétaire. Le menu est au fond, la partie se pousse par-dessus ; `main` boucle tant que la pile n'est pas vide et ne pilote que le sommet.

| Membre | Type | Rôle |
|---|---|---|
| `mStack` | `std::stack<std::unique_ptr<Scene>>` | Les scènes, la dernière poussée étant active. Propriété exclusive. |

#### `void change(std::unique_ptr<Scene> scene);`

- **Rôle** : remplacer la scène du sommet par une autre (transition sans empilement).
- **Paramètres** : `scene` — scène transférée par `unique_ptr` (prise de possession).
- **Retour** : `void`.
- **Effets de bord** : dépile (donc **détruit**) la scène courante si la pile n'est pas vide, puis empile la nouvelle.
- **Pièges** : la nouvelle scène a déjà été **construite** par l'appelant avant que l'ancienne soit détruite ; les deux coexistent brièvement (deux renderers, deux polices…). Si l'ancienne scène appelle `change` depuis une de ses propres méthodes, elle se suicide en pleine exécution. **Aucun appelant** (code mort).

#### `void push(std::unique_ptr<Scene> scene);`

- **Rôle** : empiler une scène par-dessus la courante, qui est conservée en dessous.
- **Paramètres** : `scene` — prise de possession.
- **Retour** : `void`.
- **Effets de bord** : `mStack.push(std::move(scene))`.
- **Pièges** : la scène du dessous n'est plus ni mise à jour ni dessinée (seul le sommet est piloté par `main`) mais reste vivante avec toutes ses ressources — c'est exactement ce qui permet au `MenuScene` de retrouver ses slots de joueurs au retour de partie. Aucune limite de profondeur ; appeler `push` depuis `handleEvent` de la scène courante est sûr (la scène courante survit), contrairement à `pop`.

#### `void pop();`

- **Rôle** : retirer et détruire la scène du sommet.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : détruit la scène du sommet si la pile n'est pas vide (no-op sinon).
- **Pièges** : **piège central du projet** — appeler `pop()` depuis une méthode de la scène du sommet détruit `*this` au milieu de l'appel ; tout accès à un membre après ce point est un *use-after-free*. `MenuScene::handleEvent` le fait (`pop()` puis `mDone = true`). Après un `pop()` déclenché depuis `update()`, `main` enchaîne sur `manager.current()->render()` **sans revérifier la nullité** : si la pile se vide à ce moment, c'est un déréférencement de `nullptr`.

#### `Scene* current();`

- **Rôle** : accès à la scène active.
- **Paramètres** : aucun.
- **Retour** : pointeur **emprunté** sur la scène du sommet, ou `nullptr` si la pile est vide.
- **Effets de bord** : aucun.
- **Pièges** : le pointeur est invalidé par `pop`/`change`. `main` le rappelle à chaque usage, ce qui limite le risque, sauf entre `update()` et `render()` (lignes 93-94 de `main.cpp`).

#### `bool isEmpty();`

- **Rôle** : savoir s'il reste une scène (condition de la boucle principale).
- **Paramètres** : aucun.
- **Retour** : `mStack.empty()`.
- **Effets de bord** : aucun.
- **Pièges** : non `const`. Vider la pile est le mécanisme de sortie du jeu : `SDL_QUIT` boucle sur `pop()` jusqu'à ce que `isEmpty()` soit vrai, et `ESC` dans le menu dépile la dernière scène.

**Code mort / non utilisé dans ce fichier**
- `change()` : aucun appelant. La navigation du jeu se fait uniquement par `push` (menu → partie) et `pop` (retour/sortie).
- Aucune méthode n'est `const`.
- Pas de garde contre le `pop()` réentrant (une file de commandes différées — « pop à la fin de la frame » — serait la correction habituelle).

---

## include/MenuScene.hpp + src/MenuScene.cpp

### `class MenuScene`

Écran de sélection des joueurs : jusqu'à 4 colonnes, chacune occupée dès qu'un périphérique inconnu (clavier selon un preset, ou manette) produit une entrée. Chaque joueur navigue dans un petit menu vertical pour choisir chapeau et skin, se déclare prêt, et quand tous le sont un compte à rebours de 5 s lance la partie.

| Membre | Type | Rôle |
|---|---|---|
| `mRenderer` | `SDL_Renderer*` = nullptr | Renderer, transmis aux dessins et à `GameScene`. |
| `mManager` | `SceneManager&` | Référence sur le gestionnaire de pile (pour `push`/`pop`). Membre référence ⇒ la classe n'est ni copiable ni assignable. |
| `mWindow` | `SDL_Window*` | Fenêtre, transmise telle quelle à `GameScene`/`Game`. |
| `mFont` | `TTF_Font*` = nullptr | Police `assets/KiwiSoda.ttf` taille 30, possédée (fermée au destructeur). |
| `mDone` | `bool` = false | Drapeau de fin, jamais consulté (voir code mort). |
| `playBtnHitbox` | `SDL_Rect` | Zone de clic du bouton « play », calculée mais jamais utilisée. |
| `mScreenWidth`, `mScreenHeight` | `int` | 1024 et 576, **codés en dur** dans le constructeur (doivent coïncider avec le `SDL_RenderSetLogicalSize` de `main`). |
| `mSlots[4]` | `PlayerSlot` | Les quatre emplacements joueur (preset clavier, id manette, curseur, indices de skin/chapeau, prêt). |
| `mJoinedCount` | `int` = 0 | Nombre de slots occupés ; les slots sont remplis dans l'ordre 0,1,2,3 sans trou. |
| `mSkinIds` | `std::vector<std::string>` | Ids de skins chargés depuis `assets/skins/`, dans l'ordre (non déterministe) du système de fichiers. |
| `mHatIds` | `std::vector<std::string>` | Idem pour `assets/hats/`. |
| `ticksLeft` | `int` = 5000 | Compte à rebours en **millisecondes** avant lancement. |
| `starting` | `bool` = false | Vrai pendant le compte à rebours. |

#### `MenuScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager);`

- **Rôle** : préparer le menu : dimensions, police, textures de skins et chapeaux.
- **Paramètres** :
  - `renderer` — renderer de `main`, non possédé.
  - `window` — fenêtre, non possédée, seulement relayée à `GameScene`.
  - `manager` — gestionnaire de scènes, capturé par référence (doit vivre plus longtemps que la scène : c'est le cas, il est local à `main`).
- **Retour** : —
- **Effets de bord** :
  - `mScreenWidth = 1024`, `mScreenHeight = 576` ; `playBtnHitbox` centré, 100×20 px.
  - `TTF_OpenFont("assets/KiwiSoda.ttf", 30)` → **allocation à libérer** (faite au destructeur).
  - `TextureManager::loadTexture("playBtn", "assets/buttons/playBtn.png")`.
  - `loadDirectory("assets/hats/", "hat_")` et `loadDirectory("assets/skins/", "skin_")` → remplissent `mHatIds`/`mSkinIds` et chargent les textures correspondantes.
- **Pièges** :
  - **Le retour de `TTF_OpenFont` n'est pas vérifié** : si la police manque, `mFont` est `nullptr`, et `renderText` appellera `TTF_RenderText_Blended(nullptr, ...)` → `surf == NULL` → `surf->w` déréférence un pointeur nul → crash au premier rendu.
  - Nécessite `TTF_Init()` et `TextureManager::init()` faits avant (c'est le cas dans `main`).
  - `loadDirectory` **lève une exception non attrapée** si `assets/hats/` ou `assets/skins/` n'existe pas.
  - Si l'un des dossiers est vide, `mSkinIds`/`mHatIds` sont vides et l'accès `mSkinIds[mSlots[pIndex].skinIndex]` dans `handleEvent` lit hors limites (UB) dès qu'un joueur rejoint.
  - Les dimensions codées en dur dupliquent la vérité détenue par `main` : un changement de taille logique nécessite deux modifications.

#### `~MenuScene();`

- **Rôle** : libérer la police.
- **Paramètres** : aucun.
- **Retour** : —
- **Effets de bord** : `TTF_CloseFont(mFont)` si non nul.
- **Pièges** : les textures de skins/chapeaux chargées par le constructeur ne sont **pas** libérées ici — volontaire, elles appartiennent au `TextureManager` et sont réutilisées par la partie. Le destructeur doit tourner avant `TTF_Quit()` : c'est garanti car la pile est vidée dans la boucle de `main`, avant les `*_Quit`.

#### `void handleEvent(const SDL_Event &e) override;`

- **Rôle** : point d'entrée unique du menu : sortie du jeu, inscription d'un nouveau joueur, navigation, choix de cosmétiques, bascule « prêt ».
- **Paramètres** : `e` — événement SDL brut. Types traités : `SDL_KEYDOWN`, `SDL_JOYHATMOTION`, `SDL_JOYAXISMOTION`, `SDL_JOYBUTTONDOWN`.
- **Retour** : `void` ; sort tôt si l'événement ne produit aucune intention (`hasInput == false`).
- **Effets de bord** (dans l'ordre) :
  1. `ESCAPE` → `mManager.pop()` puis `mDone = true` et retour : la scène se dépile, la pile se vide, la boucle de `main` s'arrête → **sortie du jeu**.
  2. Traduction de l'événement en intentions `navUp/navDown/navLeft/navRight` et en identité de périphérique (`inputPresetId` pour le clavier, `inputJoyId` pour une manette). Le clavier est comparé aux **3** presets de `KeyPreset.hpp` : `left`→navLeft, `right`→navRight, `thrust`→navDown, `missile`→navUp.
  3. Recherche du slot correspondant au périphérique. Sinon, si `mJoinedCount < 4`, **inscription** d'un nouveau joueur : annule le compte à rebours (`starting = false`), renseigne preset/manette, remet curseur et axes à zéro, affecte `skinId`/`hatId` d'après les indices par défaut (0), incrémente `mJoinedCount`.
  4. Pour une manette, conversion des axes en front montant avec une **zone morte de 8000** (sur ±32767) et mémorisation dans `slot.lastAxisX/Y`, pour ne pas répéter la navigation quand le stick reste poussé.
  5. Navigation : `menuCursorY` borné à [0, 3] (0 = chapeau, 1 = skin, 2 = « Come later », 3 = « Ready ? »). Gauche/droite font tourner `hatIndex` ou `skinIndex` circulairement et mettent à jour `hatId`/`skinId`.
  6. Sur la ligne 3, gauche/droite basculent `ready`. Si tous les joueurs inscrits sont prêts : `starting = true`, `ticksLeft = 5000`.
- **Pièges** :
  - **Use-after-free sur ESCAPE** : `mManager.pop()` détruit la `MenuScene`, puis la ligne suivante écrit `mDone = true` **dans l'objet libéré**. Bug réel, silencieux la plupart du temps (la mémoire vient d'être libérée et n'est pas réutilisée), à corriger en inversant les deux lignes ou en différant le `pop`.
  - **`actReady` et `actCancel` sont déclarés `false` et jamais affectés** : le `if(actCancel) slot.ready = false;` est du code mort, donc **un joueur déclaré prêt ne peut plus se dé-déclarer** (sa branche `else` n'est plus atteinte, seul le test de la ligne 221 passe encore et bascule bien `ready`… ce qui rend le dé-cochage possible mais par le chemin latéral de `navLeft/navRight`, pas par un bouton dédié). Aucun bouton de manette n'est mappé sur « valider » : `SDL_JOYBUTTONDOWN` ne met que `hasInput = true` et sert uniquement à rejoindre.
  - `SDL_JOYAXISMOTION` met `hasInput = true` **systématiquement** : la moindre dérive de stick d'une manette inconnue inscrit un joueur (la zone morte n'est testée que sur le chemin d'inscription, ligne 163).
  - Seuls 3 presets clavier existent pour 4 colonnes : le 4e joueur doit être une manette.
  - La boucle « tous prêts » utilise `allReady = mSlots[i].ready;` (affectation au lieu d'un `&&`) ; comme elle `break` dès qu'un slot n'est pas prêt, le résultat final est correct, mais la formulation est trompeuse et fragile.
  - Un joueur qui se déclare prêt pendant le compte à rebours ne l'interrompt pas (seule une nouvelle inscription le fait).
  - Les slots sont remplis sans trou et **jamais libérés** : il n'y a aucun moyen de quitter un slot, et une manette débranchée laisse son slot occupé.

#### `void update(float deltaTime) override;`

- **Rôle** : faire avancer le compte à rebours de lancement.
- **Paramètres** : `deltaTime` — secondes.
- **Retour** : `void`.
- **Effets de bord** : si `starting`, `ticksLeft -= deltaTime * 1000` (conversion en ms) ; à `<= 0`, appelle `startGame()`.
- **Pièges** : `ticksLeft` est un `int` et `deltaTime*1000` un `float` → troncature vers zéro à chaque frame, donc le compte à rebours **dérive légèrement et dure un peu plus de 5 s** (perte de la partie fractionnaire, environ 0,x ms par frame). Avec le `SDL_Delay(16)`, `deltaTime*1000` vaut ~16,x ms et on en retire 16.

#### `void render() override;`

- **Rôle** : dessiner les 4 colonnes et le compte à rebours, et présenter la frame.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** :
  - Fond violet foncé `(23, 0, 44)` + `SDL_RenderClear`.
  - Pour chaque colonne `i` (largeur `mScreenWidth / 4` = 256 px) : si `i < mJoinedCount`, un panneau arrondi (y=50, h=500, marge 10 px), le personnage, puis 4 libellés à y=350/400/450/500 — l'entrée pointée par `menuCursorY` est en cyan `(0,255,255)`, les autres en blanc —, et `[ READY ]` en jaune à y=550 si le slot est prêt. Sinon, un slot vide.
  - `SDL_RenderPresent(mRenderer)`.
- **Pièges** :
  - Le bloc `if(starting)` qui affiche « Game start in N... » est **à l'intérieur de la boucle sur les 4 colonnes** : le même texte est redessiné 4 fois au même endroit (inoffensif, mais 4 textures créées/détruites pour rien).
  - Ce texte est passé en `SDL_Color{255, 255, 255, 0}` — **alpha 0** — et `renderText` utilise `TTF_RenderText_Blended`, qui respecte l'alpha : **le compte à rebours est invisible**. Bug visuel réel ; mettre 255.
  - `ticksLeft/1000 + 1` donne l'affichage 5,4,3,2,1 (et 0 si `ticksLeft` est devenu négatif avant l'appel à `startGame`).
  - `[ READY ]` à y=550 et le panneau finissant à y=550 : le texte dépasse en bas d'un écran de 576 px de haut.
  - Le libellé de la ligne 2 est « Come later » (emplacement réservé, probablement pour le jetpack : `PlayerSlot::jetpackId`/`jetpackIndex` existent mais ne sont jamais renseignés).
  - Les ids bruts (`hat_cowboy`, `skin_bleu`) sont affichés tels quels, préfixe compris.
  - `drawPanel`/`drawPlayer` utilisent `colWidth` et `colX` calculés deux fois de façon légèrement différente (`i * (mScreenWidth / 4)` vs `mScreenWidth / 4`), équivalent ici.

#### `bool isDone() override;`

- **Rôle** : exposer `mDone`.
- **Paramètres** : aucun.
- **Retour** : `mDone` — passe à `true` uniquement sur ESCAPE (dans l'objet déjà détruit, cf. plus haut).
- **Effets de bord** : aucun.
- **Pièges** : **aucun appelant** ; la sortie se fait par le dépilement direct.

#### `void startGame();` *(privé)*

- **Rôle** : lancer la partie.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** :
  - Si `mJoinedCount <= 0` : `printf("No player")` et rien d'autre (le menu reste).
  - Sinon : `mManager.push(std::make_unique<GameScene>(mRenderer, mWindow, mManager, mSlots, mJoinedCount))` — la `MenuScene` **reste vivante sous** la nouvelle scène, ce qui permet de revenir au menu avec les mêmes joueurs.
  - `starting = false`, puis pour chaque slot inscrit : `ready = false`, `menuCursorY = 0` (le menu est donc déjà « rangé » pour le retour).
- **Pièges** :
  - `mSlots` est passé en **pointeur brut** sur le tableau membre ; `GameScene`/`Game` doivent l'avoir entièrement consommé avant de rendre la main (c'est le cas, `Game::init` recopie ce qui l'intéresse dans ses `PlayerInfo`/`PlayerConfig`).
  - `push` est appelé depuis `update()`, donc depuis la scène du dessous : c'est sûr (contrairement à `pop`), mais `main` appellera ensuite `render()` sur la **nouvelle** scène dans la même frame.
  - La construction de `GameScene` fait tout le chargement de la partie (textures, sons, police, Lua) **de façon synchrone** : gel visible de quelques dizaines/centaines de ms, sans écran de chargement.
  - `ticksLeft` n'est pas remis à 5000 ici ; il l'est au moment où `starting` repasse à `true`. Correct, mais la responsabilité est éclatée.

#### `void drawPanel(int x, int y, int w, int h, bool isReady);` *(privé)*

- **Rôle** : dessiner le cadre d'une colonne joueur.
- **Paramètres** : `x`, `y`, `w`, `h` — rectangle en pixels ; `isReady` — choisit la couleur de fond.
- **Retour** : `void`.
- **Effets de bord** : `util::drawRoundedRect(mRenderer, rect, 15, bgColor)` — rayon 15 px, fond bleu nuit `(8,45,92)` si prêt, bleu `(0,86,135)` sinon — puis un contour rectangulaire **non arrondi** turquoise `(68,220,197)` via `SDL_RenderDrawRect`. Modifie la couleur de dessin du renderer sans la restaurer.
- **Pièges** : le contour droit par-dessus un fond arrondi laisse voir les coins du contour dépasser du fond. L'alpha 255 des couleurs rend le panneau opaque.

#### `void drawPlayer(int colX, int colWidth, std::string skinId, std::string hatId);` *(privé)*

- **Rôle** : dessiner l'aperçu du personnage (skin puis chapeau par-dessus).
- **Paramètres** :
  - `colX` — abscisse du bord gauche de la colonne, en pixels.
  - `colWidth` — largeur de la colonne, en pixels.
  - `skinId`, `hatId` — ids de texture. **Passés par valeur** (deux copies de `std::string` par appel, 4 fois par frame).
- **Retour** : `void`.
- **Effets de bord** : deux `LTexture::render` à l'échelle 5 (32 px natifs → 160 px), centrés horizontalement dans la colonne, à `y = 150`.
- **Pièges** :
  - **`getTexture(...)->render(...)` sans test de nullité** : un id inconnu (dossier vide, fichier supprimé entre deux lancements) déréférence `nullptr` → crash.
  - La taille 32 px et l'échelle 5 sont codées en dur ; un asset de taille différente serait déformé.
  - Le chapeau est dessiné au même rectangle que le skin : l'alignement repose entièrement sur la convention d'art (même canevas 32×32).

#### `void renderText(std::string text, int x, int y, SDL_Color color);` *(privé)*

- **Rôle** : dessiner une chaîne à une position donnée.
- **Paramètres** :
  - `text` — texte, **par valeur**.
  - `x`, `y` — coin haut-gauche en pixels.
  - `color` — couleur RGBA ; l'alpha **est** respecté (`TTF_RenderText_Blended`, rendu antialiasé 32 bits).
- **Retour** : `void`.
- **Effets de bord** : crée une `SDL_Surface` puis une `SDL_Texture`, dessine, puis libère les deux (`SDL_FreeSurface` + `SDL_DestroyTexture`) — pas de fuite.
- **Pièges** :
  - **Aucun test de nullité** sur `surf` ni `tex` : police nulle, texte vide (`TTF_RenderText_Blended` renvoie une surface 0×1 pour une chaîne vide, pas `NULL`, donc ça passe) ou échec mémoire → `surf->w` sur `nullptr` = crash.
  - **Une surface + une texture créées et détruites par appel, à chaque frame** : avec 4 colonnes × 4 libellés + le compte à rebours ×4, cela fait ~20 allocations/destructions GPU par frame. C'est la principale inefficacité du menu ; mettre en cache les textures de texte (ou utiliser `LTexture::loadFromRenderedText` et ne régénérer qu'au changement) serait la correction naturelle.
  - Ne restaure pas la couleur de dessin du renderer (elle n'est pas modifiée ici, mais la texture reste teintée par défaut).

#### `void drawEmptySlot(int x, int colWidth);` *(privé)*

- **Rôle** : dessiner une colonne libre avec l'invite de participation.
- **Paramètres** : `x` — bord gauche de la colonne en pixels ; `colWidth` — largeur en pixels.
- **Retour** : `void`.
- **Effets de bord** : rectangle plein gris `(30,30,30)` **alpha 150** (semi-transparent, ce qui fonctionne car `main` n'a pas désactivé le blend… en réalité le blend mode du renderer n'est mis à `BLEND` que par `Game::start`, donc **au premier affichage du menu l'alpha 150 est ignoré** et le rectangle est opaque ; après un retour de partie il devient translucide). Puis `renderText("Press a touch", ...)` centré approximativement (`colWidth/2 - 70`).
- **Pièges** : incohérence visuelle entre le premier affichage du menu et les suivants, due à l'état global du renderer modifié par `Game::start` (voir ci-dessus). Le libellé « Press a touch » est un anglicisme de « Appuyez sur une touche ». Le centrage à `-70` est un décalage en dur, non dérivé de la largeur réelle du texte.

**Code mort / non utilisé dans ce fichier**
- `playBtnHitbox` : calculé dans le constructeur, **jamais lu** — et `mScreenHeight` ne sert qu'à ce calcul, donc mort lui aussi en pratique.
- La texture `"playBtn"` est chargée et **jamais dessinée** (reliquat d'un menu à la souris ; aucun événement souris n'est traité).
- `actReady` et `actCancel` : variables locales toujours `false` → la branche `if(actCancel)` est inatteignable, et aucun bouton de manette ne valide.
- `mDone` / `isDone()` : écrits/implémentés mais jamais consultés.
- `PlayerSlot::jetpackId` et `jetpackIndex` : jamais renseignés ni affichés (la ligne de menu correspondante affiche « Come later »).
- `#include "LTimer.hpp"` dans le header : aucun `LTimer` membre (le compte à rebours utilise un `int`). `#include "LTexture.hpp"`, `<SDL2/SDL_surface.h>`, `<cstddef>` dans le `.cpp` sont superflus ou redondants.
- `#include "KeyPreset.hpp"` est en revanche nécessaire (tableau global `presets`).

---

## include/GameScene.hpp + src/GameScene.cpp

### `class GameScene`

Adaptateur très fin entre l'interface `Scene` et la classe `Game` (qui contient serveur, client, monde, audio, particules). Elle crée la partie à la construction, relaie les trois appels du cycle de vie, et se dépile quand la partie est terminée ou qu'on appuie sur Échap.

| Membre | Type | Rôle |
|---|---|---|
| `mManager` | `SceneManager&` | Référence sur la pile de scènes (pour se dépiler). Note : l'indentation du header est décalée sur cette ligne. |
| `mGame` | `Game*` | Pointeur possédant la partie, alloué par `new` et **jamais libéré**. |
| `mDone` | `bool` = false | Drapeau de fin, jamais mis à `true` (la ligne correspondante est commentée). |

#### `GameScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager, PlayerSlot* slots, int joinedCount);`

Définition : `GameScene::GameScene(SDL_Renderer* renderer, SDL_Window* window, SceneManager& manager, PlayerSlot* playerSlots, int joinedCount)` (le nom du 4e paramètre diffère entre header et `.cpp`).

- **Rôle** : créer et démarrer la partie.
- **Paramètres** :
  - `renderer`, `window` — relayés à `Game::init`, non possédés.
  - `manager` — capturé par référence.
  - `slots` — pointeur sur le tableau de 4 `PlayerSlot` du menu ; **emprunté le temps du constructeur seulement** (`Game::init` recopie ce qu'il lui faut). Aucune vérification de nullité.
  - `joinedCount` — nombre de slots valides à lire au début du tableau ; aucune borne vérifiée (une valeur > 4 lirait hors du tableau du menu).
- **Retour** : —
- **Effets de bord** : `new Game()` (gros objet : `ParticleManager` de 2000 particules, deux `GameConfig` lus sur disque dans la liste d'initialisation de `Game`, serveur et client en loopback), puis `init()` (sous-système audio, monde, transports, chargement des configs de particules, construction des joueurs et de leurs capacités Lua), `loadMedia()` (textures des dossiers, animation d'explosion, police de score, 4 SFX, 1 musique) et `start()` (blend mode, musique, `srand`, démarrage du monde).
- **Pièges** :
  - **Fuite mémoire certaine** : il n'existe **aucun destructeur `~GameScene`**, donc le `delete mGame` n'a jamais lieu. Chaque aller-retour menu → partie → menu abandonne un `Game` complet : plusieurs mégaoctets de pool de particules, une `TTF_Font` jamais fermée, toutes les `Mix_Chunk`/`Mix_Music`, le monde et les joueurs. C'est le défaut le plus coûteux de ce fichier ; la correction minimale est `std::unique_ptr<Game> mGame;` (le header inclut déjà `Game.hpp`, donc le type est complet).
  - Les codes de retour de `init()` et `loadMedia()` (tous deux `bool`) sont **ignorés** : une police ou un son manquant ne fait que logguer, et la partie démarre dans un état dégradé.
  - Tout le chargement est synchrone dans le constructeur → gel de la frame.
  - `Game::Game()` lit `assets/config.ini` à chaque partie : un `config.ini` modifié entre deux parties est repris à chaud (effet de bord utile pour le réglage, mais surprenant).

#### `void handleEvent(const SDL_Event& e) override;`

- **Rôle** : interception d'Échap, sinon relais à la partie.
- **Paramètres** : `e` — événement SDL brut.
- **Retour** : `void`.
- **Effets de bord** : si `SDL_KEYDOWN` + `SDLK_ESCAPE` → `mManager.pop()` et retour immédiat ; la `GameScene` est **détruite ici** et le menu redevient actif. Sinon `mGame->handleEvents(e)` (qui gère `SDL_QUIT` et F1).
- **Pièges** :
  - `pop()` détruit `*this` pendant l'exécution ; ici c'est **sans conséquence** car aucun membre n'est touché après (contrairement à `MenuScene::handleEvent`) — mais c'est une sûreté accidentelle, pas intentionnelle.
  - Sortir par Échap **ne coupe pas les sons** (contrairement au chemin « partie terminée » qui appelle `Mix_HaltChannel(-1)`) : le son continu du jetpack peut rester bloqué en boucle infinie, puisque le canal n'est jamais arrêté et que l'`AudioManager` fuit avec le `Game`.
  - Échap est traité par la scène du sommet seulement, donc il n'y a pas de double dépilement menu+partie.
  - `SDL_QUIT` n'est pas traité ici : il est géré à la fois par `main` (qui vide la pile) et par `Game::handleEvents` (qui met `mQuit`).

#### `void update(float deltaTime) override;`

- **Rôle** : faire avancer la partie, ou quitter si elle est finie.
- **Paramètres** : `deltaTime` — secondes, plafonné à 0,05 s par `main`.
- **Retour** : `void`.
- **Effets de bord** : si `mGame->isOver()` → `Mix_HaltChannel(-1)` (tous les SFX coupés, pas la musique) puis `mManager.pop()` et retour. Sinon `mGame->update(deltaTime)` (échantillonnage des entrées, envoi au serveur, simulation, réception du snapshot, particules, effets, événements audio).
- **Pièges** :
  - **`pop()` appelé depuis `update()`** : `main.cpp` enchaîne immédiatement sur `manager.current()->render()` sans revérifier la nullité. Ici le menu est toujours sous la partie, donc `current()` n'est pas nul et on rend le menu — mais si `GameScene` était la seule scène de la pile, ce serait un déréférencement de `nullptr`.
  - `Mix_HaltChannel(-1)` en dur alors que `AudioManager::stopAllChannel()` existe : fuite d'abstraction (le fichier dépend de SDL_mixer sans l'inclure explicitement — il arrive via `Game.hpp`).
  - La musique n'est pas arrêtée, et le `Game` fuyant n'étant jamais détruit, son `AudioManager::clean()` (qui fermerait le périphérique) n'est jamais exécuté : la musique de la partie continue de jouer dans le menu.
  - Le `//mDone = true;` commenté témoigne de l'abandon du mécanisme `isDone()`.

#### `void render() override;`

- **Rôle** : déléguer le rendu à la partie.
- **Paramètres** : aucun.
- **Retour** : `void`.
- **Effets de bord** : `mGame->render()`, qui ne fait rien tant qu'aucun snapshot n'est arrivé du serveur, et qui effectue lui-même le `clear`/`present`.
- **Pièges** : pendant la ou les premières frames (avant le premier snapshot) **rien n'est présenté** : l'écran garde l'image précédente (le menu).

#### `bool isDone() override;`

- **Rôle** : exposer `mDone`.
- **Paramètres** : aucun.
- **Retour** : `mDone`, **toujours `false`** (la seule affectation est commentée).
- **Effets de bord** : aucun.
- **Pièges** : aucun appelant ; implémentée uniquement parce que `Scene` l'impose.

**Code mort / non utilisé dans ce fichier**
- **Destructeur absent** : `mGame` alloué par `new` n'est jamais `delete` → fuite complète de la partie (mémoire, police TTF, ressources SDL_mixer). À convertir en `std::unique_ptr<Game>`.
- `mDone` et `isDone()` : le drapeau n'est jamais mis à `true` (`//mDone = true;`), la méthode n'est jamais appelée.
- Les valeurs de retour de `Game::init()` et `Game::loadMedia()` sont ignorées.
- `#include "PlayerSlot.hpp"` dans le `.cpp` est redondant (déjà inclus par le header).
- `Mix_HaltChannel(-1)` double la méthode `AudioManager::stopAllChannel()`, elle-même sans appelant.

---

## assets/config.ini

Lu par `GameConfig mConfig("assets/config.ini")` dans la liste d'initialisation de `Game::Game()`. Les valeurs sont exploitées dans `Game::Game()`, `Game::init()` (pour remplir chaque `player::PlayerConfig`) et `Game::start()`. **Le fichier est relu à chaque création de `GameScene`**, donc à chaque partie. Le suffixe `f` des nombres est toléré parce que `std::stof`/`std::stoi` s'arrêtent au premier caractère non numérique. Commentaires acceptés avec `#` ou `;` en début de ligne ; les sections `[...]` ne sont pas gérées.

| Clé | Type lu | Valeur dans le fichier | Défaut dans le code | Effet |
|---|---|---|---|---|
| `player_jetpack_force` | float (`getFloat`) | `700.0f` | `700.f` | Poussée verticale du jetpack appliquée tant que la touche est maintenue, en pixels/s² (accélération vers le haut). Plus haut = montée plus vive. |
| `player_max_vx` | float | `1000.0f` | `1000.f` | Vitesse horizontale maximale du joueur, en pixels/s. Plafonne l'effet de `player_acceleration`. |
| `player_acceleration` | float | `1000.0f` | `1000.f` | Accélération horizontale quand gauche/droite est maintenue, en pixels/s². |
| `player_deceleration` | float | `0.8f` | `0.8f` | Facteur de freinage horizontal appliqué quand aucune direction n'est pressée (0 = arrêt net, 1 = glisse infinie). Sans unité. |
| `player_health` | int (`getInt`) | `100` | `100` | Points de vie initiaux et maximum du joueur ; recopié dans `PlayerInfo::maxLife`, donc aussi dans la barre de vie affichée. |
| `player_bounce` | bool (`getBool`) | `true` | `true` | Si vrai, le joueur rebondit sur les bords de l'écran au lieu d'y être bloqué. Valeurs acceptées pour « vrai » : `true`, `1`, `yes`, `on` (insensible à la casse) ; **tout le reste vaut faux**. |
| `bounce_restitution` | float | `0.4f` | `0.4f` | Fraction de la vitesse conservée au rebond (0 = s'arrête, 1 = rebond parfait). Sans effet si `player_bounce=false`. |
| `show_player_collider` | bool | `false` | `false` | Debug : dessine le rectangle de collision du joueur. Transmis au client via `PlayerInfo::showCollider`. |
| `gravity` | float | `-500.0f` | `-500.f` | Accélération de gravité, en pixels/s². **La valeur est négative** : le code l'ajoute à une vitesse dont l'axe est inversé par rapport à l'axe Y de SDL ; mettre une valeur positive ferait tomber les joueurs vers le haut. |
| `music` | bool | `false` | `true` | Si vrai, `Game::start()` lance la musique de fond en boucle. Mis à `false` dans le fichier actuel, donc pas de musique — à noter que le défaut du code est l'inverse. |
| `SCREEN_WIDTH` | int | **absente** | `800` | Lue par `Game::Game()` dans `world().screenWidth`, mais **immédiatement écrasée** par `SDL_RenderGetLogicalSize` dans `Game::init()` (1024). Clé sans effet. |
| `SCREEN_HEIGHT` | int | **absente** | `600` | Idem, écrasée par 576. Clé sans effet. |
| `PLAYER_NUMBER` | int | **absente** | `2` | Lue dans `mPlayerNumber`, mais écrasée par `joinedCount` dès `Game::init()`. Clé sans effet. |

Remarques transverses : le volume de la musique (32/128) et la cadence de spawn des particules de jetpack (20 ms) sont **codés en dur** et non configurables ici. Le fichier doit se terminer par un retour à la ligne, sinon la dernière clé (`music`) serait ignorée, et doit utiliser des fins de ligne Unix (un `\r` resterait collé à la valeur et ferait échouer `getBool`).

---

## assets/playerThrustParticle.ini

Lu par `GameConfig mThrustParticleGameConfig("assets/playerThrustParticle.ini")` (construit avec `Game`) puis interprété par `ParticleConfig::load()` appelé depuis `Game::init()`. La configuration résultante est copiée dans chaque `player::PlayerConfig::thrustParticleConfig`, puis **dans chaque particule** au spawn. Décrit les particules du propulseur du joueur (carrés pleins qui montent et s'estompent).

### Dégradé de couleur

| Clé | Type lu | Valeur dans le fichier | Défaut dans le code | Effet |
|---|---|---|---|---|
| `particle_color_count` | int (`getInt`) | `5` | `0` | Nombre de keyframes à lire (`particle_color_0` … `particle_color_{n-1}`). **Un 0 ou une absence provoque un comportement indéfini** dans `setColorFrameList()` (`keyframes.back()` sur vecteur vide). Une valeur supérieure au nombre de lignes réellement présentes insère des keyframes issues du défaut `"255,255,255,255,500"`. |
| `particle_color_0` … `particle_color_4` | string (`getString`) puis `sscanf("%hhu,%hhu,%hhu,%hhu,%d")` | voir ci-dessous | `"255,255,255,255,500"` | Keyframe au format `r,g,b,a,temps` : R/G/B/A sur 0-255, temps en **millisecondes** depuis la naissance de la particule. Les keyframes **doivent être triées par temps croissant** (non vérifié). Le temps de la dernière définit la durée de vie (ici 800 ms, exploitée jusqu'à 799 ms). |

Valeurs actuelles du dégradé :

| Keyframe | r,g,b,a | temps (ms) | Rendu |
|---|---|---|---|
| `particle_color_0` | 255,200,50,255 | 0 | jaune-orangé opaque à la naissance |
| `particle_color_1` | 255,100,0,255 | 50 | orange vif |
| `particle_color_2` | 230,230,230,220 | 150 | fumée blanche presque opaque |
| `particle_color_3` | 220,220,220,150 | 400 | fumée grise à moitié transparente |
| `particle_color_4` | 255,255,255,0 | 800 | blanc totalement transparent (disparition) |

Entre deux keyframes, les quatre composantes sont interpolées **linéairement**, et le résultat est précalculé une fois pour chaque milliseconde dans `colorFrameList` (ici 800 entrées, soit 3,2 Ko — table dupliquée dans chaque particule du pool, cf. les pièges de `ThrustParticle::init`).

### Physique et taille

| Clé | Type lu | Valeur dans le fichier | Défaut dans le code | Effet |
|---|---|---|---|---|
| `growRate` | float (`getFloat`) | `0.15` | `0.0f` | Pixels ajoutés au côté du carré **à chaque appel d'`update`** (≈ par frame, pas par seconde). 0,15 px/frame ≈ +9 px sur 800 ms à 60 fps : le panache s'épaissit en s'éloignant. Une valeur négative ferait rétrécir puis disparaître (rectangle de largeur négative non dessiné). |
| `friction` | float | `0.98` | `0.0f` | Multiplicateur appliqué à `vx` à chaque `update`. 0,98 = perte de 2 % de vitesse horizontale par frame (dispersion initiale qui s'amortit). **Le défaut 0 annulerait instantanément toute dispersion**, et une valeur > 1 ferait diverger. |
| `riseForce` | float | `0.05` | `0.0f` | Valeur **retirée** à `vy` à chaque `update` (px/frame²). Fait remonter la fumée de plus en plus vite ; c'est l'effet de flottaison. |
| `vxSpread` | int (`getInt`) | `100` | `0` | Dispersion horizontale initiale : `vx` est tiré dans `[-vxSpread, vxSpread[` puis **divisé par 100** → ici ±1 px/frame. **La valeur 0 (ou l'absence) laisse `vx` à la valeur résiduelle de la particule précédente du pool** (le membre n'est pas réinitialisé). |
| `vyMin` | int | `0` | `0` | Borne inférieure de la vitesse verticale initiale, en **centièmes** de pixel/frame. |
| `vyMax` | int | `5` | `0` | Borne supérieure (exclusive) de la vitesse verticale initiale, en centièmes de pixel/frame → ici `vy` ∈ {0, 0.01, 0.02, 0.03, 0.04} px/frame, soit une légère dérive vers le bas aussitôt contrée par `riseForce`. **Attention** : si `vyMin == vyMax`, le code prend le chemin `vy = vyMax` **sans diviser par 100** (incohérence d'unité × 100). Et si `vyMax < vyMin`, le modulo devient négatif → comportement indéfini. |
| `sizeMin` | int | `4` | `10` | Côté minimal du carré à la naissance, en pixels. |
| `sizeMax` | int | `6` | `10` | Côté maximal (exclusif) à la naissance, en pixels → ici 4 ou 5 px. Si `sizeMin == sizeMax`, la taille vaut exactement `sizeMax`. Si `sizeMax < sizeMin`, modulo négatif → comportement indéfini. |

Remarques transverses : toutes les grandeurs de déplacement sont **par frame et non par seconde**, donc l'allure du panache dépend de la cadence de la boucle principale (fixée par le `SDL_Delay(16)` de `main`). La durée de vie n'est pas une clé propre : elle est **déduite du temps de la dernière keyframe de couleur** (`getMaxTime() = dernier temps - 1`), ce qui couple étroitement le dégradé et la physique. La cadence de spawn (une particule toutes les 20 ms par joueur, `Player::thrustParticlesTimer`) et la taille du pool (2000) ne sont pas configurables ici.

---

