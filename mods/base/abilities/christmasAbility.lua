local christmas = require("base.projectiles.christmas")

registerAbility{
    id = "christmas",
    cost = 1000,
    cooldown = 20.0,

    onUse = function(self, player, ctx)
        return christmas.spawn(ctx, player) ~= nil
    end
}
