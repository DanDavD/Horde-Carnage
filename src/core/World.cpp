#include "core/World.h"

#include <algorithm>
#include <numbers>
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

constexpr float kRadiansPerDegree = std::numbers::pi_v<float> / 180.0f;

// A hit flashes the body white. A hairball also stops chasing for a moment, so the hit reads as a stagger.
constexpr float kHitFlashSeconds = 0.1f;
constexpr float kHitStunSeconds = 0.15f;
constexpr float kSummonShakeSeconds = 0.3f;

// The camera looks at a point this far from the player toward the boss, so the boss stays in view. The
// offset is capped, so a boss across the arena never pushes the player toward the edge of the screen.
constexpr float kBossFraming = 0.4f;
constexpr float kMaxBossFramingOffset = 200.0f;

void Record(World& world, WorldEventKind kind, Vec2 position, Vec2 direction = {}, int amount = 0) {
    world.events.Spawn(WorldEvent{.kind = kind, .position = position, .direction = direction, .amount = amount});
}

// The way a projectile was flying, for the events it leaves.
Vec2 Heading(const Projectile& projectile) {
    return LengthSquared(projectile.motion.velocity) > 0.0f ? Normalized(projectile.motion.velocity) : Vec2{};
}

// Every source of damage to the player goes through here, so a dash protects from all of them, a hit that
// lands spoils any drink in progress, and the world remembers what landed it.
//
// @return Whether the hit landed, so a hit with extra effects (the charge's knockback) applies them once.
bool HurtPlayer(World& world, int damage, DamageSource source) {
    Player& player = world.player;
    if (!IsDashInvulnerable(player.dash) && ApplyDamage(player.health, damage)) {
        InterruptDrinking(player.flasks);
        world.lastHitBy = source;
        Record(world, IsDead(player.health) ? WorldEventKind::PlayerKilled : WorldEventKind::PlayerHurt,
               player.motion.position, {}, damage);
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

void DrinkFlasks(World& world, float dt) {
    Player& player = world.player;
    if (const int healed = StepFlasks(player.flasks, dt); healed > 0) {
        Heal(player.health, healed);
        Record(world, WorldEventKind::FlaskDrunk, player.motion.position, {}, healed);
    }
}

// A dash goes where the player is pushing. Standing still, it goes toward the aim point of this step, not
// the facing: the facing is only updated after moving, so it is a step old, and a flick of the mouse
// followed by a dash in the same frame would go the wrong way.
void StartDashIfAsked(World& world, const PlayerCommands& commands) {
    Player& player = world.player;
    if (!commands.dashPressed || IsDashing(player.dash) || IsDrinking(player.flasks)) {
        return;
    }
    // With the cursor right on the player there is no aim direction (the same one unit tolerance as
    // AimFacing), and the facing is the best guess.
    Vec2 direction = player.facing;
    if (LengthSquared(commands.move) > 0.0f) {
        direction = commands.move;
    } else if (commands.aimDirection && LengthSquared(*commands.aimDirection) > 0.0f) {
        direction = *commands.aimDirection;
    } else if (commands.aimInView) {
        const Vec2 aimAt = world.camera.center + *commands.aimInView;
        if (LengthSquared(aimAt - player.motion.position) > 1.0f) {
            direction = aimAt - player.motion.position;
        }
    } else if (commands.aimAt && LengthSquared(*commands.aimAt - player.motion.position) > 1.0f) {
        direction = *commands.aimAt - player.motion.position;
    }
    if (TryStartDash(player.dash, player.motion.position, direction)) {
        Record(world, WorldEventKind::DashStarted, player.motion.position, player.dash.direction);
    }
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
        Record(world, WorldEventKind::ShotFired, muzzle, player.facing, weapon.stats.damage);
    }
}

// A summoned hairball's strand: aimed where the player is, from the edge of its body.
void HairballSpit(World& world, const Enemy& hairball) {
    const SummonStats& stats = world.boss.summon;
    const Vec2 toPlayer = world.player.motion.position - hairball.motion.position;
    if (LengthSquared(toPlayer) <= 1.0f) {
        return;
    }
    const Vec2 direction = Normalized(toPlayer);
    Projectile strand = LaunchProjectile(hairball.motion.position + direction * hairball.radius, direction,
                                         stats.spitSpeed, stats.spitRange);
    strand.radius = stats.spitRadius;
    strand.damage = stats.spitDamage;
    strand.source = DamageSource::Hairball;
    world.hostileProjectiles.Spawn(strand);
    Record(world, WorldEventKind::StrandsSpat, hairball.motion.position, direction, 1);
}

void StepEnemies(World& world, float dt) {
    const Player& player = world.player;
    const float spitInterval = world.boss.summon.spitIntervalSeconds;
    for (Enemy& enemy : world.enemies) {
        enemy.hitFlashSeconds = std::max(enemy.hitFlashSeconds - dt, 0.0f);
        enemy.stunSeconds = std::max(enemy.stunSeconds - dt, 0.0f);
        if (enemy.spits) {
            enemy.secondsUntilSpit -= dt;
            if (enemy.secondsUntilSpit <= 0.0f) {
                HairballSpit(world, enemy);
                enemy.secondsUntilSpit += spitInterval;
            }
        }

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

// A bullet meets the shield first, and reaches the health only while the shield is down. The boss is too
// heavy for one bullet to stagger or push: the bullet that breaks the shield staggers it, the others only
// take health and flash it. While it transforms nothing hurts it: the bullet is spent on it like on a
// wall. The bullet that empties the first phase's bar starts the transformation instead of killing it, and
// the one that takes the second phase's bar to half throws the boss into a frenzy, with a roar.
void HitBoss(World& world, const Projectile& bullet) {
    Boss& boss = world.boss;
    if (IsTransforming(boss)) {
        return;
    }
    const Vec2 at = bullet.motion.position;
    const Vec2 along = Heading(bullet);
    switch (HitShield(boss.shield, bullet.damage)) {
    case ShieldHit::Exposed:
        ApplyDamage(boss.health, bullet.damage);
        boss.hitFlashSeconds = kHitFlashSeconds;
        Record(world, WorldEventKind::BossHurt, at, along, bullet.damage);
        if (IsDead(boss.health) && boss.phase == BossPhase::One) {
            StartTransformation(boss);
            Record(world, WorldEventKind::FirstPhaseEnded, boss.motion.position);
        } else if (IsBossDefeated(world)) {
            Record(world, WorldEventKind::BossDefeated, boss.motion.position);
        }
        if (ShouldFrenzy(boss)) {
            EnterFrenzy(boss);
            world.shockwaves.Spawn(MakeShockwave(boss.motion.position, boss.radius, boss.frenzy.roar));
            Record(world, WorldEventKind::BossRoared, boss.motion.position);
        }
        break;
    case ShieldHit::Broken:
        StaggerBoss(boss);
        Record(world, WorldEventKind::ShieldBroken, at, along);
        break;
    case ShieldHit::Bounced:
        Record(world, WorldEventKind::ShieldStruck, at, along, 0);
        break;
    case ShieldHit::Absorbed:
        Record(world, WorldEventKind::ShieldStruck, at, along, bullet.damage);
        break;
    }
}

// A ring around the gap the boss's attack points to, one strand per slot outside the gap. A full pool drops
// the strands that find no slot, like a bullet that is not fired.
void SpitHairRing(World& world) {
    const Boss& boss = world.boss;
    const Vec2 gap = boss.attack->direction;
    for (int i = 0; i < HairRingStrands(boss.hairRing); ++i) {
        world.hostileProjectiles.Spawn(MakeHairStrand(boss, HairStrandDirection(gap, i, boss.hairRing)));
    }
    Record(world, WorldEventKind::StrandsSpat, boss.motion.position, gap, HairRingStrands(boss.hairRing));
}

// A strand of the boss's hair thrown in a frenzy: as fast as a Volley's, and named for the frenzy on the
// death screen.
void ThrowFrenzyStrand(World& world, Vec2 direction) {
    Projectile strand = MakeVolleyStrand(world.boss, direction);
    strand.source = DamageSource::Frenzy;
    world.hostileProjectiles.Spawn(strand);
}

// While it charges in a frenzy: one strand at the player, one where the player is going. A player who only
// keeps running from the charge runs into the second.
void ThrowBolts(World& world) {
    const Boss& boss = world.boss;
    const Vec2 from = boss.motion.position;
    const Vec2 player = world.player.motion.position;
    if (LengthSquared(player - from) <= 1.0f) {
        return;
    }
    ThrowFrenzyStrand(world, Normalized(player - from));
    const Vec2 ahead = LeadPoint(boss, world.player.motion, world.arena);
    if (LengthSquared(ahead - from) > 1.0f) {
        ThrowFrenzyStrand(world, Normalized(ahead - from));
    }
    Record(world, WorldEventKind::StrandsSpat, from, Normalized(player - from), 2);
}

// The Summon's marks, around the player, turned by a random angle so no two summons look the same.
void PlaceSummon(World& world) {
    const SummonStats& stats = world.boss.summon;
    const float turn = RandomRange(world.random, 0.0f, 2.0f * std::numbers::pi_v<float>);
    const SummonPoints plan =
        PlanSummon(world.player.motion.position, world.boss.motion.position, world.arena, stats.placement, turn);
    for (std::size_t i = 0; i < plan.count; ++i) {
        world.spawnMarkers.Spawn(MakeSpawnMarker(plan.points[i], stats.markerSeconds));
    }
}

// A mark that runs out lets its hairball out, unless the arena already holds as many as the boss allows.
// The hairball comes out shaking itself for a moment before it chases, like one that was just hit.
void StepSpawnMarkers(World& world, float dt) {
    const int maxAlive = world.boss.summon.maxAlive;
    world.spawnMarkers.RemoveIf([&world, maxAlive, dt](SpawnMarker& marker) {
        if (StepSpawnMarker(marker, dt)) {
            return false;
        }
        if (static_cast<int>(world.enemies.Size()) < maxAlive) {
            const SummonStats& stats = world.boss.summon;
            Enemy hairball;
            hairball.motion = MotionAt(marker.position);
            hairball.stunSeconds = kSummonShakeSeconds;
            hairball.spits = true;
            hairball.secondsUntilSpit =
                RandomRange(world.random, stats.firstSpitMinSeconds, stats.firstSpitMaxSeconds);
            world.enemies.Spawn(hairball);
        }
        return true;
    });
}

// One burst of a Volley: a few strands fanned around the direction the boss is aiming in.
void SpitVolley(World& world) {
    const Boss& boss = world.boss;
    const VolleyStats& stats = boss.volley;
    const float middle = 0.5f * static_cast<float>(stats.strandsPerBurst - 1);
    for (int i = 0; i < stats.strandsPerBurst; ++i) {
        const float degrees = (static_cast<float>(i) - middle) * stats.spreadDegrees;
        const Vec2 direction = Rotated(boss.attack->direction, degrees * kRadiansPerDegree);
        world.hostileProjectiles.Spawn(MakeVolleyStrand(boss, direction));
    }
    Record(world, WorldEventKind::StrandsSpat, boss.motion.position, boss.attack->direction, stats.strandsPerBurst);
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
            if (IsDead(hit.enemy->health)) {
                Record(world, WorldEventKind::HairballKilled, hit.enemy->motion.position, Heading(bullet));
            }
            return true;
        }
        if (hit.boss) {
            HitBoss(world, bullet);
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
            HurtPlayer(world, boss.contactDamage, DamageSource::BossTouch);
        } else if (HurtPlayer(world, boss.charge.damage, DamageSource::Charge)) {
            player.motion.velocity += ChargeKnockback(boss, player.motion.position);
        }
    }
    for (const Enemy& enemy : world.enemies) {
        if (AreTouching(player.motion.position, player.radius, enemy.motion.position, enemy.radius)) {
            HurtPlayer(world, enemy.contactDamage, DamageSource::Hairball);
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
            HurtPlayer(world, strand.damage, strand.source);
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
    if (Length(away) <= leap.impactRadius + player.radius &&
        HurtPlayer(world, leap.impactDamage, DamageSource::LeapImpact)) {
        const Vec2 out = LengthSquared(away) > 1.0f ? Normalized(away) : Vec2{0.0f, 1.0f};
        player.motion.velocity += out * leap.impactKnockback;
    }
    world.shockwaves.Spawn(MakeShockwave(center, boss.radius, leap.shockwave));
    Record(world, WorldEventKind::BossLanded, center);
    // In a frenzy, a ring of strands with no gap leaves with the wave, one of them at the player: the wave
    // is dashed through, and the strands are slipped between.
    const Vec2 toPlayer = LengthSquared(away) > 1.0f ? Normalized(away) : Vec2{0.0f, 1.0f};
    for (int i = 0; i < leap.landingStrands; ++i) {
        const float turn = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) /
                           static_cast<float>(leap.landingStrands);
        Projectile strand = MakeHairStrand(boss, Rotated(toPlayer, turn));
        strand.source = DamageSource::Frenzy;
        world.hostileProjectiles.Spawn(strand);
    }
}

// A wave hurts the player once, the first time its band reaches them. Like the strands, it passes a
// dashing player by; any other touch spends it, even inside the window after an earlier hit.
void StepShockwaves(World& world, float dt) {
    Player& player = world.player;
    world.shockwaves.RemoveIf([&world, &player, dt](Shockwave& wave) {
        const bool going = StepShockwave(wave, dt);
        if (!wave.spent && !IsDashInvulnerable(player.dash) &&
            ShockwaveTouches(wave, player.motion.position, player.radius)) {
            HurtPlayer(world, wave.stats.damage, DamageSource::Shockwave);
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

// Aim after moving and after collisions, so the gun points from where the player actually ended the step,
// and a screen-space mouse offset resolves against the camera center of this same step instead of the last.
void AimPlayer(World& world, const PlayerCommands& commands, float dt) {
    Player& player = world.player;
    if (commands.aimDirection) {
        UpdateFacing(player.facing, *commands.aimDirection);
    } else if (commands.aimInView) {
        const Vec2 nextCamera =
            NextFollowCameraCenter(world.camera, CameraFocus(world), player.motion.velocity, world.arena, dt);
        AimFacing(player.facing, player.motion.position, nextCamera + *commands.aimInView);
    } else if (commands.aimAt) {
        AimFacing(player.facing, player.motion.position, *commands.aimAt);
    } else {
        UpdateFacing(player.facing, commands.move);
    }
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
    StartDashIfAsked(world, commands);
    StartDrinkIfAsked(player, commands);
    DrinkFlasks(world, dt);
    MovePlayer(player, commands.move, dt);
    StepEnemies(world, dt);
    const BossActions actions =
        StepBoss(world.boss, player.motion, player.radius, world.arena, world.random, dt);
    if (actions.attackStarted) {
        const BossAttack& attack = *world.boss.attack;
        Record(world, WorldEventKind::AttackStarted, world.boss.motion.position, attack.direction,
               static_cast<int>(attack.kind));
    }
    if (actions.spitHairRing) {
        SpitHairRing(world);
    }
    if (actions.landed) {
        LandLeap(world);
    }
    if (actions.spitVolley) {
        SpitVolley(world);
    }
    if (actions.throwBolts) {
        ThrowBolts(world);
    }
    if (actions.summon) {
        PlaceSummon(world);
    }
    if (actions.hitWall) {
        Record(world, WorldEventKind::BossHitWall, world.boss.motion.position, world.boss.attack->direction);
    }
    // Before the collisions, so a hairball that comes out is pushed apart and held inside the walls at once.
    StepSpawnMarkers(world, dt);
    if (actions.roared) {
        const Boss& boss = world.boss;
        world.shockwaves.Spawn(MakeShockwave(boss.motion.position, boss.radius, boss.phaseTwo.roar));
        Record(world, WorldEventKind::BossRoared, boss.motion.position);
    }

    // Collisions run after everyone has moved, so the order in which bodies moved does not decide who
    // gets pushed.
    for (int pass = 0; pass < kCollisionPasses; ++pass) {
        SeparateEveryone(world);
        ConfineEveryone(world);
    }

    // Aim and fire once every body is in its final spot for this step, so a bullet leaves from the real
    // muzzle straight through the crosshair that this step's camera will show.
    AimPlayer(world, commands, dt);
    HandleWeapons(world, commands, dt);

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
    return world.boss.phase == BossPhase::Two && IsDead(world.boss.health);
}

float Accuracy(const World& world) {
    if (world.shotsFired == 0) {
        return 0.0f;
    }
    return static_cast<float>(world.shotsHit) / static_cast<float>(world.shotsFired);
}

void DefeatBoss(World& world) {
    world.boss.attack.reset();
    world.boss.phase = BossPhase::Two;
    world.boss.health.current = 0;
    Record(world, WorldEventKind::BossDefeated, world.boss.motion.position);
}

void TransformBoss(World& world) {
    if (world.boss.phase != BossPhase::One) {
        return;
    }
    StartTransformation(world.boss);
    Record(world, WorldEventKind::FirstPhaseEnded, world.boss.motion.position);
}

void FrenzyBoss(World& world) {
    Boss& boss = world.boss;
    if (boss.phase != BossPhase::Two || boss.frenzied) {
        return;
    }
    const float half = boss.frenzy.healthFraction * static_cast<float>(boss.health.max);
    boss.health.current = std::min(boss.health.current, static_cast<int>(half));
    EnterFrenzy(boss);
    world.shockwaves.Spawn(MakeShockwave(boss.motion.position, boss.radius, boss.frenzy.roar));
    Record(world, WorldEventKind::BossRoared, boss.motion.position);
}

void BreakBossShield(World& world) {
    BreakShield(world.boss.shield);
    StaggerBoss(world.boss);
    Record(world, WorldEventKind::ShieldBroken, world.boss.motion.position);
}

} // namespace horde
