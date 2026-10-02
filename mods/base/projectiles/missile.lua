local missile = {}

local TEXTURE         = "missile"
local SIZE            = 32
local PRECISION       = 3.0
local VELOCITY        = 1000
local MIN_SPEED_RATIO = 0.8
local TRIGGER_RANGE   = 70
local FUSE            = 0.07
local DAMAGE          = 40
local RADIUS          = 100
local FORCE           = 1000
local SPAWN_MARGIN    = 200

local function spawnPoint(ctx)
    local w, h = ctx:screenWidth(), ctx:screenHeight()
    local m = SPAWN_MARGIN
    local side = math.random(4)

    if side == 1 then
        return math.random(-m, w + m), -m
    elseif side == 2 then
        return math.random(-m, w + m), h + m
    elseif side == 3 then
        return -m, math.random(-m, h + m)
    else
        return w + m, math.random(-m, h + m)
    end
end

local function findTarget(self, ctx, cx, cy)
    local owner = self.data.owner
    local best, bestDist

    for _, p in ipairs(ctx:players()) do
        if p.isAlive and p ~= owner then
            local px, py = p:getCenter()
            local dx, dy = px - cx, py - cy
            local dist = dx * dx + dy * dy
            if not bestDist or dist < bestDist then
                best, bestDist = p, dist
            end
        end
    end

    return best or owner
end

local function explode(self, ctx, cx, cy)
    if self.data.channel then ctx:stopSFX(self.data.channel) end

    ctx:spawnEffect("explosion_missile", cx, cy, 2.0)
    ctx:playSFX("explosion")
    ctx:shakeScreen(8, 0.3)

    ctx:explode{
        x = cx, y = cy, radius = RADIUS,
        damage = DAMAGE, force = FORCE,
        owner = self.data.owner,
    }

    self:kill()
end

local function onUpdate(self, ctx, dt)
    local x, y = self:getPosition()
    local cx, cy = x + SIZE / 2, y + SIZE / 2

    local target = findTarget(self, ctx, cx, cy)
    if not target then return end

    local tx, ty = target:getCenter()
    local dx, dy = tx - cx, ty - cy
    local dist = math.sqrt(dx * dx + dy * dy)

    local wanted = math.atan(dy, dx)
    local angle = self.data.angle or 0
    local diff = (wanted - angle + math.pi) % (2 * math.pi) - math.pi

    local maxTurn = PRECISION * dt
    if diff > maxTurn then
        angle = angle + maxTurn
    elseif diff < -maxTurn then
        angle = angle - maxTurn
    else
        angle = wanted
    end
    self.data.angle = angle

    local align = 1.0 - math.abs(diff) / math.pi
    local speed = VELOCITY * math.max(MIN_SPEED_RATIO, align)

    self:setVelocity(math.cos(angle) * speed, math.sin(angle) * speed)
    self:setAngle(math.deg(angle) + 90)

    if not self.data.fuse and dist < TRIGGER_RANGE then
        self.data.fuse = FUSE
    end

    if self.data.fuse then
        self.data.fuse = self.data.fuse - dt
        if self.data.fuse <= 0 then
            explode(self, ctx, cx, cy)
        end
    end
end

function missile.spawn(ctx, owner)
    local x, y = spawnPoint(ctx)

    local p = ctx:spawnProjectile{
        texture = TEXTURE,
        x = x, y = y,
        width = SIZE, height = SIZE,
        onUpdate = onUpdate,
    }

    if not p then return nil end

    p.data.owner = owner
    p.data.channel = ctx:playSFX("missileLaunch")
    return p
end

return missile
