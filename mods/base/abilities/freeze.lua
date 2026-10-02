registerAbility{
    id = "freeze",
    cost = 500,
    cooldown = 1.0,

    onUse = function(self, player, ctx)
        local cx, cy = player:getCenter()
        local cibles = ctx:playersInRadius(cx, cy, 300, {player})
        for _, cible in ipairs(cibles) do
            cible:setVelocity(0, 0)
        end

        return #cibles > 0
    end
}
