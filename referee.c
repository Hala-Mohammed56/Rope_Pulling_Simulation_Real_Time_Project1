#include "header.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define NUM_PLAYERS 8
#define TEAM_SIZE 4

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

GameConfig config;

void read_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open config file");
        exit(1);
    }

    fscanf(fp, "%d %d", &config.energy_min, &config.energy_max);
    fscanf(fp, "%d %d", &config.effort_decrease_min, &config.effort_decrease_max);
    fscanf(fp, "%d %d", &config.recovery_time_min, &config.recovery_time_max);
    fscanf(fp, "%d", &config.win_threshold);
    fscanf(fp, "%d", &config.game_duration);
    fscanf(fp, "%d", &config.rounds_to_win);

    fclose(fp);
}

void assign_positions(int energies[], int positions[]) {
    int sorted_indices[TEAM_SIZE];
    for (int i = 0; i < TEAM_SIZE; i++) {
        sorted_indices[i] = i;
    }

    for (int i = 0; i < TEAM_SIZE - 1; i++) {
        for (int j = i + 1; j < TEAM_SIZE; j++) {
            if (energies[sorted_indices[i]] > energies[sorted_indices[j]]) {
                int tmp = sorted_indices[i];
                sorted_indices[i] = sorted_indices[j];
                sorted_indices[j] = tmp;
            }
        }
    }

    for (int i = 0; i < TEAM_SIZE; i++) {
        positions[sorted_indices[i]] = i;  // position: 0 (lowest) to 3 (highest)
    }

    // Debug print
    printf("\n Team Energies & Assigned Positions:\n");
    for (int i = 0; i < TEAM_SIZE; i++) {
        printf("Player %d → Energy = %d → Position = %d\n", i, energies[i], positions[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s config.txt\n", argv[0]);
        return 1;
    }

    read_config(argv[1]);

    int team1_pipe[2], team2_pipe[2];
    pipe(team1_pipe);
    pipe(team2_pipe);

    pid_t players_pids[NUM_PLAYERS];
    int team1_energies[TEAM_SIZE];
    int team2_energies[TEAM_SIZE];
    int team1_positions[TEAM_SIZE];
    int team2_positions[TEAM_SIZE];


    for (int player_id = 0; player_id < NUM_PLAYERS; player_id++) {
        pid_t child_pid = fork();

        if (child_pid < 0) {
            perror("Error: fork failed");
            exit(1);
        }

        if (child_pid == 0) {
            for (int j = 0; j < 2; j++) {
                if (player_id < 4) close(team1_pipe[0]);
                else close(team2_pipe[0]);
            }

            char position_str[4];
            char pipe_fd_str[4];
            sprintf(position_str, "%d", player_id % 4);
            sprintf(pipe_fd_str, "%d", player_id < 4 ? team1_pipe[1] : team2_pipe[1]);
            execl("./player", "player", position_str, pipe_fd_str, argv[1], NULL);
            perror("execl failed");
            exit(1);
        } else {
            players_pids[player_id] = child_pid;
        }
    }


    printf("\nAll player processes created successfully:\n");
    for (int i = 0; i < NUM_PLAYERS; i++) {
        printf("Player %d → PID: %d\n", i, players_pids[i]);
    }

    sleep(1);
    printf("\nReferee: Sending SIGUSR1 to all players (Get Ready)...\n");
    for (int i = 0; i < NUM_PLAYERS; i++) kill(players_pids[i], SIGUSR1);
    sleep(1);

    printf("\nReferee: Sending SIGUSR2 to all players (Start Pulling)...\n");
    for (int i = 0; i < NUM_PLAYERS; i++) kill(players_pids[i], SIGUSR2);
    sleep(1);

    printf("\nReferee: Reading efforts from players...\n");

    int team1_total = 0, team2_total = 0;
    int effort;

    for (int i = 0; i < TEAM_SIZE; i++) {
        if (read(team1_pipe[0], &effort, sizeof(int)) > 0) {
            team1_energies[i] = effort;
            printf("Team 1 - Player %d effort: %d\n", i, effort);
            team1_total += effort;
        } else {
            perror("Failed to read from team1_pipe");
        }
    }

    for (int i = 0; i < TEAM_SIZE; i++) {
        if (read(team2_pipe[0], &effort, sizeof(int)) > 0) {
            team2_energies[i] = effort;
            printf("Team 2 - Player %d effort: %d\n", i + 4, effort);
            team2_total += effort;
        } else {
            perror("Failed to read from team2_pipe");
        }
    }

    assign_positions(team1_energies, team1_positions);
    assign_positions(team2_energies, team2_positions);

    // Send position to each player via pipe
    for (int i = 0; i < TEAM_SIZE; i++) {
        write(team1_pipe[1], &team1_positions[i], sizeof(int));
    }
    for (int i = 0; i < TEAM_SIZE; i++) {
        write(team2_pipe[1], &team2_positions[i], sizeof(int));
    }
    close(team1_pipe[1]);
    close(team2_pipe[1]);

    printf("\nTeam 1 Total Effort: %d\n", team1_total);
    printf("Team 2 Total Effort: %d\n", team2_total);

    if (team1_total > team2_total) {
        printf("************Team 1 wins this round!************\n");
    } else if (team2_total > team1_total) {
        printf("************Team 2 wins this round!************\n");
    } else {
        printf("************It's a tie!************\n");
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
        kill(players_pids[i], SIGKILL);
        wait(NULL);
    }

    return 0;
}
