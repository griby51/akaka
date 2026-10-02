local christmas = {}

local SLEIGH_TEXTURE = "christmasSleigh"
local GIFT_TEXTURE   = "gift"
local SLEIGH_SPEED   = 200
local GIFT_SPEED      = 200
local RIDER_OFFSET_X = 20
local START_X        = -60
local START_Y        = 60
local FIRST_GIFT_MAX = 2.0
local GIFT_MIN       = 0.3
local GIFT_MAX       = 0.7
local DAMAGE         = 50
local RADIUS         = 100
local FORCE          = 1000

local function giftUpdate(self, ctx, dt)
    local x, y = self:getPosition()

    if y < self.data.explodeY then return end

    local w, h = self:getSize()
    local cx, cy = x + w / 2, y + h / 2

    ctx:spawnEffect("explosion_missile", cx, cy, 2.0)
    ctx:playSFX("explosion")
    ctx:shakeScreen(5, 0.3)

    ctx:explode{
        x = cx, y = cy, radius = RADIUS,
        damage = DAMAGE, force = FORCE,
        owner = self.data.owner,
    }

    self:kill()
end

local function dropGift(self, ctx, x, y)
    local maxY = ctx:effectiveHeight()
    local p = ctx:spawnProjectile{
        texture = GIFT_TEXTURE,
        x = x, y = y,
        vy = GIFT_SPEED,
        onUpdate = giftUpdate,
    }

    if not p then return end

    p.data.owner = self.data.owner
    p.data.explodeY = math.random(100, maxY - 50)
end

local function sleighUpdate(self, ctx, dt)
    local x, y = self:getPosition()
    local rider = self.data.owner

    if x > ctx:screenWidth() + 50 then
        if rider and rider.isControlled then
            rider.isControlled = false
            rider:teleport(ctx:screenWidth() / 2, ctx:screenHeight() / 2)
        end
        self:kill()
        return
    end

    if rider and rider.isAlive then
        rider.isControlled = true
        rider:teleport(x + RIDER_OFFSET_X, y)
    end

    self.data.nextGift = self.data.nextGift - dt
    if self.data.nextGift <= 0 then
        self.data.nextGift = math.random() * (GIFT_MAX - GIFT_MIN) + GIFT_MIN
        dropGift(self, ctx, x, y)
    end
end

function christmas.spawn(ctx, owner)
    local p = ctx:spawnProjectile{
        texture = SLEIGH_TEXTURE,
        x = START_X, y = START_Y,
        vx = SLEIGH_SPEED,
        onUpdate = sleighUpdate,
    }

    if not p then return nil end

    p.data.owner = owner
    p.data.nextGift = math.random() * FIRST_GIFT_MAX
    return p
end

return christmas
