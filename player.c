#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define MAX_PLAYERS 4

int energy;
int min_energy, max_energy;
int min_decrease, max_decrease;
int min_recovery, max_recovery;
int position;  // 0 to 3
int pipe_fd;   // passed from parent via argv

// Read ranges from config
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d %d", &min_energy, &max_energy);
    fscanf(file, "%d %d", &min_decrease, &max_decrease);
    fscanf(file, "%d %d", &min_recovery, &max_recovery);
    fclose(file);
}

// Generate random int between [min, max]
int rand_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Energy decrease simulation
void handle_round(int signum) {
    if (energy > 0) {
        int dec = rand_range(min_decrease, max_decrease);
        energy -= dec;
        if (energy < 0) energy = 0;
    }
}

// Referee requests effort level
void send_effort(int signum) {
    int effort = energy * (position + 1); // 1 to 4 multiplier
    write(pipe_fd, &effort, sizeof(int));
}

// Fall and recovery
void check_fall_and_recover() {
    if (energy == 0) {
        int delay = rand_range(min_recovery, max_recovery);
        sleep(delay);
        energy = rand_range(min_energy, max_energy);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <position> <pipe_fd> <config_file>\n", argv[0]);
        return 1;
    }

    position = atoi(argv[1]);
    pipe_fd = atoi(argv[2]);
    const char* config_file = argv[3];

    srand(time(NULL) + getpid()); // better randomness

    read_config(config_file);
    energy = rand_range(min_energy, max_energy);

    signal(SIGUSR1, handle_round);  // Start pulling
    signal(SIGUSR2, send_effort);   // Send effort

    while (1) {
        pause();  // Wait for signal
        check_fall_and_recover();
    }

    return 0;
}


/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define MAX_PLAYERS 4

// Struct to hold player-related data
typedef struct {
    int energy;
    int position;
    int pipe_fd;
} Player;

Player player;

int min_energy, max_energy;
int min_decrease, max_decrease;
int min_recovery, max_recovery;

// Read config values from file
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d %d", &min_energy, &max_energy);
    fscanf(file, "%d %d", &min_decrease, &max_decrease);
    fscanf(file, "%d %d", &min_recovery, &max_recovery);
    fclose(file);
}

// Generate random value in range [min, max]
int rand_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Decrease energy every round
void handle_round(int signum) {
    if (player.energy > 0) {
        int dec = rand_range(min_decrease, max_decrease);
        player.energy -= dec;
        if (player.energy < 0) player.energy = 0;
    }
}

// Send effort to referee when requested
void send_effort(int signum) {
    int effort = player.energy * (player.position + 1);  // position multiplier: 1 to 4
    write(player.pipe_fd, &effort, sizeof(int));
}

// Simulate fall and recovery
void check_fall_and_recover() {
    if (player.energy == 0) {
        int delay = rand_range(min_recovery, max_recovery);
        sleep(delay);
        player.energy = rand_range(min_energy, max_energy);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <position> <pipe_fd> <config_file>\n", argv[0]);
        return 1;
    }

    player.position = atoi(argv[1]);
    player.pipe_fd = atoi(argv[2]);
    const char* config_file = argv[3];

    srand(time(NULL) + getpid());  // Unique randomness per process

    read_config(config_file);
    player.energy = rand_range(min_energy, max_energy);

    // Register signal handlers
    signal(SIGUSR1, handle_round);  // Decrease energy
    signal(SIGUSR2, send_effort);   // Send effort to referee

    while (1) {
        pause();  // Wait for signal
        check_fall_and_recover();
    }

    return 0;
}

*/
