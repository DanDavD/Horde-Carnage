#include <doctest/doctest.h>

#include "core/Health.h"

using horde::Health;

namespace {

// A window of 0.5 s and steps of 0.125 s, so the window lasts exactly four steps.
constexpr float kDt = 0.125f;

Health Fresh() {
    return Health{.current = 100, .max = 100, .invulnerabilityAfterHit = 0.5f};
}

} // namespace

TEST_CASE("a hit takes health and starts the invulnerability window") {
    Health health = Fresh();
    CHECK(horde::ApplyDamage(health, 10));
    CHECK(health.current == 90);
    CHECK(horde::IsInvulnerable(health));
    CHECK(health.invulnerableSeconds == doctest::Approx(0.5f));
}

TEST_CASE("a second hit inside the window does nothing") {
    Health health = Fresh();
    horde::ApplyDamage(health, 10);
    horde::TickHealth(health, kDt);
    CHECK_FALSE(horde::ApplyDamage(health, 10));
    CHECK(health.current == 90);
}

TEST_CASE("once the window has passed, the next hit lands") {
    Health health = Fresh();
    horde::ApplyDamage(health, 10);
    for (int i = 0; i < 4; ++i) {
        horde::TickHealth(health, kDt);
    }
    CHECK_FALSE(horde::IsInvulnerable(health));
    CHECK(horde::ApplyDamage(health, 10));
    CHECK(health.current == 80);
}

TEST_CASE("the window never counts below zero") {
    Health health = Fresh();
    horde::TickHealth(health, 3.0f);
    CHECK(health.invulnerableSeconds == 0.0f);
}

TEST_CASE("health stops at zero and the body is dead") {
    Health health = Fresh();
    health.current = 5;
    CHECK(horde::ApplyDamage(health, 10));
    CHECK(health.current == 0);
    CHECK(horde::IsDead(health));
}

TEST_CASE("a dead body takes no more hits") {
    Health health = Fresh();
    health.current = 0;
    CHECK_FALSE(horde::ApplyDamage(health, 10));
    CHECK_FALSE(horde::IsInvulnerable(health));
}
