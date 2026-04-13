#include "maze.h"
#include "music_game.h"
#include "shooter_game.h"
#include "snake_game.h"           // ← 新增
#include "MusicManager.h"
#include "enemy_quiz.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <time.h>
#include <fstream>
#include <ctime>
#include <sstream>
#include <cctype>


#if defined(_WIN32)
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif
#include <thread>
#include <chrono>

// --- Cross Platform Console Helpers ---
namespace console {
    inline void setColor(int code) {
#if defined(_WIN32)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (code == 1) SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    else if (code == 2) SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (code == 3) SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    else if (code == 4) SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (code == 5) SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    else SetConsoleTextAttribute(hConsole, 7);
#else
    if (code == 1) std::cout << "\x1b[94m";
    else if (code == 2) std::cout << "\x1b[93m";
    else if (code == 3) std::cout << "\x1b[91m";
    else if (code == 4) std::cout << "\x1b[92m";
    else if (code == 5) std::cout << "\x1b[95m";
    else std::cout << "\x1b[0m";
#endif
}

    inline void setPos(int x, int y) {
#if defined(_WIN32)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD pos = { (short)x, (short)y };
        SetConsoleCursorPosition(hConsole, pos);
#else
        std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H";
#endif
    }

    inline void clear() {
#if defined(_WIN32)
        system("cls");
#else
        std::cout << "\x1b[2J\x1b[H";
#endif
    }

    inline char getInput() {
        char lastChar = 0;
#if defined(_WIN32)
        while (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) { _getch(); }
            else lastChar = (char)std::toupper(ch);
        }
#else
        char buf[64];
        ssize_t n;
        while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < n; i++) {
                if (buf[i] == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
                    if (buf[i + 2] == 'A') lastChar = 'W';
                    else if (buf[i + 2] == 'B') lastChar = 'S';
                    else if (buf[i + 2] == 'C') lastChar = 'D';
                    else if (buf[i + 2] == 'D') lastChar = 'A';
                    i += 2;
                }
                else if (isalpha(buf[i])) {
                    lastChar = (char)std::toupper(buf[i]);
                }
            }
        }
#endif
        return lastChar;
    }

    inline void clearInputBuffer() {
        getInput();
    }

    inline void sleep(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

#if !defined(_WIN32)
    struct LinuxTermGuard {
        termios orig;
        bool active = false;
        int origFlags = 0;
        LinuxTermGuard() {
            if (!isatty(STDIN_FILENO)) return;
            tcgetattr(STDIN_FILENO, &orig);
            termios raw = orig;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            origFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, origFlags | O_NONBLOCK);
            active = true;
        }
        ~LinuxTermGuard() {
            if (active) {
                tcsetattr(STDIN_FILENO, TCSANOW, &orig);
                fcntl(STDIN_FILENO, F_SETFL, origFlags);
            }
        }
    };
#endif
}

#include <vector>
#include <filesystem>
#include <algorithm>

using namespace std;

string generateSaveFileName() {
    time_t now = time(0);
    tm localTime;
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    stringstream ss;
    ss << "save_"
        << (localTime.tm_year + 1900) << "_";

    if (localTime.tm_mon + 1 < 10) ss << "0";
    ss << (localTime.tm_mon + 1) << "_";

    if (localTime.tm_mday < 10) ss << "0";
    ss << localTime.tm_mday << "_";

    if (localTime.tm_hour < 10) ss << "0";
    ss << localTime.tm_hour << "_";

    if (localTime.tm_min < 10) ss << "0";
    ss << localTime.tm_min << "_";

    if (localTime.tm_sec < 10) ss << "0";
    ss << localTime.tm_sec << ".txt";

    return ss.str();
}

vector<string> getSaveFiles() {
    vector<string> saveFiles;

    for (const auto& entry : filesystem::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            string filename = entry.path().filename().string();

            if (filename.rfind("save_", 0) == 0 &&
                filename.size() >= 9 &&
                filename.substr(filename.size() - 4) == ".txt") {
                saveFiles.push_back(filename);
            }
        }
    }
    sort(saveFiles.begin(), saveFiles.end());
    return saveFiles;
}



bool saveMazeStateToFile(const string& filename, const MazeState& state) {
    ofstream fout(filename);
    if (!fout) {
        return false;
    }

    fout << state.playerX << ' ' << state.playerY << '\n';

    for (int i = 0; i < 3; i++) {
        fout << state.noteX[i] << ' ' << state.noteY[i] << '\n';
    }

    for (int i = 0; i < 3; i++) {
        fout << state.shooterX[i] << ' ' << state.shooterY[i] << '\n';
    }

    // ← 新增：保存贪吃蛇坐标
    for (int i = 0; i < 3; i++) {
        fout << state.snakeX[i] << ' ' << state.snakeY[i] << '\n';
    }
    
    for (int i = 0; i < ENEMY_COUNT; i++) {
    fout << state.enemyX[i] << ' ' << state.enemyY[i] << '\n';
    }
    for (int i = 0; i < ENEMY_COUNT; i++) {
    fout << state.enemyMinX[i] << ' ' << state.enemyMaxX[i] << ' '
         << state.enemyMinY[i] << ' ' << state.enemyMaxY[i] << ' '
         << state.enemyDir[i] << '\n';
    }

    fout.close();
    return true;
}

bool isBlockedByStaticObjects(const MazeState& state, int x, int y);
bool isBlockedByOtherEnemies(const MazeState& state, int x, int y, int ignoreIdx);

void initEnemies(MazeState& state, const string maze[], int W, int H) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        do {
            state.enemyX[i] = rand() % W;
            state.enemyY[i] = rand() % H;
        } while (
            maze[state.enemyY[i]][state.enemyX[i]] == '#' ||
            (state.enemyX[i] == state.playerX && state.enemyY[i] == state.playerY) ||
            isBlockedByStaticObjects(state, state.enemyX[i], state.enemyY[i]) ||
            isBlockedByOtherEnemies(state, state.enemyX[i], state.enemyY[i], i)
        );

        state.enemyMinX[i] = max(1, state.enemyX[i] - 3);
        state.enemyMaxX[i] = min(W - 2, state.enemyX[i] + 3);
        state.enemyMinY[i] = max(1, state.enemyY[i] - 2);
        state.enemyMaxY[i] = min(H - 2, state.enemyY[i] + 2);

        state.enemyDir[i] = (i % 2 == 0) ? 1 : -1;
    }
}


void resetOneEnemy(MazeState& state, const string maze[], int W, int H, int idx) {
    do {
        state.enemyX[idx] = rand() % W;
        state.enemyY[idx] = rand() % H;
    } while (
        maze[state.enemyY[idx]][state.enemyX[idx]] == '#' ||
        (state.enemyX[idx] == state.playerX && state.enemyY[idx] == state.playerY) ||
        isBlockedByStaticObjects(state, state.enemyX[idx], state.enemyY[idx]) ||
        isBlockedByOtherEnemies(state, state.enemyX[idx], state.enemyY[idx], idx)
    );

    state.enemyMinX[idx] = max(1, state.enemyX[idx] - 3);
    state.enemyMaxX[idx] = min(W - 2, state.enemyX[idx] + 3);
    state.enemyMinY[idx] = max(1, state.enemyY[idx] - 2);
    state.enemyMaxY[idx] = min(H - 2, state.enemyY[idx] + 2);
    state.enemyDir[idx] = 1;
}


//判断格子能不能走
bool isInside(int x, int y, int W, int H) {
    return x >= 0 && x < W && y >= 0 && y < H;
}

bool isBlockedByStaticObjects(const MazeState& state, int x, int y) {
    for (int i = 0; i < 3; i++) {
        if (state.noteX[i] == x && state.noteY[i] == y) return true;
        if (state.shooterX[i] == x && state.shooterY[i] == y) return true;
        if (state.snakeX[i] == x && state.snakeY[i] == y) return true;
    }
    return false;
}

bool isBlockedByOtherEnemies(const MazeState& state, int x, int y, int ignoreIdx) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (i == ignoreIdx) continue;
        if (state.enemyX[i] == x && state.enemyY[i] == y) return true;
    }
    return false;
}

bool canEnemyStandAt(const string maze[], int W, int H,
                     const MazeState& state, int x, int y, int ignoreIdx) {
    if (!isInside(x, y, W, H)) return false;
    if (maze[y][x] == '#') return false;
    if (isBlockedByStaticObjects(state, x, y)) return false;
    if (isBlockedByOtherEnemies(state, x, y, ignoreIdx)) return false;
    return true;
}

bool playerInEnemyZone(const MazeState& state, int i) {
    return state.playerX >= state.enemyMinX[i] && state.playerX <= state.enemyMaxX[i] &&
           state.playerY >= state.enemyMinY[i] && state.playerY <= state.enemyMaxY[i];
}

void updateEnemies(MazeState& state, const string maze[], int W, int H) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        int ex = state.enemyX[i];
        int ey = state.enemyY[i];

        int dist = abs(state.playerX - ex) + abs(state.playerY - ey);
        bool shouldChase = playerInEnemyZone(state, i) && dist <= 5;

        if (shouldChase) {
            int nx = ex;
            int ny = ey;

            // 先尝试横向追
            if (state.playerX < ex) nx--;
            else if (state.playerX > ex) nx++;

            bool moved = false;
            if (nx >= state.enemyMinX[i] && nx <= state.enemyMaxX[i] &&
                canEnemyStandAt(maze, W, H, state, nx, ey, i) &&
                !(nx == state.playerX && ey == state.playerY)) {
                state.enemyX[i] = nx;
                moved = true;
            }

            // 横向不行再尝试纵向
            if (!moved) {
                nx = ex;
                ny = ey;
                if (state.playerY < ey) ny--;
                else if (state.playerY > ey) ny++;

                if (ny >= state.enemyMinY[i] && ny <= state.enemyMaxY[i] &&
                    canEnemyStandAt(maze, W, H, state, ex, ny, i) &&
                    !(ex == state.playerX && ny == state.playerY)) {
                    state.enemyY[i] = ny;
                }
            }
        } else {
            // 慢速水平巡逻
            int nx = ex + state.enemyDir[i];

            if (nx < state.enemyMinX[i] || nx > state.enemyMaxX[i] ||
                !canEnemyStandAt(maze, W, H, state, nx, ey, i)) {
                state.enemyDir[i] *= -1;
                nx = ex + state.enemyDir[i];

                if (nx >= state.enemyMinX[i] && nx <= state.enemyMaxX[i] &&
                    canEnemyStandAt(maze, W, H, state, nx, ey, i)) {
                    state.enemyX[i] = nx;
                }
            } else {
                state.enemyX[i] = nx;
            }
        }
    }
}

int findTriggeredEnemy(const MazeState& state) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        int dist = abs(state.playerX - state.enemyX[i]) + abs(state.playerY - state.enemyY[i]);
        if (dist <= 1) {
            return i;
        }
    }
    return -1;
}


bool loadMazeStateFromFile(const string& filename, MazeState& state) {
    ifstream fin(filename);
    if (!fin) {
        return false;
    }

    fin >> state.playerX >> state.playerY;

    for (int i = 0; i < 3; i++) {
        fin >> state.noteX[i] >> state.noteY[i];
    }

    for (int i = 0; i < 3; i++) {
        fin >> state.shooterX[i] >> state.shooterY[i];
    }

    // 兼容旧存档
    for (int i = 0; i < 3; i++) {
        if (!(fin >> state.snakeX[i] >> state.snakeY[i])) {
            state.snakeX[i] = -1;
            state.snakeY[i] = -1;
        }
    }

    // 兼容旧存档
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (!(fin >> state.enemyX[i] >> state.enemyY[i])) {
            state.enemyX[i] = -1;
            state.enemyY[i] = -1;
        }
    }

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (!(fin >> state.enemyMinX[i] >> state.enemyMaxX[i]
                  >> state.enemyMinY[i] >> state.enemyMaxY[i]
                  >> state.enemyDir[i])) {
            state.enemyMinX[i] = -1;
            state.enemyMaxX[i] = -1;
            state.enemyMinY[i] = -1;
            state.enemyMaxY[i] = -1;
            state.enemyDir[i] = 1;
        }
    }

    fin.close();
    return true;
}

void drawMazeFrame(const MazeState& state, const string maze[], int W, int H, bool nearNote) {
    console::clear();
    cout << "Use W/A/S/D to move. Press Q to quit maze.";
    if (nearNote) cout << "  Press E to interact.";
    cout << "\n\n";

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (x == state.playerX && y == state.playerY) {
                console::setColor(1);
                cout << '@';
                console::setColor(0);
                continue;
            }

            bool isEnemy = false;
            for (int i = 0; i < ENEMY_COUNT; i++) {
                if (state.enemyX[i] == x && state.enemyY[i] == y) {
                    isEnemy = true;
                    break;
                }
            }
            if (isEnemy) {
                console::setColor(5);
                cout << 'G';
                console::setColor(0);
                continue;
            }

            bool isNote = false;
            for (int i = 0; i < 3; i++) {
                if (state.noteX[i] == x && state.noteY[i] == y) {
                    isNote = true;
                    break;
                }
            }
            if (isNote) {
                console::setColor(2);
                cout << '&';
                console::setColor(0);
                continue;
            }

            bool isShooter = false;
            for (int i = 0; i < 3; i++) {
                if (state.shooterX[i] == x && state.shooterY[i] == y) {
                    isShooter = true;
                    break;
                }
            }
            if (isShooter) {
                console::setColor(3);
                cout << '!';
                console::setColor(0);
                continue;
            }

            bool isSnake = false;
            for (int i = 0; i < 3; i++) {
                if (state.snakeX[i] == x && state.snakeY[i] == y) {
                    isSnake = true;
                    break;
                }
            }
            if (isSnake) {
                console::setColor(4);
                cout << 'S';
                console::setColor(0);
                continue;
            }

            cout << (maze[y][x] == '#' ? '#' : ' ');
        }
        cout << '\n';
    }

    std::cout.flush();
}

void startMaze() {
    MazeState state;
    startMaze(state, false);
}

void startMaze(MazeState& state, bool useSavedState) {
    // 播放迷宫背景音乐
    MusicManager::playBackgroundMusic("music/maze_bg.mp3");

    const int MAZE_HEIGHT = 31;
    const int MAZE_WIDTH = 61;
    string maze[MAZE_HEIGHT] = {
        "#############################################################",
        "#.............................#.............................#",
        "#.#######.###################.#.###################.#######.#",
        "#.#.......#.................#.#.#.................#.......#.#",
        "#.#.#######.###############.#.#.#.###############.#######.#.#",
        "#.#.#.....#.......#.......#.#.#.#.#.......#.......#.....#.#.#",
        "#.#.#.###.#######.#.#####.#.#.#.#.#.#####.#.#######.###.#.#.#",
        "#.#.#.#.#.......#...#...#.#.#.#.#.#.#...#...#.......#.#.#.#.#",
        "#.#.#.#.#######.#####.#.#.#.#.#.#.#.#.#.#####.#######.#.#.#.#",
        "#.#.#.#.......#.......#.#.#.#.#.#.#.#.#.......#.......#.#.#.#",
        "#.#.#.#######.#########.#.#.#.#.#.#.#.#########.#######.#.#.#",
        "#.#.#.#.......#.........#.#.#.#.#.#.#.........#.......#.#.#.#",
        "#.#.#.#.#######.#########.#.#.#.#.#.#########.#######.#.#.#.#",
        "#.#.#.#.#.......#.........#.#.#.#.#.........#.......#.#.#.#.#",
        "#.#.#.#.#.#######.#########.#.#.#.#########.#######.#.#.#.#.#",
        "#.#.#.#.#.#.......#.........#.#.#.........#.......#.#.#.#.#.#",
        "#.#.#.#.#.#.#######.#########.#.#########.#######.#.#.#.#.#.#",
        "#.#.#.#.#.#.......#...........#...........#.......#.#.#.#.#.#",
        "#.###.#.#.#######.#########################.#######.#.#.###.#",
        "#.....#.#.......#.........................#.......#.#.....#.#",
        "#######.#######.###########################.#######.#######.#",
        "#.............#...........................#.............#...#",
        "#.###########.#############################.###########.#.#.#",
        "#.#.........#.............................#.........#...#.#.#",
        "#.#.#######.###############################.#######.#.###.#.#",
        "#.#.......#...............................#.......#.....#.#.#",
        "#.#######.#################################.#######.#####.#.#",
        "#.#.....#.................................#.....#.......#.#.#",
        "#.#.###.###################################.###.#.#####.#.#.#",
        "#...#.........................................#.........#...#",
        "#############################################################"
    };

    // 确保中心区域是连通的（主角初始位置）
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            maze[15 + dy][30 + dx] = '.';
        }
    }
    int& playerX = state.playerX;
    int& playerY = state.playerY;
    int* noteX = state.noteX;
    int* noteY = state.noteY;
    int* shooterX = state.shooterX;
    int* shooterY = state.shooterY;
    int* snakeX = state.snakeX;       // ← 新增
    int* snakeY = state.snakeY;       // ← 新增


    if (!useSavedState) {
        playerX = 30;
        playerY = 15;

        // 随机生成音符符号位置 (金黄色音符) 3个
        srand(static_cast<unsigned int>(time(0)));
        for (int i = 0; i < 3; i++) {
            do {
                noteX[i] = rand() % MAZE_WIDTH;
                noteY[i] = rand() % MAZE_HEIGHT;
            } while (maze[noteY[i]][noteX[i]] == '#' || (noteX[i] >= 29 && noteX[i] <= 31 && noteY[i] >= 14 && noteY[i] <= 16));
        }

        // 随机生成弹幕射击关卡符号位置 (鲜红醒目符号) 3个
        for (int i = 0; i < 3; i++) {
            do {
                shooterX[i] = rand() % MAZE_WIDTH;
                shooterY[i] = rand() % MAZE_HEIGHT;
            } while (maze[shooterY[i]][shooterX[i]] == '#' ||
                (shooterX[i] >= 29 && shooterX[i] <= 31 && shooterY[i] >= 14 && shooterY[i] <= 16) ||
                (shooterX[i] == noteX[0] && shooterY[i] == noteY[0]) ||
                (shooterX[i] == noteX[1] && shooterY[i] == noteY[1]) ||
                (shooterX[i] == noteX[2] && shooterY[i] == noteY[2]));
        }

        // ← 新增：随机生成贪吃蛇关卡符号位置 (绿色 S) 3个
        for (int i = 0; i < 3; i++) {
            do {
                snakeX[i] = rand() % MAZE_WIDTH;
                snakeY[i] = rand() % MAZE_HEIGHT;
            } while (maze[snakeY[i]][snakeX[i]] == '#' ||
                (snakeX[i] >= 29 && snakeX[i] <= 31 && snakeY[i] >= 14 && snakeY[i] <= 16) ||
                (snakeX[i] == noteX[0] && snakeY[i] == noteY[0]) ||
                (snakeX[i] == noteX[1] && snakeY[i] == noteY[1]) ||
                (snakeX[i] == noteX[2] && snakeY[i] == noteY[2]) ||
                (snakeX[i] == shooterX[0] && snakeY[i] == shooterY[0]) ||
                (snakeX[i] == shooterX[1] && snakeY[i] == shooterY[1]) ||
                (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2]));
        }
    initEnemies(state, maze, MAZE_WIDTH, MAZE_HEIGHT);
    } else {
    bool needInitEnemy = false;
for (int i = 0; i < ENEMY_COUNT; i++) {
    if (state.enemyX[i] == -1 || state.enemyY[i] == -1 ||
        state.enemyMinX[i] == -1 || state.enemyMaxX[i] == -1 ||
        state.enemyMinY[i] == -1 || state.enemyMaxY[i] == -1) {
        needInitEnemy = true;
        break;
    }
}
if (needInitEnemy) {
    initEnemies(state, maze, MAZE_WIDTH, MAZE_HEIGHT);
}
        // 如果是旧存档加载（snakeX/Y == -1），那么即使在加载存档模式下，也需重新生成贪吃蛇位置
        for (int i = 0; i < 3; i++) {
            if (snakeX[i] == -1 || snakeY[i] == -1) {
                do {
                    snakeX[i] = rand() % MAZE_WIDTH;
                    snakeY[i] = rand() % MAZE_HEIGHT;
                } while (maze[snakeY[i]][snakeX[i]] == '#' ||
                    (snakeX[i] >= 29 && snakeX[i] <= 31 && snakeY[i] >= 14 && snakeY[i] <= 16) ||
                    (snakeX[i] == noteX[0] && snakeY[i] == noteY[0]) ||
                    (snakeX[i] == noteX[1] && snakeY[i] == noteY[1]) ||
                    (snakeX[i] == noteX[2] && snakeY[i] == noteY[2]) ||
                    (snakeX[i] == shooterX[0] && snakeY[i] == shooterY[0]) ||
                    (snakeX[i] == shooterX[1] && snakeY[i] == shooterY[1]) ||
                    (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2]) || 
                    isBlockedByOtherEnemies(state, snakeX[i], snakeY[i], -1));
            }
        }
    }

    console::clear();
    cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";

#if !defined(_WIN32)
    console::LinuxTermGuard termGuard;
#endif

    bool nearNote = false;
    drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);

    bool inMaze = true;
    const int MOVE_DELAY_MS = 60;
    console::clearInputBuffer();
    
    int enemyTick = 0;
    const int ENEMY_MOVE_INTERVAL = 3; // 3 个玩家循环才动 1 次


    while (inMaze) {
        // ===================== 自动触发弹幕射击 =====================
        bool nearShooter = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - shooterX[i]) <= 1 && abs(playerY - shooterY[i]) <= 1) {
                nearShooter = true;
                break;
            }
        }

        if (nearShooter) {
            startShooterGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");


            for (int i = 0; i < 3; i++) {
                do {
                    shooterX[i] = rand() % MAZE_WIDTH;
                    shooterY[i] = rand() % MAZE_HEIGHT;
                } while (maze[shooterY[i]][shooterX[i]] == '#' ||
                    (shooterX[i] == playerX && shooterY[i] == playerY) ||
                    (shooterX[i] == noteX[0] && shooterY[i] == noteY[0]) ||
                    (shooterX[i] == noteX[1] && shooterY[i] == noteY[1]) ||
                    (shooterX[i] == noteX[2] && shooterY[i] == noteY[2]) ||
                    (shooterX[i] == snakeX[0] && shooterY[i] == snakeY[0]) ||
                    (shooterX[i] == snakeX[1] && shooterY[i] == snakeY[1]) ||
                    (shooterX[i] == snakeX[2] && shooterY[i] == snakeY[2])|| 
                    isBlockedByOtherEnemies(state, shooterX[i], shooterY[i], -1));
            }

            // ---- 重绘迷宫 ----
            console::clear();
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        console::setColor(1); cout << '@'; console::setColor(0);
                    }
                    else {
                        bool isNote = false;
                        for (int i = 0; i < 3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i = 0; i < 3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }
                        bool isSnake = false;
                        for (int i = 0; i < 3; i++) { if (x == snakeX[i] && y == snakeY[i]) isSnake = true; }

                        if (isNote) { console::setColor(2); cout << '&'; console::setColor(0); }
                        else if (isShooter) { console::setColor(3); cout << '!'; console::setColor(0); }
                        else if (isSnake) { console::setColor(4); cout << 'S'; console::setColor(0); }
                        else if (maze[y][x] == '#') { cout << '#'; }
                        else { cout << ' '; }
                    }
                }
                cout << '\n';
            }
            console::clearInputBuffer();
            continue;
        }

        // ===================== 自动触发贪吃蛇 =====================  ← 整段新增
        bool nearSnake = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - snakeX[i]) <= 1 && abs(playerY - snakeY[i]) <= 1) {
                nearSnake = true;
                break;
            }
        }

        if (nearSnake) {
            startSnakeGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");

            // 重新随机放置贪吃蛇入口
            for (int i = 0; i < 3; i++) {
                do {
                    snakeX[i] = rand() % MAZE_WIDTH;
                    snakeY[i] = rand() % MAZE_HEIGHT;
                } while (maze[snakeY[i]][snakeX[i]] == '#' ||
                    (snakeX[i] == playerX && snakeY[i] == playerY) ||
                    (snakeX[i] == noteX[0] && snakeY[i] == noteY[0]) ||
                    (snakeX[i] == noteX[1] && snakeY[i] == noteY[1]) ||
                    (snakeX[i] == noteX[2] && snakeY[i] == noteY[2]) ||
                    (snakeX[i] == shooterX[0] && snakeY[i] == shooterY[0]) ||
                    (snakeX[i] == shooterX[1] && snakeY[i] == shooterY[1]) ||
                    (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2])|| 
                    isBlockedByOtherEnemies(state, snakeX[i], snakeY[i], -1));
            }

            // 重绘迷宫
            console::clear();
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        console::setColor(1); cout << '@'; console::setColor(0);
                    }
                    else {
                        bool isNote = false;
                        for (int i = 0; i < 3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i = 0; i < 3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }
                        bool isSnake = false;
                        for (int i = 0; i < 3; i++) { if (x == snakeX[i] && y == snakeY[i]) isSnake = true; }

                        if (isNote) { console::setColor(2); cout << '&'; console::setColor(0); }
                        else if (isShooter) { console::setColor(3); cout << '!'; console::setColor(0); }
                        else if (isSnake) { console::setColor(4); cout << 'S'; console::setColor(0); }
                        else if (maze[y][x] == '#') { cout << '#'; }
                        else { cout << ' '; }
                    }
                }
                cout << '\n';
            }
            console::clearInputBuffer();
            continue;
        }
        // ===================== 贪吃蛇触发结束 =====================

        // 检查是否靠近音符
        bool nearNote = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - noteX[i]) <= 1 && abs(playerY - noteY[i]) <= 1) {
                nearNote = true;
                break;
            }
        }

        static bool lastNearNote = false;
        if (nearNote != lastNearNote) {
            console::setPos(0, MAZE_HEIGHT + 2);
            if (nearNote) {
                cout << "Press E to interact...                                  ";
            }
            else {
                cout << "                                                        ";
            }
            lastNearNote = nearNote;
        }

        int nextX = playerX;
        int nextY = playerY;
        bool tryMove = false;
        bool doInteract = false;
        bool doQuit = false;

#if defined(_WIN32)
        if (GetAsyncKeyState('W') & 0x8000) { nextY--; tryMove = true; }
        else if (GetAsyncKeyState('S') & 0x8000) { nextY++; tryMove = true; }
        else if (GetAsyncKeyState('A') & 0x8000) { nextX--; tryMove = true; }
        else if (GetAsyncKeyState('D') & 0x8000) { nextX++; tryMove = true; }
        else if (nearNote && (GetAsyncKeyState('E') & 0x8000)) { doInteract = true; }
        else if (GetAsyncKeyState('Q') & 0x8000) { doQuit = true; }
        console::clearInputBuffer();
#else
        char inKey = console::getInput();
        if (inKey == 'W') { nextY--; tryMove = true; }
        else if (inKey == 'S') { nextY++; tryMove = true; }
        else if (inKey == 'A') { nextX--; tryMove = true; }
        else if (inKey == 'D') { nextX++; tryMove = true; }
        else if (nearNote && inKey == 'E') { doInteract = true; }
        else if (inKey == 'Q') { doQuit = true; }
#endif

        if (doInteract) {
            console::sleep(200);
            startMusicGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");

            for (int i = 0; i < 3; i++) {
                do {
    noteX[i] = rand() % MAZE_WIDTH;
    noteY[i] = rand() % MAZE_HEIGHT;
} while (
    maze[noteY[i]][noteX[i]] == '#' ||
    (noteX[i] == playerX && noteY[i] == playerY) ||
    isBlockedByOtherEnemies(state, noteX[i], noteY[i], -1)
);
}
            console::clear();
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        console::setColor(1); cout << '@'; console::setColor(0);
                    }
                    else {
                        bool isNote = false;
                        for (int i = 0; i < 3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i = 0; i < 3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }
                        bool isSnake = false;                                  // ← 新增
                        for (int i = 0; i < 3; i++) { if (x == snakeX[i] && y == snakeY[i]) isSnake = true; }

                        if (isNote) { console::setColor(2); cout << '&'; console::setColor(0); }
                        else if (isShooter) { console::setColor(3); cout << '!'; console::setColor(0); }
                        else if (isSnake) { console::setColor(4); cout << 'S'; console::setColor(0); }
                        else if (maze[y][x] == '#') { cout << '#'; }
                        else { cout << ' '; }
                    }
                }
                cout << '\n';
            }
            console::clearInputBuffer();
            continue;
        }
        else if (doQuit) {
        console::setPos(0, MAZE_HEIGHT + 4);
        string filename = generateSaveFileName();
        if (saveMazeStateToFile(filename, state)) {
            cout << "Game saved to " << filename;
        }
        else {
            cout << "Failed to save game.";
        }
        console::sleep(500);
        inMaze = false;
        continue;
    }

    if (tryMove && nextX >= 0 && nextX < MAZE_WIDTH && nextY >= 0 && nextY < MAZE_HEIGHT) {
    if (maze[nextY][nextX] != '#') {
        playerX = nextX;
        playerY = nextY;
    }
}

enemyTick++;
if (enemyTick >= ENEMY_MOVE_INTERVAL) {
    updateEnemies(state, maze, MAZE_WIDTH, MAZE_HEIGHT);
    enemyTick = 0;
}

int hitEnemy = findTriggeredEnemy(state);
if (hitEnemy != -1) {
    bool win = startEnemyQuiz();
    MusicManager::playBackgroundMusic("music/maze_bg.mp3");

    // 把怪物重新放开
    resetOneEnemy(state, maze, MAZE_WIDTH, MAZE_HEIGHT, hitEnemy);

    console::clearInputBuffer();
}


    std::cout.flush();
    nearNote = false;
for (int i = 0; i < 3; i++) {
    if (abs(playerX - noteX[i]) <= 1 && abs(playerY - noteY[i]) <= 1) {
        nearNote = true;
        break;
    }
}

drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);
        console::sleep(MOVE_DELAY_MS);
    }

    console::setPos(0, MAZE_HEIGHT + 3);
    console::clearInputBuffer();
}
