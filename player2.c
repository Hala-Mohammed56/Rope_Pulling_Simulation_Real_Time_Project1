#include "header.h"
#include "structs.h"

// ##################################
// Stores player status and config values used during the game
// ##################################
Player player;
int min_energy, max_energy;
int min_decrease, max_decrease;
int min_recovery, max_recovery;
int win_threshold;
volatile sig_atomic_t terminate = 0;

// ##################################
// Loads energy, decrease, and recovery settings from config
// ##################################
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    char line[100];

    // Read energy range
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &min_energy, &max_energy) != 2) {
        fprintf(stderr, "Error: Invalid energy range in config\n"); exit(EXIT_FAILURE);
    }

    // Read decrease range
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &min_decrease, &max_decrease) != 2) {
        fprintf(stderr, "Error: Invalid decrease range in config\n"); exit(EXIT_FAILURE);
    }

    // Read recovery range
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d %d", &min_recovery, &max_recovery) != 2) {
        fprintf(stderr, "Error: Invalid recovery range in config\n"); exit(EXIT_FAILURE);
    }

    // Read win threshold
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d", &win_threshold) != 1) {
        fprintf(stderr, "Error: Invalid win threshold in config\n"); exit(EXIT_FAILURE);
    }

    // Skip game duration and rounds to win
    fgets(line, sizeof(line), file);
    fgets(line, sizeof(line), file);

    fclose(file);
}

// Generates a random number between min and max
int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Handles exit signals to shut down cleanly
void handle_termination(int signum) {
    terminate = 1;
}

// ##################################
// On SIGUSR1: reduce energy by random amount from config
// ##################################
void handle_round(int signum) {
    signal(SIGUSR1, handle_round);  // Reinstall handler

    if (!player.active) {
        // Try to recover
        int recover = rand_range(min_recovery, max_recovery);
        player.energy += recover;
        if (player.energy > 0) {
            player.active = 1;
        }
    } else {
        // Random energy decrease
        int dec = rand_range(min_decrease, max_decrease);
        player.energy -= dec;
        if (player.energy <= 0) {
            player.energy = 0;
            player.active = 0;
        }
    }

    // Send updated energy to referee
    PlayerStats energy_update;
    energy_update.player_id = player.player_id;
    energy_update.energy = player.energy;
    write(player.write_fd, &energy_update, sizeof(PlayerStats));
}

// ##################################
// On SIGUSR2: calculate and send effort
// ##################################
void send_effort(int signum) {
    signal(SIGUSR2, send_effort);  // Reinstall handler

    if (player.read_fd < 0 || player.write_fd < 0) return;

    int position_factor;
    read(player.read_fd, &position_factor, sizeof(int));
    player.position = position_factor;

    PlayerStats stats;
    stats.player_id = player.player_id;
    stats.position = position_factor;
    stats.energy = player.energy;
    stats.effort = (player.active) ? player.energy * position_factor : 0;

    write(player.write_fd, &stats, sizeof(PlayerStats));
}

// ##################################
// Main function
// ##################################
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <position> <read_fd> <write_fd> <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialize player
    player.player_id = atoi(argv[1]);
    player.read_fd = atoi(argv[2]);
    player.write_fd = atoi(argv[3]);
    player.active = 1;
    player.recovering = 0;

    srand(time(NULL) ^ (getpid() << 16));
    read_config(argv[4]);

    // Assign sorted random energy based on ID
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

    // Handle signals
    signal(SIGUSR1, handle_round);
    signal(SIGUSR2, send_effort);
    signal(SIGTERM, handle_termination);
    signal(SIGINT, handle_termination);

    while (!terminate) {
        pause();
    }

    close(player.read_fd);
    close(player.write_fd);
    return EXIT_SUCCESS;
}
