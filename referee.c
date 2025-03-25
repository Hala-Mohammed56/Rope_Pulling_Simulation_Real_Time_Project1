#include "header.h"
#define NUM_PLAYERS 8

// Store all config. values loaded from the file
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

// Global variable for config values
GameConfig config;

// Reads config. values into the global var.
void read_config(const char *filename) {
    FILE *fp = fopen(filename, "r");  // Open the file in read mode
    if (!fp) {
        perror("Failed to open config file");  // Print error
        exit(1);  // Exit the program
    }

    // Read each line from the file
    fscanf(fp, "initial_energy_min=%d\n", &config.energy_min);
    fscanf(fp, "initial_energy_max=%d\n", &config.energy_max);
    fscanf(fp, "effort_decrease_min=%d\n", &config.effort_decrease_min);
    fscanf(fp, "effort_decrease_max=%d\n", &config.effort_decrease_max);
    fscanf(fp, "recovery_time_min=%d\n", &config.recovery_time_min);
    fscanf(fp, "recovery_time_max=%d\n", &config.recovery_time_max);
    fscanf(fp, "win_threshold=%d\n", &config.win_threshold);
    fscanf(fp, "game_duration=%d\n", &config.game_duration);
    fscanf(fp, "rounds_to_win=%d\n", &config.rounds_to_win);

    fclose(fp);  // Close the file
}

int main(int argc, char *argv[]) {
    // Check if the file path is provided as a command line argument
    if (argc < 2) {
        printf("Usage: %s config.txt\n", argv[0]);
        return 1;
    }

    // Call function to read from file
    read_config(argv[1]);

    // print all config. values
    printf("Energy: %d - %d\n", config.energy_min, config.energy_max);
    printf("Effort decrease: %d - %d\n", config.effort_decrease_min, config.effort_decrease_max);
    printf("Recovery time: %d - %d\n", config.recovery_time_min, config.recovery_time_max);
    printf("Win threshold: %d\n", config.win_threshold);
    printf("Game duration: %d\n", config.game_duration);
    printf("Rounds to win: %d\n", config.rounds_to_win);



    // Create 8 player processes
    pid_t players_pids[NUM_PLAYERS];  // Array to store PIDs of players

    for (int player_id = 0; player_id < NUM_PLAYERS; player_id++) {
        pid_t child_pid = fork();  // Create a new player

        if (child_pid < 0) {
            perror("Error: fork failed");
            exit(1);
        }

        if (child_pid == 0) {
            printf("Player %d created. PID = %d, Parent PID = %d\n",
                   player_id, getpid(), getppid());

            // Placeholder for player logic – will be replaced later
            exit(0);
        } else {
            // the parent process (referee)
            players_pids[player_id] = child_pid;  // Store player's PID
        }
    }

    // Referee prints the PIDs of players
    printf("\nAll player processes created successfully:\n");
    for (int i = 0; i < NUM_PLAYERS; i++) {
        printf("Player %d → PID: %d\n", i, players_pids[i]);
    }

    return 0;
}


