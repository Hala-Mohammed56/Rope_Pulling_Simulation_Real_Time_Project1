// Part 1: Reading config.txt
#include "header.h"
#define NUM_PLAYERS 8

// Struct to store all configuration values loaded from config.txt
typedef struct {
    int energy_min;
    int energy_max;
    int effort_decrease_min;
    int effort_decrease_max;
    int recovery_time_min;
    int recovery_time_max;
    int win_threshold;
    int game_duration;
    int rounds_to_win;
} GameConfig;

// Global variable to hold the config values
GameConfig config;

// Reads configuration values into the global config struct
void read_config(const char *filename) {
    FILE *fp = fopen(filename, "r");  // Open the config file in read mode
    if (!fp) {
        perror("Failed to open config file");  // Print error if file doesn't open
        exit(1);  // Exit the program
    }

    // Read each line from the file and assign the value to the corresponding struct field
    fscanf(fp, "initial_energy_min=%d\n", &config.energy_min);
    fscanf(fp, "initial_energy_max=%d\n", &config.energy_max);
    fscanf(fp, "effort_decrease_min=%d\n", &config.effort_decrease_min);
    fscanf(fp, "effort_decrease_max=%d\n", &config.effort_decrease_max);
    fscanf(fp, "recovery_time_min=%d\n", &config.recovery_time_min);
    fscanf(fp, "recovery_time_max=%d\n", &config.recovery_time_max);
    fscanf(fp, "win_threshold=%d\n", &config.win_threshold);
    fscanf(fp, "game_duration=%d\n", &config.game_duration);
    fscanf(fp, "rounds_to_win=%d\n", &config.rounds_to_win);

    fclose(fp);  // Close the file after reading
}

int main(int argc, char *argv[]) {
    // Check if the config file path is provided as a command line argument
    if (argc < 2) {
        printf("Usage: %s config.txt\n", argv[0]);
        return 1;
    }

    // Call function to read the config values from file
    read_config(argv[1]);

    // For testing: print all loaded configuration values
    printf("Energy: %d - %d\n", config.energy_min, config.energy_max);
    printf("Effort decrease: %d - %d\n", config.effort_decrease_min, config.effort_decrease_max);
    printf("Recovery time: %d - %d\n", config.recovery_time_min, config.recovery_time_max);
    printf("Win threshold: %d\n", config.win_threshold);
    printf("Game duration: %d\n", config.game_duration);
    printf("Rounds to win: %d\n", config.rounds_to_win);



    // Part 2: Create 8 player processes using fork()

    pid_t players_pids[NUM_PLAYERS];  // Array to store PIDs of all 8 players

    for (int player_id = 0; player_id < NUM_PLAYERS; player_id++) {
        pid_t child_pid = fork();  // Create a new player process

        if (child_pid < 0) {
            perror("Error: fork failed");
            exit(1);
        }

        if (child_pid == 0) {
            // This block runs in the child process (a player)
            printf("Player %d created. PID = %d, Parent PID = %d\n",
                   player_id, getpid(), getppid());

            // Placeholder for player logic – will be replaced later
            exit(0);
        } else {
            // This block runs in the parent process (referee)
            players_pids[player_id] = child_pid;  // Store player's PID
        }
    }

    // After all players are created, referee prints the PIDs
    printf("\nAll player processes created successfully:\n");
    for (int i = 0; i < NUM_PLAYERS; i++) {
        printf("Player %d → PID: %d\n", i, players_pids[i]);
    }

    return 0;
}


