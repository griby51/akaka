local cone = {}

local TEXTURE = "trafficCone"
local SIZE = 16
local TRIGGER_RANGE = 20
local RISE_SPEED = 1500
local RADIUS = 200
local FORCE = 2000
local DAMAGE = 50

local function explode(self, ctx, cx, cy)
    ctx:spawnEffect("explosion_missile", cx, cy, 4.0)
    if self.data.channel then ctx:stopSFX(self.data.channel) end
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
    local cx, cy = self:getPosition()
    local owner = self.data.owner

    if not self.data.triggered then
        self:setVelocity(-ctx:worldSpeed(), 0)

        if self:isOffScreen(50) then
            self:kill()
            return
        end

        for _, p in ipairs(ctx:players()) do
            if p.isAlive and p ~= owner then
                local px = p:getPosition()
                if math.abs(px - cx) < TRIGGER_RANGE then
                    self.data.triggered = true
                    self:setVelocity(0, -RISE_SPEED)
                    self.data.channel = ctx:playSFX("missileLaunch")
                    break
                end
            end
        end
    else
        local best, bestDist
        for _, p in ipairs(ctx:players()) do
            if p.isAlive then
                local px, py = p:getPosition()
                local dx, dy = px - cx, py - cy
                local dist = dx*dx + dy*dy
                if not bestDist or dist < bestDist then
                    best, bestDist = p, dist
                end
            end
        end

        if best then
            local _, py = best:getPosition()

            if py > cy then
                explode(self, ctx, cx, cy)
            end
        elseif self:isOffScreen(50) then
            self:kill()
        end
    end
end

function cone.spawn(ctx, owner)
    local p = ctx:spawnProjectile{
        texture = TEXTURE,
        x = ctx:screenWidth() + 10,
        y = ctx:effectiveHeight() - SIZE,
        width = SIZE, height = SIZE,
        vx = -ctx:worldSpeed(),
        onUpdate = onUpdate,
    }

    if not p then return nil end

    p.data.owner = owner
    return p
end

return cone
                
            

