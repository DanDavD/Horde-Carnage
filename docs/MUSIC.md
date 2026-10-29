# Music

## What it is

The boss fight has its own theme, and it follows the fight: one piece of music in four layers that come and go with what is happening. The whole fight has drums, a galloping guitar and a bass. The second phase adds a choir and timpani. When the boss is close to death, or in her frenzy, a double kick and a lead melody come in. When the player is close to death, a racing heartbeat and trembling strings come in. The beat never changes, only how many layers play over it, so a low bar or a dying player sounds like the same song getting worse.

Like the sound effects (see [Audio](AUDIO.md)), every note is synthesized in code when the game starts. There are no music files.

The code:

- `src/core/Sequencer.h`, `src/core/Sequencer.cpp`: `NoteHz` (the pitch of a MIDI note) and `MixInto`, which places a synthesized note into a loop and wraps its tail round to the start.
- `src/core/MusicTrack.h`: `MusicStem`, the four layers, and `MusicTrack`, a loop of each.
- `src/core/BossTheme.h`, `src/core/BossTheme.cpp`: `ComposeBossTheme`, the piece itself: its chords, its rhythms, its melody and the recipe of every instrument.
- `src/core/MusicDirector.h`, `src/core/MusicDirector.cpp`: `StepMusicDirector`, which reads the session and decides how loud each layer and the whole theme should be.
- `src/core/MusicMixer.h`, `src/core/MusicMixer.cpp`: `MixMusic`, which plays the four layers from one position and fades each one toward its level.
- `src/app/audio/MusicPlayer.h`, `src/app/audio/MusicPlayer.cpp`: composes the theme at start-up, opens a raylib `AudioStream`, and refills it once a frame.
- `src/core/Synth.h`, `src/core/Synth.cpp`: `SynthLayer::highPassHz`, added for the guitar.

## How it works

**The piece.** 150 beats a minute in 4/4, so a sixteenth note lasts exactly a tenth of a second, and 16 bars in D minor: 25.6 seconds that loop. The first eight bars walk D minor, B flat, C and A major, two bars each; the major A pulls back toward D, which is what makes the key sound like a threat. The second eight move every bar (Dm, C, Bb, A, Dm, F, Gm, A), so the loop tightens before it comes round. Every eighth bar ends in a snare roll into the next phrase.

**The layers.**

| Layer | What plays | When |
|---|---|---|
| Pulse | Kick on the eighths around the snare on 2 and 4, eighth-note hats, a crash every four bars; palm-muted power chords in a gallop (an eighth and two sixteenths on every beat); a bass an octave under them | The whole fight |
| Hymn | A choir-like pad, one chord a bar, voiced close between A4 and G#5, with the root an octave under; timpani tuned to the chord on beats 1 and 3, rolling into each phrase | The second phase |
| Wrath | A double kick on every sixteenth, a crash on every bar, and a lead melody written on the chord tones | The boss's bar low, and her frenzy |
| Desperation | A heartbeat on every beat (lub, then dub a sixteenth later, between the kicks); strings trembling on the chord's fifth and the half step above it, a stroke every sixteenth | The player's health low |

**Composing.** `ComposeBossTheme` builds each instrument as a sound recipe, the same kind the effects use, synthesizes each note it needs once, and copies it into its layer wherever it plays (`MixInto`). A note that rings past the end of the loop wraps round into its first bar, so when the loop comes round, the tail is already there and the seam cannot be heard. When all four layers are written, they are scaled together by one number, from the loudest moment of all four at once, so their balance is kept and the full mix never clips. It takes about 0.26 s at 44.1 kHz in a Release build.

**The power chords.** A root, a slightly detuned double and the fifth on saws, through a high-pass at 180 Hz and then the soft clip at a drive of 6. The high-pass is the trick a metal player's boost pedal does: cutting the lows before the distortion keeps fast chords tight instead of muddy. The low-pass closes as the note dies, like a muted string.

**Deciding what plays.** Every frame, `StepMusicDirector` reads the session through `SituationOf` and returns a `MusicCue`:

- **Pulse** is up through the whole fight. **Hymn** is up in the second phase.
- **Wrath** starts to come in when the boss's bar in progress drops under 35%, and is all the way in at 15%. Her frenzy brings it all the way in at once.
- **Desperation** starts to come in when the player drops under 45 health, and is all the way in at 30.
- A layer that comes in stays at least 2.5 s before it goes down again, so a flask or a hit right at a threshold does not make it flicker.
- **The volume** is full while playing, drops to 15% while the boss transforms (the band stops while she roars) and to 35% behind the pause menu. It fades out over 0.6 s after a death, over 2 s after a victory while she dies, and over 1 s on the title screen, which is silent.
- **Restarts.** A new fight starts the theme from its first bar, with the last fight forgotten. The end of the transformation starts it from the first bar too, at full volume, with the Hymn in: the second phase slams in on the downbeat.

**Mixing.** `MixMusic` reads all four layers from one position, so they can never drift apart. Each layer's gain moves toward its cue a sample at a time: 0.6 s to come all the way in, 1.5 s to go all the way out, so the music builds faster than it calms down. The volume moves at the speed the cue asks for. A restart jumps to the first frame with the gains already at their levels.

**Playing.** `LoadMusicPlayer` opens one mono 32-bit stream at 44.1 kHz with blocks of 4096 frames, about a tenth of a second each. Once a frame, `UpdateMusicPlayer` cues the mixer and refills every block raylib has played. The stream plays at 0.7 under raylib's master volume, so the settings' volume turns the music down with the effects. The music follows real time, not the fight's clock: a hit stop slows the fight, not the band.

## Design decisions

- **Layers of one piece, not one track per situation.** Swapping songs when the player's health drops would restart the music at the worst moment, and a crossfade between two songs smears two tempos together. Layers written over the same bars only ever add or take away, so a change lands in time, mid-bar, and the player hears the fight getting worse instead of a new song starting. Games like Doom (2016) and Hades do the same.
  - Rejected: separate tracks for phase one, phase two and low health, with crossfades.
- **I mix the layers myself into one stream.** Four raylib `Music` streams started together would each be decoded and queued on their own, and nothing would keep them in step after a hitch. One position in one mixer cannot drift.
  - Rejected: four `Music` streams. Also rejected: raylib's audio callback, which runs on the audio thread in a desktop build and would need locks around the cue; refilling from the main loop is what raylib's own `UpdateMusicStream` does, and it works the same in a browser.
- **Synthesized, like the effects.** It keeps the game free of audio files and licenses, the loop is exactly 16 bars by construction, and every layer is in step because they are written from the same chords. I looked at licensed music first: the cheap Epidemic Sound plans cover social media and podcasts, not music inside a game you distribute, and free libraries rarely give the separate layers this needs.
  - Rejected: a licensed track. Also rejected: music generated by an AI service, whose terms for commercial use were unclear.
- **The director is pure logic in `horde_core`, the mixer too.** What plays when, the fades and the loop are tested with numbers, like the boss. Only the stream lives in the app.
- **A layer comes in at once and leaves late.** Danger should be heard the moment it starts; calm should be believed only after a while.
- **The layers are balanced by ear-weighted loudness, not by peak.** Measured with an A-weighting, the whole fight's mix is about 26 dBA under full scale; the Wrath raises it by about 4 dB, the Desperation by about 2 dB plus a new sound, and all four together by about 5 dB. My first balance had the Wrath 6.5 dB over the fight, so the lead drowned the guitars, and the Desperation's heartbeat under 100 Hz, where laptop speakers play almost nothing.
- **The first phase's Wrath is not carried into the second.** The transformation lasts 2 s and a layer holds for 2.5 s, so without a reset the second phase would open with the end of the first phase's Wrath. A restart sets every layer straight to the new situation.

## How to tune it

| What | Where | Now |
|---|---|---|
| Tempo, length | `kBossThemeBpm`, `kBossThemeBars` (`BossTheme.h`) | 150, 16 bars |
| Chords, melody | `kChords`, `kMelody` (`BossTheme.cpp`) | D minor, as above |
| How each instrument sounds | `Kick`, `Chug`, `HymnVoice`, `Lead`, `Heartbeat`, `Tremolo`... (`BossTheme.cpp`) | One recipe each |
| How loud each part is in its layer | The gains in `ComposePulse`, `ComposeHymn`, `ComposeWrath`, `ComposeDesperation` | |
| When Wrath and Desperation come in | `MusicDirectorStats` (`MusicDirector.h`) | 35% to 15% of the bar; 45 to 30 health |
| How long a layer holds | `MusicDirectorStats::holdSeconds` | 2.5 s |
| The volume when paused, transforming, and the fades | `MusicDirectorStats` | 35%, 15%; 0.6 s death, 2 s victory |
| How fast layers come in and go out | `MusicMixerStats` (`MusicMixer.h`) | 0.6 s, 1.5 s |
| The music under the effects | `kMusicVolume` (`MusicPlayer.cpp`) | 0.7 |

In a Debug build, the `F1` overlay has a `music` line: the volume, the gain of each layer (p, h, w, d) and the bar playing.

## Tests

- `tests/core/SequencerTests.cpp`: a note's pitch from its number; a note mixed at its frame, scaled, on top of what is there; a note wrapping round the end of the loop; the release fading the last frames out.
- `tests/core/BossThemeTests.cpp`: a sixteenth is a whole number of frames; the loop is 16 bars at 150 beats a minute and every layer fills it; every layer plays and all four together peak at 0.89; the Pulse hits on every one of the 64 beats; the heartbeat's dub lands a sixteenth after the beat; the same rate always composes the same theme.
- `tests/core/MusicDirectorTests.cpp`: a fight starts over with the Pulse alone; the band drops while the boss transforms and the second phase restarts with the Hymn; the first phase's Wrath does not carry over; the Wrath's and the Desperation's ramps; the frenzy; the hold, and a layer going back up cancelling it; the pause; the fades after a death and a victory; a retry forgetting the last fight; the silent title; reading the session.
- `tests/core/MusicMixerTests.cpp`: silence until cued; a restart from the first frame; the layers together and wrapping round; a restart mid-loop; the fade in and out times; the volume's speed; the clamp; an empty track.
- `tests/core/SynthTests.cpp`: the high-pass thins out a low tone and lets a high one through.

I checked the rest by running the game. A temporary tool wrote the four layers, five mixes and two scripted fights (played through the real director and mixer at 60 frames a second) to WAV files, which I measured for peak, loudness and spectrum and plotted as spectrograms and loudness over time; that is how I found the first balance above. In a Debug build, captures of the `F1` line showed the theme at full volume in the first phase, at 15% during the transformation, back on bar 1 with the Hymn in the second phase, the Desperation coming in as the player lost health, the Wrath all the way in during the frenzy, the volume fading after a death, and a retry back on bar 1 with nothing held over. In the web build, the page's audio output measured silence on the title and -26 dB once the fight began. I could not listen to any of it myself.

## Limitations

- **It is synthesized, not played.** The guitar is saws through a clip, not a guitar through an amp, and the choir is a pad, not voices. It reads as dark synth metal more than as a band.
- **25.6 seconds repeat.** A long fight hears the loop many times. A second section, or variations picked at random, would help.
- **No stingers yet.** A death and a victory only fade the theme out.
- **No menu music.** The title screen is silent.
- **No music volume of its own.** The settings' volume turns music and effects down together.
- **About 18 MB of samples** for the four layers at 44.1 kHz, composed at every start in about a quarter of a second.
