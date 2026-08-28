# ULA → Tessera port analysis: UI & Input

Working document (not yet a committed decision). Compares the UI and input
systems of *Until Last Asteroid* (ULA) with the current Tessera code and
proposes what to port, what to adapt, and what to improve along the way.

---

## 1. UI

### 1.1 Two different philosophies

| | **Tessera** | **ULA** |
|---|---|---|
| Model | Retained-mode **layout engine**: `Element` tree with a `Measure()` → `Arrange()` pass, `Layout` does flexbox-style stacking (orientation, gap, padding, alignment, `SizeMode` Auto/Pixels/Percent/Fill) | Concrete widget classes that own their sprites/text and draw themselves; screens position them with explicit coordinates + tuning constants |
| Widgets | `Label`, `Button`, `Panel`, `Slider`, `Spacer` | `MenuButton`, `NineSliceFrame`, `RoundedRectangleShape`, `HUD`, `ResultScreen`, `GameOverScreen`, … |
| Selection / navigation | **Re-implemented in every state** (`selectedIndex`, `SelectPrevious/Next`, `UpdateSelection`, `ActivateSelectedButton`) — copy-pasted ×4 | `MenuButtonList` — one controller: wrap-around prev/next that skips disabled buttons, mouse hover-select, plays the select sound + invalidates the glow **only when the selection actually changes** |
| Mouse | `Button`/`Slider` have `Contains()` but **nothing routes mouse events** — keyboard only | `MenuButtonList::UpdateMouseSelection`, click-to-activate |
| Scaling | `Button`/`Panel` stretch **one whole sprite** (`setScale(size / bounds)`) → corners distort | `NineSliceFrame` + `MenuButton`'s 3-slice caps — corners stay crisp at any size |
| Text fitting | `Label` measures raw `getLocalBounds()`; no fit-to-width | `TextLayout::FitWidth` scales the text down (never steps `characterSize` in a loop — that rebuilds the glyph atlas per size and is *tens of ms each* on this SFML/driver combo) |
| Transitions | none | `ScreenFade` (fade in/out overlay) |
| Cursor | hidden entirely (`setMouseCursorVisible(false)`), no replacement | `GlowingCursor` (sprite + `NeonGlow`) |
| Localization | none | `LocalizationRevision` triggers a lazy re-layout only when the language changed |
| Menu polish | none | `MenuIntroAnimation` (type title → ease up → type items → reveal frames), `MenuBackground` parallax |
| Options rows | `SettingsState` builds every row inline (601 lines) | `OptionsWidgets` — stateless "how to draw a slider/toggle/dropdown row" helpers; `OptionsState` decides *what* |
| Glow | fake (scaled sprite + additive) | `NeonGlow` real bloom, invalidated on selection change |
| State lifecycle | rebuilt from scratch every visit | `State::OnReactivated()` + optional caching for expensive screens |

### 1.2 Recommendation: **hybrid, keep the layout engine**

Tessera's `Element`/`Layout` is genuinely **more capable** than ULA's manual
positioning — for a menu-heavy game, auto-centering / alignment / fill is an
asset. Replacing it with hardcoded coordinates would destroy working value and
contradict "don't rewrite what works".

So: **keep `Element` / `Layout`. Port the infrastructure ULA has and Tessera
lacks, and rebuild the widget *internals* on ULA's rendering techniques while
keeping their `Element` interface.**

Port / adapt:

1. **`MenuList`** selection controller — the `MenuButtonList` logic, but
   operating on `Element*` (or `Button*`) inside a `Layout`. Deletes the
   copy-pasted navigation from `MainMenuState` / `PauseState` / `GameOverState`
   / `SettingsState`.
2. **Mouse routing** — `State::HandleEvent` dispatches `MouseMoved` /
   `MouseButtonPressed` into the focused `Layout`; hover selects, click
   activates. (`Contains()` already exists on the widgets.)
3. **`NineSliceFrame`** — rebuild `Button` and `Panel` backgrounds on it so
   they stop distorting.
4. **`TextLayout`** (`CenterText`, `FitWidth`) — standardise `Label` on it;
   bake in the "never loop `characterSize`" rule now, before localization makes
   it bite.
5. **`ScreenFade`** — cross-state transitions.
6. **`GlowingCursor`** + enable the mouse (stop hiding the cursor with nothing
   behind it).
7. **`NeonGlow`** — real bloom for the selection glow, the active piece, the
   ghost and line-clear flashes (see the separate glow task).
8. **`LocalizationRevision`** + a `LocalizationManager` skeleton — re-layout on
   language change.
9. **`MenuIntroAnimation`** + **`MenuBackground`** parallax — main-menu polish.
10. **`OptionRow`** widget — collapse `SettingsState`'s inline row building.

### 1.3 Improvements noticed while reading (fold into the port)

- **`Button` conflates "selected" and "enabled".** ULA's `MenuButton` has
  `SetSelected` **and** `SetEnabled` with distinct visuals. Adopting this fixes
  the Game Over "Save" button directly — it currently abuses `SetSelected` as
  an enabled flag.
- **No mouse at all** in a mouse-friendly genre. Even hover-to-highlight +
  click-to-select is a large UX win for little code.
- **Sprite-stretch distortion** on every `Button`/`Panel` — nine-slice fixes it
  everywhere at once.
- **Select sound fires on every Up/Down**, including at the list ends after
  wrap. ULA only plays it (and invalidates the glow) when the index actually
  changed — cleaner and quieter.
- **`Label::Arrange` positions text via `bounds.position` offset** — fragile
  with multi-line strings (the gameplay controls panel). `TextLayout` handles
  origin properly.
- **`SettingsState` is 601 lines of inline row construction.** An `OptionRow`
  widget (label + control + value, one focusable unit) plus the `Element` tree
  cuts this hard and makes a Controls / Gameplay / Language section trivial to
  add.
- **States rebuilt on every visit.** Fine now; add `OnReactivated()` before the
  Options screen gets heavy with localized text.

---

## 2. Input

### 2.1 Current state

- **Tessera:** every state's `ProcessEvents` has a raw `switch` on
  `sf::Keyboard::Scancode`. No abstraction, no rebinding, no gamepad, no
  auto-repeat (movement relies on the OS key-repeat — wrong delay/rate for a
  falling-block game).
- **ULA:**
  - `InputBinding` = `variant<Key, MouseButton>` + `TriggerType`
    (`OnPress` / `OnRelease` / `WhileHeld`).
  - `ActionMap<Action>` = `Action` enum → list of bindings (just a table).
  - `InputHandler<Action>` = subscribe callbacks per action; `HandleEvent()`
    for discrete triggers, `Update()` for held ones; dispatches via
    `std::visit`.
  - `GamepadManager` = **deliberately separate**. Xbox vs PlayStation by USB
    vendor ID; dead zones; per-family stick-axis mapping; `NavigationAction`
    (Up/Down/Left/Right/Confirm/Back); `IsPausePressed`; `IsInUse`. **No
    rumble** (SFML 3.1's `sf::Joystick` is read-only).

### 2.2 Recommendation

**Port `InputBinding` / `ActionMap` / `InputHandler`, with three Tessera-specific
changes:**

1. **Add a gamepad button to the binding variant:**
   `variant<sf::Keyboard::Scancode, sf::Mouse::Button, GamepadButton>`.
   Also switch gameplay bindings from `sf::Keyboard::Key` to **`Scancode`**
   (physical-position, layout-independent — matches what Tessera already uses).
2. **DAS / ARR** — not in ULA, and it's the single biggest feel issue. A
   dedicated `DirectionalRepeater` in the gameplay input layer: per-action
   initial delay (DAS) + repeat rate (ARR, `0` = instant to wall), direction
   change resets the charge, soft-drop has its own rate. This does **not**
   belong in `InputHandler` — keep that generic.
3. **Slim `GamepadManager`** — port connection + Xbox/PS layout detection +
   button/d-pad → discrete `GamepadButton` + `NavigationAction` + `IsInUse`.
   Drop all the twin-stick / aim-direction code (irrelevant to Tessera).

**Tessera action set:** `MoveLeft`, `MoveRight`, `SoftDrop`, `HardDrop`,
`RotateCW`, `RotateCCW`, `Rotate180`, `Hold`, `Pause`, plus menu
`NavUp/Down/Left/Right`, `Confirm`, `Back`.

### 2.3 Rumble (new — ULA does not have this)

SFML 3.1 cannot drive rumble. Plan:

- **`GamepadRumble` over XInput** (`XInputSetState`, low-frequency + high-
  frequency motors) — covers Xbox and XInput-compatible controllers. This is
  the realistic v1.
- **DualSense / DualShock 4** rumble needs raw HID output reports — **stretch
  goal**, noted but not blocking.
- **Events → rumble:** line clear (intensity scaled by lines: single = light
  tap, Tessera = strong), hard drop, lock, level up, top-out, T-spin.
- Gated by a **Vibration** on/off + intensity slider in Options; auto-off when
  no gamepad or when `IsInUse()` is false.

---

## 3. Proposed Phase 1b sequencing (the port)

Supersedes the coarse `v1.0.3`–`v1.0.6` in `BACKLOG.md`:

| Version | Content |
|---|---|
| **v1.0.3** | Centralised window events (`State::HandleEvent(const sf::Event&)`, polling in `Game`); base `State` with `RequestPush/Pop/Clear`; `MenuList` selection controller + **mouse support**; delete the copy-pasted menu navigation; proper `Button` enabled state |
| **v1.0.4** | Persistence: `AppDataPath`, `SafeFileWrite`, settings format version + validation, single high-score source |
| **v1.0.5** | Input system: `InputBinding` / `ActionMap` / `InputHandler` (Scancode + gamepad variant); `DirectionalRepeater` (DAS/ARR); slim `GamepadManager` (nav + gameplay buttons); rebinding UI. Split `GameState` into `TesseraGame` + `GameplayState` + `BoardRenderer` + `EffectsController` |
| **v1.0.6** | Visual pass: `NeonGlow` real bloom; `NineSliceFrame` in `Button`/`Panel`; `TextLayout`; `ScreenFade`; `GlowingCursor`; per-effect toggles + screen-shake / reduce-motion; `/W4` → 0 + warnings-as-errors |
| **v1.0.7** | `GamepadRumble` (XInput) + Vibration settings; `MenuBackground` parallax + `MenuIntroAnimation`; company splash + `GameVersion.h` |
| **v1.0.8** | `LocalizationManager` skeleton (all strings extracted, English wired) + `LocalizationRevision` + font system; threaded loading screen; data-driven content skeleton (piece defs, scoring, gravity as JSON) |

Full 5-language localization stays a **v1.4.0** decision as agreed; v1.0.8 only
lays the pipe.

Phase 2 (`v1.1.0`+) — gameplay — unchanged from `BACKLOG.md`.
