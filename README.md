# Rope Pulling Game Simulation - Full Algorithm & Project Overview

## Overview
This project simulates a "rope pulling game" using multiple processes in a Linux environment. The goal is to simulate two teams (Team 1 and Team 2), each consisting of 4 players. The simulation uses processes, signals, pipes/FIFOs, and OpenGL to visualize the game. A referee (parent process) coordinates the game.

## Process Structure
```
Referee (Parent Process)
├── Player 0 (Team 1)
├── Player 1 (Team 1)
├── Player 2 (Team 1)
├── Player 3 (Team 1)
├── Player 4 (Team 2)
├── Player 5 (Team 2)
├── Player 6 (Team 2)
└── Player 7 (Team 2)
```

## Project File Structure
```
rope-pulling-sim/
├── referee.c       // Game controller logic
├── player.c        // Logic for individual players
├── comm.c          // Pipes/FIFOs setup and communication
├── config.c        // Reads settings from config.txt
├── gui.c           // OpenGL visualization
├── config.txt      // User-defined game settings
├── main.c          // Integration of all modules
├── Makefile        // Compilation script
└── Dockerfile      // Development environment (optional)
```

## Algorithm – Step-by-Step Explanation

### 🔹 1. Initialization (Referee)
- Read all settings from `config.txt`: energy range, decrease rate, win threshold, etc.
- Create pipes or FIFOs for communication with players.
- Use `fork()` to create 8 player processes (4 for Team 1 and 4 for Team 2).
- Save each player's PID in an array for communication.

### 🔹 2. Game Loop (Per Round)
1. Referee sends signal `SIGUSR1` to all players → tells them to get ready.
2. Players sort themselves (internally) based on their current energy. Their position affects the weight of their energy in the calculation (center = ×1, farthest = ×4).
3. Referee sends `SIGUSR2` → tells all players to start pulling.
4. Every second:
   - Each player:
     - Decreases their energy using a random value from a defined range.
     - Multiplies energy by their position weight.
     - Sends the value back to the referee via pipe/FIFO.
   - Referee reads from all 8 players.
   - Referee calculates total team effort:
     - Team 1 = sum of adjusted efforts of players 0–3
     - Team 2 = sum of adjusted efforts of players 4–7
   - If the absolute difference exceeds the threshold, the stronger team wins the round.

### 🔹 3. Additional Game Rules
- At any second, a player might “fall” and their effort becomes 0 temporarily.
- After a random time, the fallen player re-joins.
- After each round:
  - Referee sends message to all players with win/loss result.
  - Referee updates score and checks for these end conditions:
    - Game time is over
    - A team reaches a defined score
    - A team wins 2 rounds consecutively

### 🔹 4. End of Simulation
- Once an end condition is met, the referee terminates all player processes.
- Optionally, display final score via OpenGL or in the terminal.

## Example config.txt
```
initial_energy_min=80
initial_energy_max=100
effort_decrease_min=2
effort_decrease_max=5
recovery_time_min=3
recovery_time_max=7
win_threshold=500
game_duration=60
rounds_to_win=2
```

## How to Build & Run
### Compile:
```bash
gcc referee.c player.c comm.c config.c gui.c -o rope_game -lGL -lGLU -lglut
```

### Run:
```bash
./rope_game config.txt
```

## Team Task Breakdown

### Hala (Referee Developer)
**File(s):** `referee.c`

**Responsibilities:**
1. Read configuration from `config.txt` (e.g., energy ranges, thresholds, game time).
2. Create 8 player processes using `fork()`.
3. Store all PIDs in an array.
4. Send signals to players:
   - `SIGUSR1` → “Get Ready”
   - `SIGUSR2` → “Start Pulling”
5. Every second:
   - Collect energy values from all 8 players via pipe or FIFO.
   - Multiply each player’s energy by their position weight (1–4).
   - Calculate total effort for each team.
   - Determine if one team passes the threshold and wins the round.
6. Track number of rounds, scores, and winning streaks.
7. Notify all players about win/loss.
8. Check end conditions and terminate the game if reached.

---

### Obayda (Player Logic Developer)
**File(s):** `player.c`

**Responsibilities:**
1. Wait for signals from referee (`SIGUSR1`, `SIGUSR2`).
2. When signal `SIGUSR2` is received:
   - Start pulling: decrease energy each second using random value from range.
   - Multiply energy by position weight.
   - Send updated energy value to referee.
3. Handle “fall” condition:
   - Randomly fall (set energy to 0).
   - Wait a random recovery time, then rejoin.
4. Receive message from referee after each round (win/loss).

---

### Jenan (Communication Developer)
**File(s):** `comm.c`

**Responsibilities:**
1. Create and initialize communication channels:
   - Use `pipe()` or `mkfifo()`
   - Set one per player or per team.
2. Handle `read()` and `write()` functions:
   - Players send their energy to referee.
   - Referee reads and stores them.
3. Ensure proper closing of pipes when game ends.
4. Help ensure synchronization in message passing.

---

### Raghad (GUI & Config Developer)
**File(s):** `gui.c`, `config.c`, `config.txt`

**Responsibilities:**
1. Create `config.txt` file with all user-defined values (energy ranges, duration, thresholds).
2. Write `config.c` to read config file and initialize global variables.
3. Develop simple OpenGL interface (`gui.c`):
   - Display two teams and a rope.
   - Show direction of pull and who’s winning.
   - Display round results and final outcome.
4. Optionally add animation or symbolic visualization of effort.

---
