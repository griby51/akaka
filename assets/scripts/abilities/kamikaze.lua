registerAbility{
    id = "kamikaze",
    cost = 100,
    cooldown = 2.0,

    onUse = function(self, player, ctx)
        local scale = 3.0
        local cx, cy = player:getCenter()
        ctx:spawnEffect("explosion_missile", cx, cy, scale)
        ctx:playSFX("explosion")
        ctx:shakeScreen(16, 0.3)

        local radius = 150
        local maxDmg = 50
        local maxForce = 1500

        local cibles = ctx:playersInRadius(cx, cy, radius, player)
        for _, cible in ipairs(cibles) do
            local cibleCx, cibleCy = cible:getCenter()
            local dx, dy = cibleCx - cx, cibleCy - cy

            local factor = 1.0 - (dx*dx + dy*dy)/(radius*radius)

            local forceX = (dx > 0 and maxForce or -maxForce) * factor
            local forceY = (dy > 0 and maxForce or -maxForce) * factor

            cible:applyKnockBack(forceX, forceY)
            cible:damage(math.floor(maxDmg*factor))
        end

        return true
    end
}
