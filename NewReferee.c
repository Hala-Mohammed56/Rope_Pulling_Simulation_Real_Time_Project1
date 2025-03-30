#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

#define TEAM_SIZE 4
#define NUM_PLAYERS 8

typedef struct {
    int energy_min, energy_max;
    int decrease_min, decrease_max;
    int recovery_min, recovery_max;
    int win_threshold;
    int game_duration;
    int rounds_to_win;
} GameConfig;

GameConfig config;
pid_t players[NUM_PLAYERS];

void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d %d %d %d %d %d %d %d %d",
               &config.energy_min, &config.energy_max,
               &config.decrease_min, &config.decrease_max,
               &config.recovery_min, &config.recovery_max,
               &config.win_threshold,
               &config.game_duration,
               &config.rounds_to_win) != 9) {
        fprintf(stderr, "Error: Invalid config file format\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

void assign_positions(int energies[], int positions[]) {
    int indices[TEAM_SIZE] = {0, 1, 2, 3};

    // Sort by energy (ascending)
    for (int i = 0; i < TEAM_SIZE-1; i++) {
        for (int j = i+1; j < TEAM_SIZE; j++) {
            if (energies[indices[i]] > energies[indices[j]]) {
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }

    // Assign positions (lowest energy in middle)
    for (int i = 0; i < TEAM_SIZE; i++) {
        positions[indices[i]] = i + 1; // Factors 1-4
    }
}

void print_team_details(const char* team_name, int efforts[], int positions[], int size) {
    printf("\n%s Team Details:\n", team_name);
    printf("Player\tPosition\tEnergy\tEffort (Energy*Position)\n");
    printf("------------------------------------------------\n");

    for (int i = 0; i < size; i++) {
        float energy = (float)efforts[i] / positions[i]; // استخدام float للدقة
        printf("%d\t%d\t\t%.1f\t%d\n",
               i, positions[i], energy, efforts[i]);
    }
}
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    read_config(argv[1]);
    time_t start_time = time(NULL);
    int team1_score = 0, team2_score = 0;
    int team1_consec = 0, team2_consec = 0;
    int rounds_played = 0;

    // Create pipes
    int team1_pipe[2], team2_pipe[2];
    pipe(team1_pipe);
    pipe(team2_pipe);

    // Fork players
    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i] = fork();
        if (players[i] == 0) {
            close(i < TEAM_SIZE ? team1_pipe[0] : team2_pipe[0]);
            close(i < TEAM_SIZE ? team2_pipe[1] : team1_pipe[1]);

            char pos[10], fd[10];
            sprintf(pos, "%d", i % TEAM_SIZE);
            sprintf(fd, "%d", i < TEAM_SIZE ? team1_pipe[1] : team2_pipe[1]);

            execl("./player", "player", pos, fd, argv[1], NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }

    // Referee setup
    close(team1_pipe[1]);
    close(team2_pipe[1]);
    printf("Referee: Game started (Duration: %d sec)\n", config.game_duration);

    while (rounds_played < config.rounds_to_win &&
           (time(NULL) - start_time) < config.game_duration &&
           team1_consec < 2 && team2_consec < 2) {

        printf("\n--- Round %d ---\n", rounds_played + 1);

        // Signal players to prepare
        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR1);
        sleep(1);

        // Signal players to send effort
        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR2);
        sleep(1);

        // Read efforts
        int team1_total = 0, team2_total = 0;
        int effort, team1_efforts[TEAM_SIZE], team2_efforts[TEAM_SIZE];
        int team1_pos[TEAM_SIZE], team2_pos[TEAM_SIZE];

        for (int i = 0; i < TEAM_SIZE; i++) {
            read(team1_pipe[0], &effort, sizeof(int));
            team1_efforts[i] = effort;
            team1_total += effort;
        }

        for (int i = 0; i < TEAM_SIZE; i++) {
            read(team2_pipe[0], &effort, sizeof(int));
            team2_efforts[i] = effort;
            team2_total += effort;
        }

        // Assign positions based on energy
        assign_positions(team1_efforts, team1_pos);
        assign_positions(team2_efforts, team2_pos);

        // Print team details
        print_team_details("Team 1", team1_efforts, team1_pos, TEAM_SIZE);
        print_team_details("Team 2", team2_efforts, team2_pos, TEAM_SIZE);

        printf("\nTeam 1 Total Effort: %d | Team 2 Total Effort: %d\n", team1_total, team2_total);

        // Determine round winner
        if (team1_total > team2_total) {
            printf("Team 1 wins round %d!\n", rounds_played + 1);
            team1_score++;
            team1_consec++;
            team2_consec = 0;
        } else if (team2_total > team1_total) {
            printf("Team 2 wins round %d!\n", rounds_played + 1);
            team2_score++;
            team2_consec++;
            team1_consec = 0;
        } else {
            printf("Round %d is a tie!\n", rounds_played + 1);
            team1_consec = team2_consec = 0;
        }

        rounds_played++;
        sleep(2); // Round cooldown
    }

    // Game over conditions
    printf("\n=== Game Over ===\n");
    if (team1_consec >= 2) printf("Team 1 won 2 consecutive rounds!\n");
    else if (team2_consec >= 2) printf("Team 2 won 2 consecutive rounds!\n");
    else if ((time(NULL) - start_time) >= config.game_duration) printf("Time limit reached!\n");
    else printf("Maximum rounds played!\n");

    printf("Final Score: Team 1 (%d) - Team 2 (%d)\n", team1_score, team2_score);

    // Cleanup
    for (int i = 0; i < NUM_PLAYERS; i++) {
        kill(players[i], SIGTERM);
        wait(NULL);
    }

    close(team1_pipe[0]);
    close(team2_pipe[0]);
    return EXIT_SUCCESS;
}
