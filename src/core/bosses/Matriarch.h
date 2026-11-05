#pragma once

#include "core/Arena.h"
#include "core/Boss.h"
#include "core/Movement.h"
#include "core/Random.h"
#include "core/Vec2.h"

// The Matriarch's rules: how she stalks and picks her attacks, her alternating shield and the stagger when
// it breaks, the transformation into her second phase, and her frenzy. Her row in the table of bosses
// (Boss.cpp) points at MakeMatriarch and StepMatriarch.

namespace horde {

/// A fresh Matriarch standing still at `position`, with her first phase's numbers.
[[nodiscard]] Boss MakeMatriarch(Vec2 position);

/// The Matriarch's step, once `StepBoss` has found her alive: still while she transforms, then her shield,
/// her stagger, her stalk, the attack in progress and the start of a combo's second attack.
BossActions StepMatriarch(Boss& boss, const Motion& player, float playerRadius, const Arena& arena, Random& random,
                          float dt);

/// Whether the boss is between its two phases: invulnerable, still and roaring.
[[nodiscard]] bool IsTransforming(const Boss& boss);

/// Starts the transformation into the second phase: the attack is dropped, the health is refilled at once
/// (the bar draws it filling up), and nothing hurts the boss until it ends. Only from the first phase.
void StartTransformation(Boss& boss);

/// Whether the boss has just dropped low enough in its second phase to fall into a frenzy.
[[nodiscard]] bool ShouldFrenzy(const Boss& boss);

/// Puts the boss in a frenzy: `FrenzyStats` replace the numbers of the second phase.
void EnterFrenzy(Boss& boss);

/// How far into the Summon's far range the player stands, from 0 inside `midDistance` (400) to 1 at or
/// past `farDistance` (550), eased with smoothstep. Zero in the first phase, while transforming, or once
/// the boss is dead.
[[nodiscard]] float SummonThreat(const Boss& boss, float distance);

/// Whether the boss is reeling from its broken shield.
[[nodiscard]] bool IsStaggered(const Boss& boss);

/// Cuts the boss's attack off and holds it still for `breakStaggerSeconds`, with the shortest stalk after
/// it. Called when its shield breaks, so the break is felt, not only seen on a bar.
void StaggerBoss(Boss& boss);

} // namespace horde
