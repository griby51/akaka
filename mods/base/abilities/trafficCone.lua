local cone = require("base.projectiles.cone")

registerAbility{
    id = "cone",
    cost = 200,
    cooldown = 2.0,
    onUse = function(self, player, ctx)
        return cone.spawn(ctx, player) ~= nil
    end
}
