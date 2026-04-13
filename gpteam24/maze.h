#ifndef MAZE_H
#define MAZE_H
#include <string>
#include <vector>

constexpr int ENEMY_COUNT = 4;

struct MazeState{
    int playerX;
    int playerY;

    int noteX[3];
    int noteY[3];

    int shooterX[3];
    int shooterY[3];

    int snakeX[3];
    int snakeY[3];

    // 新增：怪物
    int enemyX[ENEMY_COUNT];
    int enemyY[ENEMY_COUNT];

    // 每只怪物的活动矩形范围
    int enemyMinX[ENEMY_COUNT];
    int enemyMaxX[ENEMY_COUNT];
    int enemyMinY[ENEMY_COUNT];
    int enemyMaxY[ENEMY_COUNT];

    // 巡逻方向：-1 表示向左，1 表示向右
    int enemyDir[ENEMY_COUNT];
};

void startMaze();
void startMaze(MazeState& state, bool useSavedState);
bool saveMazeStateToFile(const std::string& filename, const MazeState& state);
bool loadMazeStateFromFile(const std::string& filename, MazeState& state);
std::string generateSaveFileName();
std::vector<std::string> getSaveFiles();

#endif