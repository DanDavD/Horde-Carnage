#pragma once

#include "core/SoundCue.h"
#include "core/Synth.h"

namespace horde {

/// How each sound of the fight is made, as a recipe for `Synthesize`. Every sound in the game is
/// synthesized from these at start-up, like the art is drawn in code: nothing is recorded or downloaded.
[[nodiscard]] const SoundRecipe& RecipeFor(SoundId id);

} // namespace horde
