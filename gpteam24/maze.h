#ifndef MAZE_H
#define MAZE_H
#include <string>

struct MazeState{
    int playerX;
    int playerY;
    int noteX[3];
    int noteY[3];
    int shooterX[3];
    int shooterY[3];
};
void startMaze();
void startMaze(MazeState& state, bool useSavedState);
bool saveMazeStateToFile(const std::string& filename, const MazeState& state);
bool loadMazeStateFromFile(const std::string& filename, MazeState& state);

#endif // MAZE_H
