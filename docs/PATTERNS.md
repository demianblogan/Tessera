# Design patterns

How this codebase maps onto the common game-programming patterns — what each
one solves, how it's built here, and what it costs. Reference frame: Robert
Nystrom's *Game Programming Patterns*.

Some of these were designed in deliberately (the state machine, the
event-batch reaction split, the data-driven piece/kick tables); others
emerged naturally out of solving a concrete bug (the pending-transition
queue, the dirty-flag localization refresh). Both are called out.

- [Program-shaping patterns](#program-shaping-patterns)
- [Screens and flow](#screens-and-flow)
- [Simulation and reaction](#simulation-and-reaction)
- [Input](#input)
- [Data and resources](#data-and-resources)
- [Deliberately not used](#deliberately-not-used)
- [Summary](#summary)

---

## Program-shaping patterns

### Game Loop

**Problem.** A game can't block waiting for input like a console program. It
has to run continuously — read input, advance the world, draw — at a rate
independent of how fast the machine is.

**Here.** [`Application::Run`](../src/app/Application.cpp):

```cpp
sf::Clock clock;
while (window.isOpen())
{
    const float frameSeconds = clock.restart().asSeconds();
    const float deltaTime = std::min(frameSeconds, MaxFrameTime);   // clamp

    HandleInput();
    Update(deltaTime);
    Render();
}
```

Every piece of movement and animation is multiplied by `deltaTime`, so a
piece falls at the same real-world speed at 30 and at 240 FPS.
`std::min(frameSeconds, MaxFrameTime)` is the safety valve: if the game hangs
for a second (alt-tab, a Windows dialog), an unclamped `deltaTime` of 1.0
would drop a falling piece a full second's worth of rows in a single step.
The clamp caps the simulated step so a stall slows the game down instead of
tunnelling state.

### Service Locator

**Problem.** Nearly every screen needs the window, the asset caches, audio,
settings, the state machine itself — passing each one through every
constructor individually turns every signature into a wall of parameters,
and adding a new shared service means touching every call site.

**Here.** [`Context`](../src/core/Context.h) is one struct of references,
built once in `Application` and handed to every state and screen by
reference:

```cpp
struct Context
{
    StateMachine& stateMachine;
    sf::RenderWindow& window;
    Display::DisplayManager& display;
    FontManager& fonts;
    // ... every other shared service, by reference
};
```

A `State` stores its `Context&` and reaches into it for whatever it needs —
`context.audioPlayer.Play(...)`, `context.settings.GetSettings()`. Nothing
is a global; everything is still owned by `Application` and threaded through
explicitly, so ownership and lifetime stay obvious, but a state never has to
declare "I also need the display manager" in its constructor signature just
because one deeply nested call needs it.

**Cost.** A `Context&` member makes any class holding one look like it could
touch *anything*, which is a real discoverability cost versus tightly scoped
dependencies. In practice most classes only ever touch a handful of its
fields, and the alternative — threading nine separate references through
every constructor in `src/ui/panels/` — was worse.

---

## Screens and flow

### State (as a stack)

**Problem.** The game has a genuine stack of "what's on screen" — pause sits
*on top of* gameplay, not instead of it, so resuming it doesn't need to
rebuild the board; the loading screen fully replaces itself with the splash
once done. A raw `if (currentScreen == X)` ladder in `Application` can't
express "on top of" without becoming its own ad-hoc stack.

**Here.** [`StateMachine`](../src/core/StateMachine.h) owns a
`std::vector<std::unique_ptr<State>>`. Every concrete state — `LoadingState`,
`CompanySplashState`, `MenuShellState`, `GameplayState`, `PauseState`,
`GameOverState` — derives from `State` and overrides `HandleEvent` /
`Update` / `Render`. `ScreenHost` (`MenuShellState`, `PauseState`) is itself
one `State` that hosts a *second*, lighter-weight hierarchy underneath it —
`MenuScreen` — for the menu/options/pause content that doesn't need its own
place on the state stack (see [Screen-within-a-state](#screen-within-a-state)
below).

**The pending-transition queue.** A state asking to pop itself from inside
its own `HandleEvent` — pause's "Resume" button, say — can't have the stack
destroy it synchronously; the call would be executing on a freed object the
moment it returned. `StateMachine` queues every push/pop/clear/change as a
`PendingTransition` and only actually applies them from
`Application::Run`, between the input and update phases:

```cpp
void StateMachine::PushState(std::unique_ptr<State> state)
{
    pendingTransitions.push_back({ TransitionType::Push, std::move(state) });
}
// ... ApplyPendingChanges() runs the queue for real, once per frame,
// from a point in the loop where nothing is mid-call on the old state.
```

This is the one piece of "we hit a real bug, so this exists" plumbing in the
list — an early version applied transitions immediately and crashed the
first time a state tried to pop itself.

### Screen-within-a-state

**Problem.** The main menu, its Options sub-screen, Credits and Records all
share a header that morphs between a menu entry and a screen title, a
persistent background, and the gamepad lightbar handoff — but none of that
belongs on the `StateMachine`'s stack, because none of it should survive a
screen swap independently or be poppable on its own.

**Here.** [`ScreenHost`](../src/states/ScreenHost.h) is a `State` that owns
exactly one `MenuScreen` at a time, plus the header and background that
outlive a screen swap. `BeginForward`/`BeginBack` animate one `MenuScreen`
into the next (the activated entry becomes the header, which then becomes
the new screen's title) without the `StateMachine` ever seeing a
push/pop — from its point of view, `MenuShellState` is one state, the whole
time. It's a small state machine of its own (`Phase::Steady/Forward/Back`),
scoped one level below the real one.

---

## Simulation and reaction

### Update Method

**Problem.** Every active object — the falling piece, the row-clear effects,
particles, the escalation clock — needs to advance itself once a frame, and
that update logic has to live *with* the object, not in one giant switch in
the state.

**Here.** `Update(float deltaTime)` is the one interface every simulated or
animated object implements: `GameplaySession::Update`,
`EffectsController::Update`, `EscalationDirector::Update`, every `UI::`
widget. `GameplayState::Update` is mostly just calling `Update` down its own
collaborators in order, then reacting to what changed.

### Event Queue (batch, not per-callback)

**Problem.** `GameplaySession` is pure rules — it should never know that
`EffectsController` exists, let alone call into it directly to trigger a
screen shake. But `GameplayState` needs to know precisely what happened
during that `Update()` call to react correctly (a Tetris needs a different
shake than a Single; a T-spin fires a burst *and* may or may not have
cleared anything).

**Here.** Instead of firing callbacks mid-simulation, `GameplaySession`
accumulates a plain data batch —
[`GameplaySession::Events`](../src/gameplay/GameplaySession.h) — across the
frame (`hasLanded`, `detectedRows`, `hasClearedRows`, `isTSpin`,
`hasSpeedSurgeStarted`, ...) and hands the whole thing back once:

```cpp
session.Update(deltaTime);
boardRenderer.Update(deltaTime, session);
ReactToEvents(session.ConsumeEvents());
```

`ReactToEvents` reads the batch and decides sound, haptics, HUD updates and
callouts — all in one place, with the full context of everything that
happened this frame, rather than five separate callbacks each seeing one
fact in isolation. `ConsumeEvents()` clears the batch on the way out, so it
can never be read twice or leak into the next frame.

### Command-ish input, not a bare key check

**Problem.** `if (Keyboard::isKeyPressed(Keyboard::Left))` scattered through
gameplay code can't be rebound, can't distinguish "pressed this frame" from
"held," and treats keyboard and gamepad as two separate code paths.

**Here.** [`ActionMap<TAction>`](../src/input/ActionMap.h) maps a
project-defined enum (`GameplayAction::MoveLeft`, `...::HardDrop`, ...) to a
binding and a trigger type (`OnPress` / `WhileHeld`); `InputHandler<TAction>`
turns raw SFML events into calls to subscribed handlers for that action.
`GameplayInputController` and the menu input path both go through this
layer instead of touching `sf::Keyboard` directly, which is what makes
Options → Controls rebinding possible at all — the binding is data, not a
hardcoded comparison.

---

## Input

### Dirty Flag (localization revision)

**Problem.** Every screen caches its own localized strings as `sf::Text`
objects (re-fetching from the catalog and re-laying-out text every frame
would be wasteful) — but a language switch in Options has to refresh *all*
of them, including screens that aren't currently visible and panels that
haven't been rebuilt yet.

**Here.** `LocalizationManager` keeps an incrementing revision counter,
bumped on every `SetLanguage`. Anything with cached text keeps its own
`seenLocalizationRevision` and compares it once a frame:

```cpp
if (seenLocalizationRevision != context.localization.GetRevision())
{
    seenLocalizationRevision = context.localization.GetRevision();
    HUD.RefreshText();
}
```

Cheap when nothing changed (one integer compare), and correct regardless of
which screen was active when the language actually changed, since every
holder checks independently the next time it updates.

---

## Data and resources

### Type Object / data-driven content

**Problem.** Piece shapes and SRS wall-kick offsets are the kind of numbers
that benefit from being tweakable without a rebuild — and hardcoding them
directly into `Tetromino`'s rotation logic would make "what does an S-piece
look like" and "how does an S-piece rotate" the same piece of code.

**Here.** `PieceDataFile` / `KickDataFile` load
`assets/data/pieces.json` / `assets/data/srs_kicks.json` at startup into
plain data (`PieceData`, `KickData`), with the standard guideline shapes and
kick tables compiled in as a fallback if the file is missing or fails to
parse. `Tetromino` and the rotation code consume that data; neither has the
numbers baked in. The same shape applies to `HapticSettings` (per-event
rumble/lightbar tuning) and `AudioBalance` (per-track volume) — small,
frequently-retuned data kept external, while the level curve, scoring table
and escalation timings stay in code as named constants, since those change
rarely and reviewing a code change for them is fine.

### Facade (asset access)

**Problem.** Loading, caching and looking up textures/fonts/sounds/shaders
by ID shouldn't leak `sf::Texture` construction details or file paths into
every screen that just wants "the block spritesheet."

**Here.** `ResourceManager<TId, TResource>` is one small generic cache
(`FontManager`, `TextureManager`, `SoundBufferManager`, `MusicManager` are
instantiations of it), and `Assets::TextureID` / `Assets::SoundID` / ... are
the enums a caller actually asks for (`context.textures.Get(Assets::TextureID::BlockSpritesheetWithOutline)`).
File paths and load order live in `Assets.h` / `AssetLoadJob`, not scattered
across every screen that needs a texture.

---

## Deliberately not used

**ECS.** One dynamic piece plus a 10×20 grid of cells is not enough moving
parts to earn a full entity-component-system; `Board`, `Tetromino` and a
handful of `Cell`s cover it more directly than a general-purpose component
store would.

**Singleton.** Every shared service (`SettingsManager`, `HighScoreManager`,
`AudioPlayer`, ...) is a normal object owned by `Application` and reached
through `Context`, not a static/global instance. It costs one extra
indirection (`context.settings` instead of `Settings::Instance()`) in
exchange for explicit ownership, no static-init-order surprises, and a
`Context` that can be swapped for a test double if a test ever needed to.

**Object Pool for particles.** Row-clear shards, dust and sparks are plain
`std::vector`s that grow and shrink normally. At this scale (bursts of tens
to low hundreds of short-lived particles) the allocator churn has never
shown up as a real cost worth the complexity of a pool; revisit if a
profiler ever says otherwise.

---

## Summary

| Pattern | Where | Why |
|---|---|---|
| Game Loop | `Application::Run` | Frame-rate-independent, stall-safe timestep |
| Service Locator | `Context` | One reference bundle instead of N constructor params |
| State (stack) | `StateMachine` | Pause-over-gameplay, loading → splash → menu flow |
| Pending-transition queue | `StateMachine` | A state can safely pop/push itself mid-callback |
| Screen-within-a-state | `ScreenHost` / `MenuScreen` | Shared header/background without polluting the real stack |
| Update Method | every `Update(deltaTime)` | Per-object logic stays with the object |
| Event Queue (batch) | `GameplaySession::Events` | Decouples pure rules from sound/haptics/HUD reactions |
| Command-ish input | `ActionMap` / `InputHandler` | Rebindable actions instead of hardcoded key checks |
| Dirty Flag | `seenLocalizationRevision` | Cheap per-frame check for "does my cached text need refreshing" |
| Type Object / data-driven | `PieceDataFile`, `KickDataFile`, `HapticSettings` | Tunable content without a rebuild, safe fallback if the file is bad |
| Facade | `ResourceManager<TId, TResource>` | Asset lookup by ID, load details hidden from callers |
