# Contributing and Git workflow

Horde Carnage is a solo project, but I work on it the way a small team would: short-lived branches, pull requests, a readable history and tagged releases.

## Branching model (trunk-based)

- `main` is always **playable**: it builds without warnings, the tests pass and the game starts.
- Every change happens in a short-lived branch, made from an up-to-date `main`, and goes in through a **pull request** (I review it myself) in a day or two at most.

| Prefix | Use | Example |
|---|---|---|
| `feat/` | New feature | `feat/melee-combat` |
| `fix/` | Bug fix | `fix/enemy-stuck-in-wall` |
| `docs/` | Documentation only | `docs/architecture-diagram` |
| `refactor/` | No change in behaviour | `refactor/split-enemy-states` |
| `chore/` | Tooling, dependencies, releases | `chore/release-v0.1.0` |

## Commit messages ([Conventional Commits](https://www.conventionalcommits.org/))

```
<type>(<scope>): <short imperative summary>

feat(combat): add melee swing with knockback
fix(waves): stop enemies spawning inside the player
docs(readme): add controls section
```

Types: `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `chore`.
Scopes are the module that changes: `core`, `player`, `enemies`, `combat`, `waves`, `ui`, `build`, `release`...

## Before opening a pull request

1. `cmake --build --preset release` builds with no warnings (they are errors for project code anyway).
2. `ctest --preset release` passes, and I write down the numbers in the PR.
3. I play the change in a Release build.
4. If it changes something visual, I attach a before and after screenshot or a short GIF.
5. Nothing from `build/`, `out/` or `.vs/` is committed.

## Pull request flow

1. Create the branch from an up-to-date `main`: `git switch -c feat/my-feature`.
2. Make small, focused commits.
3. Open a PR with the template. The PR title is the commit title, because it becomes the squashed commit.
4. Wait for CI to pass, and read the diff on GitHub before merging, as a reviewer would.
5. Use **Squash and merge**, so `main` gets one clean commit per feature, then delete the branch.
6. Every change adds an entry to [CHANGELOG.md](CHANGELOG.md) under `[Unreleased]`.

## Releases

- `0.1.1`: fixes only. `0.2.0`: something new and playable. `1.0.0`: finished.
- Steps:
  1. Branch `chore/release-vX.Y.Z`.
  2. Bump `VERSION` in the `project()` call of the root `CMakeLists.txt`.
  3. Move `[Unreleased]` in the changelog to `## [X.Y.Z] - YYYY-MM-DD`, with a one-line summary.
  4. Release build, and a zip with `HordeCarnage.exe` and its data folder. Web build (`cmake --preset web`), and a zip with `index.html`, `index.js` and `index.wasm` for itch.io (see [The web build](docs/WEB_BUILD.md)).
  5. PR, and squash merge.
  6. GitHub > Releases > Draft a new release: tag `vX.Y.Z` on `main`, title `Horde Carnage vX.Y.Z`, notes (what you can play, how to run it, known problems) and the zip. Marked as pre-release until the version I present. Then the web zip goes to the itch.io page, replacing the last one.
- Every version is a new release. I never replace the zip of a published one.

## Code conventions

- Pure game logic lives in `src/core/` (library `horde_core`) and never calls window, input or drawing functions, so it can be unit tested. Anything that touches raylib's window lives in `src/app/` or in a module that the game executable owns.
- One responsibility per file. Files are named after the class they hold (`FixedTimestep.h`, `FixedTimestep.cpp`).
- Everything is in the `horde` namespace. Types and functions are `PascalCase`, private members are `_camelCase`, constants are `kPascalCase`.
- Comment the *why*, not the *what*. Public APIs get `///` doc comments.
- Debug-only shortcuts are compiled only in Debug builds.
