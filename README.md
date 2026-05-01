# Escape From The Main Building (COMP2113/1340 Group Project)

**Course:** HKU COMP2113 / COMP1340  
**Team:** Group 24  
**Repository:** `gwdsd1/COMP2113GP_group24`  
**Link to the demo video:** [Project Demo Video](https://youtu.be/xNYkGMGFsL0)

---
## Table of Contents
- [Team Members](#team-members)
- [Game Description](#game-description)
- [Compilation and Execution Instructions (Quick Start)](#compilation-and-execution-instructions-quick-start)
  - [Manual compilation (g++17)](#manual-compilation-g17)
- [Controls & Key Bindings (Detailed)](#controls--key-bindings-detailed)
  - [1) Main Menu](#1-main-menu)
  - [2) Maze (Core Gameplay)](#2-maze-core-gameplay)
  - [3) Shop System (Coins → Items)](#3-shop-system-coins--items)
  - [4) Wall Breaker Tool (Detailed)](#4-wall-breaker-tool-detailed)
- [Mini-Games / Encounters](#mini-games--encounters)
  - [A) Shooter Mini-game (Entrance `!`)](#a-shooter-mini-game-entrance-)
  - [B) Snake Mini-game (Entrance `S`) — Multiple Difficulty Levels Required Feature](#b-snake-mini-game-entrance-s--multiple-difficulty-levels-required-feature)
  - [C) Music / Rhythm Mini-game (Entrance `&`)](#c-music--rhythm-mini-game-entrance-)
  - [D) Enemy Quiz Encounter (Enemy `G`)](#d-enemy-quiz-encounter-enemy-g)
- [Save / Load System](#save--load-system)
  - [Save](#save)
  - [Load](#load)
- [Implemented Features and COMP2113 Coding Requirements Mapping](#implemented-features-and-comp2113-coding-requirements-mapping)
  - [(1) Generation of Random Events](#1-generation-of-random-events)
  - [(2) Data Structures for Storing Data](#2-data-structures-for-storing-data)
  - [(3) Dynamic Memory Management](#3-dynamic-memory-management)
  - [(4) File Input/Output](#4-file-inputoutput)
  - [(5) Program Codes in Multiple Files](#5-program-codes-in-multiple-files)
- [Non-standard / Third-party Libraries](#non-standard--third-party-libraries)
  - [miniaudio (`miniaudio.h`)](#miniaudio-miniaudioh)
- [Folder Structure (Important)](#folder-structure-important)
- [License](#license)

---

## Team Members

| Name           | UID        | GitHub Username |
|----------------|------------|-----------------|
| WANG Zhengwei  | 3036483777 | gwdsd1          |
| SU Yihan       | 3036482670 | fdhyng          |
| ZHENG Jianbin  | 3036483284 | jbin17          |
| LUO Zihan      | 3036482589 | TTNKBDR0426     |

---

## Game Description
## With a blank mind, find your way out from HKU main building......
**Escape From The Main Building** is a console-based adventure game set in HKU’s Main Building.  
You wake up with no memory and must navigate a large maze, survive enemy encounters, and clear multiple mini-games. Winning mini-games earns **coins**, which can be spent in the **in-maze shop** to purchase healing or special tools. Your ultimate goal is to reach the **exit zone** of the maze and escape.


The game includes:
- A **main menu** (New Game / Load Game / Quit)
- A **maze exploration** core loop (player movement, enemies, events)
- Multiple **mini-games** integrated into maze progression:
  - **Rhythm/Music mini-game**
  - **Shooter mini-game**
  - **Snake mini-game** (with difficulty levels)
  - **Enemy Quiz encounter**
- A **Shop system** (coins → heal / wall-breaking tool)
- **Save/Load** (persist and resume maze state)
- Background music powered by a third-party audio library

---

## Compilation and Execution Instructions (Quick Start)

1. Download this repository as a ZIP file from GitHub, then extract it to your Desktop (or any working directory).

2. Open a terminal and navigate to the extracted folder, then enter the source directory:
   ```bash
   cd COMP2113GP_group24/gpteam24
   ```

3. Compile the game (requires **g++** with **C++17** support):
   ```bash
   make
   ```

4. Run the game:
   ```bash
   ./gpteam24_game
   ```

### Manual compilation (g++17)
From the `gpteam24/` directory:
```bash
g++ -std=c++17 -O2 -Wall -I. \
  main.cpp game.cpp maze.cpp miniaudio_imple.cpp music_core.cpp music_game.cpp MusicManager.cpp shooter_game.cpp snake_game.cpp enemy_quiz.cpp \
  -o gpteam24_game -pthread -ldl -lm
./gpteam24_game
```

**Requirements**
- C++17 compiler (uses e.g. `std::filesystem`)
- Terminal that supports ANSI escape sequences (Linux/macOS terminals work well)

---

## Controls & Key Bindings (Detailed)

### 1) Main Menu
- Input: type `1 / 2 / 3` then press **Enter**
  - `1` New Game
  - `2` Load Game
  - `3` Quit

---

### 2) Maze (Core Gameplay)
**Movement**
- `W` Up
- `A` Left
- `S` Down
- `D` Right

**Maze Actions**
- `E` Interact (only appears when you are near a Music-Note entrance; a hint will show on screen)
- `P` Open Shop
- `K` Exit Shop (inside shop)
- `B` Use Wall Breaker tool (only if you have at least 1 Wall Breaker)
- `Q` Quit maze **and auto-save** (see Save System below)

**Map Symbols (as rendered in maze)**
- `@` Player
- `G` Enemy (triggers an Enemy Quiz encounter when adjacent)
- `&` Music mini-game entrance (press `E` when near)
- `!` Shooter mini-game entrance (auto-enter when near)
- `S` Snake mini-game entrance (auto-enter when near)
- `#` Wall (blocked)
- `.` / space: walkable floor (visual may vary by screen refresh)

**Exit Condition**
- Reach the **exit area** at the bottom of the maze (row near the bottom; columns roughly centered).  
Upon reaching it, the victory screen is shown and you return to the menu.

**Health / HP**
- HP is shown under the maze as a bar and numeric value (max HP = 15).
- Failing some encounters reduces HP.
- If HP reaches 0 → Game Over screen.

---

### 3) Shop System (Coins → Items)
The shop is opened **inside the maze** by pressing:
- `P` to open shop
- `K` to exit shop

**What coins are used for**
Coins are a reward currency earned by clearing/doing well in encounters. Coins can be spent on:

1. **Heal Potion (Restore HP)**
   - Cost: **1 coin**
   - Effect: **+1 HP** (cannot exceed 15)

2. **Wall Breaker (Break a wall tile)**
   - Cost: **2 coins**
   - Effect: Gain **1 Wall Breaker charge**
   - Usage: press `B` in the maze (details below)

**How to earn coins (reward rules)**
Coins are awarded when you **pass** certain mini-games/encounters:
- Passing **Shooter mini-game** → **+1 coin**
- Passing **Snake mini-game** → **+1 coin**
- Passing **Music mini-game** → **+1 coin**
- Passing **Enemy Quiz** → **+1 coin**

> If you fail these challenges, you typically lose HP (commonly -3 HP), so coins are both a reward and a progression tool.

---

### 4) Wall Breaker Tool (Detailed)
If you have Wall Breakers, the maze UI will suggest you can press `B`.

**How to use**
1. Press `B` in the maze to start wall-breaking mode.
2. Choose a direction key:
   - `W/A/S/D` to select which adjacent tile to break
3. If the target is a valid wall tile (not border walls), it will be removed and becomes walkable.
4. Your Wall Breaker count decreases by 1.

**Cancel**
- Press any key that is NOT `W/A/S/D` to cancel.

**Restrictions**
- Border walls cannot be broken.
- Only `#` wall tiles can be broken.

---

## Mini-Games / Encounters

### A) Shooter Mini-game (Entrance `!`)
- Trigger: When you move close to a Shooter entrance marker (`!`) in the maze, the game **auto-enters** the Shooter mini-game.
- Reward:
  - **Pass** → **+1 coin**
  - **Fail** → **HP -3**
- After the mini-game ends, the shooter entrance is re-randomized to a new location.
- Press `A` or `D` to move with laser lights automatically shooting forward


---

### B) Snake Mini-game (Entrance `S`) — Multiple Difficulty Levels Required Feature
- Trigger: When you move close to a Snake entrance marker (`S`), the game **auto-enters** Snake.
- Difficulty selection at start:
  - Press `1` Easy (1.0× speed)
  - Press `2` Medium (1.5× speed)
  - Press `3` Hard (2.0× speed)

**Snake Controls**
- `W/A/S/D` Move
- `Q` Quit snake game early

**Snake Rules (overview)**
- Eat food `*` to gain score (+10 each).
- Avoid walls and your own body.
- Additional random items may appear:
  - Poison `X` (score penalty / body shrink effect)
  - Speed `^` (temporary speed boost)

**Reward**
- **Pass** (reach target score) → **+1 coin**
- **Fail** → **HP -3**

---

### C) Music / Rhythm Mini-game (Entrance `&`)   
**Notice: You have to play on Windows or MacOS instead of Linux if you want to hear music due to the sound card issue.**
- Trigger: When you are near a music-note entrance (`&`), a hint appears. Press:
  - `E` to enter the music game
  - `S``D``F``J``K``L` to strike the corresponding notes
  - `Q` to quit game early
- Reward:
  - **Pass** → **+1 coin**
  - **Fail** → **HP -3**

**stage list：**

| stage | singer | name |
|:----:|:----:|:----:|
| 1 | Yorushika | Paddle |
| 2 | ZUTOMAYO | Justice |
| 3 | r-906 | manimani |
| 4 | n-buna | because summer will end |

The rhythm system loads charts from text files in `gpteam24/charts/` (see “File I/O” section).


---

### D) Enemy Quiz Encounter (Enemy `G`)
- Trigger: If you move adjacent to an enemy (`G`), the quiz starts.
- The quiz randomly selects a subset of questions from a question bank each run.

**Reward**
- **Pass** → **+1 coin**
- **Fail** → **HP -3**
- After the encounter, the enemy is relocated to a new position.

---

## Save / Load System
### Save
In the maze:
- Press `Q` to quit the maze, and the game will **automatically save** the full maze state into a timestamped file:
  - Format: `save_YYYY_MM_DD_HH_MM_SS.txt`

Saved state includes (examples):
- Player position, HP, coins, wall breakers
- Positions of entrances/enemies
- Broken wall history (so walls you broke remain broken after loading)

### Load
From main menu:
- Choose `2` Load Game
- Select one of the detected save files

---

## Implemented Features and COMP2113 Coding Requirements Mapping

### (1) Generation of Random Events
- Randomized placement of mini-game entrances in the maze.
- Enemy positions and patrol zones randomized.
- Quiz question selection randomized (`shuffle` with RNG).
- Snake item placement randomized.

### (2) Data Structures for Storing Data
- `MazeState` struct stores player/enemy/entrance positions, HP, coins, tool counts, and broken wall coordinates.
- Uses `std::vector`, `std::deque`, `std::string`, and custom structs (e.g., chart data).

### (3) Dynamic Memory Management
- Dynamic containers (`std::vector`, `std::deque`, `std::string`) are used heavily and grow/shrink at runtime:
  - Snake body grows dynamically.
  - Quiz bank and chart notes are dynamically stored.
  - Broken wall records are stored in a vector.

### (4) File Input/Output
- Save/Load: persists and restores full maze state via text save files.
- Rhythm charts: loads note timing and metadata from `.chart` files.
- Directory scanning for save files uses `std::filesystem`.

### (5) Program Codes in Multiple Files
The program is modularized across multiple source files:
- `main.cpp`, `game.cpp/.h`, `maze.cpp/.h`
- `snake_game.cpp/.h`, `shooter_game.cpp/.h`, `enemy_quiz.cpp/.h`
- `music_core.cpp`, `music_game.cpp/.h`, `ChartLoader.h`
- `MusicManager.*`, `MusicPlayer.h`, `miniaudio_imple.cpp`

---

## Non-standard / Third-party Libraries
### miniaudio (`miniaudio.h`)
A single-header audio library integrated into the project to support:
- Background music for menu/maze/mini-games
- Loading and playing `.mp3` files with looping

---

## Folder Structure (Important)
- `gpteam24/` main source folder
  - `Makefile`
  - `*.cpp / *.h`
  - `music/` audio assets
  - `charts/` rhythm chart files

---

## License
For HKU COMP2113/1340 course project use.
