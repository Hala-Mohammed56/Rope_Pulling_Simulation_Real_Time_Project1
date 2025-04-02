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

    if (fscanf(file, "%d %d", &min_energy, &max_energy) != 2 ||
        fscanf(file, "%d %d", &min_decrease, &max_decrease) != 2 ||
        fscanf(file, "%d %d", &min_recovery, &max_recovery) != 2 ||
        fscanf(file, "%d", &win_threshold) != 1) {
        fprintf(stderr, "Error: Invalid config file format\n");
        exit(EXIT_FAILURE);
    }
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
// On SIGUSR1, reduce energy or make player fall
// ##################################
void handle_round(int signum) {
    signal(SIGUSR1, handle_round);  // Reinstall handler

    if (!player.active) return;

    // 10% chance to fall instantly
    if ((rand() % 10) == 0 && player.energy > 0) {
        player.energy = 0;
        player.active = 0;
    } else {
        int dec = rand_range(min_decrease, max_decrease);
        player.energy = (player.energy - dec > 0) ? player.energy - dec : 0;
        if (player.energy == 0) player.active = 0;
    }

    // Send current energy to referee
    PlayerStats energy_update;
    energy_update.player_id = player.player_id;
    energy_update.energy = player.energy;
    write(player.write_fd, &energy_update, sizeof(PlayerStats));
}

// ##################################
// On SIGUSR2, calculate and send effort
// ##################################
void send_effort(int signum) {
    signal(SIGUSR2, send_effort);  // Reinstall handler

    if (player.read_fd < 0 || player.write_fd < 0) return;

    // Get assigned position from referee
    int position_factor;
    read(player.read_fd, &position_factor, sizeof(int));
    player.position = position_factor;

    // Calculate and send effort
    PlayerStats stats;
    stats.player_id = player.player_id;
    stats.position = position_factor;
    stats.energy = player.energy;
    stats.effort = player.energy * position_factor;

    write(player.write_fd, &stats, sizeof(PlayerStats));
}

// ##################################
// Recover energy after being inactive
// ##################################
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


// ##################################
// Main loop for the player process
// ##################################
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <position> <read_fd> <write_fd> <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialize player settings
    player.player_id = atoi(argv[1]);
    player.read_fd = atoi(argv[2]);
    player.write_fd = atoi(argv[3]);
    player.active = 1;
    player.recovering = 0;
    srand(time(NULL) ^ (getpid() << 16));

    read_config(argv[4]);

    // Give player a random sorted energy
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

    // Setup signal handlers
    signal(SIGUSR1, handle_round);
    signal(SIGUSR2, send_effort);
    signal(SIGTERM, handle_termination);
    signal(SIGINT, handle_termination);

    // Wait for signals and recover if needed
    while (!terminate) {
        pause();
        if (!player.active && !player.recovering) {
            recover_player();
        }
    }

    // Close pipes and exit
    close(player.read_fd);
    close(player.write_fd);
    return EXIT_SUCCESS;
}
