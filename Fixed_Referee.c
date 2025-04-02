
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define TEAM_SIZE 4
#define NUM_PLAYERS 8

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define CYAN    "\033[1;36m"
#define RESET   "\033[0m"

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

    fscanf(file, "%d %d %d %d %d %d %d %d %d",
           &config.energy_min, &config.energy_max,
           &config.decrease_min, &config.decrease_max,
           &config.recovery_min, &config.recovery_max,
           &config.win_threshold,
           &config.game_duration,
           &config.rounds_to_win);
    fclose(file);
}

void assign_positions(int efforts[], int positions[]) {
    int order[TEAM_SIZE] = {0, 1, 2, 3};
    for (int i = 0; i < TEAM_SIZE-1; i++) {
        for (int j = i+1; j < TEAM_SIZE; j++) {
            if (efforts[order[i]] > efforts[order[j]]) {
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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

     // log to file
    read_config(argv[1]);
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

    for (int round = 1; round <= config.rounds_to_win; round++) {
        printf("\n=== Round %d ===\n", round);
        usleep(500000);

        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR1);
        sleep(1);

        int team1_pos[TEAM_SIZE], team2_pos[TEAM_SIZE];
        int team1_eff[TEAM_SIZE], team2_eff[TEAM_SIZE];

        assign_positions((int[4]){1, 2, 3, 4}, team1_pos);
        assign_positions((int[4]){1, 2, 3, 4}, team2_pos);

        for (int i = 0; i < TEAM_SIZE; i++) {
            write(write_pipes[i][1], &team1_pos[i], sizeof(int));
            write(write_pipes[i + TEAM_SIZE][1], &team2_pos[i], sizeof(int));
        }

        for (int i = 0; i < NUM_PLAYERS; i++) kill(players[i], SIGUSR2);
        sleep(1);

        int total1 = 0, total2 = 0;
        for (int i = 0; i < TEAM_SIZE; i++) {
            int effort1, effort2;
            read(read_pipes[i][0], &effort1, sizeof(int));
            read(read_pipes[i + TEAM_SIZE][0], &effort2, sizeof(int));
            total1 += effort1;
            total2 += effort2;
            printf("Player T1-P%d: Effort=%d\t| Player T2-P%d: Effort=%d\n", i, effort1, i, effort2);
        }

        printf(">> Team 1 Total: %d | Team 2 Total: %d\n", total1, total2);
        if (total1 > total2) {
            printf("🏅 Round %d Winner: Team 1\n", round);
            score1++;
        } else if (total2 > total1) {
            printf("🏅 Round %d Winner: Team 2\n", round);
            score2++;
        } else {
            printf("🤝 Round %d is a tie!\n", round);
        }
        usleep(500000);
    }

    printf("\n=== Game Over ===\n");
    if (score1 > score2)
        printf("🏆 Final Winner: Team 1!\n");
    else if (score2 > score1)
        printf("🏆 Final Winner: Team 2!\n");
    else
        printf("🏁 Final Result: It's a tie!\n");

    for (int i = 0; i < NUM_PLAYERS; i++) {
        kill(players[i], SIGTERM);
        wait(NULL);
    }

    return 0;
}
