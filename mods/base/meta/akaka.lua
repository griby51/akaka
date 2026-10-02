---@meta
--
-- Definitions des fonctions injectees dans Lua depuis le C++
-- (voir ScriptEngine::registerBindings dans src/ScriptEngine.cpp).
--
-- Ce fichier n'est JAMAIS execute : il ne sert qu'a l'autocompletion et a
-- documenter l'API de modding. A tenir a jour a chaque nouveau binding.
--
-- C'est aussi le CONTRAT de l'API : le C++ doit correspondre a ce fichier.
-- Tout ce qui est marque [TODO] est prevu mais pas encore implemente.
--
-- Conventions valables pour toute l'API :
--   * Temps    : toujours en SECONDES (le C++ convertit en ms en interne).
--   * Position : `getPosition` = coin HAUT-GAUCHE (le x/y du moteur),
--                `getCenter` = centre. Les fonctions de portee du moteur
--                (playersInRadius, explode) mesurent de centre a centre.
--                Pixels logiques (1024x576).
--   * Methodes : appel avec deux-points -> player:setVelocity(0, 0)
--   * Les objets C++ (Player, Projectile...) ne se creent jamais depuis Lua,
--     le moteur les fournit.


-- =============================================================================
-- Contenu : textures, animations
-- =============================================================================

---Charge une texture et l'enregistre sous l'identifiant `id`.
---@param id string Identifiant utilise ensuite pour retrouver la texture
---@param path string Chemin du fichier, relatif a la racine du jeu
---@return boolean ok `false` si le fichier est introuvable
function loadTexture(id, path) end

---@class AnimationDef
---@field texture string Identifiant de la sprite sheet (deja chargee avec loadTexture)
---@field frameW? integer Largeur d'une frame en pixels (defaut 64)
---@field frameH? integer Hauteur d'une frame en pixels (defaut 64)
---@field columns? integer Nombre de frames par ligne dans la sheet (defaut 5)
---@field frameCount? integer Nombre total de frames (defaut 5)
---@field frameDuration? number Duree d'une frame en secondes
---@field loop? boolean Rejouer en boucle (defaut false)

---[TODO] Declare une animation jouable ensuite avec `ctx:spawnEffect`.
---@param id string
---@param def AnimationDef
---@return boolean ok `false` si `texture` est absent
function registerAnimation(id, def) end


-- =============================================================================
-- Declarations : abilities et chapeaux
-- =============================================================================

---Etat propre a UN joueur pour UNE ability. Chaque joueur recoit sa propre
---table : on peut y ranger ce qu'on veut (charges, timers, projectiles...).
---Les champs de l'AbilityDef (cost, cooldown...) sont lisibles a travers elle.
---@class AbilityInstance : AbilityDef
---@field [string] any

---@class AbilityDef
---@field id string Identifiant unique, reference par registerHat
---@field cost? integer Score depense a chaque utilisation reussie (defaut 0)
---@field cooldown? number Temps minimum entre deux utilisations, en secondes (defaut 0)
---Appele quand le joueur active l'ability, SI le cooldown est fini et que le
---joueur a assez de score. Retourner `true` si l'ability a vraiment servi :
---c'est seulement dans ce cas que le cout est debite et le cooldown relance.
---@field onUse fun(self: AbilityInstance, player: Player, ctx: GameContext): boolean
---[TODO] Appele a chaque frame, meme sans activation (passifs, auras, charges).
---@field onUpdate? fun(self: AbilityInstance, player: Player, ctx: GameContext, dt: number)
---[TODO] Appele une fois au debut de la partie.
---@field onMatchStart? fun(self: AbilityInstance, player: Player, ctx: GameContext)
---[TODO] Appele quand le joueur prend des degats.
---@field onDamaged? fun(self: AbilityInstance, player: Player, ctx: GameContext, amount: number, source?: Player)
---[TODO] Appele quand le joueur meurt.
---@field onDeath? fun(self: AbilityInstance, player: Player, ctx: GameContext, killer?: Player)

---Declare une ability.
---@param def AbilityDef
function registerAbility(def) end

---@class HatDef
---@field id string Identifiant de la texture du chapeau (ex: "hat_witch")
---@field texture string Chemin de l'image, chargee automatiquement sous `id`
---@field ability? string Identifiant d'une ability declaree avec registerAbility
---@field title? string [TODO] Nom affiche dans le menu (defaut : l'id)

---Declare un chapeau selectionnable dans le menu.
---Les chapeaux apparaissent dans le menu dans l'ordre de declaration.
---@param def HatDef
function registerHat(def) end


-- =============================================================================
-- Player (fourni par le moteur, jamais cree depuis Lua)
-- =============================================================================

---@class Player
---@field isAlive boolean Lecture seule
local Player = {}

---Remplace la vitesse du joueur.
---@param vx number pixels/seconde
---@param vy number pixels/seconde
function Player:setVelocity(vx, vy) end

---Coin haut-gauche du joueur (le x/y du moteur).
---@return number x
---@return number y
function Player:getPosition() end

---Centre du joueur.
---@return number cx
---@return number cy
function Player:getCenter() end

---Taille du collider.
---@return integer w
---@return integer h
function Player:getSize() end

---[TODO]
---@return number vx
---@return number vy
function Player:getVelocity() end

---Deplace instantanement le joueur (coin haut-gauche).
---@param x number
---@param y number
function Player:teleport(x, y) end

---Ajoute une force ponctuelle (explosions, coups...).
---@param fx number
---@param fy number
function Player:applyKnockBack(fx, fy) end

---@return integer
function Player:getLife() end

---@return integer
function Player:getMaxLife() end

---Inflige des degats. `source` [TODO] servira a attribuer le kill / les points.
---@param amount integer
---@param source? Player
function Player:damage(amount, source) end

---Soigne le joueur.
---@param amount integer
function Player:heal(amount) end

---@return integer
function Player:getScore() end

---Ajoute (ou retire si negatif) du score.
---@param amount integer
function Player:addScore(amount) end

---[TODO] Position du joueur dans la partie (1, 2, 3...), stable pendant le match.
---@return integer
function Player:getIndex() end

---[TODO]
---@return string
function Player:getHatId() end

---[TODO] Applique un statut temporaire (ex: "frozen", "slowed").
---@param name string
---@param duration number secondes
function Player:addStatus(name, duration) end

---[TODO]
---@param name string
---@return boolean
function Player:hasStatus(name) end


-- =============================================================================
-- GameContext (passe en 2e argument des hooks, n'existe que pendant un match)
-- =============================================================================

---@class GameContext
local GameContext = {}

---Tous les joueurs de la partie, morts compris.
---@return Player[]
function GameContext:players() end

---Joueurs vivants dont le centre est a moins de `radius` de (x, y).
---@param x number
---@param y number
---@param radius number
---@param ignore? Player[] Joueurs a ignorer, ex: `{ player }` pour exclure le lanceur.
---Attention : `player` seul (sans accolades) est silencieusement ignore.
---@return Player[]
function GameContext:playersInRadius(x, y, radius, ignore) end

---[TODO] Joueur vivant le plus proche de `from` (lui-meme exclu).
---@param from Player
---@return Player|nil
function GameContext:nearestPlayer(from) end

---Vitesse de defilement du decor, en pixels/seconde. Un projectile pose dans
---le monde (un cone, un obstacle) doit reculer a cette vitesse pour rester
---colle au sol.
---@return number
function GameContext:worldSpeed() end

---Largeur logique de l'ecran (1024).
---@return integer
function GameContext:screenWidth() end

---Hauteur logique de l'ecran (576), barre du bas comprise.
---@return integer
function GameContext:screenHeight() end

---Hauteur jouable, sans la barre du bas : c'est le "sol".
---@return integer
function GameContext:effectiveHeight() end

---Joue une animation, centree sur (x, y).
---@param animId string
---@param x number
---@param y number
---@param scale? number (defaut 1)
function GameContext:spawnEffect(animId, x, y, scale) end

---Joue un son deja charge. Rend le canal SDL_mixer utilise, a garder si on
---veut pouvoir couper le son avant sa fin (`stopSFX`), ou -1 en cas d'echec.
---@param id string
---@return integer channel
function GameContext:playSFX(id) end

---Coupe le son qui joue sur ce canal.
---ATTENTION : si le son s'est deja termine, le canal a pu etre reattribue a un
---autre son, qui sera coupe a sa place.
---@param channel integer Valeur rendue par `playSFX`
function GameContext:stopSFX(channel) end

---Fait trembler l'ecran. Si une secousse est deja en cours, garde la plus forte
---intensite et la plus longue duree (deux explosions ne s'additionnent pas).
---@param intensity number Amplitude en pixels (ex: 16 pour une grosse explosion)
---@param duration number secondes
function GameContext:shakeScreen(intensity, duration) end

---@class ExplodeParams
---@field x number Centre de l'explosion
---@field y number
---@field radius number Portee ; degats et force diminuent avec la distance (1 au centre, 0 au bord)
---@field damage? number Degats au centre (defaut 0)
---@field force? number Knockback au centre (defaut 0)
---@field owner? Player Lanceur. Ne change PAS qui est touche ; [TODO] servira a crediter degats et kills
---@field ignore? Player[] Joueurs epargnes par l'explosion (defaut : personne, lanceur compris)

---Explosion de GAMEPLAY uniquement : degats + knockback, de centre a centre.
---Aucun rendu : l'effet, le son et la secousse se font a part, pour que chaque
---explosion ait les siens :
---```lua
---ctx:explode{ x = cx, y = cy, radius = 150, damage = 50, force = 1500, ignore = { player } }
---ctx:spawnEffect("explosion_missile", cx, cy)
---ctx:playSFX("explosion")
---ctx:shakeScreen(16, 0.3)
---```
---@param params ExplodeParams
---@return Player[] hit Joueurs touches
function GameContext:explode(params) end

---@class Timer
local Timer = {}

---[TODO] Annule le timer (sans effet s'il est deja fini).
function Timer:cancel() end

---[TODO] Appelle `fn` une fois apres `delay` secondes.
---Le timer est annule automatiquement a la fin du match.
---@param delay number
---@param fn fun()
---@return Timer
function GameContext:after(delay, fn) end

---[TODO] Appelle `fn` toutes les `interval` secondes jusqu'a `cancel()`.
---@param interval number
---@param fn fun()
---@return Timer
function GameContext:every(interval, fn) end


-- =============================================================================
-- Projectiles
-- =============================================================================

---Handle vers un projectile. Peut survivre au projectile : verifier isValid()
---avant de s'en servir si on l'a garde dans une table.
---
---Le moteur ne fait que le strict minimum : il applique vx/vy, dessine la
---texture et teste les collisions. Gravite, duree de vie et destruction hors
---ecran sont du CONTENU : c'est au script de les faire dans `onUpdate`.
---@class Projectile
local Projectile = {}

---Etat libre du script, propre a ce projectile (compteurs, cibles, flags).
---C'est ici que ca se range : `self` est un objet C++, on ne peut pas lui
---ajouter de champs.
---  self.data.timer = (self.data.timer or 0) + dt
---@type table
Projectile.data = {}

---`false` si le projectile a ete detruit.
---@return boolean
function Projectile:isValid() end

---Coin haut-gauche.
---@return number x
---@return number y
function Projectile:getPosition() end

---
---@return number vx
---@return number vy
function Projectile:getVelocity() end

---Le moteur applique cette vitesse chaque frame. Pour une gravite :
---  local vx, vy = self:getVelocity()
---  self:setVelocity(vx, vy + 900 * dt)
---@param vx number
---@param vy number
function Projectile:setVelocity(vx, vy) end

---Taille du collider.
---@return integer w
---@return integer h
function Projectile:getSize() end

---Teleporte le projectile (coin haut-gauche).
---@param x number
---@param y number
function Projectile:setPosition(x, y) end

---Rotation de l'image, en degres. Elle tourne autour de son centre.
---@param degrees number
function Projectile:setAngle(degrees) end

---`true` quand le projectile est ENTIEREMENT sorti de l'ecran.
---Le moteur ne detruit rien tout seul : a toi d'appeler `kill()` si c'est ce
---que tu veux (un projectile peut vouloir entrer depuis l'exterieur).
---@param margin? number Tolerance en pixels au-dela du bord (defaut 0)
---@return boolean
function Projectile:isOffScreen(margin) end

---[TODO]
---@return Player|nil
function Projectile:getOwner() end

---Detruit le projectile ([TODO] declenche onDeath).
function Projectile:kill() end

---@class ProjectileParams
---@field texture string Identifiant de texture (obligatoire)
---@field x number Coin haut-gauche (obligatoire)
---@field y number (obligatoire)
---@field vx? number Pixels/seconde (defaut 0)
---@field vy? number
---L'image est TOUJOURS dessinee a la taille native de la texture ; width et
---height ne changent que la hitbox.
---@field width? integer Taille du collider (defaut: taille de la texture)
---@field height? integer
---@field owner? Player Ignore par onHit
---@field onUpdate? fun(self: Projectile, ctx: GameContext, dt: number)
---Appele UNE SEULE FOIS PAR CIBLE quand le projectile touche un joueur
---(le owner est ignore). Pour mourir au premier contact : `self:kill()`.
---@field onHit? fun(self: Projectile, ctx: GameContext, target: Player)
---@field onDeath? fun(self: Projectile, ctx: GameContext)

---Cree un projectile gere par le moteur.
---Le plus simple ne demande aucun code : il part tout droit.
---  ctx:spawnProjectile{ texture = "bullet", x = cx, y = cy, vx = 800 }
---@param params ProjectileParams
---@return Projectile
function GameContext:spawnProjectile(params) end
