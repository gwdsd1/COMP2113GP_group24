#ifndef MAZE_H
#define MAZE_H
#include <string>
#include <vector>
#include <utility>

constexpr int ENEMY_COUNT = 4;

struct MazeState {
    int playerX;
    int playerY;
    int health;  // Player health, initialized to 15.

    int noteX[3];
    int noteY[3];

    int shooterX[3];
    int shooterY[3];

    int snakeX[3];
    int snakeY[3];

    int enemyX[ENEMY_COUNT];
    int enemyY[ENEMY_COUNT];

    int enemyMinX[ENEMY_COUNT];
    int enemyMaxX[ENEMY_COUNT];
    int enemyMinY[ENEMY_COUNT];
    int enemyMaxY[ENEMY_COUNT];

    int enemyDir[ENEMY_COUNT];

    int coins = 0;  // Number of coins collected by the player.
	int wallBreakers = 0;  // Number of wall-breaker tools collected by the player.
    std::vector<std::pair<int, int>> brokenWalls;  // Coordinates of all walls that were broken.
};

// What it does: Starts a new maze run with default state.
// Inputs: None.
// Outputs: None.
void startMaze();

// What it does: Starts the maze loop using either a fresh or loaded state.
// Inputs: state is the current maze state; useSavedState indicates whether to use loaded data.
// Outputs: None.
void startMaze(MazeState& state, bool useSavedState);

// What it does: Saves the complete maze state to a save file.
// Inputs: filename is the target save file path; state is the data to save.
// Outputs: Returns true if saving succeeds, otherwise false.
bool saveMazeStateToFile(const std::string& filename, const MazeState& state);

// What it does: Loads maze state from a save file.
// Inputs: filename is the source save file path; state receives loaded data.
// Outputs: Returns true if loading succeeds, otherwise false.
bool loadMazeStateFromFile(const std::string& filename, MazeState& state);

// What it does: Generates a timestamp-based save file name.
// Inputs: None.
// Outputs: Returns a generated save file name string.
std::string generateSaveFileName();

// What it does: Lists all save files in the current directory.
// Inputs: None.
// Outputs: Returns a sorted list of save file names.
std::vector<std::string> getSaveFiles();

#endif

