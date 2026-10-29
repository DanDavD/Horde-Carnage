# Music

## What it is

The boss fight has its own theme, and it follows the fight: one piece of music in five layers that come and go with what is happening. The whole fight has drums, guitars and a bass, with a guitar tune over them. The second phase adds a choir and timpani. When the boss is close to death in that phase, or in her frenzy, a double kick comes in and a lead takes the tune over. When the player is close to death, a racing heartbeat and trembling strings come in. The beat never changes, only how many layers play over it, so a low bar or a dying player sounds like the same song getting worse. The fight ends on a short piece of its own: a fanfare for a victory, and a blow, a gong and a heart that stops for a death.

Like the sound effects (see [Audio](AUDIO.md)), every note is synthesized in code when the game starts. There are no music files.

The code:

- `src/core/Sequencer.h`, `src/core/Sequencer.cpp`: `NoteHz` (the pitch of a MIDI note) and `MixInto`, which places a synthesized note into a loop and wraps its tail round to the start.
- `src/core/MusicTrack.h`: `MusicStem`, the five layers; `MusicStinger`, the two endings; `MusicTrack`, a loop of each layer and the two stingers.
- `src/core/BossTheme.h`, `src/core/BossTheme.cpp`: `ComposeBossTheme`, the piece itself: its chords, rhythms, melodies, stingers and the recipe of every instrument.
- `src/core/MusicDirector.h`, `src/core/MusicDirector.cpp`: `StepMusicDirector`, which reads the session and decides how loud each layer and the whole theme should be, and when a stinger starts.
- `src/core/MusicMixer.h`, `src/core/MusicMixer.cpp`: `MixMusic`, which plays the layers from one position, fades each one toward its level, and plays a stinger once over them.
- `src/app/audio/MusicPlayer.h`, `src/app/audio/MusicPlayer.cpp`: composes the theme at start-up, opens a raylib `AudioStream`, and refills it once a frame.
- `src/core/Synth.h`, `src/core/Synth.cpp`: `SynthLayer::highPassHz`, added for the guitars.

## How it works

**The piece.** 150 beats a minute in 4/4, so a sixteenth note lasts exactly a tenth of a second, and 16 bars in D minor: 25.6 seconds that loop, in two phrases of eight bars. The first phrase walks D minor, B flat, C and A major, two bars each; the major A pulls back toward D, which is what makes the key sound like a threat. The second moves every bar (Dm, C, Bb, A, Dm, F, Gm, A), so the loop tightens before it comes round.

**The two phrases sound different.** The first gallops: an eighth and two sixteenths on every beat, the run of the heavy metal of the eighties, with the kick on the eighths around a snare on 2 and 4. The second turns to a thrash beat (the kick on the beats, the snare on every off beat) with straight eighths on the guitar, and three of its bars end on a riff of power chords that walks down into the next chord: E, D, C, B into B flat; A, C sharp, E, C sharp into D; C, B flat, A, A flat into G. Halfway through each phrase the toms roll down the kit. The last bar of each phrase is a break: the band stops dead on three hits a dotted eighth apart, holds its breath for a beat and a half while the crash rings, and the snare rolls into the next phrase.

**The layers.**

| Layer | What plays | When |
|---|---|---|
| Pulse | The drums, the rhythm guitar and a bass an octave under it, as above, with a crash every four bars | The whole fight |
| Hook | A guitar tune in the register under the lead. Over the first phrase, a figure on each chord that climbs, answered by a held note; over the second, long notes on the chords' thirds and fifths under the riffs; in the last bar, it hits with the band and goes quiet with it | The whole fight, until the Wrath takes over |
| Hymn | A choir-like pad, one chord a bar, voiced close between A4 and G#5, with the root an octave under; timpani tuned to the chord on beats 1 and 3, rolling into each phrase | The second phase |
| Wrath | A double kick on every sixteenth, a crash on every bar, and a lead melody written on the chord tones | The boss's second bar low, and her frenzy |
| Desperation | A heartbeat on every beat (lub, then dub a sixteenth later, between the kicks); strings trembling on the chord's fifth and the half step above it, a stroke every sixteenth | The player's health low |

**The stingers.**

- **Victory, 10.5 s.** The gallop climbs B flat, C and D *major* under a fanfare from the lead (D, F sharp, A, D; then down from D to B flat; then up to E), with the choir on each chord and a timpani roll into the last bar. Then one hit on D major: a power chord that rings for four seconds under the choir, and the lead's last D held over it. VI and VII rising to a major I is a classic heroic cadence in film and game music, and both chords come from the theme, so the minor theme resolves to its own major.
- **Defeat, 6.5 s.** One blow: a kick, a timpani and a gong, a power chord that goes slack, sliding down a whole step as it dies, and a choir on D minor with the E flat a half step above its root. Then the Desperation's heartbeat, three beats further and further apart, and nothing.

**Composing.** `ComposeBossTheme` builds each instrument as a sound recipe, the same kind the effects use, synthesizes each note it needs once, and copies it into its layer wherever it plays (`MixInto`). A note that rings past the end of the loop wraps round into its first bar, so when the loop comes round, the tail is already there and the seam cannot be heard. When all five layers are written, they are scaled together by one number, from the loudest moment of all of them at once, so their balance is kept and the full mix never clips. Each stinger is scaled on its own to the same peak, 0.89, and is long enough that none of its notes wraps round. It takes about 0.7 s at 44.1 kHz in a Release build, and about 1.7 s in a Debug build.

**The guitars.** A power chord is a root, a slightly detuned double and the fifth on saws, through a high-pass at 180 Hz and then the soft clip at a drive of 6. The high-pass is the trick a metal player's boost pedal does: cutting the lows before the distortion keeps fast chords tight instead of muddy. The low-pass closes as the note dies, like a muted string. The Hook's guitar is two saws a little apart through less clip, darker than the lead, so the two tunes never sound alike.

**Deciding what plays.** Every frame, `StepMusicDirector` reads the session through `SituationOf` and returns a `MusicCue`:

- **Pulse** is up through the whole fight. **Hymn** is up in the second phase.
- **Wrath** starts to come in when the boss's second bar drops under 35%, and is all the way in at 15%. Her frenzy brings it all the way in at once. The first bar running low changes nothing.
- **Hook** gives way to the Wrath in an equal-power crossfade: the squares of the two gains always add up to 1, so the lead takes the tune over without the two playing over each other, and without a dip in loudness half way.
- **Desperation** starts to come in when the player drops under 45 health, and is all the way in at 30.
- A layer that comes in stays at least 2.5 s before it goes down again, so a flask or a hit right at a threshold does not make it flicker.
- **The volume** is full while playing, drops to 15% while the boss transforms (the band stops while she roars) and to 35% behind the pause menu, and fades out over 1 s on the title screen, which is silent.
- **The end.** A death starts the Defeat stinger and a victory the Victory stinger, on the same frame as the fight ends, while the theme gets out of their way (0.15 s after a death, 0.4 s after a victory). A retry cuts a stinger still playing in 0.05 s; going back to the title fades it over 1 s.
- **Restarts.** A new fight starts the theme from its first bar, with the last fight forgotten. The end of the transformation starts it from the first bar too, at full volume, with the Hymn in: the second phase slams in on the downbeat.

**Mixing.** `MixMusic` reads all five layers from one position, so they can never drift apart. Each layer's gain moves toward its cue a sample at a time: 0.6 s to come all the way in, 1.5 s to go all the way out, so the music builds faster than it calms down. The volume moves at the speed the cue asks for. A restart jumps to the first frame with the gains already at their levels. A stinger plays once from its first frame, added over the loop with a volume of its own.

**Playing.** `LoadMusicPlayer` opens one mono 32-bit stream at 44.1 kHz with blocks of 4096 frames, about a tenth of a second each. Once a frame, `UpdateMusicPlayer` cues the mixer and refills every block raylib has played. The stream plays at 0.8 times the settings' music volume (`SetMusicPlayerVolume`), under raylib's master volume, so the master turns the music down with the effects and the music volume turns it down alone (see [Settings](SETTINGS.md)). The music follows real time, not the fight's clock: a hit stop slows the fight, not the band.

## Design decisions

- **Layers of one piece, not one track per situation.** Swapping songs when the player's health drops would restart the music at the worst moment, and a crossfade between two songs smears two tempos together. Layers written over the same bars only ever add or take away, so a change lands in time, mid-bar, and the player hears the fight getting worse instead of a new song starting. Games like Doom (2016) and Hades do the same.
  - Rejected: separate tracks for phase one, phase two and low health, with crossfades.
- **I mix the layers myself into one stream.** Five raylib `Music` streams started together would each be decoded and queued on their own, and nothing would keep them in step after a hitch. One position in one mixer cannot drift.
  - Rejected: one `Music` stream per layer. Also rejected: raylib's audio callback, which runs on the audio thread in a desktop build and would need locks around the cue; refilling from the main loop is what raylib's own `UpdateMusicStream` does, and it works the same in a browser.
- **Synthesized, like the effects.** It keeps the game free of audio files and licenses, the loop is exactly 16 bars by construction, and every layer is in step because they are written from the same chords. I looked at licensed music first: the cheap Epidemic Sound plans cover social media and podcasts, not music inside a game you distribute, and free libraries rarely give the separate layers this needs.
  - Rejected: a licensed track. Also rejected: music generated by an AI service, whose terms for commercial use were unclear.
- **The opening has a tune of its own.** At first the Pulse played alone until the boss's bar ran low: the same gallop on one note for the whole first phase, often 40 seconds or more, with no melody and no change of groove. Playing it, that was the part that dragged. The Hook gives the opening a tune, the second phrase a different groove, and each phrase a break, so the loop has shape before any other layer comes in.
  - Rejected: putting the tune in the Pulse itself. It would play on under the Wrath's lead, and two melodies in the same range at once turn to mud. As its own layer it can hand over.
- **Stingers over the fade, not in place of the theme.** The theme gets out of the way in a fraction of a second, so the stinger's first blow lands on the moment the fight ends, and the end never cuts off mid-note into silence.
- **The defeat ends on the Desperation's heartbeat stopping.** The player who died low on health has been hearing that heart race; hearing it slow and stop says what happened before the words do.
- **The director is pure logic in `horde_core`, the mixer too.** What plays when, the fades, the loop and the stingers are tested with numbers, like the boss. Only the stream lives in the app.
- **A layer comes in at once and leaves late.** Danger should be heard the moment it starts; calm should be believed only after a while.
- **The layers are balanced by ear-weighted loudness, not by peak.** Measured with an A-weighting: the opening (Pulse and Hook) sits about 25 dBA under full scale, the second phase about 0.6 dB over it, the boss's second bar low about 1 dB, the player's health low about 3 dB, and every layer together about 4 dB; the Victory is about 3 dB over the opening, the Defeat 3 dB under it. My first balance had the Wrath 6.5 dB over the fight, so the lead drowned the guitars, and the Desperation's heartbeat under 100 Hz, where laptop speakers play almost nothing. When the Hook first played over the whole loop, the opening came out louder than the Wrath, so the tension went down when it should go up; the Hook now sits as loud as the Pulse.
- **The Wrath belongs to the second phase only.** At first it also came in as the first bar ran low. But the first bar running out is not the end of her, only the start of the transformation, so the climax came too early and the second phase had nothing new to build to. Now the first phase keeps the opening's music all the way, and the Wrath is saved for the end of the fight.
- **The Desperation is part of the mix, not under it.** Playing, its heartbeat and strings sat far behind the band, so a dying player barely heard the music change. It now sits as loud as the rest of the opening put together: it lifts the mix by about 3 dB, as clear a change as the Wrath's, without covering the tune.
- **A restart sets every layer straight to the new situation.** The transformation lasts 2 s and a layer holds for 2.5 s, so without a reset a layer that came in during the first phase, such as the Desperation of a player who drank a flask, could hang over the start of the second.

## How to tune it

| What | Where | Now |
|---|---|---|
| Tempo, length | `kBossThemeBpm`, `kBossThemeBars` (`BossTheme.h`) | 150, 16 bars |
| Chords, tunes, riffs | `kChords`, `kMelody`, `kHookMelody`, `kRiffs`, `kFanfare` (`BossTheme.cpp`) | D minor, as above |
| How each instrument sounds | `Kick`, `Chug`, `Stab`, `Ring`, `HookGuitar`, `HymnVoice`, `Lead`, `Gong`, `Heartbeat`, `Tremolo`... (`BossTheme.cpp`) | One recipe each |
| How loud each part is in its layer | The gains in `ComposePulse`, `ComposeHook`, `ComposeHymn`, `ComposeWrath`, `ComposeDesperation`, `ComposeVictory`, `ComposeDefeat` | |
| The stingers' lengths | `kVictorySeconds`, `kDefeatSeconds` (`BossTheme.cpp`) | 10.5 s, 6.5 s |
| When Wrath and Desperation come in | `MusicDirectorStats` (`MusicDirector.h`) | 35% to 15% of the second bar; 45 to 30 health |
| How long a layer holds | `MusicDirectorStats::holdSeconds` | 2.5 s |
| The volume when paused and transforming, and the fades | `MusicDirectorStats` | 35%, 15%; 0.15 s death, 0.4 s victory, 1 s title, 0.05 s stinger cut |
| How fast layers come in and go out | `MusicMixerStats` (`MusicMixer.h`) | 0.6 s, 1.5 s |
| The music under the effects | `kMusicVolume` (`MusicPlayer.cpp`), times the settings' music volume squared | 0.8 at full |

In a Debug build, the `F1` overlay has a `music` line: the volume, the gain of each layer (p, h, w, d and k for the Hook), the bar playing, and the stinger while one plays, with its volume.

## Tests

- `tests/core/SequencerTests.cpp`: a note's pitch from its number; a note mixed at its frame, scaled, on top of what is there; a note wrapping round the end of the loop; the release fading the last frames out.
- `tests/core/BossThemeTests.cpp`: a sixteenth is a whole number of frames; the loop is 16 bars at 150 beats a minute and every layer fills it; every layer plays and all of them together peak at 0.89; the Pulse hits on every beat but in its two breaks; the band stops in the last bar of each phrase; the Hook plays through both phrases and goes quiet with the band; both stingers open on their blow, peak at 0.89 and die away to silence; the heartbeat's dub lands a sixteenth after the beat; the same rate always composes the same theme and stingers.
- `tests/core/MusicDirectorTests.cpp`: a fight starts over with the Pulse and the Hook; the band drops while the boss transforms and the second phase restarts with the Hymn; the first bar running low leaves the music alone; the Wrath's ramp on the second bar and the Desperation's ramp; the Hook's equal-power handover; the frenzy; the hold, and a layer going back up cancelling it; the pause; a death starting the Defeat once and a victory the Victory; a retry cutting the stinger and the title fading it; the theme getting out of the way; a retry forgetting the last fight; the silent title; reading the session.
- `tests/core/MusicMixerTests.cpp`: silence until cued; a restart from the first frame; the layers together and wrapping round; a restart mid-loop; the fade in and out times; the volume's speed; a stinger playing once over the theme; a stinger's own volume, also after it has ended; the clamp; an empty track.
- `tests/core/SynthTests.cpp`: the high-pass thins out a low tone and lets a high one through.

I checked the rest by running the game. A temporary tool wrote the layers, five mixes, both stingers, the first 40 seconds of a fight and two scripted fights (played through the real director and mixer at 60 frames a second) to WAV files, which I measured for peak, loudness and spectrum and plotted as spectrograms and loudness over time; that is how I found the balances above, the break that was too short to read as a stop (the roll first started on beat 3; it now waits for beat 4), and the second phrase that dropped 3 dB when it had no tune. In a Debug build, captures of the `F1` line showed the theme at full volume with the Hook in the first phase, at 15% during the transformation, back on bar 1 with the Hymn in the second phase, the Desperation coming in as the player lost health, the Wrath all the way in during the frenzy, the Defeat stinger playing over a silent theme after a death, a retry back on bar 1 with the stinger cut, and the Victory stinger after the boss fell. In the web build, the page's audio output measured silence on the title, -25 dB once the fight began, and after a death a blow that died away over about five seconds to silence, the shape of the Defeat. I could not listen to any of it myself.

## Limitations

- **It is synthesized, not played.** The guitars are saws through a clip, not guitars through an amp, and the choir is a pad, not voices. It reads as dark synth metal more than as a band.
- **25.6 seconds repeat.** A long fight hears the loop many times. A third phrase, or variations picked at random, would help.
- **No menu music.** The title screen is silent.
- **About 25 MB of samples** for the five layers and the stingers at 44.1 kHz, composed at every start: about 0.7 s in a Release build, about 1.7 s in a Debug build.
