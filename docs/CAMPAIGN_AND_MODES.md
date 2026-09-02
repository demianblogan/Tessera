# Campaign, Game Modes & AI — concept draft

**Status:** draft. Written during v1.3.0 to frame the `ModeSelectScreen` stubs.
The gameplay-facing details (rule modifiers, scoring, balance) are finalised in
v1.4.0+; nothing here is committed until then.

This is the source of the "Start Game" flow. From the main menu, "Start Game"
opens a **mode select** screen instead of dropping straight into gameplay.

---

## Mode select screen

Entries, in order (a `MenuScreen` on the shared `ScreenHost`, styled like
Options):

| Entry | v1.3.0 | Later |
|-------|--------|-------|
| Marathon | **live** — current endless gameplay | records, level curve tuning (v1.5.0) |
| Start Campaign | disabled stub | v1.6.0 |
| Continue Campaign | hidden until a campaign is in progress | v1.6.0 |
| Select Level | disabled stub — replay cleared campaign levels for stars | v1.6.0 |
| Player vs Player | disabled stub | v1.7.0 |
| Back to Main Menu | live | — |

"Other single-player modes" (Sprint / Ultra / Zen) slot in here in v1.5.0 —
either as their own entries or under a "Solo" sub-list if the list grows too
long (same pattern as Options → Controls).

---

## Campaign (v1.6.0)

**Goal:** a ~30-minute campaign that keeps changing so it does not get stale.

- **~12–16 levels**, ~1.5–2.5 min each.
- Each level has a **score gate** to advance, plus **one rule modifier** that
  changes how the game plays.
- Modifiers are introduced one at a time, then **combined** two or three levels
  later. Pressure levels alternate with breather levels.
- **Boss level every 5th** (5, 10, 15): a duel against the AI opponent.
- **1–3 stars per level** (by score / time / no-hold etc.) → replay value via
  Select Level.
- **Progress** persists to `%LOCALAPPDATA%\Alone Bull Company\Tessera\`
  (`campaign_progress.json` or similar) — never the repo. Format-versioned and
  corruption-preserving like the other save files.

### Rule modifier pool (candidates)

Speed / gravity: progressive gravity ramp (faster every N seconds), 20G,
sudden-death speed after a time limit.
Board: narrowing / shrinking field, locked columns, moving walls, board shift
("earthquake"), gravity flip.
Garbage: periodic rising garbage lines, garbage in bursts on a timer.
Visibility: fog over the upper board, blocks turn invisible N seconds after
locking, "invisible" mode.
Pieces: fixed piece sequence, no-hold, big mode (2×2 blocks), wind (active
piece drifts), bomb blocks, key blocks that must be cleared.
Objectives: "clear in N pieces", time-limited, "reach X lines without a
mistake".

### Open questions (decide in v1.4.0)

- Does the campaign **absorb** the old "challenge ladder" idea, or do both
  exist (campaign = score-gated progression; ladder = discrete skill puzzles)?
  Current lean: absorb.
- Which modifiers need engine hooks that should land with the v1.4.0 rules work
  (e.g. gravity control, garbage injection, board resize).

---

## Game modes

### Solo (v1.5.0)

- **Marathon** — endless, level curve. The current game.
- **Sprint** — clear 40 lines as fast as possible.
- **Ultra** — highest score in 2 minutes.
- **Zen** — no top-out, relax. Optional.

Per-mode records; extended stats behind the Records ring entry.

### Local PvP (v1.7.0)

- Split-screen, two `GameplaySession`s on one machine.
- Input: **both on gamepads**, or **one keyboard + one gamepad**. Never both on
  keyboard.
- **Garbage / attack model**: line clears send garbage rows to the opponent
  (this is the "garbage model" long noted in the backlog's deferred ideas).
- Versus HUD, win condition (opponent tops out), best-of rounds.

---

## AI opponent

- One heuristic **"virtual player"** that reads a board and picks a placement:
  weighted features (aggregate height, holes, bumpiness, lines cleared) in the
  El-Tetris / Dellacherie style. It drives a `GameplaySession` through the same
  intents a human does, with a configurable move delay for difficulty.
- **Reused** in two places:
  1. a dedicated **VS AI** mode (PvP with player 2 = the bot, difficulty
     levels),
  2. the **campaign boss levels** every 5th level.
- Build order: the PvP infrastructure (split sessions, garbage model, versus
  HUD) comes first; the AI then slots in as "player 2".
