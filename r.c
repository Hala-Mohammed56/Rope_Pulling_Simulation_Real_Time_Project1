#include "header.h"
#include "constants.h"
#include "structs.h"
#include <sys/stat.h>
#include <fcntl.h>


// Global configuration and players array
GameConfig config;
pid_t players[NUM_PLAYERS];

// ##################################
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) { perror("Failed to open config file"); exit(EXIT_FAILURE); }
    char line[100];
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &config.energy_min, &config.energy_max) != 2) {
        fprintf(stderr, "Error: Invalid energy range in config\n"); exit(EXIT_FAILURE);
    }
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &config.decrease_min, &config.decrease_max) != 2) {
        fprintf(stderr, "Error: Invalid decrease range in config\n"); exit(EXIT_FAILURE);
    }
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &config.recovery_min, &config.recovery_max) != 2) {
        fprintf(stderr, "Error: Invalid recovery range in config\n"); exit(EXIT_FAILURE);
    }
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d", &config.win_threshold) != 1) {
        fprintf(stderr, "Error: Invalid win threshold in config\n"); exit(EXIT_FAILURE);
    }
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d", &config.game_duration) != 1) {
        fprintf(stderr, "Error: Invalid game duration in config\n"); exit(EXIT_FAILURE);
    }
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d", &config.rounds_to_win) != 1) {
        fprintf(stderr, "Error: Invalid rounds to win in config\n"); exit(EXIT_FAILURE);
    }
    fclose(file);
}

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
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    read_config(argv[1]);
    srand(time(NULL)); // 🆕 Seed random

    pid_t visual_pid = fork();
    if (visual_pid == 0) {
        execl("./visual", "visual", NULL);
        perror("Failed to launch visual");
        exit(1);
    } else {
        sleep(1);
    }

    int read_pipes[NUM_PLAYERS][2], write_pipes[NUM_PLAYERS][2];
    for (int i = 0; i < NUM_PLAYERS; i++) {
        pipe(read_pipes[i]);
        pipe(write_pipes[i]);
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
        players[i] = fork();
        if (players[i] == 0) {
            for (int j = 0; j < NUM_PLAYERS; j++) {
                if (j != i) {
                    close(read_pipes[j][0]); close(read_pipes[j][1]);
                    close(write_pipes[j][0]); close(write_pipes[j][1]);
                }
            }
            char pos[10], rfd[10], wfd[10];
            sprintf(pos, "%d", i % TEAM_SIZE);
            sprintf(rfd, "%d", write_pipes[i][0]);
            sprintf(wfd, "%d", read_pipes[i][1]);
            execl("./player", "player", pos, rfd, wfd, argv[1], NULL);
            perror("execl failed");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_PLAYERS; i++) {
        close(read_pipes[i][1]);
        close(write_pipes[i][0]);
    }

    sleep(1);
    int score1 = 0, score2 = 0;
    int last_winner = 0;
    int consecutive_wins = 0;
    int prev_energy_t1[TEAM_SIZE] = {0}, prev_energy_t2[TEAM_SIZE] = {0};
    time_t start_time = time(NULL);

    for (int i = 0; i < NUM_PLAYERS; i++) {
        char pipe_name[50];
        sprintf(pipe_name, "/tmp/player_pipe_%d", i);
        mkfifo(pipe_name, 0666);
    }

    for (int round = 1; round <= config.rounds_to_win; round++) {
        int round_duration = rand() % 4 + 2; // 🆕 2 to 5 seconds
        printf("\n=== Round %d ===\n", round);
        printf("⏱️ Round duration: %d seconds\n\n", round_duration);
        sleep(round_duration);

        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR1);
        sleep(1);

        PlayerStats t1_stats[TEAM_SIZE], t2_stats[TEAM_SIZE];
        int team1_energies[TEAM_SIZE], team2_energies[TEAM_SIZE];

        for (int i = 0; i < TEAM_SIZE; i++) {
            read(read_pipes[i][0], &t1_stats[i], sizeof(PlayerStats));
            team1_energies[i] = t1_stats[i].energy;
            read(read_pipes[i + TEAM_SIZE][0], &t2_stats[i], sizeof(PlayerStats));
            team2_energies[i] = t2_stats[i].energy;
        }

        int team1_pos[TEAM_SIZE], team2_pos[TEAM_SIZE];
        assign_positions(team1_energies, team1_pos);
        assign_positions(team2_energies, team2_pos);

        for (int i = 0; i < TEAM_SIZE; i++) {
            write(write_pipes[i][1], &team1_pos[i], sizeof(int));
            write(write_pipes[i + TEAM_SIZE][1], &team2_pos[i], sizeof(int));
        }

        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR2);
        sleep(1);

        int total1 = 0, total2 = 0;
        for (int i = 0; i < TEAM_SIZE; i++) {
            read(read_pipes[i][0], &t1_stats[i], sizeof(PlayerStats));
            read(read_pipes[i + TEAM_SIZE][0], &t2_stats[i], sizeof(PlayerStats));
            total1 += t1_stats[i].effort;
            total2 += t2_stats[i].effort;
        }

        for (int i = 0; i < TEAM_SIZE; i++) {
            char pipe_name[50];
            sprintf(pipe_name, "/tmp/player_pipe_%d", i);
            int fd = open(pipe_name, O_WRONLY | O_NONBLOCK);
            if (fd >= 0) { write(fd, &t1_stats[i], sizeof(PlayerStats)); close(fd); }

            sprintf(pipe_name, "/tmp/player_pipe_%d", i + TEAM_SIZE);
            fd = open(pipe_name, O_WRONLY | O_NONBLOCK);
            if (fd >= 0) { write(fd, &t2_stats[i], sizeof(PlayerStats)); close(fd); }
        }

        printf("Team 1:Player | Position | Energy | Effort\n");
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

        printf("\nTeam 2:Player | Position | Energy | Effort\n");
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

        printf("⏹️ End of Round %d (duration: %d seconds)\n", round, round_duration);

        if (consecutive_wins >= 2) {
            printf("\n\U0001F389 Team %d won 2 rounds in a row! Game ends early.\n", last_winner);
            break;
        }

        time_t current_time = time(NULL);
        int elapsed = (int)(current_time - start_time);
        if (elapsed >= config.game_duration) {
            printf("\n⏰ Game duration of %d seconds reached! Game ends now.\n", config.game_duration);
            break;
        }

        sleep(2);
    }

    printf("\n=== Game Over ===\n");
    if (score1 > score2)
        printf("\U0001F3C6 Final Winner: Team 1!\n");
    else if (score2 > score1)
        printf("\U0001F3C6 Final Winner: Team 2!\n");
    else
        printf("\U0001F3C1 Final Result: It's a tie!\n");

    for (int i = 0; i < NUM_PLAYERS; i++) {
        kill(players[i], SIGTERM);
        wait(NULL);
    }

    return 0;
}
