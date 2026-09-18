# Controls

The game detects your input device automatically and switches on the fly —
touching a key or a gamepad button hides the mouse cursor, moving the mouse
brings it back. No menu toggle needed.

Keyboard bindings are fully rebindable in **Options → Controls → Keyboard**.
The table below is the defaults. Gamepad bindings are fixed.

---

## Keyboard

| Action | Binding |
|---|---|
| Move left | `←` |
| Move right | `→` |
| Soft drop | `↓` |
| Hard drop | `Space` |
| Rotate clockwise | `E` |
| Rotate counter-clockwise | `Q` |
| Hold | `C` |
| Pause | `Esc` (fixed, not rebindable) |
| Menu navigation | Arrow keys or mouse |
| Menu confirm | `Enter` / Left Mouse Button |
| Menu back | `Esc` |
| Skip splash / intro | any key |

A key already bound to another action flashes both rows red and is rejected;
a reserved key (`Esc`, modifiers, function keys, ...) flashes amber. Accepted
rebinds flash green.

---

## Xbox controller

| Action | Input |
|---|---|
| Move | D-pad / Left stick |
| Soft drop | D-pad / Left stick (down) |
| Hard drop | A |
| Rotate clockwise | Right bumper |
| Rotate counter-clockwise | Left bumper |
| Hold | Y |
| Pause | Menu (☰) |
| Menu navigation | D-pad / left stick |
| Menu confirm | A |
| Menu back | B |

---

## PlayStation controller (DualSense / DualShock 4)

| Action | Input |
|---|---|
| Move | D-pad / Left stick |
| Soft drop | D-pad / Left stick (down) |
| Hard drop | Cross (✕) |
| Rotate clockwise | R1 |
| Rotate counter-clockwise | L1 |
| Hold | Triangle (△) |
| Pause | Options |
| Menu navigation | D-pad / left stick |
| Menu confirm | Cross (✕) |
| Menu back | Circle (○) |

The layout (Xbox vs. PlayStation vs. a generic fallback) is auto-detected
from the connected controller — no setting to pick it manually.

---

## DualSense extras

Built on the vendored [`Ohjurot/DualSense-Windows`](https://github.com/Ohjurot/DualSense-Windows)
library, since neither Windows nor XInput expose the extended DualSense
features publicly:

- **Rumble** — two-motor low/high haptics, with a per-event preset (piece
  landed, hard drop, wall hit, row cleared, Tetris, level up, hold, T-spin,
  back-to-back, Perfect Clear, game over, Speed Surge, garbage row)
- **Lightbar** — a resting color tied to context (menu screen, board
  escalation tier, ...) plus short pulse/flash overrides on big moments
- **Adaptive triggers** — right-trigger recoil and sustained-resistance
  presets exist in `GamepadHaptics` and are wired end to end, but nothing in
  the current build drives them yet; a candidate for a future version

Xbox controllers get rumble only — the lightbar and adaptive-trigger calls
are no-ops on that layout. Both **Gamepad vibration** and **Gamepad
lightbar** can be turned off independently in Options → Gameplay.
