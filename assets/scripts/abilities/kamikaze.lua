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

        ctx:explode{
            x = cx, y = cy, radius = radius,
            damage = maxDmg, force = maxForce,
            owner = player, ignore = { player },
        }

        return true
    end
}
