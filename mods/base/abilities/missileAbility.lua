local missile = require("base.projectiles.missile")

registerAbility{
    id = "missile",
    cost = 200,
    cooldown = 5.0,

    onUse = function(self, player, ctx)
        return missile.spawn(ctx, player) ~= nil
    end
}
