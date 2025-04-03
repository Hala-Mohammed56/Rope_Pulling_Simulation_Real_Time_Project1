#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Global variables for game state
float rope_offset = 0.0f; // Offset for the rope position
float player_offset = 0.0f; // Offset for player positions

int team1_score = 0; // Score for Team 1
int team2_score = 0; // Score for Team 2
int current_round = 1; // Current round number
int round_winner = 0; // Winner of the current round (0 = tie, 1 = Team 1, 2 = Team 2)
int game_over = 0; // Flag to indicate if the game is over
int final_winner = 0; // Final winner of the game (0 = tie, 1 = Team 1, 2 = Team 2)
int previous_winner = 0; // Tracks the winner of the previous round
int consecutive_wins = 0; // Tracks consecutive wins by a team

int team1_effort = 0; // Total effort of Team 1
int team2_effort = 0; // Total effort of Team 2
int player_effort[8] = {90, 85, 88, 92, 91, 87, 86, 89}; // Effort values for each player

// Player positions (x-coordinates for Team 1 and Team 2)
float team1_x[4] = {350, 400, 450, 500}; // Team 1 positions (left side)
float team2_x[4] = {550, 600, 650, 700}; // Team 2 positions (right side)
float team_y[2] = {150, 150}; // Y-coordinate for both teams

int flash_toggle = 0; // Toggle for flashing effect when the game ends
int flash_count = 0; // Counter for flashing effect

// Function prototypes (declarations)
void flash_timer(int value);
void next_round(int value);

// Function to draw centered text
void draw_centered_text(float x, float y, const char* str) {
    void* font = GLUT_BITMAP_HELVETICA_18;
    float width = 0;
    for (int i = 0; i < strlen(str); i++)
        width += glutBitmapWidth(font, str[i]);
    glColor3f(0, 0, 0);
    glRasterPos2f(x - width / 2, y);
    for (int i = 0; i < strlen(str); i++)
        glutBitmapCharacter(font, str[i]);
}

// Function to update the display based on current game state
void update_display() {
    team1_effort = team2_effort = 0;
    for (int i = 0; i < 4; i++) {
        team1_effort += player_effort[i]; // Calculate total effort for Team 1
        team2_effort += player_effort[i + 4]; // Calculate total effort for Team 2
    }
    glutPostRedisplay(); // Trigger redrawing
}

// Function to move the rope and players based on the round result
void move_rope(int delta) {
    rope_offset += delta;
    player_offset += delta / 2.0f;
    if (rope_offset > 200) rope_offset = 200; // Limit maximum offset
    if (rope_offset < -200) rope_offset = -200; // Limit minimum offset
    if (player_offset > 100) player_offset = 100; // Limit player offset
    if (player_offset < -100) player_offset = -100;
    glutPostRedisplay(); // Trigger redrawing
}

// Function to end the game and display the final result
void end_game(int winner) {
    game_over = 1; // Set game over flag
    final_winner = winner; // Set final winner
    flash_toggle = flash_count = 0; // Reset flashing effect
    glutTimerFunc(0, flash_timer, 0); // Start flashing effect
}

// Timer function for flashing effect
void flash_timer(int value) {
    if (!game_over) return; // Exit if the game is not over
    flash_toggle = !flash_toggle; // Toggle flashing effect
    flash_count++;
    glutPostRedisplay(); // Trigger redrawing
    if (flash_count < 10) // Continue flashing for 10 cycles
        glutTimerFunc(200, flash_timer, 0);
}

// Simulate a new round by assigning random effort values to players
void simulate_round(int value) {
    for (int i = 0; i < 8; i++)
        player_effort[i] = rand() % 21 + 80; // Random effort between 80 and 100
    update_display();
    glutTimerFunc(1000, next_round, 0); // Move to the next round after 1 second
}

// Function to process the result of the current round
void next_round(int value) {
    team1_effort = team2_effort = 0;
    for (int i = 0; i < 4; i++) {
        team1_effort += player_effort[i]; // Calculate total effort for Team 1
        team2_effort += player_effort[i + 4]; // Calculate total effort for Team 2
    }

    // Determine the winner of the round
    if (team1_effort > team2_effort) {
        round_winner = 1; // Team 1 wins the round
        team1_score++;
    } else if (team2_effort > team1_effort) {
        round_winner = 2; // Team 2 wins the round
        team2_score++;
    } else {
        round_winner = 0; // Tie
    }

    // Check for consecutive wins
    if (round_winner == previous_winner && round_winner != 0) {
        consecutive_wins++;
    } else {
        consecutive_wins = 1;
    }
    previous_winner = round_winner;

    // Move the rope based on the round result
    move_rope((round_winner == 1) ? -20 : (round_winner == 2) ? 20 : 0);

    // Check if the game should end
    if (consecutive_wins >= 2 || current_round >= 4) {
        final_winner = (team1_score > team2_score) ? 1 : (team2_score > team1_score) ? 2 : 0;
        end_game(final_winner);
    } else {
        current_round++; // Move to the next round
        glutTimerFunc(2000, simulate_round, 0); // Start a new round after 2 seconds
    }
}

// Function to draw a stickman player
void draw_stickman_player(float x, float y, float r, float g, float b, int effort) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 100; i++) {
        float theta = 2.0f * M_PI * i / 100;
        glVertex2f(x + 10 * cos(theta), y + 30 + 10 * sin(theta));
    }
    glEnd();

    glBegin(GL_LINES);
        glVertex2f(x, y + 20); glVertex2f(x, y - 20); // Body
        glVertex2f(x, y + 10); glVertex2f(x - 15, y); // Left arm
        glVertex2f(x, y + 10); glVertex2f(x + 15, y); // Right arm
        glVertex2f(x, y - 20); glVertex2f(x - 10, y - 40); // Left leg
        glVertex2f(x, y - 20); glVertex2f(x + 10, y - 40); // Right leg
    glEnd();

    char str[10];
    sprintf(str, "%d", effort); // Display effort value above the player
    draw_centered_text(x, y + 50, str);
}

// Function to draw the rope
void draw_rope() {
    glColor3f(0.5f, 0.3f, 0.1f); // Brown color for the rope
    glBegin(GL_LINES);
        glVertex2f(300 + rope_offset, 140); // Start of the rope (left side) - Lowered slightly
        glVertex2f(700 + rope_offset, 140); // End of the rope (right side) - Lowered slightly
    glEnd();
}

// Function to draw the ground
void draw_ground() {
    glColor3f(0.8f, 0.8f, 0.8f); // Gray color for the ground
    glBegin(GL_POLYGON);
        glVertex2f(0, 110); glVertex2f(1000, 110); // Top edge
        glVertex2f(1000, 100); glVertex2f(0, 100); // Bottom edge
    glEnd();
}

// Main display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
    draw_ground(); // Draw the ground

    if (game_over) {
        // Flashing effect when the game is over
        glColor3f(flash_toggle ? 1.0f : 0.6f, 1.0f, flash_toggle ? 0.6f : 0.6f);
        glBegin(GL_POLYGON);
            glVertex2f(0, 0); glVertex2f(500, 0); glVertex2f(500, 700); glVertex2f(0, 700);
        glEnd();

        glColor3f(1.0f, flash_toggle ? 0.8f : 0.6f, flash_toggle ? 0.8f : 0.6f);
        glBegin(GL_POLYGON);
            glVertex2f(500, 0); glVertex2f(1000, 0); glVertex2f(1000, 700); glVertex2f(500, 700);
        glEnd();

        // Display win/lose messages
        const char* win_text = (final_winner == 1) ? "Team 1 WINS!" : (final_winner == 2) ? "Team 2 WINS!" : "It's a TIE!";
        const char* lose_text = (final_winner == 1) ? "Team 2 LOSES" : (final_winner == 2) ? "Team 1 LOSES" : "Both TIED";
        draw_centered_text(250, 350, win_text);
        draw_centered_text(750, 350, lose_text);
        glFlush();
        return;
    }

    // Display game information
    char buffer[100];
    sprintf(buffer, "Round: %d", current_round);
    draw_centered_text(500, 650, buffer);
    sprintf(buffer, "Winner: %s", round_winner == 1 ? "Team 1" : round_winner == 2 ? "Team 2" : "---");
    draw_centered_text(500, 620, buffer);
    sprintf(buffer, "Score – Team 1: %d | Team 2: %d", team1_score, team2_score);
    draw_centered_text(500, 590, buffer);

    sprintf(buffer, "Team 1 Effort: %d", team1_effort);
    draw_centered_text(250 + player_offset, 550, buffer);
    sprintf(buffer, "Team 2 Effort: %d", team2_effort);
    draw_centered_text(750 + player_offset, 550, buffer);

    draw_rope(); // Draw the rope

    // Draw players for Team 1
    for (int i = 0; i < 4; i++) {
        int e = player_effort[i];
        draw_stickman_player(team1_x[i] + player_offset, team_y[0], e == 0 ? 0.5 : 0.2, e == 0 ? 0.5 : 0.4, e == 0 ? 0.5 : 1.0, e);
    }

    // Draw players for Team 2
    for (int i = 0; i < 4; i++) {
        int e = player_effort[i + 4];
        draw_stickman_player(team2_x[i] + player_offset, team_y[1], e == 0 ? 0.5 : 1.0, e == 0 ? 0.5 : 0.3, e == 0 ? 0.5 : 0.3, e);
    }

    glFlush(); // Render everything
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); // Single buffer mode with RGB colors
    glutInitWindowSize(1000, 700); // Window size
    glutInitWindowPosition(100, 100); // Window position
    glutCreateWindow("Rope Pulling Game"); // Create the window

    gluOrtho2D(0, 1000, 0, 700); // Set orthographic projection
    glClearColor(1, 1, 1, 1); // Set background color to white
    glutDisplayFunc(display); // Set display function

    glutTimerFunc(1000, simulate_round, 0); // Start the first round
    glutMainLoop(); // Enter the main loop
    return 0;
}
