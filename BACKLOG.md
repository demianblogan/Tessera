# Tetris — Development Backlog

This file is the source of truth for completed work, the patch currently in
progress, and planned versions. Update it whenever an item changes scope or
release.

The project follows the same two-phase path as *Until Last Asteroid*:

1. **Phase 1 — code maturity (`v1.0.x`).** No new gameplay. Turn the existing
   prototype-grade code into a professional, bug-free, de-duplicated, well
   layered codebase.
2. **Phase 2 — features (`v1.1.0`+).** Build new systems and content on the
   cleaned-up foundation, porting proven pieces from *Until Last Asteroid*.

Each version gets its own `codex/vX.Y.Z-*` branch, a pull request with a clear
summary, a git tag, and a GitHub Release build.

---

## Released

### v1.0

- Original portfolio version: playable Tetris with 7-bag pieces, ghost piece,
  progressive difficulty, next-piece preview, screen shake, row-clear and
  landing effects, a custom UI framework, a settings menu, top-5 high scores,
  and shader-based CRT / blur / glow post-processing.

---

## In progress

### v1.0.1 — Build hygiene & core loop

- Attach the local snapshot to the GitHub repository and adopt the
  branch / PR / tag / release workflow; add `BACKLOG.md` and `.editorconfig`.
- Drop the broken Win32/x86 build configurations (they had no SFML paths and
  could never link); keep a single x64 target on C++23.
- Raise the warning level to `/W4` and fix the `[[nodiscard]]` render-texture
  results that were being discarded in `Game`. `/W4` now surfaces ~40
  pre-existing warnings (member shadowing in `Slider` / `Layout`, unreferenced
  parameters, a few narrowing conversions in the gameplay renderer); clearing
  them and turning on warnings-as-errors is tracked for v1.0.6.
- Add a compile-time `static_assert` that the vendored SFML is exactly 3.1.0.
- Stop committing runtime player data (`data/scores.txt`, `data/settings.txt`);
  keep the directory with a `.gitkeep`.
- Clamp the per-frame delta time (`Game::MaxFrameTime`) so a stall can no
  longer teleport the falling piece or expire a timer instantly.
- Replace the `dynamic_cast<PauseState*>` render-pipeline check with an explicit
  `StateId` enum and `State::GetId()`.
- Stop locking the tetromino twice on a hard drop.
- Compute the screen-shake offset in `Update` and only apply it in `Render`, so
  rendering stops pulling from the shared RNG; guard against a zero-length
  shake dividing by zero.
- Bounds-check `Board::IntersectsLockedCells` before indexing the grid.
- `StatisticsState` now reads the shared `HighScoreManager` from the context
  instead of loading a second private copy, so clearing records in-session
  stays consistent with the Game Over screen.

---

## Planned

### Phase 1 — code maturity

- **v1.0.2 — Board & piece model.** Bounds-safety pass across `Board`;
  de-duplicate `GetFullRows` / `ClearFullRows`; turn the row-clear sequence into
  an explicit phase so the locked piece and its ghost stop being drawn on top of
  the board during the clear animation; add buffer rows above the field and a
  guideline-correct top-out / block-out check.
- **v1.0.3 — State & menu de-duplication.** A base `State` with
  `RequestPush/Pop/Clear`; centralize window `Closed` / `Resized` handling in
  `Game` (`State::HandleEvent(const sf::Event&)`); extract a reusable
  `MenuList` widget and delete the copy-pasted menu navigation from four states;
  fix the Save button that abuses "selected" as "enabled".
- **v1.0.4 — Persistence.** Save to `%LOCALAPPDATA%` via an `AppDataPath`
  helper; atomic writes via `SafeFileWrite`; a settings format version with
  validation and safe defaults; single source of truth for high scores.
- **v1.0.5 — Split `GameState`.** Separate the rules/model (`TetrisGame`) from
  input translation and rendering (`GameplayState`, `BoardRenderer`,
  `EffectsController`); pool sound voices in `AudioPlayer`.
- **v1.0.6 — Render pipeline & warnings.** Move the post-processing pipeline
  into its own type; remove per-frame allocations in the gameplay renderer;
  modernize the GLSL; reach zero `/W4` warnings and enable warnings-as-errors;
  first unit tests (`Board`, `Tetromino`, `TetrominoBag`, scoring).

### Phase 2 — features

- **v1.1.0 — Input system.** DAS/ARR with tuning; rebindable keyboard controls
  (`ActionMap` / `InputBinding` port); Xbox / PlayStation gamepad support
  (`GamepadManager` port); on-screen button prompts.
- **v1.2.0 — Modern rules.** SRS rotation with wall kicks and T-spin detection;
  lock delay with move reset; hold piece; a 5-deep next queue; guideline
  scoring (back-to-back, combo, soft/hard-drop points, perfect clear); on-board
  "TETRIS" / "T-SPIN" callouts.
- **v1.3.0 — Real visuals.** True bloom (`NeonGlow` port) for the active piece,
  ghost, clears and menu selection; a particle system; a row-collapse
  animation; board skins and a parallax background; the Alone Bull Company
  splash with logo and sound; an animated main-menu intro; individual toggles
  for every effect plus screen-shake and reduce-motion.
- **v1.4.0 — Localization & options.** `LocalizationManager` + font system port
  (final language list decided here); a full Options screen (graphics / audio /
  controls / gameplay / language); a letterboxing `DisplayManager`; an FPS
  toggle.
- **v1.5.0 — Game modes.** Marathon, Sprint (40 lines), Ultra (2 minutes), Zen;
  per-mode records and extended stats (PPS, Tetris rate, longest combo).
- **v1.6.0 — Challenge ladder & achievements.** A ladder of short objective
  stages with modifiers (fixed piece sequences, garbage starts, invisible
  blocks, 20G, big mode, "clear in N pieces") with escalating difficulty and
  per-stage stars; `AchievementManager` port.
- **v1.7.0 — Audio & HUD.** Dynamic-intensity music, a full SFX set, menu
  ambience; a nine-slice HUD redesign.
- **v2.0.0 — Polish & docs.** Final polish pass, GitHub documentation, preview
  GIFs, release.

---

## Deferred ideas

- Garbage / attack model as groundwork for a future versus mode or AI opponent.
- Colorblind palettes and an adjustable grid opacity.
- Replays / seed sharing for the Sprint and Ultra modes.
