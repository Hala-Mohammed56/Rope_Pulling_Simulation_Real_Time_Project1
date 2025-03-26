#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_PLAYERS 4

typedef struct {
    int energy;
    int player_index;
} PlayerInfo;

int compare_players(const void* a, const void* b) {
    return ((PlayerInfo*)a)->energy - ((PlayerInfo*)b)->energy;  // ascending: p0 = lowest, p3 = highest
}

void assign_initial_positions(int pipe_from_child[MAX_PLAYERS][2], int pipe_to_child[MAX_PLAYERS][2]) {
    PlayerInfo players[MAX_PLAYERS];

    // Step 1: Read initial energy from each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        read(pipe_from_child[i][0], &players[i].energy, sizeof(int));
        players[i].player_index = i;
    }

    // Step 2: Sort players by energy ascending
    qsort(players, MAX_PLAYERS, sizeof(PlayerInfo), compare_players);

    // Step 3: Send new position to each player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        int idx = players[i].player_index;
        int new_position = i;
        write(pipe_to_child[idx][1], &new_position, sizeof(int));
    }
}

// ------------------ PLAYER PROCESS FUNCTION ------------------

int rand_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void player_process(int read_fd, int write_fd, const char* config_file) {
    int energy, min_energy, max_energy;
    int min_decrease, max_decrease;
    int min_recovery, max_recovery;
    int position;

    // Config
    FILE* file = fopen(config_file, "r");
    fscanf(file, "%d %d", &min_energy, &max_energy);
    fscanf(file, "%d %d", &min_decrease, &max_decrease);
    fscanf(file, "%d %d", &min_recovery, &max_recovery);
    fclose(file);

    srand(time(NULL) + getpid());
    energy = rand_range(min_energy, max_energy);

    // Send energy to referee
    write(write_fd, &energy, sizeof(int));

    // Receive new position from referee
    read(read_fd, &position, sizeof(int));
    printf("Player %d initial energy = %d, assigned position = %d\n", getpid(), energy, position);

    // Setup signal handlers
    void handle_round(int signum) {
        if (energy > 0) {
            int dec = rand_range(min_decrease, max_decrease);
            energy -= dec;
            if (energy < 0) energy = 0;
        }
    }

    void send_effort(int signum) {
        int effort = energy * (position + 1); // 1 to 4 multiplier
        write(write_fd, &effort, sizeof(int));
    }

    signal(SIGUSR1, handle_round);
    signal(SIGUSR2, send_effort);

    while (1) {
        pause();

        // Recovery
        if (energy == 0) {
            int delay = rand_range(min_recovery, max_recovery);
            sleep(delay);
            energy = rand_range(min_energy, max_energy);
        }
    }
}

// ------------------ MAIN ------------------

int main() {
    int pipe_to_child[MAX_PLAYERS][2];   // parent writes to child
    int pipe_from_child[MAX_PLAYERS][2]; // parent reads from child
    pid_t pids[MAX_PLAYERS];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        pipe(pipe_to_child[i]);
        pipe(pipe_from_child[i]);

        pid_t pid = fork();
        if (pid == 0) {
            // Child
            for (int j = 0; j < MAX_PLAYERS; j++) {
                if (j != i) {
                    close(pipe_to_child[j][0]);
                    close(pipe_to_child[j][1]);
                    close(pipe_from_child[j][0]);
                    close(pipe_from_child[j][1]);
                }
            }

            close(pipe_to_child[i][1]);   // child reads only
            close(pipe_from_child[i][0]); // child writes only

            dup2(pipe_to_child[i][0], 3);     // fd 3 = read_fd
            dup2(pipe_from_child[i][1], 4);   // fd 4 = write_fd
            close(pipe_to_child[i][0]);
            close(pipe_from_child[i][1]);

            player_process(3, 4, "config.txt");
            exit(0);
        } else {
            // Parent
            pids[i] = pid;
            close(pipe_to_child[i][0]);   // parent writes only
            close(pipe_from_child[i][1]); // parent reads only
        }
    }

    sleep(1);  // let players init

    assign_initial_positions(pipe_from_child, pipe_to_child);

    // Example game logic: signal all to play
    for (int i = 0; i < MAX_PLAYERS; i++) kill(pids[i], SIGUSR1);
    sleep(1);
    for (int i = 0; i < MAX_PLAYERS; i++) kill(pids[i], SIGUSR2);

    // Read effort
    for (int i = 0; i < MAX_PLAYERS; i++) {
        int effort;
        read(pipe_from_child[i][0], &effort, sizeof(int));
        printf("Player %d effort: %d\n", i, effort);
    }

    // Cleanup
    for (int i = 0; i < MAX_PLAYERS; i++) {
        kill(pids[i], SIGKILL);
        wait(NULL);
    }

    return 0;
}
