#ifndef STRUCTS_H
#define STRUCTS_H

// Game Configuration Struct
typedef struct {
    int energy_min, energy_max;
    int decrease_min, decrease_max;
    int recovery_min, recovery_max;
    int win_threshold;
    int game_duration;
    int rounds_to_win;
} GameConfig;

// Stats exchanged between referee and players
typedef struct {
    int player_id;
    int position;
    int energy;
    int effort;
} PlayerStats;

// Player internal state
typedef struct {
    int player_id;
    int position;
    int energy;
    int effort;
    int read_fd;
    int write_fd;
    int active;
    int recovering;
} Player;

#endif
