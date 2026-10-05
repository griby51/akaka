# API de modding Lua — akaka

Jeu de combat local en écran partagé (SDL2). Les mods sont du Lua pur, chargés au démarrage du jeu. Ce document décrit **uniquement ce qui existe réellement** dans le moteur (bindings de `ScriptEngine`). Tout ce qui est annoncé dans `mods/base/meta/akaka.lua` mais pas branché est listé à la fin, section « Ce qui n'existe pas encore ».

Conventions valables partout :

| Sujet | Règle |
| --- | --- |
| Temps | **secondes** (`cooldown = 2.0`, `dt`, durée de secousse). Le moteur convertit en interne. |
| Distances / vitesses | **pixels logiques** et **pixels/seconde**. L'écran logique est 1024 x 576. |
| Axe Y | **vers le bas**. `vy = 200` tombe, `vy = -200` monte. |
| Positions | `getPosition()` = **coin haut-gauche**. `getCenter()` = centre (Player uniquement). |
| Portées | `playersInRadius` et `explode` mesurent **de centre à centre**. |
| Appels | les objets du moteur s'appellent avec **deux-points** : `player:getLife()`. |
| Création | on ne construit jamais un `Player`, un `Projectile` ou le `ctx` depuis Lua : le moteur les fournit. |

Bibliothèques Lua disponibles : `base`, `math`, `string`, `table`, `package`. **Pas** de `io`, `os`, `coroutine`, `debug`, et pas de modules C (`package.cpath` est vidé et `package.loadlib` supprimé). Un mod ne peut donc ni lire ni écrire de fichier.

---

## Démarrage rapide

### Structure d'un mod

Un mod est un dossier dans `mods/`, avec un `init.lua` à sa racine :

```
mods/
  base/                  <- le mod officiel, toujours chargé en premier
    init.lua
    textures.lua
    hats.lua
    abilities/
    projectiles/
    meta/akaka.lua       <- annotations pour l'autocomplétion (jamais exécuté)
  monmod/                <- ton mod
    init.lua
    abilities/laser.lua
    projectiles/laser.lua
```

Un dossier sans `init.lua` est ignoré, avec le message `[lua] mod monmod : no init.lua, ignored`.

### `require` : toujours préfixé par le nom du mod

Le chemin de recherche de `package` est réglé sur :

```
mods/?.lua;mods/?/init.lua
```

Le point de départ est donc `mods/`, pas ton dossier. Il faut écrire le nom du mod en premier :

```lua
require("monmod.abilities.laser")   -- charge mods/monmod/abilities/laser.lua
require("base.projectiles.missile") -- parfaitement légal : on peut réutiliser le code de base
```

`require("abilities.laser")` **ne marche pas** (il chercherait `mods/abilities/laser.lua`).

### `MOD_DIR` et `MOD_NAME`

Juste avant d'exécuter chaque `init.lua`, le moteur pose deux globales :

| Globale | Valeur | Exemple |
| --- | --- | --- |
| `MOD_NAME` | nom du dossier | `"monmod"` |
| `MOD_DIR` | chemin du dossier, **avec le `/` final** | `"mods/monmod/"` |

Elles servent à charger les assets livrés avec le mod :

```lua
loadTexture("monmod_laser", MOD_DIR .. "assets/laser.png")
```

:warning: Ce sont des **globales partagées**, écrasées avant chaque mod. Elles ne valent ce que tu crois **que pendant le chargement**. Si tu en as besoin plus tard (dans un `onUse`, un `onUpdate`), copie-les dans une locale au moment du chargement :

```lua
local MY_DIR = MOD_DIR   -- figé maintenant, fiable pour toujours
```

### Ordre de chargement

1. Le moteur liste les sous-dossiers de `mods/`.
2. Il les trie par ordre alphabétique, **sauf `base` qui passe toujours en premier**.
3. Pour chacun : il pose `MOD_DIR`/`MOD_NAME`, puis exécute `init.lua`.

Conséquences pratiques :

* Les textures, abilities et chapeaux de `base` existent déjà quand ton mod démarre : tu peux t'appuyer dessus.
* Deux mods qui déclarent la même ability : c'est **le dernier chargé qui gagne** (ordre alphabétique), avec le message `registerAbility : <id> was already declared, replaced`.
* Pour une texture, c'est l'inverse : **le premier gagne** (voir `loadTexture`).
* Tout se joue au démarrage du jeu, avant le menu. Il n'y a pas de rechargement à chaud : il faut relancer le jeu après chaque modification.

Les erreurs ne font pas tomber le jeu : un `init.lua` qui plante affiche
`Error Script Engine : mods/monmod/init.lua : <message Lua>` et le mod est simplement sauté.

### Un mod minimal complet

`mods/monmod/init.lua` :

```lua
-- 1. Les assets. Le chemin est relatif au dossier du jeu (ou utilise MOD_DIR).
loadTexture("monmod_bullet", "assets/dot.bmp")

-- 2. Une ability.
registerAbility{
    id = "tir_droit",
    cost = 100,      -- points de score dépensés
    cooldown = 1.0,  -- secondes

    onUse = function(self, player, ctx)
        local cx, cy = player:getCenter()

        ctx:spawnProjectile{
            texture = "monmod_bullet",
            x = cx, y = cy,
            vx = 800,          -- part vers la droite, tout droit
        }

        ctx:playSFX("missileLaunch")
        return true            -- true = ça a servi : le coût est débité
    end
}

-- 3. Un chapeau pour la rendre jouable.
--    L'image DOIT exister dans assets/hats/ (voir le piège plus bas).
registerHat{
    id = "hat_cowboy",
    texture = "assets/hats/cowboy.png",
    ability = "tir_droit",
}
```

Relance le jeu : la console affiche `[lua] ability saved : tir_droit`, `[lua] hat saved : hat_cowboy`, `[lua] mod loaded : monmod`. Choisis le chapeau cowboy dans le menu, et la touche d'ability tire.

---

## Fonctions globales

Il y a exactement **trois** fonctions globales, plus les deux variables `MOD_DIR` / `MOD_NAME`.

### `loadTexture(id, path)`

```lua
---@param id   string  identifiant sous lequel la texture sera retrouvée
---@param path string  chemin du fichier image, relatif au dossier de lancement du jeu
---@return boolean ok
loadTexture(id, path)
```

Charge une image (PNG, BMP…) et l'enregistre sous `id`. Cet `id` est ce qu'on passe ensuite à `texture = ...` dans `spawnProjectile` ou `registerHat`.

Comportement exact :

* Si `id` **est déjà pris**, la fonction retourne `true` immédiatement **sans rien recharger**. Il est donc impossible de remplacer une texture déjà chargée (notamment celles de `base`). Préfixe tes ids avec le nom de ton mod.
* Si le fichier est introuvable ou illisible : affiche `Error TextureManager : can't load : <path>` et retourne `false`.
* Les chemins sont relatifs au **répertoire courant du jeu**, pas à ton mod. Pour un asset livré avec le mod, passe par `MOD_DIR`.

```lua
if not loadTexture("monmod_laser", MOD_DIR .. "assets/laser.png") then
    print("[monmod] laser.png manquant, le mod tournera sans")
end
```

### `registerAbility(def)`

```lua
---@param def table  voir « Les abilities »
registerAbility(def)
```

Enregistre une ability dans le registre interne, indexée par son `id`. Pas de valeur de retour.

Validations, dans cet ordre, avec les messages exacts :

| Cas | Message console | Effet |
| --- | --- | --- |
| `def.id` absent ou pas une chaîne | `registerAbility : no id, ability ignored` | ability ignorée |
| `def.onUse` absent ou pas une fonction | `regiterAbility : <id> : onUse not found, ability ignored` *(oui, la faute de frappe est dans le moteur)* | ability ignorée |
| `id` déjà enregistré | `registerAbility : <id> was already declared, replaced` | l'ancienne est **remplacée** |
| succès | `[lua] ability saved : <id>` | enregistrée |

```lua
registerAbility{
    id = "monmod_soin",
    cost = 300,
    cooldown = 8.0,
    onUse = function(self, player, ctx)
        player:heal(30)
        ctx:playSFX("boing")
        return true
    end
}
```

### `registerHat(def)`

```lua
---@param def table  { id = string, texture = string, ability = string? }
registerHat(def)
```

Déclare un chapeau. Pas de valeur de retour. Le chapeau est ajouté à une liste interne ; `ability` fait le lien avec un `registerAbility`.

Champs lus par le moteur :

| Champ | Type | Obligatoire | Rôle |
| --- | --- | --- | --- |
| `id` | string | oui | identifiant du chapeau **et** id de sa texture |
| `texture` | string | oui | chemin de l'image, chargé automatiquement sous `id` (via `loadTexture`) |
| `ability` | string | en pratique oui (voir ci-dessous) | `id` d'une ability déclarée avec `registerAbility` |

Validations, messages exacts :

| Cas | Message console | Effet |
| --- | --- | --- |
| `id` absent | `No id, hat ignored` | chapeau ignoré |
| `texture` absent | `registerHat <id> : no texture, ignored` | chapeau ignoré |
| image illisible | `Error TextureManager : can't load : <path>` puis `registerHat <id> : texture cant be loaded : <path>` | chapeau ignoré |
| succès | `[lua] hat saved : <id>` | enregistré |

**Deux points critiques** (vérifiés dans le code, et c'est le piège n°1 des nouveaux mods) :

1. La **liste du menu** n'est pas construite à partir de `registerHat` : elle est construite en scannant les fichiers `*.png` de `assets/hats/`, et l'id de chaque entrée vaut `"hat_" .. <nom du fichier sans extension>`. Donc pour qu'un chapeau soit **sélectionnable**, il faut que `assets/hats/<nom>.png` existe et que tu appelles `registerHat{ id = "hat_<nom>", ... }`. Un chapeau déclaré avec un id arbitraire est bien enregistré, mais n'apparaîtra jamais dans le menu. L'ordre d'affichage est celui du système de fichiers, **pas** l'ordre de déclaration.
2. Un chapeau sélectionnable **sans ability valide fait planter le jeu** au démarrage du match (le moteur lit la progression du cooldown sans vérifier qu'une ability existe). Autrement dit : tout `.png` posé dans `assets/hats/` doit avoir son `registerHat` avec un `ability` qui pointe sur une ability réellement enregistrée. Si l'ability est inconnue, la console affiche `createAbility : unknown ability : <id>` juste avant.

```lua
-- mods/base/hats.lua, tel quel : plusieurs chapeaux peuvent partager une ability
registerHat{ id = "hat_witch",   texture = "assets/hats/witch.png",   ability = "freeze" }
registerHat{ id = "hat_cowboy",  texture = "assets/hats/cowboy.png",  ability = "missile" }
registerHat{ id = "hat_soldier", texture = "assets/hats/soldier.png", ability = "missile" }
```

---

## Les abilities

Une ability, c'est une table passée à `registerAbility`. Elle est associée à un chapeau, et déclenchée par la touche d'ability du joueur.

### Forme d'une définition

```lua
registerAbility{
    id = "mon_ability",   -- string, obligatoire, unique
    cost = 200,           -- integer, optionnel, défaut 0 : score dépensé par usage réussi
    cooldown = 5.0,       -- number, optionnel, défaut 0 : secondes entre deux usages réussis
    onUse = function(self, player, ctx) ... return true end,  -- obligatoire
}
```

Tout autre champ est ignoré par le moteur, mais reste **lisible depuis `self`** : c'est le bon endroit pour ranger tes constantes.

### Instances : `self`

À la création de la partie, le moteur crée **une instance par joueur** : une table vide dont le métatable pointe (`__index`) sur ta définition. Donc :

* Lire `self.cost`, `self.ma_constante` → retombe sur la définition, partagée.
* Écrire `self.charges = 3` → écrit dans **l'instance du joueur**, invisible pour les autres joueurs. C'est le moyen de garder un état par joueur pour toute la durée du match.

```lua
registerAbility{
    id = "triple_tir",
    cost = 0,
    cooldown = 0.2,
    MUNITIONS = 3,        -- constante partagée, lue via self.MUNITIONS

    onUse = function(self, player, ctx)
        self.restant = self.restant or self.MUNITIONS   -- état propre au joueur
        if self.restant <= 0 then return false end      -- false : rien débité, cooldown non relancé
        self.restant = self.restant - 1

        local cx, cy = player:getCenter()
        ctx:spawnProjectile{ texture = "monmod_bullet", x = cx, y = cy, vx = 900 }
        return true
    end
}
```

Contrairement à un projectile, `self` est ici une **table Lua** : on peut y écrire librement.

### Quand `onUse` est appelé

Signature : `onUse(self, player, ctx) -> boolean`

Dans un tick de simulation, le moteur fait, dans cet ordre :

1. mise à jour des projectiles (tous les `onUpdate`),
2. pour chaque joueur : lecture de l'entrée → **si la touche d'ability est pressée, `onUse`** → puis physique du joueur (vitesse, gravité, bords),
3. résolution des collisions entre joueurs,
4. collectables (pizzas), puis envoi du snapshot (sons, effets, secousses partent à ce moment).

Donc :

* `onUse` tourne **avant** la physique du joueur sur ce tick, et **après** les `onUpdate` des projectiles.
* `onUse` n'est pas appelé si le joueur est mort (`isAlive == false`), ni s'il est `isControlled`… non : il *est* appelé même si `isControlled`, seule la mort bloque l'appel. (Un joueur mort ne joue plus du tout.)
* Avant même d'appeler `onUse`, le moteur vérifie deux conditions ; si l'une échoue, **rien ne se passe et `onUse` n'est pas appelé** :
  * le cooldown doit être écoulé (comparaison stricte : il faut dépasser `cooldown`),
  * le joueur doit avoir `score >= cost`.
* Le chronomètre de cooldown démarre à la **création de l'ability**, c'est-à-dire au début du match : une ability à `cooldown = 20.0` n'est pas utilisable pendant les 20 premières secondes.
* Le pas de simulation est fixe : **1/60 s**. Le moteur peut rattraper jusqu'à 5 pas par image.

### Ce que change la valeur de retour

C'est le point le plus important :

| Retour de `onUse` | Coût débité | Cooldown relancé |
| --- | --- | --- |
| `true` | **oui** (`score -= cost`) | **oui** |
| `false` | non | non |
| `nil` / rien retourné | non | non |

Autrement dit : **on ne paie que ce qui a servi**, et on peut réessayer immédiatement. C'est ce qui permet le motif des mods de base :

```lua
-- freeze.lua : on ne paie que si quelqu'un était à portée
onUse = function(self, player, ctx)
    local cx, cy = player:getCenter()
    local cibles = ctx:playersInRadius(cx, cy, 300, { player })
    for _, cible in ipairs(cibles) do
        cible:setVelocity(0, 0)
    end
    return #cibles > 0
end
```

```lua
-- missileAbility.lua : on ne paie que si le projectile a vraiment été créé
onUse = function(self, player, ctx)
    return missile.spawn(ctx, player) ~= nil
end
```

Retourne toujours un **booléen**. `return 1` ou `return {}` n'est pas une valeur fiable : le moteur attend un booléen et traitera le reste comme « pas utilisé ».

### Erreurs Lua dans `onUse`

Si `onUse` lève une erreur (index d'un `nil`, appel avec `.` au lieu de `:`…) :

* la console affiche `Ability error : <fichier>:<ligne>: <message>`,
* le coût n'est **pas** débité et le cooldown n'est **pas** relancé,
* le reste du tick continue normalement, le jeu ne plante pas.

Attention : certaines actions peuvent avoir déjà été exécutées avant l'erreur (un projectile créé, un son mis en file). Une ability qui plante au milieu peut laisser des choses derrière elle.

---

## Les projectiles

Le projectile est l'objet à tout faire : balle, missile, objet posé au sol, véhicule. Le moteur en fait **le minimum** :

* il applique `vx` / `vy` chaque tick,
* il dessine la texture à la position du projectile, avec l'angle demandé,
* il appelle ton `onUpdate`.

Il n'y a **pas** de gravité, **pas** de durée de vie, **pas** de collision projectile/joueur, **pas** de dégâts automatiques. Tout ça, c'est à ton script de le faire (typiquement : `ctx:explode` depuis `onUpdate`).

### `ctx:spawnProjectile(params)`

```lua
---@param params table
---@return Projectile|nil
local p = ctx:spawnProjectile{ ... }
```

Retourne le projectile créé, ou `nil` en cas d'échec de validation.

| Champ | Type | Obligatoire | Défaut réel | Unité / effet |
| --- | --- | --- | --- | --- |
| `texture` | string | **oui** | — | id d'une texture chargée par `loadTexture` |
| `x` | number | **oui** | — | px, coin haut-gauche |
| `y` | number | **oui** | — | px, coin haut-gauche |
| `vx` | number | non | `0` | px/s, positif = vers la droite |
| `vy` | number | non | `0` | px/s, **positif = vers le bas** |
| `width` | integer | non | largeur native de la texture (`0` si texture inconnue) | px, taille de la « boîte » du projectile |
| `height` | integer | non | hauteur native de la texture (`0` si texture inconnue) | px |
| `onUpdate` | function | non | aucun | `function(self, ctx, dt)`, appelé chaque tick |

**Tout autre champ est purement ignoré** par le moteur : `owner`, `onHit`, `onDeath`, `angle`, `lifetime`… n'existent pas. Pour l'owner, utilise `p.data.owner` (voir plus bas).

Validations :

| Cas | Message console | Retour |
| --- | --- | --- |
| `texture`, `x` ou `y` manquant | `[lua] spawnProjectile : texture, x and y are required` | `nil` |
| id de texture inconnu | `[lua] spawnProjectile : unknown texture <id>` | **le projectile est quand même créé** |

Ce deuxième cas est sournois : le projectile existe, bouge, exécute son `onUpdate`, mais il est **invisible** et sa taille vaut `0 x 0` (donc `getSize()` rend `0, 0` et `isOffScreen()` se comporte comme un point). Surveille la console.

`width` / `height` ne changent **pas** le rendu : l'image est toujours dessinée à sa taille native, tournée autour de son centre. Ils ne servent qu'à `getSize()` et à `isOffScreen()` — et à tes propres calculs.

### Cycle de vie, tick par tick

Le pas de simulation est fixe : `dt = 1/60 ≈ 0.0167 s`.

**Au moment de l'appel à `spawnProjectile`** (depuis un `onUse` ou depuis un autre `onUpdate`) :

* l'objet est créé tout de suite : `texture`, `x`, `y`, `width`, `height`, `vx`, `vy`, `onUpdate` sont lus **une seule fois**, maintenant ;
* sa table `data` est créée, vide ;
* il est mis dans une **file d'attente** et le handle t'est rendu. Tu peux donc remplir `p.data.*` immédiatement, avant le premier `onUpdate` ;
* il n'est **pas encore** affiché, **pas** déplacé, **pas** mis à jour.

**Au tick suivant**, le gestionnaire de projectiles fait, dans cet ordre : mise à jour des projectiles déjà vivants → suppression des morts → **intégration de la file d'attente**. Ton projectile devient vivant à ce moment, et son premier `onUpdate` tombe donc au tick d'après. En clair : compte **deux ticks (≈ 33 ms) avant le premier `onUpdate`** d'un projectile créé par une ability. C'est pour ça que les mods de base calculent la position de départ eux-mêmes plutôt que de compter sur le premier `onUpdate`.

**Puis, à chaque tick, pour chaque projectile vivant :**

1. s'il est mort, on passe ;
2. si `onUpdate` existe → `onUpdate(self, ctx, dt)` ;
   * si le script lève une erreur : `Projectile error : <message>` est affiché et **le projectile est tué** ;
   * si le script a appelé `self:kill()`, on s'arrête ici : **pas de déplacement ce tick** ;
3. déplacement : `x += vx * dt`, `y += vy * dt` ;
4. filet de sécurité : si le projectile est complètement sorti de l'écran **avec une marge de 5000 px**, il est tué. C'est une protection contre les fuites, pas une règle de gameplay : un projectile peut donc errer très loin hors écran pendant longtemps. **Si tu veux qu'il meure en sortant de l'écran, fais-le toi-même** avec `isOffScreen()`.

**Mort.** Un projectile mort (`kill()` ou erreur) disparaît du rendu dès le tick courant, et il est retiré de la liste au début du tick suivant. Son `onUpdate` ne sera plus jamais appelé. Il n'y a pas de hook de mort.

Note sur l'ordre dans un tick : les projectiles sont mis à jour **avant** les joueurs. Les positions de joueurs que tu lis dans un `onUpdate` sont donc celles de la fin du tick précédent.

### `onUpdate(self, ctx, dt)`

| Paramètre | Type | Détail |
| --- | --- | --- |
| `self` | `Projectile` | le projectile lui-même (objet C++) |
| `ctx` | `GameContext` | le contexte de la partie |
| `dt` | number | secondes depuis le tick précédent, toujours `1/60` |

C'est là que vit tout le comportement : gravité, guidage, fusée, minuterie, détection de proximité, explosion, nettoyage.

```lua
-- une gravité, en trois lignes
local function onUpdate(self, ctx, dt)
    local vx, vy = self:getVelocity()
    self:setVelocity(vx, vy + 900 * dt)   -- 900 px/s² vers le bas

    if self:isOffScreen(50) then self:kill() end
end
```

### `self.data` : le seul endroit où ranger ton état

`self` est un objet C++ : **on ne peut pas lui ajouter de champs**. Toute écriture doit passer par `self.data`, une table Lua libre, propre à ce projectile et créée automatiquement.

```lua
-- NON : silencieusement inutile ou source d'erreur
self.timer = (self.timer or 0) + dt

-- OUI
self.data.timer = (self.data.timer or 0) + dt
```

On peut y mettre n'importe quoi, y compris des `Player` et d'autres `Projectile` :

```lua
local p = ctx:spawnProjectile{ texture = "missile", x = x, y = y, onUpdate = onUpdate }
if not p then return nil end
p.data.owner   = owner                        -- le Player lanceur
p.data.channel = ctx:playSFX("missileLaunch") -- le handle du son, pour pouvoir le couper
```

`data` se lit aussi depuis l'extérieur (`p.data.owner`), ce qui en fait le canal de communication naturel entre une ability et ses projectiles.

### Et le réseau ?

La simulation (donc `onUse`, `onUpdate`, `data`) ne tourne que côté hôte. Ce qui est répliqué pour chaque projectile, c'est uniquement : identifiant, id de texture, position, angle, largeur/hauteur. La logique Lua et `data` ne traversent pas le réseau. Les sons, animations et secousses passent par la file d'événements et sont rejoués à l'identique sur chaque machine.

---

## Référence : objet Player

Fourni par le moteur (`onUse`, `ctx:players()`, `ctx:playersInRadius()`, `ctx:explode()`). Jamais construit depuis Lua. Appels avec `:`, propriétés avec `.`.

Point commun à toutes les positions : elles sont lues sur la **boîte de collision** du joueur, qui est en coordonnées **entières** et n'est synchronisée qu'à la fin de la mise à jour du joueur. Deux conséquences : les positions de joueur sont arrondies au pixel, et juste après un `teleport()` les getters renvoient encore l'**ancienne** position jusqu'à la mise à jour du joueur. Tant que `isControlled` est `true`, la boîte n'est plus synchronisée du tout : la position lue est figée.

### Propriétés

| Propriété | Type | Accès | Détail |
| --- | --- | --- | --- |
| `isAlive` | boolean | **lecture seule** | `false` dès que la vie tombe à 0 ou moins. Écrire dessus lève une erreur. |
| `isControlled` | boolean | lecture / **écriture** | `true` = le moteur ne pilote plus ce joueur : plus d'entrées, plus de physique, plus de collision entre joueurs, et **plus de dégâts ni de soins** (`damage`/`heal` sont ignorés). Sert aux abilities qui prennent le contrôle du joueur. **Repasse-le à `false` quand tu as fini**, sinon le joueur reste inerte pour le reste du match. |

```lua
-- extrait de base/projectiles/christmas.lua
if rider and rider.isAlive then
    rider.isControlled = true
    rider:teleport(x + RIDER_OFFSET_X, y)
end
-- ... et plus tard, quand le traîneau sort de l'écran :
if rider and rider.isControlled then
    rider.isControlled = false
    rider:teleport(ctx:screenWidth() / 2, ctx:screenHeight() / 2)
end
```

### Méthodes

#### `player:getPosition() -> x, y`
Coin **haut-gauche**, en px. Deux valeurs de retour.

```lua
local x, y = player:getPosition()
```

#### `player:getCenter() -> cx, cy`
Centre, en px. C'est presque toujours ce que tu veux pour viser, faire partir un projectile ou centrer une explosion.

```lua
local cx, cy = player:getCenter()
ctx:spawnEffect("explosion_missile", cx, cy, 3.0)
```

#### `player:getSize() -> w, h`
Taille de la boîte de collision, en px (entiers ; 32 x 32 par défaut).

#### `player:setVelocity(vx, vy)`
**Remplace** la vitesse. px/s, `vy` positif = vers le bas. Aucun retour.

```lua
cible:setVelocity(0, 0)   -- freeze : coupe net l'élan (la gravité reprend au tick suivant)
```

#### `player:applyKnockBack(fx, fy)`
**Ajoute** à la vitesse (`vx += fx`, `vy += fy`). px/s. C'est ce qu'utilise `ctx:explode` en interne.

```lua
player:applyKnockBack(0, -800)   -- petit saut vers le haut
```

#### `player:teleport(x, y)`
Déplace instantanément, sans vitesse ajoutée. `x`/`y` = coin haut-gauche, px. Attention : la position *lue* ne suit qu'au prochain pas du joueur (voir l'avertissement ci-dessus).

#### `player:getLife() -> integer`
Vie courante. Peut devenir négative avant que la mort soit constatée.

#### `player:getMaxLife() -> integer`
Vie maximale (100 par défaut, réglée par la config du jeu).

#### `player:damage(amount)`
Retire `amount` points de vie. `amount` est un **entier positif** (`damage(30)` enlève 30). Pas de retour.
Nuances du moteur : ignoré si le joueur est `isControlled` ; certains skins encaissent moins (le skin tortue prend 75 % des dégâts) ; la mort n'est constatée qu'à la mise à jour du joueur, donc `isAlive` peut rester `true` jusqu'à la fin du tick. Un éventuel 2e argument (source du dégât) **n'existe pas** et est ignoré.

#### `player:heal(amount)`
Ajoute `amount` points de vie. **Pas de plafond** : on peut dépasser `getMaxLife()`. Ignoré si `isControlled`.

```lua
player:heal(math.min(30, player:getMaxLife() - player:getLife()))  -- plafonner soi-même
```

#### `player:getScore() -> integer`
Score courant. C'est aussi la monnaie des abilities (`cost`).

#### `player:addScore(amount)`
Ajoute `amount` (négatif pour retirer). Pas de plafond ni de plancher : le score peut devenir négatif.

#### `player:isAlive()` — n'existe pas comme méthode
`isAlive` est une **propriété** : `if p.isAlive then`, jamais `p:isAlive()`.

### Comparer des joueurs

Le moteur expose le même objet d'un appel à l'autre, donc `==` / `~=` fonctionnent pour exclure le lanceur — c'est ce que font les mods de base :

```lua
for _, p in ipairs(ctx:players()) do
    if p.isAlive and p ~= self.data.owner then
        -- cible valable
    end
end
```

---

## Référence : objet Projectile

Fourni par `ctx:spawnProjectile` (valeur de retour) et par `onUpdate` (`self`). Jamais construit depuis Lua.

### Propriété

| Propriété | Type | Accès | Détail |
| --- | --- | --- | --- |
| `data` | table | lecture (et écriture **dans** la table) | table Lua libre, propre à ce projectile, créée vide au spawn. Le seul endroit où stocker ton état. `p.data.x = 1` est correct ; remplacer la table elle-même n'est pas supporté. |

### Méthodes

#### `self:isValid() -> boolean`
`false` si le projectile a été détruit. Indispensable si tu as gardé un handle ailleurs (dans le `data` d'un autre projectile, dans le `self` d'une ability) : le handle Lua survit à la mort du projectile.

```lua
local p = self.data.cible
if p and p:isValid() then
    p:kill()
end
```

#### `self:getPosition() -> x, y`
Coin **haut-gauche**, en px, en flottant (plus précis que pour un joueur).

#### `self:setPosition(x, y)`
Téléporte le projectile (coin haut-gauche). Effet immédiat, pas de vitesse ajoutée.

#### `self:getVelocity() -> vx, vy`
px/s.

#### `self:setVelocity(vx, vy)`
**Remplace** la vitesse. Le moteur l'appliquera à la fin du tick courant.

```lua
-- rester collé au décor qui défile
self:setVelocity(-ctx:worldSpeed(), 0)
```

#### `self:getSize() -> w, h`
`width`/`height` du spawn (ou la taille native de la texture), en entiers. Rappel : c'est une boîte informative — le moteur **ne teste aucune collision** avec, et le rendu l'ignore.

#### `self:setAngle(degrees)`
Angle de **rendu**, en degrés, rotation autour du centre de l'image. Purement visuel : la direction de déplacement reste `vx`/`vy`.

```lua
self:setAngle(math.deg(angle) + 90)   -- +90 car le sprite missile pointe vers le haut
```

#### `self:isOffScreen(margin) -> boolean`
`true` quand le projectile est **entièrement** hors de l'écran, au-delà de `margin` pixels. `margin` est optionnel, défaut `0`.

Détail utile : le test se fait sur l'écran **complet** (1024 x 576), barre du bas comprise — pas sur `effectiveHeight`. Et il utilise `width`/`height` : avec une texture inconnue (taille 0), le projectile est traité comme un point.

```lua
if self:isOffScreen(50) then self:kill() end
```

#### `self:kill()`
Détruit le projectile. Immédiat : s'il est tué dans son propre `onUpdate`, il **ne bouge pas** ce tick, disparaît du rendu dès maintenant et ne sera plus mis à jour. Il n'y a aucun hook appelé à la mort : fais tes effets avant de l'appeler.

---

## Référence : objet GameContext (ctx)

Passé en 3e argument de `onUse` et en 2e de `onUpdate`. Il n'existe que pendant un match. Toutes les méthodes s'appellent avec `:`.

### `ctx:players() -> Player[]`
Tableau (1..n) de **tous** les joueurs de la partie, **morts compris**. Filtre toi-même sur `isAlive`. Chaque appel reconstruit un tableau : hors d'une boucle serrée, ce n'est pas grave, mais ne l'appelle pas deux fois quand une fois suffit.

```lua
for _, p in ipairs(ctx:players()) do
    if p.isAlive then p:heal(5) end
end
```

### `ctx:playersInRadius(x, y, radius, ignore) -> Player[]`

| Paramètre | Type | Détail |
| --- | --- | --- |
| `x`, `y` | number | centre de la zone, px |
| `radius` | number | rayon, px |
| `ignore` | `Player[]` ou absent | **un tableau** de joueurs à exclure |

Ne rend que les joueurs **vivants** dont le **centre** est à `radius` ou moins du point (comparaison en distance au carré, bords inclus).

:warning: `ignore` doit être un **tableau**. `ctx:playersInRadius(x, y, r, player)` est accepté sans erreur et **ignore silencieusement** l'argument : tu te touches toi-même. Écris `{ player }`.

```lua
local cibles = ctx:playersInRadius(cx, cy, 300, { player })
print(#cibles .. " joueurs à portée")
```

### `ctx:explode(params) -> Player[]`
Explosion **de gameplay uniquement** : dégâts + recul. Aucun rendu, aucun son, aucune secousse — c'est volontaire, pour que chaque explosion ait les siens. Retourne le tableau des joueurs touchés.

| Champ | Type | Obligatoire | Défaut | Détail |
| --- | --- | --- | --- | --- |
| `x`, `y` | number | **oui** | — | centre de l'explosion, px |
| `radius` | number | **oui** | — | portée, px (de centre à centre) |
| `damage` | number | non | `0` | dégâts **au centre** |
| `force` | number | non | `0` | recul **au centre**, en px/s ajoutés à la vitesse |
| `ignore` | `Player[]` | non | `{}` | joueurs épargnés |

Si `x`, `y` ou `radius` manque : `[lua] explode : x, y and radius are required`, retour vide, rien ne se passe.

Atténuation exacte : `facteur = 1 - (distance² / radius²)`. Elle est donc **quadratique** : on garde presque tous les dégâts près du centre, et ça s'effondre vers le bord (`facteur = 1` au centre, `0` au bord). Les dégâts appliqués valent `damage * facteur`, **tronqués à l'entier**. Le recul est dirigé du centre de l'explosion vers le centre du joueur, de norme `force * facteur` ; si le joueur est pile au centre, le recul est **vers le haut**.

:warning: Le champ `owner` est **lu par personne**. Passer `owner = player` ne protège pas le lanceur. Pour l'épargner, il faut `ignore = { player }`. Les projectiles de base (missile, cône, cadeaux) passent `owner` sans `ignore` : **leur propriétaire est donc touché par sa propre explosion**, c'est le comportement actuel du jeu.

```lua
ctx:explode{
    x = cx, y = cy, radius = 150,
    damage = 50, force = 1500,
    ignore = { player },     -- la seule façon d'épargner le lanceur
}
```

### `ctx:playSFX(id) -> integer`
Joue un son déjà chargé par le jeu. Retourne un **handle** (entier > 0) à garder si tu veux pouvoir couper le son avant la fin ; `0` si le son n'a pas pu être mis en file.

Sons disponibles (chargés en C++, un mod ne peut pas en ajouter) : `"jetpackThrust"`, `"missileLaunch"`, `"explosion"`, `"boing"`.

Le handle **n'est pas un canal audio**. Le son ne part pas pendant l'exécution de ton script : l'ordre est empilé dans une file d'événements et joué **après le tick**, à la réception du snapshot. Le handle est en revanche identique sur toutes les machines, ce que ne serait pas un numéro de canal.

Un id inconnu ne provoque aucun message : la file reçoit un ordre qui ne jouera rien, et tu récupères quand même un handle non nul.

```lua
self.data.channel = ctx:playSFX("missileLaunch")
```

### `ctx:stopSFX(handle)`
Coupe le son correspondant. Sans effet si le son est déjà terminé, si le handle est inconnu ou vaut `0`. Aucun retour.

```lua
if self.data.channel then ctx:stopSFX(self.data.channel) end
```

### `ctx:spawnEffect(animId, x, y, scale)`

| Paramètre | Type | Obligatoire | Détail |
| --- | --- | --- | --- |
| `animId` | string | oui | id d'une animation enregistrée par le jeu |
| `x`, `y` | number | oui | **centre** de l'animation, px |
| `scale` | number | non, défaut `1` | facteur d'échelle (2.0 = double taille) |

Joue une animation une fois, centrée sur `(x, y)`. Purement visuel, aucun effet de gameplay. Aucun retour, aucun handle : on ne peut pas l'arrêter.

Animations disponibles (enregistrées en C++, un mod ne peut pas en ajouter — voir section 10) : **`"explosion_missile"`** uniquement. Un id inconnu ne fait rien et n'affiche rien.

```lua
ctx:spawnEffect("explosion_missile", cx, cy, 2.0)
```

### `ctx:shakeScreen(intensity, duration)`

| Paramètre | Type | Détail |
| --- | --- | --- |
| `intensity` | number | amplitude en px (≈ 5 pour un petit coup, 16 pour une grosse explosion) |
| `duration` | number | **secondes** |

Fait trembler l'écran. Aucun retour.

Les secousses **ne s'additionnent pas** : si une secousse est déjà en cours, le moteur garde le **maximum** des deux intensités et le **maximum** des deux durées. Deux explosions simultanées ne font donc pas une secousse deux fois plus forte. L'intensité décroît ensuite d'environ 20 px/s, et en dessous de 1 px il n'y a plus de tremblement visible : une `intensity` inférieure à 1 ne sert à rien.

```lua
ctx:shakeScreen(16, 0.3)
```

### `ctx:spawnProjectile(params) -> Projectile|nil`
Voir la section « Les projectiles ».

### `ctx:worldSpeed() -> number`
Vitesse de défilement du décor, en px/s (50 actuellement). Un objet censé rester **posé dans le monde** (un cône, un obstacle) doit reculer à cette vitesse :

```lua
self:setVelocity(-ctx:worldSpeed(), 0)
```

### `ctx:screenWidth() -> integer`
Largeur logique de l'écran : **1024**.

### `ctx:screenHeight() -> integer`
Hauteur logique **totale** : **576**, barre d'interface du bas comprise.

### `ctx:screenHeight()` vs `ctx:effectiveHeight() -> integer`
`effectiveHeight` est la hauteur **jouable**, soit `screenHeight - 50` = **526**. C'est le « sol » : les joueurs sont bloqués à ce niveau. Pour poser un objet au sol :

```lua
y = ctx:effectiveHeight() - hauteurDeLObjet
```

À noter : `Projectile:isOffScreen()` raisonne sur `screenHeight` (576), pas sur `effectiveHeight`.

---

## Recettes

### 1. Projectile en ligne droite (le minimum viable)

Aucun `onUpdate` : le moteur applique la vitesse, c'est tout. Le projectile finira par être nettoyé par le filet de sécurité très loin hors écran — ajoute un `onUpdate` si tu veux qu'il meure proprement au bord.

```lua
loadTexture("monmod_bullet", "assets/dot.bmp")

registerAbility{
    id = "monmod_tir",
    cost = 50,
    cooldown = 0.4,

    onUse = function(self, player, ctx)
        local cx, cy = player:getCenter()

        local p = ctx:spawnProjectile{
            texture = "monmod_bullet",
            x = cx, y = cy,
            vx = 900,          -- vers la droite
            width = 8, height = 8,

            -- nettoyage explicite : sans ça, le projectile vit jusqu'à 5000 px hors écran
            onUpdate = function(self, ctx, dt)
                if self:isOffScreen(20) then self:kill() end
            end,
        }

        if not p then return false end   -- texture absente, x/y manquants...
        p.data.owner = player
        ctx:playSFX("missileLaunch")
        return true
    end
}
```

### 2. Projectile avec gravité (grenade)

La gravité est du **contenu**, pas du moteur : on la fabrique dans `onUpdate`.

```lua
local GRAVITE = 900     -- px/s²
local MECHE   = 1.5     -- secondes avant explosion

local function onUpdate(self, ctx, dt)
    -- 1. gravité : on accumule sur la vitesse verticale
    local vx, vy = self:getVelocity()
    self:setVelocity(vx, vy + GRAVITE * dt)

    -- 2. rebond sur le sol (le sol = effectiveHeight)
    local x, y = self:getPosition()
    local w, h = self:getSize()
    if y + h >= ctx:effectiveHeight() then
        self:setPosition(x, ctx:effectiveHeight() - h)
        local nvx, nvy = self:getVelocity()
        self:setVelocity(nvx * 0.7, -nvy * 0.5)   -- amorti
    end

    -- 3. mèche : self.data, jamais self
    self.data.meche = (self.data.meche or MECHE) - dt
    if self.data.meche > 0 then return end

    local cx, cy = x + w / 2, y + h / 2
    ctx:spawnEffect("explosion_missile", cx, cy, 2.0)
    ctx:playSFX("explosion")
    ctx:shakeScreen(10, 0.25)
    ctx:explode{
        x = cx, y = cy, radius = 120,
        damage = 45, force = 1200,
        ignore = { self.data.owner },   -- épargne le lanceur : il faut bien ignore
    }
    self:kill()
end

registerAbility{
    id = "monmod_grenade",
    cost = 250,
    cooldown = 3.0,

    onUse = function(self, player, ctx)
        local cx, cy = player:getCenter()
        local p = ctx:spawnProjectile{
            texture = "monmod_bullet",
            x = cx, y = cy,
            vx = 500, vy = -400,   -- lancée en cloche
            width = 12, height = 12,
            onUpdate = onUpdate,
        }
        if not p then return false end
        p.data.owner = player
        return true
    end
}
```

### 3. Explosion de zone instantanée (sans projectile)

Modèle de `base/abilities/kamikaze.lua`. Remarque l'ordre : effet, son et secousse sont indépendants de `explode`, et on retourne `true` inconditionnellement — l'ability sert toujours, même si elle ne touche personne.

```lua
registerAbility{
    id = "monmod_kamikaze",
    cost = 100,
    cooldown = 2.0,

    onUse = function(self, player, ctx)
        local cx, cy = player:getCenter()

        -- la partie visible / sonore
        ctx:spawnEffect("explosion_missile", cx, cy, 3.0)
        ctx:playSFX("explosion")
        ctx:shakeScreen(16, 0.3)

        -- la partie gameplay
        local touches = ctx:explode{
            x = cx, y = cy, radius = 150,
            damage = 50, force = 1500,
            ignore = { player },
        }

        -- explode rend la liste des joueurs touchés : on peut enchaîner
        for _, cible in ipairs(touches) do
            player:addScore(25)
            cible:setVelocity(select(1, cible:getCenter()) and 0 or 0, -600)  -- petit pop vertical
        end

        return true
    end
}
```

### 4. Projectile à tête chercheuse

Modèle de `base/projectiles/missile.lua`, resserré. Les points à retenir : l'angle est gardé dans `data` (le moteur ne le relit jamais), le braquage est limité par `dt`, et `setAngle` n'est que du décor.

```lua
local SIZE      = 32
local VITESSE   = 1000   -- px/s
local BRAQUAGE  = 3.0    -- radians/s
local PORTEE    = 70     -- px : distance à laquelle la mèche s'amorce
local MECHE     = 0.07   -- s

local function cible(self, ctx, cx, cy)
    local owner = self.data.owner
    local best, bestDist
    for _, p in ipairs(ctx:players()) do
        if p.isAlive and p ~= owner then           -- jamais le lanceur
            local px, py = p:getCenter()
            local d = (px - cx)^2 + (py - cy)^2    -- distance au carré : pas de sqrt inutile
            if not bestDist or d < bestDist then best, bestDist = p, d end
        end
    end
    return best
end

local function onUpdate(self, ctx, dt)
    local x, y = self:getPosition()
    local cx, cy = x + SIZE / 2, y + SIZE / 2

    local t = cible(self, ctx, cx, cy)
    if not t then return end   -- plus personne : le missile continue tout droit

    local tx, ty = t:getCenter()
    local dx, dy = tx - cx, ty - cy
    local dist = math.sqrt(dx * dx + dy * dy)

    -- angle voulu, puis braquage limité (sinon le missile se colle à la cible)
    local voulu = math.atan(dy, dx)
    local angle = self.data.angle or 0
    local diff  = (voulu - angle + math.pi) % (2 * math.pi) - math.pi
    local maxTurn = BRAQUAGE * dt
    if diff > maxTurn then angle = angle + maxTurn
    elseif diff < -maxTurn then angle = angle - maxTurn
    else angle = voulu end
    self.data.angle = angle

    self:setVelocity(math.cos(angle) * VITESSE, math.sin(angle) * VITESSE)
    self:setAngle(math.deg(angle) + 90)   -- +90 : le sprite pointe vers le haut

    -- mèche de proximité : on ne l'amorce qu'une fois
    if not self.data.meche and dist < PORTEE then self.data.meche = MECHE end
    if self.data.meche then
        self.data.meche = self.data.meche - dt
        if self.data.meche <= 0 then
            if self.data.channel then ctx:stopSFX(self.data.channel) end
            ctx:spawnEffect("explosion_missile", cx, cy, 2.0)
            ctx:playSFX("explosion")
            ctx:shakeScreen(8, 0.3)
            ctx:explode{ x = cx, y = cy, radius = 100, damage = 40, force = 1000,
                         ignore = { self.data.owner } }
            self:kill()
        end
    end
end

registerAbility{
    id = "monmod_missile",
    cost = 200,
    cooldown = 5.0,
    onUse = function(self, player, ctx)
        -- apparition hors écran, pour qu'il « arrive » de l'extérieur
        local p = ctx:spawnProjectile{
            texture = "missile",
            x = -200, y = math.random(0, ctx:effectiveHeight()),
            width = SIZE, height = SIZE,
            onUpdate = onUpdate,
        }
        if not p then return false end
        p.data.owner   = player
        p.data.channel = ctx:playSFX("missileLaunch")
        return true
    end
}
```

### 5. Un son qu'on coupe avant la fin

`playSFX` rend un handle ; on le range dans `data` (ou dans `self` pour une ability) et on le passe à `stopSFX`. Typiquement : un bruit de propulsion qu'on coupe quand le projectile explose, pour qu'il ne reste pas audible après la disparition de l'objet.

```lua
local function onUpdate(self, ctx, dt)
    self.data.t = (self.data.t or 0) + dt

    if self.data.t >= 2.0 then
        -- on coupe la boucle AVANT de jouer l'explosion
        if self.data.channel then
            ctx:stopSFX(self.data.channel)
            self.data.channel = nil
        end
        ctx:playSFX("explosion")
        self:kill()
    end
end

-- au spawn :
local p = ctx:spawnProjectile{ texture = "missile", x = cx, y = cy, vx = 400, onUpdate = onUpdate }
if p then
    p.data.channel = ctx:playSFX("missileLaunch")
end
```

À savoir : si tu appelles `playSFX` puis `stopSFX` **dans le même tick**, les deux ordres partent dans le même lot d'événements, donc le son est lancé puis coupé aussitôt — inaudible. Pour l'entendre, il faut couper à un tick ultérieur.

### 6. Objet posé au sol qui se déclenche au passage

Modèle de `base/projectiles/cone.lua` : un projectile qui joue le rôle de mine. Deux phases dans un seul `onUpdate`, séparées par un drapeau dans `data`.

```lua
local SIZE    = 16
local PORTEE  = 20     -- px d'écart horizontal déclenchant la mine
local MONTEE  = 1500   -- px/s

local function onUpdate(self, ctx, dt)
    local x, y = self:getPosition()
    local owner = self.data.owner

    if not self.data.armee then
        -- phase 1 : posé au sol, il recule avec le décor
        self:setVelocity(-ctx:worldSpeed(), 0)

        if self:isOffScreen(50) then self:kill() return end

        for _, p in ipairs(ctx:players()) do
            if p.isAlive and p ~= owner then
                local px = p:getPosition()
                if math.abs(px - x) < PORTEE then
                    self.data.armee = true
                    self:setVelocity(0, -MONTEE)     -- il jaillit vers le haut
                    self.data.channel = ctx:playSFX("missileLaunch")
                    break
                end
            end
        end
    else
        -- phase 2 : il monte, et explose dès qu'il a dépassé sa cible
        local cx, cy = x + SIZE / 2, y + SIZE / 2
        local best, bestDist
        for _, p in ipairs(ctx:players()) do
            if p.isAlive and p ~= owner then
                local px, py = p:getCenter()
                local d = (px - cx)^2 + (py - cy)^2
                if not bestDist or d < bestDist then best, bestDist = p, d end
            end
        end

        if best then
            local _, py = best:getCenter()
            if py > cy then       -- la cible est désormais SOUS la mine
                if self.data.channel then ctx:stopSFX(self.data.channel) end
                ctx:spawnEffect("explosion_missile", cx, cy, 4.0)
                ctx:playSFX("explosion")
                ctx:shakeScreen(8, 0.3)
                ctx:explode{ x = cx, y = cy, radius = 200, damage = 50, force = 2000,
                             ignore = { owner } }
                self:kill()
            end
        elseif self:isOffScreen(50) then
            self:kill()
        end
    end
end

registerAbility{
    id = "monmod_mine",
    cost = 200,
    cooldown = 2.0,
    onUse = function(self, player, ctx)
        local p = ctx:spawnProjectile{
            texture = "trafficCone",
            x = ctx:screenWidth() + 10,              -- entre par la droite
            y = ctx:effectiveHeight() - SIZE,        -- posé sur le sol
            width = SIZE, height = SIZE,
            vx = -ctx:worldSpeed(),
            onUpdate = onUpdate,
        }
        if not p then return false end
        p.data.owner = player
        return true
    end
}
```

---

## Pièges connus

**Passer `player` au lieu de `{ player }` à un paramètre `ignore`.**
`ctx:playersInRadius(x, y, r, player)` et `ctx:explode{ ..., ignore = player }` sont acceptés **sans aucun message d'erreur** : le moteur n'arrive pas à convertir, et traite la liste comme vide. Symptôme : le lanceur se gèle lui-même, ou prend sa propre explosion en pleine figure. Écris toujours des accolades : `{ player }`, `{ player, allie }`.

**Confondre `.` et `:` sur un objet du moteur.**
`player.getLife()` appelle la méthode sans son objet : erreur Lua immédiate. Symptôme : `Ability error : ... attempt to index a nil value` ou un message sur un argument invalide, et l'ability ne fait rien (sans débiter le coût). Les **méthodes** prennent `:` (`player:getLife()`, `self:kill()`, `ctx:players()`), les **propriétés** prennent `.` (`p.isAlive`, `p.isControlled`, `proj.data`). Mémo : si c'est dans la liste des méthodes de ce document, c'est `:`.

**Écrire sur `self` au lieu de `self.data` dans un projectile.**
`self` est un objet C++, il n'accepte pas de nouveaux champs. `self.timer = 0` ne stocke rien d'utilisable. Symptôme : un compteur qui reste à sa valeur initiale, une mèche qui ne s'écoule jamais, un projectile immortel. Tout l'état d'un projectile va dans `self.data`. (Dans une **ability**, c'est l'inverse : `self` est une vraie table Lua, propre à chaque joueur, et on peut y écrire.)

**`getPosition()` rend le coin haut-gauche, pas le centre.**
Symptôme classique : une explosion décalée d'une demi-sprite vers le haut-gauche, un missile qui vise systématiquement à côté, une détection de proximité qui se déclenche trop tôt. Pour un joueur, utilise `getCenter()`. Pour un projectile, il n'y a pas de `getCenter()` : calcule-le.

```lua
local x, y = self:getPosition()
local w, h = self:getSize()
local cx, cy = x + w / 2, y + h / 2
```

**Les temps sont en secondes, jamais en millisecondes.**
`cooldown = 2000` = 2000 secondes, soit une ability utilisable une fois toutes les 33 minutes. Pareil pour la durée de `shakeScreen` : `0.3`, pas `300`. Symptôme : une ability qui « ne marche pas » alors que la console n'affiche rien — c'est le cooldown qui n'est pas écoulé.

**Un id de texture inconnu dans `spawnProjectile`.**
Le projectile **est quand même créé**. Il bouge, exécute son `onUpdate`, explose… mais il est invisible et sa taille vaut `0 x 0`. Symptôme : « mon projectile ne s'affiche pas » alors que les dégâts, eux, arrivent. Le seul indice est la console : `[lua] spawnProjectile : unknown texture <id>`. Vérifie que le `loadTexture` correspondant a bien eu lieu, qu'il a retourné `true`, et qu'il s'est exécuté **avant** (c'est-à-dire dans l'`init.lua`, pas au premier tir).

**Un chapeau qui n'apparaît pas dans le menu.**
Le menu lit le dossier `assets/hats/`, pas ta liste de `registerHat`. Pour qu'un chapeau soit sélectionnable : le fichier `assets/hats/<nom>.png` doit exister, et l'id passé à `registerHat` doit être exactement `"hat_<nom>"`. L'ordre d'affichage est celui du système de fichiers, et l'éventuel champ `title` n'est pas lu.

**Un chapeau sans ability valide fait planter le jeu.**
Dès le début du match, le moteur lit la progression du cooldown de l'ability de chaque joueur sans vérifier qu'il y en a une. Un chapeau sélectionnable sans `ability`, ou avec un `ability` qui pointe sur un id inexistant (console : `createAbility : unknown ability : <id>`), provoque un crash. Vérifie toujours dans la console que `[lua] ability saved : <id>` est bien apparu.

**Oublier de remettre `isControlled = false`.**
Tant qu'un joueur est `isControlled`, il n'a plus d'entrées, plus de physique, plus de collisions, et il est **invulnérable** (`damage` et `heal` sont ignorés) — en plus, sa position lue par Lua est figée. Symptôme : un joueur inerte et intouchable pour le reste du match. Prévois toujours la sortie de contrôle, y compris si le projectile qui le pilote meurt autrement que prévu.

**Attendre un `onUpdate` sur le tick du spawn.**
Un projectile créé dans un `onUse` n'est ni déplacé ni mis à jour ce tick-là ; son premier `onUpdate` tombe deux ticks plus tard (≈ 33 ms). Symptôme : une position de départ « sautée », un projectile qui semble apparaître avec un temps de retard. Calcule la position de départ au spawn, ne compte pas sur `onUpdate` pour la corriger.

**Croire que `owner` protège le lanceur.**
Ni `spawnProjectile` ni `explode` ne lisent `owner`. Dans `spawnProjectile`, le champ est totalement ignoré ; dans `explode`, seul `ignore` épargne quelqu'un. Symptôme : le lanceur prend ses propres dégâts (c'est le cas, aujourd'hui, du missile, du cône et des cadeaux du traîneau). Range le lanceur toi-même dans `p.data.owner` et pense à `ignore = { owner }`.

**Compter sur une destruction automatique hors écran.**
Le moteur ne tue un projectile que s'il dépasse l'écran de **5000 px** — c'est un filet de sécurité, pas une règle de jeu. Symptôme : des dizaines de projectiles invisibles qui continuent de tourner, qui coûtent du temps de calcul et qui occupent de la bande passante. Ajoute `if self:isOffScreen(50) then self:kill() end` dans ton `onUpdate`.

**Oublier qu'il n'y a aucune collision projectile/joueur.**
Le moteur ne teste jamais le contact entre un projectile et un joueur, et `width`/`height` ne servent qu'à `getSize()` et `isOffScreen()`. Tout le « ça touche » se fait à la main : `ctx:explode`, `ctx:playersInRadius`, ou une boucle sur `ctx:players()`.

**`require` sans le nom du mod.**
`require("projectiles.laser")` échoue (`module not found`) et le mod entier est sauté avec `Error Script Engine : ...`. Il faut `require("monmod.projectiles.laser")`.

**Se reposer sur `MOD_DIR` en dehors du chargement.**
Ces globales sont écrasées à chaque mod. Lues dans un `onUse`, elles contiennent les valeurs du **dernier** mod chargé. Copie-les dans une locale au chargement.

**Chercher `io`, `os` ou un module externe.**
Seules `base`, `math`, `string`, `table` et `package` sont ouvertes ; `package.cpath` est vide et `package.loadlib` supprimé. Pas de lecture de fichier, pas d'horloge, pas de bibliothèque C.

**Compter sur un plafond de vie ou de score.**
`heal` peut dépasser `getMaxLife()`, `addScore(-1000)` peut rendre le score négatif, et `getLife()` peut être négatif pendant un tick avant que `isAlive` passe à `false`. Fais tes propres bornes si tu en veux.

---

## Ce qui n'existe pas encore

Ces éléments sont annotés dans `mods/base/meta/akaka.lua` (souvent marqués `[TODO]`) mais **aucun binding ne les expose** : les appeler lève une erreur Lua, ou le champ est silencieusement ignoré. À ne pas utiliser.

### Fonctions globales
* **`registerAnimation(id, def)`** — pas de binding. Les animations sont enregistrées en C++ ; la seule utilisable avec `ctx:spawnEffect` est `"explosion_missile"`. Un mod ne peut donc pas livrer sa propre animation (il peut en revanche charger des textures et animer un projectile à la main).
* Pas de fonction pour charger un **son**. Les sons disponibles sont ceux du jeu : `"jetpackThrust"`, `"missileLaunch"`, `"explosion"`, `"boing"`.

### Hooks d'ability
Seul `onUse` est appelé. Les champs suivants peuvent être présents dans la définition sans aucun effet :
* `onUpdate(self, player, ctx, dt)` — pas de hook par frame (donc pas de passif, d'aura ni de recharge progressive).
* `onMatchStart(self, player, ctx)`
* `onDamaged(self, player, ctx, amount, source)`
* `onDeath(self, player, ctx, killer)`

### Hooks de projectile
Seul `onUpdate` est lu par le constructeur. Sont ignorés :
* `onHit(self, ctx, target)` — **il n'y a aucune détection de collision projectile/joueur dans le moteur**. C'est la lacune la plus importante : tout contact doit être calculé dans `onUpdate`.
* `onDeath(self, ctx)` — `kill()` ne déclenche rien. Fais tes effets avant d'appeler `kill()`.
* `owner` — le champ n'est pas lu du tout. Passe par `p.data.owner`.

### Méthodes de Projectile
* `Projectile:getOwner()` — n'existe pas (le moteur ne connaît pas de propriétaire).

### Méthodes de Player
* `Player:getVelocity()` — n'existe pas. On peut poser une vitesse (`setVelocity`, `applyKnockBack`) mais pas la relire.
* `Player:getIndex()` — n'existe pas (pas de numéro de joueur accessible).
* `Player:getHatId()` — n'existe pas.
* `Player:addStatus(name, duration)` / `Player:hasStatus(name)` — n'existent pas : pas de système de statuts. Un « gel » se simule en remettant la vitesse à zéro chaque tick, ou via `isControlled`.
* Le 2e argument `source` de `Player:damage(amount, source)` — ignoré : les dégâts et les kills ne sont attribués à personne.

### Méthodes de GameContext
* `GameContext:nearestPlayer(from)` — n'existe pas. À faire à la main en bouclant sur `ctx:players()` (voir la recette « tête chercheuse »).
* `GameContext:after(delay, fn)` / `GameContext:every(interval, fn)` et l'objet `Timer` (`Timer:cancel()`) — n'existent pas. Il n'y a **aucun timer** : tout délai se compte en soustrayant `dt` dans `self.data` depuis un `onUpdate` de projectile.
* Le champ `owner` de `ctx:explode{...}` — accepté mais jamais lu. Seul `ignore` épargne quelqu'un.

### Chapeaux
* Le champ `title` de `registerHat` — jamais lu. Le menu affiche l'`id` brut.
* L'ordre de déclaration n'a pas d'influence sur l'ordre du menu, qui vient du scan de `assets/hats/`.
