# 🧵 Rope Pulling Game Simulation

A Linux-based multi-process game simulation in C, where two teams of players compete in a tug-of-war. Each player has energy, effort, and can fall or recover. The referee controls the game using signals and inter-process communication.

---

## 🛠️ Features

- 🧠 Dynamic energy updates for each player
- 🚨 Signal-based communication (`SIGUSR1`, `SIGUSR2`, `SIGTERM`)
- 🔄 Pipes for sending/receiving data
- 🧽 Player processes with realistic behaviors (fall, recover)
- 📊 Game stats printed round-by-round
- 🖼️ OpenGL visualization (`visual.c`)

---

## 📁 Project Structure

```
/Rope_Pulling_Simulation_Real_Time_Project1/
│
├── src/              # C source files
│   ├── referee.c
│   ├── player.c
│   └── visual.c      # (optional, OpenGL)
│
├── include/          # Header files
│   ├── header.h
│   ├── constants.h
│   └── structs.h
│
├── config/           # Game configuration
│   └── config.txt
|
└── README.md
```

---

## ⚙️ How to Compile

Run this in your terminal:

```bash
make
```

This builds:
- `referee` → the main game controller
- `player` → player process
- `visual` → optional OpenGL visualization

---

## ▶️ How to Run

```bash
./referee config/config.txt
```

You’ll see round-by-round stats printed in terminal.

---

## 📝 Config File Format

Each line may include comments using `#`:
```txt
80 100     # Initial energy range (min max)
5 10       # Energy decrease per round (min max)
2 4        # Recovery time if player falls (min max)
500        # Win threshold
60         # Game duration (in seconds)
2          # Rounds to win
```

---

## 👨‍💻 Contributors

- Hala
- Jenan
-
-

---

## 🏑 Notes

- Works best on Linux
- Make sure to give execution permission to your compiled files:
  ```bash
  chmod +x referee player visual
  ```

---

🎮 Let the rope-pulling begin!

