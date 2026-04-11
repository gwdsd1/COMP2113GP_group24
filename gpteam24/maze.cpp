#include "maze.h"
#include "music_game.h"
#include "shooter_game.h"
#include "snake_game.h"           // ← 新增
#include "MusicManager.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <time.h>
#include <fstream>
#include <ctime>
#include <sstream>

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
        else if (code == 4) SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);  // ← 新增: 绿色
        else SetConsoleTextAttribute(hConsole, 7);
#else
        if (code == 1) std::cout << "\x1b[94m";
        else if (code == 2) std::cout << "\x1b[93m";
        else if (code == 3) std::cout << "\x1b[91m";
        else if (code == 4) std::cout << "\x1b[92m";   // ← 新增: 绿色
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

    fout.close();
    return true;
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

    // ← 新增：读取贪吃蛇坐标（兼容旧存档）
    for (int i = 0; i < 3; i++) {
        if (!(fin >> state.snakeX[i] >> state.snakeY[i])) {
            state.snakeX[i] = -1;
            state.snakeY[i] = -1;
        }
    }

    fin.close();
    return true;
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
    } else {
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
                    (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2]));
            }
        }
    }

    console::clear();
    cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";

#if !defined(_WIN32)
    console::LinuxTermGuard termGuard;
#endif

    // 首次绘制迷宫
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (x == playerX && y == playerY) {
                console::setColor(1);
                cout << '@';
                console::setColor(0);
            }
            else {
                bool isNote = false;
                for (int i = 0; i < 3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                bool isShooter = false;
                for (int i = 0; i < 3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }
                bool isSnake = false;                                          // ← 新增
                for (int i = 0; i < 3; i++) { if (x == snakeX[i] && y == snakeY[i]) isSnake = true; }

                if (isNote) {
                    console::setColor(2);
                    cout << '&';
                    console::setColor(0);
                }
                else if (isShooter) {
                    console::setColor(3);
                    cout << '!';
                    console::setColor(0);
                }
                else if (isSnake) {                                          // ← 新增
                    console::setColor(4);
                    cout << 'S';
                    console::setColor(0);
                }
                else if (maze[y][x] == '#') {
                    cout << '#';
                }
                else {
                    cout << ' ';
                }
            }
        }
        cout << '\n';
    }

    bool inMaze = true;
    const int MOVE_DELAY_MS = 60;
    console::clearInputBuffer();

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
                    (shooterX[i] == snakeX[2] && shooterY[i] == snakeY[2]));
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
                    (snakeX[i] == shooterX[2] && snakeY[i] == shooterY[2]));
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
                } while (maze[noteY[i]][noteX[i]] == '#' || (noteX[i] == playerX && noteY[i] == playerY));
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
        }

        // 碰撞检测与移动
        if (tryMove && nextX >= 0 && nextX < MAZE_WIDTH && nextY >= 0 && nextY < MAZE_HEIGHT) {
            if (maze[nextY][nextX] != '#') {
                int oldX = playerX;
                int oldY = playerY;
                console::setPos(oldX, oldY + 2);
                cout << ' ';

                playerX = nextX;
                playerY = nextY;

                console::setPos(playerX, playerY + 2);
                console::setColor(1);
                cout << '@';
                console::setColor(0);

                // 恢复旧位置被覆盖的图标
                bool redrew = false;
                for (int i = 0; i < 3; i++) {
                    if (oldX == noteX[i] && oldY == noteY[i]) {
                        console::setPos(oldX, oldY + 2);
                        console::setColor(2); cout << '&'; console::setColor(0);
                        redrew = true; break;
                    }
                }
                if (!redrew) {
                    for (int i = 0; i < 3; i++) {
                        if (oldX == shooterX[i] && oldY == shooterY[i]) {
                            console::setPos(oldX, oldY + 2);
                            console::setColor(3); cout << '!'; console::setColor(0);
                            redrew = true; break;
                        }
                    }
                }
                if (!redrew) {                                                 // ← 新增
                    for (int i = 0; i < 3; i++) {
                        if (oldX == snakeX[i] && oldY == snakeY[i]) {
                            console::setPos(oldX, oldY + 2);
                            console::setColor(4); cout << 'S'; console::setColor(0);
                            break;
                        }
                    }
                }
            }
        }

        std::cout.flush();
        console::sleep(MOVE_DELAY_MS);
    }

    console::setPos(0, MAZE_HEIGHT + 3);
    console::clearInputBuffer();
}
