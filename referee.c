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
    // printf("Energy: %d - %d\n", config.energy_min, config.energy_max);
    // printf("Effort decrease: %d - %d\n", config.effort_decrease_min, config.effort_decrease_max);
    // printf("Recovery time: %d - %d\n", config.recovery_time_min, config.recovery_time_max);
    // printf("Win threshold: %d\n", config.win_threshold);
    // printf("Game duration: %d\n", config.game_duration);
    // printf("Rounds to win: %d\n", config.rounds_to_win);



    // Create 8 player processes
    pid_t players_pids[NUM_PLAYERS];  // Array to store PIDs of players

    for (int player_id = 0; player_id < NUM_PLAYERS; player_id++) {
        pid_t child_pid = fork();  // Create a new player

        if (child_pid < 0) {
            perror("Error: fork failed");
            exit(1);
        }

        if (child_pid == 0) {
            // printf("Player %d created. PID = %d, Parent PID = %d\n",
                   // player_id, getpid(), getppid());
            char *team = (player_id < 4) ? "team1" : "team2";


            execl("./player", "player", team, NULL);   // Replace child with player executable
            perror("execl failed");
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

    //Send signals to all players (SIGUSR1 = Get Ready, SIGUSR2 = Start Pulling)

    sleep(1);//short delay

    //Referee sends SIGUSR1 to all players to tell them "Get Ready"
    printf("\nReferee: Sending SIGUSR1 to all players (Get Ready)...\n");

    for (int player_index = 0; player_index < NUM_PLAYERS; player_index++) {
        pid_t target_pid = players_pids[player_index];  // Get the player's PID
        int result = kill(target_pid, SIGUSR1);         // Send SIGUSR1

        if (result == 0) {
            // printf("Referee: SIGUSR1 sent to Player %d (PID %d)\n", player_index, target_pid);
        } else {
            perror("Referee: Failed to send SIGUSR1");
        }
    }

    // Wait for players respond
    sleep(1);

    // Referee sends SIGUSR2 to all players to tell them "Start Pulling"
    printf("\nReferee: Sending SIGUSR2 to all players (Start Pulling)...\n");

    for (int player_index = 0; player_index < NUM_PLAYERS; player_index++) {
        pid_t target_pid = players_pids[player_index];  // Get the player's PID
        int result = kill(target_pid, SIGUSR2);         // Send SIGUSR2

        if (result == 0) {
            // printf("Referee: SIGUSR2 sent to Player %d (PID %d)\n", player_index, target_pid);
        } else {
            perror("Referee: Failed to send SIGUSR2");
        }
    }

    printf("\nReferee: All signals sent to players.\n");


    // Create FIFOs and Read Player Efforts
    printf("\nReferee: Creating FIFOs for both teams...\n");

    // Create two named pipes (FIFOs)
    if (mkfifo("/tmp/team1_fifo", 0666) == -1 && errno != EEXIST) {
        perror("Error creating team1_fifo");
        exit(1);
    }

    if (mkfifo("/tmp/team2_fifo", 0666) == -1 && errno != EEXIST) {
        perror("Error creating team2_fifo");
        exit(1);
    }

    // Buffers for reading efforts
    int effort;
    int team1_total = 0;
    int team2_total = 0;

    //Read Team 1 Efforts
    printf("Referee: Reading efforts from team 1...\n");
     int team1_fd = open("/tmp/team1_fifo", O_RDWR);
    if (team1_fd == -1) {
        perror("Error opening team1_fifo for reading");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        if (read(team1_fd, &effort, sizeof(int)) > 0) {
            printf("Team 1 - Player %d effort: %d\n", i, effort);
            team1_total += effort;
        } else {
            perror("Failed to read from team1_fifo");
        }
    }
    close(team1_fd);


    //Read Team 2 Efforts
    printf("Referee: Reading efforts from team 2...\n");
    int team2_fd = open("/tmp/team2_fifo", O_RDWR);
    if (team2_fd == -1) {
        perror("Error opening team2_fifo for reading");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        if (read(team2_fd, &effort, sizeof(int)) > 0) {
            printf("Team 2 - Player %d effort: %d\n", i + 4, effort);
            team2_total += effort;
        } else {
            perror("Failed to read from team2_fifo");
        }
    }
    close(team2_fd);


    //Result
    printf("\nTeam 1 Total Effort: %d\n", team1_total);
    printf("Team 2 Total Effort: %d\n", team2_total);

    if (team1_total > team2_total) {
        printf("🏆 Team 1 wins this round!\n");
    } else if (team2_total > team1_total) {
        printf("🏆 Team 2 wins this round!\n");
    } else {
        printf("🤝 It's a tie!\n");
    }


    return 0;
}


