#include "header.h"
#include "constants.h"
#include "structs.h"

// Global configuration and players array
GameConfig config;
pid_t players[NUM_PLAYERS];


// ##################################
// Loads game configuration values from a file provided by the user
// ##################################
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d %d %d %d %d %d %d %d %d",
           &config.energy_min, &config.energy_max,
           &config.decrease_min, &config.decrease_max,
           &config.recovery_min, &config.recovery_max,
           &config.win_threshold,
           &config.game_duration,
           &config.rounds_to_win);
    fclose(file);
}


// ##################################
// Gives player positions based on their current energy levels
// ##################################
void assign_positions(int energies[], int positions[]) {
    int order[TEAM_SIZE] = {0, 1, 2, 3};
    for (int i = 0; i < TEAM_SIZE-1; i++) {
        for (int j = i+1; j < TEAM_SIZE; j++) {
            if (energies[order[i]] > energies[order[j]]) {
                int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }
    for (int i = 0; i < TEAM_SIZE; i++) {
        positions[order[i]] = i + 1;
    }
}


// ##################################
// Controls the entire game flow: signals players, reads data, and decides winners
// ##################################
int main(int argc, char* argv[]) {
    // Argument validation
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    read_config(argv[1]); // Load config

    // Set up pipes so the referee and players can send and receive data
    int read_pipes[NUM_PLAYERS][2], write_pipes[NUM_PLAYERS][2];
    for (int i = 0; i < NUM_PLAYERS; i++) {
        pipe(read_pipes[i]);
        pipe(write_pipes[i]);
    }

    // Start a separate process for each player and pass them their setup info
    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i] = fork();
        if (players[i] == 0) {
            // Close unrelated pipes
            for (int j = 0; j < NUM_PLAYERS; j++) {
                if (j != i) {
                    close(read_pipes[j][0]); close(read_pipes[j][1]);
                    close(write_pipes[j][0]); close(write_pipes[j][1]);
                }
            }

            // Format arguments and start the player program using exec
            char pos[10], rfd[10], wfd[10];
            sprintf(pos, "%d", i % TEAM_SIZE);
            sprintf(rfd, "%d", write_pipes[i][0]);
            sprintf(wfd, "%d", read_pipes[i][1]);

            execl("./player", "player", pos, rfd, wfd, argv[1], NULL);
            perror("execl failed");
            exit(1);
        }
    }

    // Close unused pipe ends in referee
    for (int i = 0; i < NUM_PLAYERS; i++) {
        close(read_pipes[i][1]);
        close(write_pipes[i][0]);
    }

    // Set up all counters and timers before starting the game
    sleep(1);
    int score1 = 0, score2 = 0;
    int last_winner = 0;
    int consecutive_wins = 0;
    int prev_energy_t1[TEAM_SIZE] = {0}, prev_energy_t2[TEAM_SIZE] = {0};
    time_t start_time = time(NULL);

    // ##################################
    // Main Loop
    // ##################################
    for (int round = 1; round <= config.rounds_to_win; round++) {
        printf("\n=== Round %d ===\n\n", round);
        usleep(500000);

        // Signal players to update energy
        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR1);
        sleep(1);

        // Read updated energies
        PlayerStats t1_stats[TEAM_SIZE], t2_stats[TEAM_SIZE];
        int team1_energies[TEAM_SIZE], team2_energies[TEAM_SIZE];

        for (int i = 0; i < TEAM_SIZE; i++) {
            read(read_pipes[i][0], &t1_stats[i], sizeof(PlayerStats));
            team1_energies[i] = t1_stats[i].energy;

            read(read_pipes[i + TEAM_SIZE][0], &t2_stats[i], sizeof(PlayerStats));
            team2_energies[i] = t2_stats[i].energy;
        }

        // Assign new positions based on their current energy levels
        int team1_pos[TEAM_SIZE], team2_pos[TEAM_SIZE];
        assign_positions(team1_energies, team1_pos);
        assign_positions(team2_energies, team2_pos);

        // Inform each player of their updated position via pipe
        for (int i = 0; i < TEAM_SIZE; i++) {
            write(write_pipes[i][1], &team1_pos[i], sizeof(int));
            write(write_pipes[i + TEAM_SIZE][1], &team2_pos[i], sizeof(int));
        }

        // Signal players to send effort
        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR2);
        sleep(1);

        // Read player efforts
        int total1 = 0, total2 = 0;
        for (int i = 0; i < TEAM_SIZE; i++) {
            read(read_pipes[i][0], &t1_stats[i], sizeof(PlayerStats));
            read(read_pipes[i + TEAM_SIZE][0], &t2_stats[i], sizeof(PlayerStats));

            total1 += t1_stats[i].effort;
            total2 += t2_stats[i].effort;
        }

        // Display team stats
        printf("Team 1:\nPlayer | Position | Energy | Effort\n");
        for (int i = 0; i < TEAM_SIZE; i++) {
            char* change = " ";
            if (t1_stats[i].energy < prev_energy_t1[i]) change = " 🔻";
            else if (t1_stats[i].energy > prev_energy_t1[i]) change = " 🔺";

            if (t1_stats[i].energy == 0)
                printf("T1-P%d   |    %d     |   %3d   |  %sFALLEN%s%s\n",
                       t1_stats[i].player_id, t1_stats[i].position, t1_stats[i].energy, RED, RESET, change);
            else
                printf("T1-P%d   |    %d     |   %3d   |  %3d%s\n",
                       t1_stats[i].player_id, t1_stats[i].position, t1_stats[i].energy, t1_stats[i].effort, change);

            prev_energy_t1[i] = t1_stats[i].energy;
        }

        printf("\nTeam 2:\nPlayer | Position | Energy | Effort\n");
        for (int i = 0; i < TEAM_SIZE; i++) {
            char* change = " ";
            if (t2_stats[i].energy < prev_energy_t2[i]) change = " 🔻";
            else if (t2_stats[i].energy > prev_energy_t2[i]) change = " 🔺";

            if (t2_stats[i].energy == 0)
                printf("T2-P%d   |    %d     |   %3d   |  %sFALLEN%s%s\n",
                       t2_stats[i].player_id, t2_stats[i].position, t2_stats[i].energy, RED, RESET, change);
            else
                printf("T2-P%d   |    %d     |   %3d   |  %3d%s\n",
                       t2_stats[i].player_id, t2_stats[i].position, t2_stats[i].energy, t2_stats[i].effort, change);

            prev_energy_t2[i] = t2_stats[i].energy;
        }

        // Evaluate team efforts, determine the round winner, and update scores
        printf("\n>> Team 1 Total: %d\t| Team 2 Total: %d\n", total1, total2);

        if (total1 >= config.win_threshold && total1 > total2) {
            printf("\U0001F3C5 Round %d Winner: Team 1\n", round);
            score1++;
            if (last_winner == 1) consecutive_wins++;
            else { last_winner = 1; consecutive_wins = 1; }
        } else if (total2 >= config.win_threshold && total2 > total1) {
            printf("\U0001F3C5 Round %d Winner: Team 2\n", round);
            score2++;
            if (last_winner == 2) consecutive_wins++;
            else { last_winner = 2; consecutive_wins = 1; }
        } else {
            printf("\U0001F91D Round %d is a tie or threshold not met!\n", round);
            last_winner = 0;
            consecutive_wins = 0;
        }

        if (consecutive_wins >= 2) {
            printf("\n\U0001F389 Team %d won 2 rounds in a row! Game ends early.\n", last_winner);
            break;
        }

        // Check if the total game time has passed the allowed duration
        time_t current_time = time(NULL);
        int elapsed = (int)(current_time - start_time);
        if (elapsed >= config.game_duration) {
            printf("\n⏰ Game duration of %d seconds reached! Game ends now.\n", config.game_duration);
            break;
        }

        usleep(500000);
    }

    // Show the final winner (or a tie) after the game ends
    printf("\n=== Game Over ===\n");
    if (score1 > score2)
        printf("\U0001F3C6 Final Winner: Team 1!\n");
    else if (score2 > score1)
        printf("\U0001F3C6 Final Winner: Team 2!\n");
    else
        printf("\U0001F3C1 Final Result: It's a tie!\n");

    // Terminate all players
    for (int i = 0; i < NUM_PLAYERS; i++) {
        kill(players[i], SIGTERM);
        wait(NULL);
    }

    return 0;
}
