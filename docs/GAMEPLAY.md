# Gameplay reference

- [The board](#the-board)
- [Pieces & randomizer](#pieces--randomizer)
- [Rotation & kicks](#rotation--kicks)
- [Scoring](#scoring)
- [Levels & gravity](#levels--gravity)
- [Lock delay](#lock-delay)
- [Escalation](#escalation)
- [Settings](#settings)
- [Save data](#save-data)

---

## The board

10 columns wide, 20 rows tall and visible, plus 20 hidden rows above it (the
buffer) where a piece spawns and where a tall spin has room to happen without
being clipped by the ceiling. Nothing in the buffer is drawn.

---

## Pieces & randomizer

The 7 standard tetrominoes (I, O, T, S, Z, J, L). By default the bag is a
guideline **7-bag**: every piece type appears exactly once per shuffled
bag of 7, so you're never more than 12 pieces from seeing one again.
**Options → Gameplay → Randomizer** can switch it to pure uniform random
instead.

Piece shapes are data-driven (`assets/data/pieces.json`, loaded through
`PieceDataFile`) with a hardcoded standard-shape fallback if the file is
missing or fails to parse.

---

## Rotation & kicks

Standard **SRS** (Super Rotation System): a piece rotating near a wall or
another piece tries a short, defined sequence of offset "kicks" before
giving up. Two kick tables — one shared by J/L/S/T/Z, one for I — loaded
from `assets/data/srs_kicks.json` (`KickDataFile`), with the standard SRS
tables as a fallback.

A rotation counts as a **T-spin** (via the standard three-corner rule) when
it's a T piece, the rotation only succeeded because of a kick (or the piece
was already boxed in), and three of the T's four diagonal neighbor cells are
occupied. It's a **mini** T-spin if the two occupied corners are on the
piece's non-pointing side.

---

## Scoring

| Clear | Points (× level) |
|---|---:|
| Single | 100 |
| Double | 300 |
| Triple | 500 |
| Tetris | 800 |
| T-spin (no clear) | 400 |
| T-spin Mini (no clear) | 100 |
| T-spin Single | 800 |
| T-spin Double | 1200 |
| T-spin Triple | 1600 |
| T-spin Mini Single | 200 |
| T-spin Mini Double | 400 |

Plus:

- **Soft drop** — 1 point per cell dropped
- **Hard drop** — 2 points per cell dropped
- **Combo** — `50 × comboCount × level`, on top of the clear, for every clear
  that immediately follows another with no piece locking in between
- **Back-to-back** — a Tetris or a scoring T-spin that directly follows
  another Tetris/T-spin clear multiplies the line-clear component by ×1.5
- **Perfect Clear** — the whole board empties out: a flat bonus of 800 /
  1200 / 1800 / 2000 depending on how many rows the clear took, added on top
  of everything else
- **Golden bonus** (see [Escalation](#escalation)) — doubles the line-clear
  score if the clear includes a golden-locked piece; stacks with back-to-back

---

## Levels & gravity

The level rises by 1 every **10 total lines cleared**. Gravity follows the
standard guideline curve: roughly 1 second per row at level 1, easing down
level by level, floored at a 0.02s minimum so it never becomes literally
instant.

---

## Lock delay

A piece that lands gets 0.5 seconds before it locks, and any move or
rotation resets that timer — up to 15 resets per depth the piece has
reached, after which it locks regardless, so a piece can't be stalled
forever at the same height.

---

## Escalation

Beyond the level/gravity curve, one continuous endless run gets progressively
more hostile over elapsed time, driven by `EscalationDirector`. Tiers are
cumulative — each one keeps everything the tier before it introduced:

| Tier | Starts at | Adds |
|---|---|---|
| **Base** | 0:00 | Just the guideline gravity curve |
| **Speed Surge** | 0:30 | Short, periodic gravity spikes (~2× speed) — first around 0:38, then roughly every 35s, lasting 5s each |
| **Garbage** | 0:55 | A rising garbage row on a random 10–30s timer, re-rolled after every row that comes up |
| **Chaos** | 1:35 | A rare **golden** bonus piece — first around 2:05, then roughly every 8 spawns — that doubles the score of whatever clear it's part of |

A Speed Surge and a garbage row both telegraph themselves (a callout, a
shake, a haptic pulse) before they hit, so a sudden spike reads as a fair
warning rather than a glitch.

---

## Settings

**Graphics** — resolution, Fullscreen / Windowed / Borderless, V-Sync, FPS
counter, CRT post-processing filter.

**Audio** — music volume and sound effects volume, independently, in 10
steps.

**Gameplay** — gamepad vibration, gamepad lightbar, screen shake, ghost
piece, hold piece, next-queue preview length (1–5 pieces), 7-bag randomizer
toggle.

**HUD** — independent show/hide for the Hold panel, Next panel, Score,
Lines, Level, Time, and the on-screen controls legend.

**Controls** — full keyboard rebinding for every gameplay action; gamepad
bindings are fixed (see [CONTROLS.md](CONTROLS.md)).

**Language** — English, Spanish, German, Russian, Ukrainian. The first run
opens a one-time language picker before ever reaching the main menu.

---

## Save data

```
%LOCALAPPDATA%\Alone Bull Company\Tessera\
```

| File | Contents |
|---|---|
| `settings.json` | every setting above |
| `scores.json` | the local top-10 leaderboard (name, score, lines, level) |

Falls back to a `user_data\` folder next to the executable if
`%LOCALAPPDATA%` isn't available. Writes are atomic; a file that fails to
parse is quarantined as `<name>.corrupt` instead of being overwritten or
silently dropped.
