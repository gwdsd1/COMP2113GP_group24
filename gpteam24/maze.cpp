#include "maze.h"
#include "music_game.h"
#include "shooter_game.h"
#include "snake_game.h"
#include "MusicManager.h"
#include "enemy_quiz.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <cctype>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

using namespace std;

// 控制台相关工具：负责颜色、清屏、光标、跨平台输入
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

    inline void hideCursor() {
#if defined(_WIN32)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
        std::cout << "\x1b[?25l";
#endif
    }

    inline void showCursor() {
#if defined(_WIN32)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
        std::cout << "\x1b[?25h";
#endif
    }

    inline char getInput() {
        char lastChar = 0;
#if defined(_WIN32)
        while (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                _getch();
            } else {
                lastChar = (char)std::toupper((unsigned char)ch);
            }
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
                } else if (std::isalpha((unsigned char)buf[i])) {
                    lastChar = (char)std::toupper((unsigned char)buf[i]);
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

string generateSaveFileName() {
    time_t now = time(0);
    tm localTime;
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    stringstream ss;
    ss << "save_" << (localTime.tm_year + 1900) << "_";
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

// 把迷宫当前状态全部写入文件，供下次读档恢复
bool saveMazeStateToFile(const string& filename, const MazeState& state) {
    ofstream fout(filename);
    if (!fout) return false;

    fout << state.playerX << ' ' << state.playerY << '\n';
    for (int i = 0; i < 3; i++) fout << state.noteX[i] << ' ' << state.noteY[i] << '\n';
    for (int i = 0; i < 3; i++) fout << state.shooterX[i] << ' ' << state.shooterY[i] << '\n';
    for (int i = 0; i < 3; i++) fout << state.snakeX[i] << ' ' << state.snakeY[i] << '\n';
    for (int i = 0; i < ENEMY_COUNT; i++) fout << state.enemyX[i] << ' ' << state.enemyY[i] << '\n';
    for (int i = 0; i < ENEMY_COUNT; i++) {
        fout << state.enemyMinX[i] << ' ' << state.enemyMaxX[i] << ' '
             << state.enemyMinY[i] << ' ' << state.enemyMaxY[i] << ' '
             << state.enemyDir[i] << '\n';
    }
    fout << state.health << ' ' << state.coins << ' ' << state.wallBreakers << '\n';

    fout << state.brokenWalls.size() << '\n';
    for (const auto& p : state.brokenWalls) {
        fout << p.first << ' ' << p.second << '\n';
    }

    return true;
}

bool isBlockedByStaticObjects(const MazeState& state, int x, int y);
bool isBlockedByOtherEnemies(const MazeState& state, int x, int y, int ignoreIdx);

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

// 随机初始化 4 个怪物，并给每只怪物分配一个活动区域
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

// 打完问答小游戏后，把触发过的怪物重新放到别处
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

// 判断怪物是否能站在某一格：不能越界、不能穿墙、不能压住入口或别的怪物
bool canEnemyStandAt(const string maze[], int W, int H,
                     const MazeState& state, int x, int y, int ignoreIdx) {
    if (!isInside(x, y, W, H)) return false;
    if (maze[y][x] == '#') return false;
    if (isBlockedByStaticObjects(state, x, y)) return false;
    if (isBlockedByOtherEnemies(state, x, y, ignoreIdx)) return false;
    return true;
}

// 判断玩家是否处于第 i 只怪物的活动区域内
bool playerInEnemyZone(const MazeState& state, int i) {
    return state.playerX >= state.enemyMinX[i] && state.playerX <= state.enemyMaxX[i] &&
           state.playerY >= state.enemyMinY[i] && state.playerY <= state.enemyMaxY[i];
}

// 怪物逻辑：玩家靠近时追一步，否则在自己的区域内慢速水平巡逻
void updateEnemies(MazeState& state, const string maze[], int W, int H) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        int ex = state.enemyX[i];
        int ey = state.enemyY[i];
        int dist = abs(state.playerX - ex) + abs(state.playerY - ey);
        bool shouldChase = playerInEnemyZone(state, i) && dist <= 5;

        if (shouldChase) {
            int nx = ex;
            int ny = ey;

            if (state.playerX < ex) nx--;
            else if (state.playerX > ex) nx++;

            bool moved = false;
            if (nx >= state.enemyMinX[i] && nx <= state.enemyMaxX[i] &&
                canEnemyStandAt(maze, W, H, state, nx, ey, i) &&
                !(nx == state.playerX && ey == state.playerY)) {
                state.enemyX[i] = nx;
                moved = true;
            }

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

// 玩家贴近怪物时返回对应怪物编号，否则返回 -1
int findTriggeredEnemy(const MazeState& state) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        int dist = abs(state.playerX - state.enemyX[i]) + abs(state.playerY - state.enemyY[i]);
        if (dist <= 1) return i;
    }
    return -1;
}

bool loadMazeStateFromFile(const string& filename, MazeState& state) {
    ifstream fin(filename);
    if (!fin) return false;

    fin >> state.playerX >> state.playerY;
    for (int i = 0; i < 3; i++) fin >> state.noteX[i] >> state.noteY[i];
    for (int i = 0; i < 3; i++) fin >> state.shooterX[i] >> state.shooterY[i];

    for (int i = 0; i < 3; i++) {
        if (!(fin >> state.snakeX[i] >> state.snakeY[i])) {
            state.snakeX[i] = -1;
            state.snakeY[i] = -1;
        }
    }

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

    if (!(fin >> state.health >> state.coins >> state.wallBreakers)) {
        state.health = 15;
        state.coins = 0;
        state.wallBreakers = 0;
    }

    // 读破墙记录
    int numBroken = 0;
    state.brokenWalls.clear();
    if (fin >> numBroken) {
        for (int i = 0; i < numBroken; i++) {
            int bx, by;
            if (fin >> bx >> by) {
                state.brokenWalls.push_back({ bx, by });
            }
        }
    }

    return true;
}

// 整屏重绘迷宫
void drawMazeFrame(const MazeState& state, const string maze[], int W, int H, bool nearNote) {
    console::setPos(0, 0);
    cout << "Use W/A/S/D to move. Press Q to quit maze. P:Shop. Press O to quit shop.";
    if (state.wallBreakers > 0) cout << " Press B to BreakWall";
    if (nearNote) cout << "  Press E to interact.";
    else cout << "                        ";
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

// ==================== 通关结局显示 ====================
void showVictoryScreen() {
    console::clear();
    console::hideCursor();
    
    std::cout << "\n\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "         C O N G R A T U L A T I O N S ! ! !\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "       You have successfully escaped the maze!\n";
    std::cout << "       After countless challenges and adventures,\n";
    std::cout << "       you have proven yourself to be a true hero.\n\n";
    std::cout << "       Thank you for playing our game!\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "\n\n         Press Enter to return to main menu...";
    std::cout.flush();
    
    console::showCursor();
}

// ==================== 血量检测失败结局 ====================
void showGameOverScreen() {
    console::clear();
    console::hideCursor();
    
    std::cout << "\n\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "              G A M E   O V E R\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "       Your health has run out...\n";
    std::cout << "       The challenges of the maze proved too much.\n";
    std::cout << "       But don't give up - try again!\n\n";
    std::cout << "    ****************************************************\n\n";
    std::cout << "\n\n         Press Enter to return to main menu...";
    std::cout.flush();
    
    console::showCursor();
}

// ==================== 显示血量 ====================
void displayHealth(const MazeState& state, int MAZE_HEIGHT) {
    console::setPos(0, MAZE_HEIGHT + 3);
    cout << "HP: [";
    console::setColor(3);
    for (int i = 0; i < 15; i++) {
        if (i < state.health) cout << "|";
        else cout << " ";
    }
    console::setColor(0);
    cout << "] " << state.health << "/15";

    console::setColor(2);
    cout << "   Coins: " << state.coins;
    console::setColor(0);

    if (state.wallBreakers > 0) {
        console::setColor(4);
        cout << "   Breakers: " << state.wallBreakers;
        console::setColor(0);
    }
    cout << "          ";   // 清除残留字符
    cout.flush();
}

static char waitForKeyRaw() {
    console::clearInputBuffer();
    while (true) {
#if defined(_WIN32)
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) { _getch(); continue; }
            return (char)std::toupper((unsigned char)ch);
        }
#else
        char buf[16];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; i++) {
                if (buf[i] == '\x1b') { i += 2; continue; }
                unsigned char uc = (unsigned char)buf[i];
                if (uc >= ' ' && uc < 127)
                    return (char)std::toupper(uc);
            }
        }
#endif
        console::sleep(30);
    }
}

void showShop(MazeState& state) {
    bool inShop = true;
    while (inShop) {
        console::clear();
        cout << "\n";
        cout << "  ================================\n";
        cout << "           S H O P\n";
        cout << "  ================================\n\n";
        cout << "  Coins: " << state.coins
            << "    HP: " << state.health << "/15"
            << "    Breakers: " << state.wallBreakers << "\n\n";
        cout << "  [1] Heal Potion  - 1 Coin\n";
        cout << "      Restore 1 HP\n\n";
        cout << "  [2] Wall Breaker - 5 Coins\n";
        cout << "      Break one wall (press B in maze)\n\n";
        cout << "  [K] Exit Shop\n";
        cout << "  ================================\n\n";
        cout << "  Choose: ";
        cout.flush();

        char ch = waitForKeyRaw();

        if (ch == '1') {
            if (state.coins < 1) {
                cout << "Not enough coins!";
            }
            else if (state.health >= 15) {
                cout << "HP is already full!";
            }
            else {
                state.coins--;
                state.health++;
                cout << "Purchased! HP -> " << state.health;
            }
            cout.flush();
            console::sleep(800);
        }
        else if (ch == '2') {
            if (state.coins < 5) {
                cout << "Not enough coins!";
            }
            else {
                state.coins -= 5;
                state.wallBreakers++;
                cout << "Purchased! Breakers -> " << state.wallBreakers;
            }
            cout.flush();
            console::sleep(800);
        }
        else if (ch == 'K') {
            inShop = false;
        }
    }
}

bool handleWallBreaker(MazeState& state, string maze[], int W, int H) {
    console::setPos(0, H + 5);
    cout << "Break wall: W/A/S/D to pick direction, other key to cancel   ";
    cout.flush();

    char dir = waitForKeyRaw();

    int tx = state.playerX, ty = state.playerY;
    if (dir == 'W') ty--;
    else if (dir == 'S') ty++;
    else if (dir == 'A') tx--;
    else if (dir == 'D') tx++;
    else {
        console::setPos(0, H + 5);
        cout << "Cancelled.                                                   ";
        cout.flush(); console::sleep(500);
        console::setPos(0, H + 5);
        cout << "                                                              ";
        return false;
    }

    // 不能打最外一圈
    if (tx <= 0 || tx >= W - 1 || ty <= 0 || ty >= H - 1) {
        console::setPos(0, H + 5);
        cout << "Cannot break border walls!                                    ";
        cout.flush(); console::sleep(800);
        console::setPos(0, H + 5);
        cout << "                                                              ";
        return false;
    }

    if (maze[ty][tx] != '#') {
        console::setPos(0, H + 5);
        cout << "That's not a wall!                                            ";
        cout.flush(); console::sleep(800);
        console::setPos(0, H + 5);
        cout << "                                                              ";
        return false;
    }

    maze[ty][tx] = '.';
    state.wallBreakers--;
    state.brokenWalls.push_back({ tx, ty });
    console::setPos(0, H + 5);
    cout << "Wall broken!                                                  ";
    cout.flush(); console::sleep(500);
    console::setPos(0, H + 5);
    cout << "                                                              ";
    return true;
}

void startMaze() {
    MazeState state;
    startMaze(state, false);
}


void startMaze(MazeState& state, bool useSavedState) {
    MusicManager::playBackgroundMusic("music/maze_bg.mp3");
    srand(static_cast<unsigned int>(time(0)));

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
        "###########################       ###########################"
    };

    // 清出中心出生点周围的 3x3 区域，确保玩家出生后可移动
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            maze[15 + dy][30 + dx] = '.';
        }
    }

    // 把存档中记录的碎墙重新应用到迷宫数组上
    for (const auto& p : state.brokenWalls) {
        int bx = p.first;
        int by = p.second;
        if (bx > 0 && bx < MAZE_WIDTH - 1 && by > 0 && by < MAZE_HEIGHT - 1) {
            maze[by][bx] = '.';
        }
    }

    int& playerX = state.playerX;
    int& playerY = state.playerY;
    int* noteX = state.noteX;
    int* noteY = state.noteY;
    int* shooterX = state.shooterX;
    int* shooterY = state.shooterY;
    int* snakeX = state.snakeX;
    int* snakeY = state.snakeY;

    if (!useSavedState) {
        playerX = 30;
        playerY = 15;
        state.health = 15;  // 初始10点血量

        // 新开游戏时随机放置三个音游入口
        for (int i = 0; i < 3; i++) {
            do {
                noteX[i] = rand() % MAZE_WIDTH;
                noteY[i] = rand() % MAZE_HEIGHT;
            } while (maze[noteY[i]][noteX[i]] == '#' ||
                     (noteX[i] >= 29 && noteX[i] <= 31 && noteY[i] >= 14 && noteY[i] <= 16));
        }

        // 随机放置三个弹幕入口
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

        // 随机放置三个贪吃蛇入口
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

#if !defined(_WIN32)
    console::LinuxTermGuard termGuard;
#endif

    console::clear();
    console::hideCursor();

    bool nearNote = false;
    drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);
    
    // 显示初始血量
    displayHealth(state, MAZE_HEIGHT);

    bool inMaze = true;
    const int MOVE_DELAY_MS = 35;
    const int ENEMY_MOVE_INTERVAL = 5;
    int enemyTick = 0;
    console::clearInputBuffer();

    while (inMaze) {
        // 玩家靠近 shooter 入口时自动进入弹幕小游戏
        bool nearShooter = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - shooterX[i]) <= 1 && abs(playerY - shooterY[i]) <= 1) {
                nearShooter = true;
                break;
            }
        }

        if (nearShooter) {
            MusicManager::pause();  // 暂停迷宫音乐
            bool passed = startShooterGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");  // 重新播放迷宫音乐

			// 射击游戏失败则扣血，成功则加金币
            if (passed) {
                state.coins++;
            }

            if (!passed) {
                state.health -= 3;
                if (state.health <= 0) {
                    state.health = 0;
                    MusicManager::stop();  // 停止迷宫音乐，让主菜单播放回主菜单音乐
                    console::clear();
                    console::showCursor();
                    showGameOverScreen();
                    console::sleep(500);
                    console::clearInputBuffer();
                    cin.get();
                    inMaze = false;
                    continue;
                }
            }

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
                         (shooterX[i] == snakeX[2] && shooterY[i] == snakeY[2]) ||
                         isBlockedByOtherEnemies(state, shooterX[i], shooterY[i], -1));
            }

            console::clearInputBuffer();
            continue;
        }

        // 玩家靠近 snake 入口时自动进入贪吃蛇小游戏
        bool nearSnake = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - snakeX[i]) <= 1 && abs(playerY - snakeY[i]) <= 1) {
                nearSnake = true;
                break;
            }
        }

        if (nearSnake) {
            MusicManager::pause();  // 暂停迷宫音乐
            bool passed = startSnakeGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");  // 重新播放迷宫音乐

			// 贪吃蛇失败则扣血，成功则加金币
            if (passed) {
                state.coins++;
            }

            if (!passed) {
                state.health -= 3;
                if (state.health <= 0) {
                    state.health = 0;
                    MusicManager::stop();  // 停止迷宫音乐，让主菜单播放回主菜单音乐
                    console::clear();
                    console::showCursor();
                    showGameOverScreen();
                    console::sleep(500);
                    console::clearInputBuffer();
                    cin.get();
                    inMaze = false;
                    continue;
                }
            }

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
                         (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2]) ||
                         isBlockedByOtherEnemies(state, snakeX[i], snakeY[i], -1));
            }

            console::clearInputBuffer();
            continue;
        }

        // 靠近音符入口时，在顶部显示 E 键提示
        nearNote = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - noteX[i]) <= 1 && abs(playerY - noteY[i]) <= 1) {
                nearNote = true;
                break;
            }
        }

        int nextX = playerX;
        int nextY = playerY;
        bool tryMove = false;
        bool doInteract = false;
        bool doQuit = false;
        bool doShop = false;
        bool doBreak = false;

#if defined(_WIN32)
        if (GetAsyncKeyState('W') & 0x8000) { nextY--; tryMove = true; }
        else if (GetAsyncKeyState('S') & 0x8000) { nextY++; tryMove = true; }
        else if (GetAsyncKeyState('A') & 0x8000) { nextX--; tryMove = true; }
        else if (GetAsyncKeyState('D') & 0x8000) { nextX++; tryMove = true; }
        else if (nearNote && (GetAsyncKeyState('E') & 0x8000)) { doInteract = true; }
        else if (GetAsyncKeyState('Q') & 0x8000) { doQuit = true; }
        else if (GetAsyncKeyState('P') & 0x8000) { doShop = true; }
        else if (state.wallBreakers > 0 && (GetAsyncKeyState('B') & 0x8000)) { doBreak = true; }
        console::clearInputBuffer();
#else
        char inKey = console::getInput();
        if (inKey == 'W') { nextY--; tryMove = true; }
        else if (inKey == 'S') { nextY++; tryMove = true; }
        else if (inKey == 'A') { nextX--; tryMove = true; }
        else if (inKey == 'D') { nextX++; tryMove = true; }
        else if (nearNote && inKey == 'E') { doInteract = true; }
        else if (inKey == 'Q') { doQuit = true; }
        else if (inKey == 'P') { doShop = true; }
        else if (inKey == 'B' && state.wallBreakers > 0) { doBreak = true; }
#endif

        // 商店
        if (doShop) {
            showShop(state);
            console::clear();
            console::hideCursor();
            drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);
            displayHealth(state, MAZE_HEIGHT);
            console::clearInputBuffer();
            continue;
        }

        // 使用破墙道具
        if (doBreak) {
            handleWallBreaker(state, maze, MAZE_WIDTH, MAZE_HEIGHT);
            drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);
            displayHealth(state, MAZE_HEIGHT);
            console::clearInputBuffer();
            continue;
        }

        // 音符入口需要按 E 才进入音游
        if (doInteract) {
            console::sleep(200);
            MusicManager::pause();  // 暂停迷宫音乐
            bool passed = startMusicGame();
            MusicManager::playBackgroundMusic("music/maze_bg.mp3");  // 重新播放迷宫音乐

			// 音游失败则扣血，成功则加金币
            if (passed) {
                state.coins++;
            }

            if (!passed) {
                state.health -= 3;
                if (state.health <= 0) {
                    state.health = 0;
                    MusicManager::stop();  // 停止迷宫音乐，让主菜单播放回主菜单音乐
                    console::clear();
                    console::showCursor();
                    showGameOverScreen();
                    console::sleep(500);
                    console::clearInputBuffer();
                    cin.get();
                    inMaze = false;
                    continue;
                }
            }

            for (int i = 0; i < 3; i++) {
                do {
                    noteX[i] = rand() % MAZE_WIDTH;
                    noteY[i] = rand() % MAZE_HEIGHT;
                } while (maze[noteY[i]][noteX[i]] == '#' ||
                         (noteX[i] == playerX && noteY[i] == playerY) ||
                         isBlockedByOtherEnemies(state, noteX[i], noteY[i], -1));
            }

            console::clearInputBuffer();
            continue;
        }

        // 按 Q 时保存当前迷宫状态后退出
        if (doQuit) {
            console::setPos(0, MAZE_HEIGHT + 4);
            string filename = generateSaveFileName();
            if (saveMazeStateToFile(filename, state)) {
                cout << "Game saved to " << filename;
            } else {
                cout << "Failed to save game.";
            }
            console::sleep(500);
            inMaze = false;
            continue;
        }
        
        // 玩家只能走到非墙体位置
        if (tryMove && nextX >= 0 && nextX < MAZE_WIDTH && nextY >= 0 && nextY < MAZE_HEIGHT) {
            if (maze[nextY][nextX] != '#') {
                playerX = nextX;
                playerY = nextY;
                
                // 检测是否到达出口（迷宫底部第30行，空列27-33）
                if (playerY == 30 && playerX >= 27 && playerX <= 33) {
                    MusicManager::stop();  // 停止迷宫音乐，让主菜单播放回主菜单音乐
                    console::clear();
                    console::showCursor();
                    showVictoryScreen();
                    console::sleep(5000);
                    console::clearInputBuffer();
                    cin.get();
                    inMaze = false;
                    continue;
                }
            }
        }

        enemyTick++;
        if (enemyTick >= ENEMY_MOVE_INTERVAL) {
            updateEnemies(state, maze, MAZE_WIDTH, MAZE_HEIGHT);
            enemyTick = 0;
        }

        // 玩家贴近怪物时进入问答小游戏，结束后怪物重置
        int hitEnemy = findTriggeredEnemy(state);
        if (hitEnemy != -1) {
            MusicManager::pause();  // 暂停迷宫音乐
            bool passed = startEnemyQuiz();
            MusicManager::resume();  // 继续播放迷宫音乐
            
			// 问答失败则扣血,反之加金币
            if (passed) {
                state.coins++;
            }
            if (!passed) {
                state.health -= 3;
                if (state.health <= 0) {
                    state.health = 0;
                    MusicManager::stop();  // 停止迷宫音乐，让主菜单播放回主菜单音乐
                    console::clear();
                    console::showCursor();
                    showGameOverScreen();
                    console::sleep(500);
                    console::clearInputBuffer();
                    cin.get();
                    inMaze = false;
                    continue;
                }
            }
            
            resetOneEnemy(state, maze, MAZE_WIDTH, MAZE_HEIGHT, hitEnemy);
            console::clearInputBuffer();
        }

        nearNote = false;
        for (int i = 0; i < 3; i++) {
            if (abs(playerX - noteX[i]) <= 1 && abs(playerY - noteY[i]) <= 1) {
                nearNote = true;
                break;
            }
        }

        drawMazeFrame(state, maze, MAZE_WIDTH, MAZE_HEIGHT, nearNote);
        displayHealth(state, MAZE_HEIGHT);
        console::sleep(MOVE_DELAY_MS);
    }

    console::showCursor();
    console::setPos(0, MAZE_HEIGHT + 3);
    console::clearInputBuffer();
}
