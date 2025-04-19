# Rope Pulling Simulation

A Linux-based multi-process game written in C, simulating a tug-of-war between two teams. Each player has energy, effort, and can fall or recover during the match. The referee controls everything using signals and pipes.

---

## Features

- Players lose energy and recover realistically
- Referee communicates with players using Linux signals (`SIGUSR1`, `SIGUSR2`, `SIGTERM`)
- Players and referee exchange data through pipes
- Each round is printed with detailed player stats
- Optional OpenGL visualization (`visual.c`) available

---

## Project Structure

```
/Rope_Pulling_Simulation_Real_Time_Project1/
│
├── src/              # C source files
│   ├── referee.c
│   ├── player.c
│   └── visual.c      # (OpenGL)
│
├── include/          # Header files
│   ├── header.h
│   ├── constants.h
│   └── structs.h
│
├── config/           # Game configuration
│   └── config.txt
│
└── README.md
```

---

## How to Compile

Run this in your terminal:

```bash
gcc player.c -o player
gcc referee.c -o referee
gcc visual.c -o visual -lGL -lGLU -lglut
/referee config.txt
```

This builds:
- `referee` – the main game controller
- `player` – the player process
- `visual` – OpenGL visualizer

---

## How to Run

```bash
./referee config/config.txt
```

You’ll see stats for each round printed in the terminal.

---

## Config File Format

You can add comments at the end of each line using `#`:

```txt
80 100     # Players start with energy between 80 and 100
5 10       # Energy decreases by 5 to 10 each round
2 4        # Recovery time (in seconds) if a player falls
500        # Minimum total effort to win a round
60         # Total game time in seconds
2          # Number of rounds needed to win the game
```

---

## Contributors

- Hala
- Jenan
-
-

---

## Notes

- This project is designed to run on Linux systems
- Before running, make sure the compiled files are executable:

```bash
chmod +x referee player visual
```

---

Enjoy the game!

