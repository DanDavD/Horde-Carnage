#include "core/World.h"

#include <algorithm>
#include <optional>

#include "core/Chase.h"
#include "core/Collision.h"
#include "core/Dash.h"
#include "core/Flasks.h"
#include "core/Facing.h"
#include "core/Health.h"
#include "core/Loadout.h"
#include "core/Movement.h"

namespace horde {

namespace {

// Pushing one pair apart can push one of them into a third body. A few passes over all pairs settle a
// crowd. Each pass ends with the walls, so a body pushed into a wall is put back before the next pass.
constexpr int kCollisionPasses = 4;

// A hit flashes the body white. A hairball also stops chasing for a moment, so the hit reads as a stagger.
constexpr float kHitFlashSeconds = 0.1f;
constexpr float kHitStunSeconds = 0.15f;

// The camera looks at a point this far from the player toward the boss, so the boss stays in view. The
// offset is capped, so a boss across the arena never pushes the player toward the edge of the screen.
constexpr float kBossFraming = 0.4f;
constexpr float kMaxBossFramingOffset = 200.0f;

// Every source of damage to the player goes through here, so a dash protects from all of them, and a hit
// that lands spoils any drink in progress.
//
// @return Whether the hit landed, so a hit with extra effects (the charge's knockback) applies them once.
bool HurtPlayer(Player& player, int damage) {
    if (!IsDashInvulnerable(player.dash) && ApplyDamage(player.health, damage)) {
        InterruptDrinking(player.flasks);
        return true;
    }
    return false;
}

// The dash and the drink exclude each other: drinking is a commitment, and a dash cannot be cancelled into
// a heal. Drinking at full health would only waste a flask, so it is refused.
void StartDrinkIfAsked(Player& player, const PlayerCommands& commands) {
    const bool hurt = player.health.current < player.health.max;
    if (commands.healPressed && hurt && !IsDashing(player.dash)) {
        TryStartDrinking(player.flasks);
    }
}

void DrinkFlasks(Player& player, float dt) {
    if (const int healed = StepFlasks(player.flasks, dt); healed > 0) {
        Heal(player.health, healed);
    }
}

// A dash goes where the player is pushing. Standing still, it goes toward the aim point of this step, not
// the facing: the facing is only updated after moving, so it is a step old, and a flick of the mouse
// followed by a dash in the same frame would go the wrong way.
void StartDashIfAsked(Player& player, const PlayerCommands& commands) {
    if (!commands.dashPressed || IsDashing(player.dash) || IsDrinking(player.flasks)) {
        return;
    }
    // With the cursor right on the player there is no aim direction (the same one unit tolerance as
    // AimFacing), and the facing is the best guess.
    Vec2 direction = player.facing;
    if (LengthSquared(commands.move) > 0.0f) {
        direction = commands.move;
    } else if (commands.aimAt && LengthSquared(*commands.aimAt - player.motion.position) > 1.0f) {
        direction = *commands.aimAt - player.motion.position;
    }
    TryStartDash(player.dash, player.motion.position, direction);
}

void MovePlayer(Player& player, Vec2 move, float dt) {
    Dash& dash = player.dash;
    const bool dashing = IsDashing(dash);
    const Vec2 dashStep = StepDash(dash, dt);
    if (!dashing) {
        MovementStats stats = player.stats;
        if (IsDrinking(player.flasks)) {
            stats.maxSpeed *= player.flasks.stats.moveSpeedFactor;
        }
        StepMovement(player.motion, move, stats, dt);
        return;
    }
    // The burst replaces normal movement. The velocity reports running speed rather than the burst's
    // 1500 units/s: the player leaves the dash at running pace instead of sliding on, and the camera's
    // look-ahead does not lurch forward for a tenth of a second.
    player.motion.previousPosition = player.motion.position;
    player.motion.position += dashStep;
    player.motion.velocity = dash.direction * player.stats.maxSpeed;
}

// Switching happens before the trigger, so a switch and a click in the same step fire the new gun.
void HandleWeapons(World& world, const PlayerCommands& commands, float dt) {
    Player& player = world.player;
    Loadout& loadout = player.loadout;
    if (commands.selectSlot) {
        SelectSlot(loadout, *commands.selectSlot);
    }
    if (commands.cycleWeapon) {
        CycleWeapon(loadout);
    }
    TickLoadout(loadout, dt);

    Weapon& weapon = Equipped(loadout);
    const bool handsFree = !IsDashing(player.dash) && !IsDrinking(player.flasks);
    if (PullTrigger(weapon, commands.fireHeld, commands.firePressed, handsFree)) {
        const Vec2 muzzle = MuzzlePoint(player.motion.position, player.facing, player.radius, weapon.stats);
        world.bullets.Spawn(MakeBullet(muzzle, player.facing, weapon.stats));
        ++world.shotsFired;
    }
}

void StepEnemies(World& world, float dt) {
    const Player& player = world.player;
    for (Enemy& enemy : world.enemies) {
        enemy.hitFlashSeconds = std::max(enemy.hitFlashSeconds - dt, 0.0f);
        enemy.stunSeconds = std::max(enemy.stunSeconds - dt, 0.0f);

        // Chase where the player is after this step's move, so a hairball never aims at a stale position.
        // While stunned it has no input: it slides out of the knockback and holds still, instead of turning
        // straight back toward the player.
        const float stopDistance = enemy.radius + player.radius + kContactSlack;
        const Vec2 input = enemy.stunSeconds > 0.0f
                               ? Vec2{}
                               : ChaseInput(enemy.motion.position, player.motion.position, stopDistance);
        StepMovement(enemy.motion, input, enemy.stats, dt);
    }
}

// A dead boss lies on the floor and an airborne one is out of reach: neither collides, takes bullets or
// hurts on touch.
bool IsBossABody(const Boss& boss) {
    return !IsDead(boss.health) && !IsAirborne(boss);
}

void ConfineEveryone(World& world) {
    ConfineToArena(world.player.motion, world.player.radius, world.arena);
    ConfineToArena(world.boss.motion, world.boss.radius, world.arena);
    for (Enemy& enemy : world.enemies) {
        ConfineToArena(enemy.motion, enemy.radius, world.arena);
    }
}

// Every pair once: the player and the boss against each other and against each hairball, then each
// hairball against the ones after it. With a few dozen bodies that is a few hundred checks per pass, so a
// spatial grid would cost more than it saves. A dashing player passes through bodies, so a dash can escape
// a crowd or go through the boss, and a dead or airborne boss is not a body.
//
// A charging boss runs through the player rather than pushing them: at its speed a shove would carry the
// player across the arena and pin them against the wall. It still bowls hairballs out of its way.
void SeparateEveryone(World& world) {
    Player& player = world.player;
    Boss& boss = world.boss;
    const bool playerIsSolid = !IsDashing(player.dash);
    const bool bossIsSolid = IsBossABody(boss);
    if (playerIsSolid && bossIsSolid && !IsCharging(boss)) {
        SeparateCircles(player.motion, player.radius, boss.motion, boss.radius, PushShare(player.weight, boss.weight));
    }

    auto& enemies = world.enemies;
    for (std::size_t i = 0; i < enemies.Size(); ++i) {
        Enemy& enemy = enemies[i];
        if (playerIsSolid) {
            SeparateCircles(player.motion, player.radius, enemy.motion, enemy.radius,
                            PushShare(player.weight, enemy.weight));
        }
        if (bossIsSolid) {
            SeparateCircles(enemy.motion, enemy.radius, boss.motion, boss.radius, PushShare(enemy.weight, boss.weight));
        }
        for (std::size_t j = i + 1; j < enemies.Size(); ++j) {
            Enemy& other = enemies[j];
            SeparateCircles(enemy.motion, enemy.radius, other.motion, other.radius,
                            PushShare(enemy.weight, other.weight));
        }
    }
}

// What a bullet touches first along this step's flight: a hairball, the boss, or nothing.
struct BulletHit {
    Enemy* enemy = nullptr;
    bool boss = false;
};

// Comparing how far along the path each hit happens is what makes the front body of a line take the
// bullet, whether that is a hairball or the boss.
BulletHit FirstHit(World& world, const Projectile& bullet) {
    const auto sweep = [&bullet](Vec2 center, float radius) {
        return SweepCircle(bullet.motion.previousPosition, bullet.motion.position, bullet.radius, center, radius);
    };
    BulletHit hit;
    float firstFraction = 2.0f;
    for (Enemy& enemy : world.enemies) {
        // A hairball killed by an earlier bullet this step is removed only after all bullets have flown.
        // Until then it must not soak up more bullets.
        if (IsDead(enemy.health)) {
            continue;
        }
        const std::optional<float> fraction = sweep(enemy.motion.position, enemy.radius);
        if (fraction && *fraction < firstFraction) {
            hit = BulletHit{.enemy = &enemy};
            firstFraction = *fraction;
        }
    }
    const Boss& boss = world.boss;
    if (IsBossABody(boss)) {
        const std::optional<float> fraction = sweep(boss.motion.position, boss.radius);
        if (fraction && *fraction < firstFraction) {
            hit = BulletHit{.boss = true};
        }
    }
    return hit;
}

void HitEnemy(Enemy& enemy, const Projectile& bullet) {
    ApplyDamage(enemy.health, bullet.damage);
    enemy.hitFlashSeconds = kHitFlashSeconds;
    enemy.stunSeconds = kHitStunSeconds;
    enemy.motion.velocity += Normalized(bullet.motion.velocity) * bullet.knockback;
}

// The boss is too heavy to stagger or push: a bullet only takes health and flashes it. Openings to punish
// it come from its own attacks, not from the gun.
void HitBoss(Boss& boss, const Projectile& bullet) {
    ApplyDamage(boss.health, bullet.damage);
    boss.hitFlashSeconds = kHitFlashSeconds;
}

// A ring around the gap the boss's attack points to, one strand per slot outside the gap. A full pool drops
// the strands that find no slot, like a bullet that is not fired.
void SpitHairRing(World& world) {
    const Boss& boss = world.boss;
    const Vec2 gap = boss.attack->direction;
    for (int i = 0; i < HairRingStrands(boss.hairRing); ++i) {
        world.hostileProjectiles.Spawn(MakeHairStrand(boss, HairStrandDirection(gap, i, boss.hairRing)));
    }
}

// One pass moves every bullet, resolves its hit and removes the ones that are done, so a bullet is never
// checked in a step it no longer flies in.
void StepBullets(World& world, float dt) {
    world.bullets.RemoveIf([&world, dt](Projectile& bullet) {
        const bool stillFlying = StepProjectile(bullet, dt);
        const BulletHit hit = FirstHit(world, bullet);
        if (hit.enemy) {
            HitEnemy(*hit.enemy, bullet);
            ++world.shotsHit;
            return true;
        }
        if (hit.boss) {
            HitBoss(world.boss, bullet);
            ++world.shotsHit;
            return true;
        }
        return !stillFlying || !Contains(world.arena, bullet.motion.position);
    });
}

void RemoveDeadEnemies(World& world) {
    world.enemies.RemoveIf([](const Enemy& enemy) { return IsDead(enemy.health); });
}

// Sideways out of the charge's path, on the side the player already is, so the hit throws them clear of
// the rest of the run instead of along it.
Vec2 ChargeKnockback(const Boss& boss, Vec2 playerPosition) {
    const Vec2 direction = boss.attack->direction;
    Vec2 side{-direction.y, direction.x};
    if (Dot(playerPosition - boss.motion.position, side) < 0.0f) {
        side = side * -1.0f;
    }
    return side * boss.charge.knockback;
}

// After collisions, so contact means the bodies really ended the step touching. The invulnerability
// window lets only the first touch land. The boss is checked first, so when the boss and a hairball reach
// the player in the same step, the heavier hit is the one that lands.
void ApplyContactDamage(World& world) {
    Player& player = world.player;
    const Boss& boss = world.boss;
    if (IsBossABody(boss) && AreTouching(player.motion.position, player.radius, boss.motion.position, boss.radius)) {
        if (!IsCharging(boss)) {
            HurtPlayer(player, boss.contactDamage);
        } else if (HurtPlayer(player, boss.charge.damage)) {
            player.motion.velocity += ChargeKnockback(boss, player.motion.position);
        }
    }
    for (const Enemy& enemy : world.enemies) {
        if (AreTouching(player.motion.position, player.radius, enemy.motion.position, enemy.radius)) {
            HurtPlayer(player, enemy.contactDamage);
        }
    }
}

// The boss's strands fly like bullets but hit only the player, with the same swept test. A dashing player
// is not there for them: they fly on, the way an attack passes through a roll in Dark Souls. Otherwise a
// strand that touches the player is spent, like a bullet that hits a body, even when the window after an
// earlier hit keeps it from hurting.
void StepHostileProjectiles(World& world, float dt) {
    Player& player = world.player;
    world.hostileProjectiles.RemoveIf([&world, &player, dt](Projectile& strand) {
        const bool stillFlying = StepProjectile(strand, dt);
        const bool touches = SweepCircle(strand.motion.previousPosition, strand.motion.position, strand.radius,
                                         player.motion.position, player.radius)
                                 .has_value();
        if (touches && !IsDashInvulnerable(player.dash)) {
            HurtPlayer(player, strand.damage);
            return true;
        }
        return !stillFlying || !Contains(world.arena, strand.motion.position);
    });
}

// The impact lands on a player under the shadow and throws them out from under the body. The shockwave
// leaves from the edge of the body. A player caught by the impact is still inside the window when the
// wave's band passes, so one landing costs one hit, not two.
void LandLeap(World& world) {
    Player& player = world.player;
    const Boss& boss = world.boss;
    const LeapStats& leap = boss.leap;
    const Vec2 center = boss.motion.position;
    const Vec2 away = player.motion.position - center;
    if (Length(away) <= leap.impactRadius + player.radius && HurtPlayer(player, leap.impactDamage)) {
        const Vec2 out = LengthSquared(away) > 1.0f ? Normalized(away) : Vec2{0.0f, 1.0f};
        player.motion.velocity += out * leap.impactKnockback;
    }
    world.shockwaves.Spawn(MakeShockwave(center, boss.radius, leap.shockwave));
}

// A wave hurts the player once, the first time its band reaches them. Like the strands, it passes a
// dashing player by; any other touch spends it, even inside the window after an earlier hit.
void StepShockwaves(World& world, float dt) {
    Player& player = world.player;
    world.shockwaves.RemoveIf([&player, dt](Shockwave& wave) {
        const bool going = StepShockwave(wave, dt);
        if (!wave.spent && !IsDashInvulnerable(player.dash) &&
            ShockwaveTouches(wave, player.motion.position, player.radius)) {
            HurtPlayer(player, wave.stats.damage);
            wave.spent = true;
        }
        return !going;
    });
}

// What the camera follows: the player, pulled toward a living boss.
Vec2 CameraFocus(const World& world) {
    const Vec2 player = world.player.motion.position;
    if (IsDead(world.boss.health)) {
        return player;
    }
    const Vec2 towardBoss = (world.boss.motion.position - player) * kBossFraming;
    return player + ClampLength(towardBoss, kMaxBossFramingOffset);
}

} // namespace

World CreateWorld(const Arena& arena, Vec2 viewSize, std::uint64_t seed) {
    const Vec2 bossStart = Center(arena);
    World world{.arena = arena,
                .player = {.motion = MotionAt(bossStart + Vec2{0.0f, kStartingDistance})},
                .boss = {.motion = MotionAt(bossStart)},
                .seed = seed,
                .random = SeedRandom(seed)};
    // The player starts looking at the boss, so the first shot goes where the threat is.
    world.player.facing = Vec2{0.0f, -1.0f};
    world.camera = FollowCameraAt(CameraFocus(world), viewSize, arena);
    return world;
}

void StepWorld(World& world, const PlayerCommands& commands, float dt) {
    world.elapsedSeconds += dt;
    Player& player = world.player;
    TickHealth(player.health, dt);
    TickDashCooldown(player.dash, dt);
    StartDashIfAsked(player, commands);
    StartDrinkIfAsked(player, commands);
    DrinkFlasks(player, dt);
    MovePlayer(player, commands.move, dt);

    // Aim after moving, so the gun points from where the player is now, not from a step ago.
    if (commands.aimAt) {
        AimFacing(player.facing, player.motion.position, *commands.aimAt);
    } else {
        UpdateFacing(player.facing, commands.move);
    }
    HandleWeapons(world, commands, dt);
    StepEnemies(world, dt);
    const BossActions actions = StepBoss(world.boss, player.motion.position, player.radius, world.arena, dt);
    if (actions.spitHairRing) {
        SpitHairRing(world);
    }
    if (actions.landed) {
        LandLeap(world);
    }

    // Collisions run after everyone has moved, so the order in which bodies moved does not decide who
    // gets pushed.
    for (int pass = 0; pass < kCollisionPasses; ++pass) {
        SeparateEveryone(world);
        ConfineEveryone(world);
    }

    // Bullets test against where the bodies ended this step. A hairball killed now is removed before it
    // can touch the player.
    StepBullets(world, dt);
    RemoveDeadEnemies(world);
    ApplyContactDamage(world);
    // After the bodies, so when the boss's body and one of its strands reach the player in the same step,
    // the heavier hit is the one that lands.
    StepHostileProjectiles(world, dt);
    StepShockwaves(world, dt);

    // Last, so the camera follows where everything ended up in this step.
    StepFollowCamera(world.camera, CameraFocus(world), player.motion.velocity, world.arena, dt);
}

bool IsBossDefeated(const World& world) {
    return IsDead(world.boss.health);
}

float Accuracy(const World& world) {
    if (world.shotsFired == 0) {
        return 0.0f;
    }
    return static_cast<float>(world.shotsHit) / static_cast<float>(world.shotsFired);
}

void DefeatBoss(World& world) {
    world.boss.health.current = 0;
}

} // namespace horde
