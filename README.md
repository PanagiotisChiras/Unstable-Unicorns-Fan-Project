

# Unstable Unicorns Engine
 
A fully playable digital implementation of **Unstable Unicorns** built from scratch in **C++20**.  
Fan project — no affiliation with TeeTurtle or the original creators.
 
---
 
## What is this?
 
Unstable Unicorns is a card game where players race to fill their stable with unicorns while sabotaging everyone else's. This project is a console-based hot-seat implementation of that game, built as a personal engineering challenge.
 
---
 
## Current State
 
-  **Hot-seat multiplayer** — two players, one keyboard
-  **~100 card deck** — includes Basic Unicorns, 20+ Neigh cards, and all major card types
-  **50+ card effects** fully implemented
-  **Neigh / Super Neigh** system — players can counter cards and counter-counter them
-  **Full game loop** — Draw, Action, and End phases
-  **Some card effects** still in progress
---
 
## Architecture
 
The engine is built around a custom event system and effect pipeline:
 
- **`EventDispatcher`** — pub/sub event system with snapshot-before-iterate for nested dispatch safety
- **`EffectSystem`** + **`EffectRegistry`** — RAII `ListenerHandle`-based effect registration
- **`ChoiceManager`** — stack-based architecture for resolving nested player choices with async callbacks
- **`StableUtils`** — free functions for querying and manipulating player stables
- **`ActiveEffects`** — tracks ongoing effects with source-tracked restriction sets
- **`ConsoleUI`** — console rendering and input handling
 
---
 
## Build
 
**Requirements:**
- C++20 compiler (MSVC / GCC / Clang)
- CMake 3.20+
```bash
git clone https://github.com/PanagiotisChiras/Unstable-Unicorns-Fan-Project.git
cd Unstable-Unicorns-Fan-Project
cmake -B build
cmake --build build
./build/Unstable_Unicorns.exe
```
 
---
 
 
---
 
## Disclaimer
 
This is a **fan project** made for learning purposes. All card names, game mechanics, and related intellectual property belong to **Unstable Games**. No copyright infringement intended. This project is not monetized.
 
---
 
## Author
 
Built by a self-taught C++ programmer as a personal project.  
Feedback and PRs welcome.
