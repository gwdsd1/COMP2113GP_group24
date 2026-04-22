#ifndef MAZE_H
#define MAZE_H
#include <string>
#include <vector>
#include <utility>

constexpr int ENEMY_COUNT = 4;

struct MazeState {
    int playerX;
    int playerY;
    int health;  // 玩家血量，初始15点

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

    int coins = 0;  // 玩家收集的金币数量
	int wallBreakers = 0;  // 玩家收集的破墙工具数量
    std::vector<std::pair<int, int>> brokenWalls;  // 记录所有被打碎的墙坐标
};

void startMaze();
void startMaze(MazeState& state, bool useSavedState);
bool saveMazeStateToFile(const std::string& filename, const MazeState& state);
bool loadMazeStateFromFile(const std::string& filename, MazeState& state);
std::string generateSaveFileName();
std::vector<std::string> getSaveFiles();

#endif
