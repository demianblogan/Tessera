# 🧩 Tessera (C++ / SFML)

A styled falling-block puzzle game built with **C++23** and **SFML 3.1.0**.

Featuring a custom UI system, leaderboard support, configurable settings, shader-based post-processing effects, and polished gameplay feedback with progressive difficulty.

---

## 🎮 Menu & Gameplay

![Preview](https://github.com/user-attachments/assets/6ce80c68-cfcb-40a9-bb6d-c5d7045affd1)

---
![Preview](https://github.com/user-attachments/assets/7ce3882c-3c77-49cd-8658-c5f6108ecc03)
---


## 📦 Download & Play

👉 [Download Latest Release](../../releases)

Quick start:

1. Download `.zip` from Releases
2. Extract it
3. Run `Tessera.exe`

---

## 🕹 Controls

| Action            | Input |
|-------------------|-------|
| Move Left         | ←     |
| Move Right        | →     |
| Soft Drop         | ↓     |
| Rotate Tetromino  | ↑     |
| Hard Drop         | Space |
| Pause             | ESC   |

---

## ✨ Features

- Classic falling-block gameplay
- Progressive difficulty system
- Score and level system
- Next tetromino preview
- Ghost tetromino
- Screen shake effects
- Row clear animations
- Landing effects
- Shader-based glow effects
- CRT post-processing shader
- Blur effects
- Full keyboard navigation
- Configurable settings system
- High score saving system
- Custom UI framework

---

## 🎨 Visual Effects

The game includes multiple real-time shader and gameplay effects:

- Pulsating glow shader
- Ghost tetromino transparency effect
- CRT monitor simulation
- Chromatic aberration
- Scanlines and vignette
- Blur post-processing
- Animated UI glow
- Dynamic screen shake

---

## ⚙️ Settings

The game supports configurable settings:

- Music volume
- Sound effects volume
- Block rendering style
- Fullscreen mode

Settings are automatically saved and loaded from file.

---

## 🧠 Technical Features

This project was built with focus on architecture and code organization.

Includes:

- State machine system
- Custom UI framework
- Resource manager system
- Settings serialization
- Audio management system
- Shader manager
- Layout system
- Event-driven input handling
- Render pipeline with post-processing

---

## ⚙️ Requirements

- C++23 compatible compiler
- Visual Studio 2026
- SFML 3.1 (64-bit)

📦 Setup guide:  
👉 [SFML Setup](libs/SFML/README.md)

---

## ▶️ Run

1. Open `Tessera.slnx`
2. Build the project (Debug or Release)
3. Make sure SFML DLLs are available next to the executable
   > DLLs can be found in the SFML `bin` folder
4. Run the game

---

## 📁 Project Structure

```text
src/        → game source code
assets/     → textures, shaders, sounds, fonts
data/       → settings and save data
libs/       → external libraries (SFML)
