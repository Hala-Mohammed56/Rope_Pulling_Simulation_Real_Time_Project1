#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

typedef struct {
    int energy;
    int position;
    int pipe_fd;
    int active;
    int recovering;
} Player;

Player player;
int min_energy, max_energy;
int min_decrease, max_decrease;
int min_recovery, max_recovery;
int win_threshold;
volatile sig_atomic_t terminate = 0;

void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d %d", &min_energy, &max_energy) != 2 ||
        fscanf(file, "%d %d", &min_decrease, &max_decrease) != 2 ||
        fscanf(file, "%d %d", &min_recovery, &max_recovery) != 2 ||
        fscanf(file, "%d", &win_threshold) != 1) {
        fprintf(stderr, "Error: Invalid config file format\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void handle_termination(int signum) {
    printf("Player %d: Terminating...\n", player.position);
    terminate = 1;
}

void handle_round(int signum) {
    signal(SIGUSR1, handle_round); // Re-establish handler

    if (!player.active) {
        printf("Player %d: Inactive (recovering)\n", player.position);
        return;
    }

    // 10% chance to fall
    if ((rand() % 10) == 0 && player.energy > 0) {
        player.energy = 0;
        player.active = 0;
        printf("Player %d: Randomly fell down!\n", player.position);
        return;
    }

    int dec = rand_range(min_decrease, max_decrease);
    player.energy = (player.energy - dec > 0) ? player.energy - dec : 0;

    printf("Player %d: Energy decreased by %d to %d\n",
           player.position, dec, player.energy);

    if (player.energy == 0) {
        player.active = 0;
        printf("Player %d: Has fallen!\n", player.position);
    }
}

void send_effort(int signum) {
    signal(SIGUSR2, send_effort); // Re-establish handler

    if (player.pipe_fd < 0) return;

    int position_factor;
    read(player.pipe_fd, &position_factor, sizeof(int));
    int effort = player.energy * position_factor;

    printf("Player %d: Sending effort %d (energy: %d * factor: %d)\n",
           player.position, effort, player.energy, position_factor);

    write(player.pipe_fd, &effort, sizeof(int));
}

void recover_player() {
    if (!player.active && !player.recovering) {
        player.recovering = 1;
        int delay = rand_range(min_recovery, max_recovery);
        printf("Player %d: Recovering in %d sec...\n", player.position, delay);

        sleep(delay);
        player.energy = rand_range(min_energy, max_energy);
        player.active = 1;
        player.recovering = 0;

        printf("Player %d: Recovered with energy %d\n", player.position, player.energy);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <position> <pipe_fd> <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialize player
    player.position = atoi(argv[1]);
    player.pipe_fd = atoi(argv[2]);
    player.active = 1;
    player.recovering = 0;
    srand(time(NULL) ^ (getpid() << 16));

    read_config(argv[3]);
    player.energy = rand_range(min_energy, max_energy);

    printf("Player %d: Started with energy %d\n", player.position, player.energy);

    // Signal handlers
    signal(SIGUSR1, handle_round);
    signal(SIGUSR2, send_effort);
    signal(SIGTERM, handle_termination);
    signal(SIGINT, handle_termination);

    // Main loop
    while (!terminate) {
        pause();
        if (!player.active && !player.recovering) {
            recover_player();
        }
    }

    close(player.pipe_fd);
    printf("Player %d: Exited\n", player.position);
    return EXIT_SUCCESS;
}
