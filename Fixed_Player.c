#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

typedef struct {
    int player_id;
    int position;
    int energy;
    int effort;
} PlayerStats;

typedef struct {
    int energy;
    int position;
    int read_fd;
    int write_fd;
    int active;
    int recovering;
    int player_id;
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
    terminate = 1;
}

void handle_round(int signum) {
    signal(SIGUSR1, handle_round);

    if (!player.active) return;

    if ((rand() % 10) == 0 && player.energy > 0) {
        player.energy = 0;
        player.active = 0;
    } else {
        int dec = rand_range(min_decrease, max_decrease);
        player.energy = (player.energy - dec > 0) ? player.energy - dec : 0;
        if (player.energy == 0) player.active = 0;
    }

    // Send updated energy to referee
    PlayerStats energy_update;
    energy_update.player_id = player.player_id;
    energy_update.energy = player.energy;
    write(player.write_fd, &energy_update, sizeof(PlayerStats));
}

void send_effort(int signum) {
    signal(SIGUSR2, send_effort);

    if (player.read_fd < 0 || player.write_fd < 0) return;

    int position_factor;
    read(player.read_fd, &position_factor, sizeof(int));
    player.position = position_factor;

    PlayerStats stats;
    stats.player_id = player.player_id;
    stats.position = position_factor;
    stats.energy = player.energy;
    stats.effort = player.energy * position_factor;

    write(player.write_fd, &stats, sizeof(PlayerStats));
}

void recover_player() {
    if (!player.active && !player.recovering) {
        player.recovering = 1;
        int delay = rand_range(min_recovery, max_recovery);
        sleep(delay);
        player.energy = rand_range(min_energy, max_energy);
        player.active = 1;
        player.recovering = 0;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <position> <read_fd> <write_fd> <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    player.player_id = atoi(argv[1]);
    player.read_fd = atoi(argv[2]);
    player.write_fd = atoi(argv[3]);
    player.active = 1;
    player.recovering = 0;
    srand(time(NULL) ^ (getpid() << 16));

    read_config(argv[4]);

    int energies[4];
    for (int i = 0; i < 4; i++) energies[i] = rand_range(min_energy, max_energy);
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (energies[i] > energies[j]) {
                int temp = energies[i];
                energies[i] = energies[j];
                energies[j] = temp;
            }
        }
    }
    player.energy = energies[player.player_id];

    signal(SIGUSR1, handle_round);
    signal(SIGUSR2, send_effort);
    signal(SIGTERM, handle_termination);
    signal(SIGINT, handle_termination);

    while (!terminate) {
        pause();
        if (!player.active && !player.recovering) {
            recover_player();
        }
    }

    close(player.read_fd);
    close(player.write_fd);
    return EXIT_SUCCESS;
}
