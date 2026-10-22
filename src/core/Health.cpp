#include "core/Health.h"

#include <algorithm>
#include <cassert>

namespace horde {

bool ApplyDamage(Health& health, int amount) {
    assert(amount > 0 && "damage must be positive; healing will have its own function");
    if (IsDead(health) || IsInvulnerable(health)) {
        return false;
    }
    health.current = std::max(health.current - amount, 0);
    health.invulnerableSeconds = health.invulnerabilityAfterHit;
    return true;
}

void TickHealth(Health& health, float dt) {
    health.invulnerableSeconds = std::max(health.invulnerableSeconds - dt, 0.0f);
}

} // namespace horde
