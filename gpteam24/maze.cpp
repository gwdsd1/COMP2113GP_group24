#include "maze.h"
#include "music_game.h"
#include "shooter_game.h"
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
        else SetConsoleTextAttribute(hConsole, 7);
        #else
        if (code == 1) std::cout << "\x1b[94m";
        else if (code == 2) std::cout << "\x1b[93m";
        else if (code == 3) std::cout << "\x1b[91m";
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
            for (ssize_t i=0; i<n; i++) {
                if (buf[i] == '\x1b' && i + 2 < n && buf[i+1] == '[') {
                    if (buf[i+2] == 'A') lastChar = 'W';
                    else if (buf[i+2] == 'B') lastChar = 'S';
                    else if (buf[i+2] == 'C') lastChar = 'D';
                    else if (buf[i+2] == 'D') lastChar = 'A';
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



bool saveMazeStateToFile(const string& filename, const MazeState& state){
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
    for(int dy = -1; dy <= 1; dy++) {
        for(int dx = -1; dx <= 1; dx++) {
            maze[15 + dy][30 + dx] = '.';
        }
    }
    int& playerX = state.playerX;
    int& playerY = state.playerY;
    int* noteX = state.noteX;
    int* noteY = state.noteY;
    int* shooterX = state.shooterX;
    int* shooterY = state.shooterY;

    if (!useSavedState) {
        playerX = 30; // 宽度索引
        playerY = 15; // 高度索引

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
                cout << '@'; // 主角
                console::setColor(0); // 恢复默认颜色
            } else {
                bool isNote = false;
                for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                bool isShooter = false;
                for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                if (isNote) {
                    console::setColor(2); // 金黄色
                    cout << '&'; // 音符符号
                    console::setColor(0);
                } else if (isShooter) {
                    console::setColor(3); // 鲜红色
                    cout << '!'; // 弹幕关卡符号
                    console::setColor(0);
                } else if (maze[y][x] == '#') {
                    cout << '#'; // 墙壁
                } else {
                    cout << ' '; // 路径
                }
            }
        }
        cout << '\n';
    }

    bool inMaze = true;

    // Add a small delay to control movement speed
    const int MOVE_DELAY_MS = 60;

    // Clear residual input buffer before beginning the maze move loop
    console::clearInputBuffer();

    while (inMaze) {
        // Automatically trigger shooter game if near (3x3 area)
        bool nearShooter = false;
        for (int i=0; i<3; i++) {
            if (abs(playerX - shooterX[i]) <= 1 && abs(playerY - shooterY[i]) <= 1) {
                nearShooter = true;
                break;
            }
        }

        if (nearShooter) {
            startShooterGame();

             MusicManager::playBackgroundMusic("music/maze_bg.mp3");

            // Refresh shooter locations
            for (int i=0; i<3; i++) {
                do {
                    shooterX[i] = rand() % MAZE_WIDTH;
                    shooterY[i] = rand() % MAZE_HEIGHT;
                } while (maze[shooterY[i]][shooterX[i]] == '#' || 
                        (shooterX[i] == playerX && shooterY[i] == playerY) ||
                        (shooterX[i] == noteX[0] && shooterY[i] == noteY[0]) ||
                        (shooterX[i] == noteX[1] && shooterY[i] == noteY[1]) ||
                        (shooterX[i] == noteX[2] && shooterY[i] == noteY[2]));
            }

            console::clear();
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        console::setColor(1);
                        cout << '@'; 
                        console::setColor(0);
                    } else {
                        bool isNote = false;
                        for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                        if (isNote) {
                            console::setColor(2); 
                            cout << '&';
                            console::setColor(0);
                        } else if (isShooter) {
                            console::setColor(3); 
                            cout << '!';
                            console::setColor(0);
                        } else if (maze[y][x] == '#') {
                            cout << '#';
                        } else {
                            cout << ' ';
                        }
                    }
                }
                cout << '\n';
            }
            console::clearInputBuffer(); 
            continue;
        }

        // 检查是否靠近音符
        bool nearNote = false;
        for (int i=0; i<3; i++) {
            if (abs(playerX - noteX[i]) <= 1 && abs(playerY - noteY[i]) <= 1) {
                nearNote = true;
                break;
            }
        }

        // 显示或隐藏提示信息（仅在状态变化时输出，避免每帧触发滚动）
        static bool lastNearNote = false;
        if (nearNote != lastNearNote) {
            console::setPos(0, MAZE_HEIGHT + 2);
            if (nearNote) {
                cout << "Press E to interact...                                  ";
            } else {
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

        // 吃掉输入缓冲防止换行bug
        console::clearInputBuffer();
#else
        // Apply temporary momentum on Linux to bridge across keyboard auto-repeat delay gaps
        static char linuxLastDir = 0;
        static int linuxDirKeepAlive = 0;

        char inKey = console::getInput();
        if (inKey == 'W' || inKey == 'A' || inKey == 'S' || inKey == 'D') {
            linuxLastDir = inKey;
            linuxDirKeepAlive = 9; // keep alive for ~540ms to bridge standard 500ms keyboard repeat delays
        } else if (inKey == 'E' || inKey == 'Q' || inKey == ' ') { // Any interaction or spacebar acts as explicit brake
            linuxLastDir = 0;
            linuxDirKeepAlive = 0;
        }

        if (linuxDirKeepAlive > 0 && linuxLastDir != 0) {
            if (linuxLastDir == 'W') { nextY--; tryMove = true; }
            else if (linuxLastDir == 'S') { nextY++; tryMove = true; }
            else if (linuxLastDir == 'A') { nextX--; tryMove = true; }
            else if (linuxLastDir == 'D') { nextX++; tryMove = true; }
            linuxDirKeepAlive--;
        }

        if (inKey == 'E' && nearNote) { doInteract = true; }
        else if (inKey == 'Q') { doQuit = true; }
#endif

        if (doInteract) {
            // Prevent multiple rapid triggers
            console::sleep(200); 

            startMusicGame();

            MusicManager::playBackgroundMusic("music/maze_bg.mp3");

            // Generate a new position for the music game entrance
            for (int i=0; i<3; i++) {
                do {
                    noteX[i] = rand() % MAZE_WIDTH;
                    noteY[i] = rand() % MAZE_HEIGHT;
                } while (maze[noteY[i]][noteX[i]] == '#' || (noteX[i] == playerX && noteY[i] == playerY));
            }

            // 重新绘制整个迷宫（从音乐游戏回来后）
            console::clear();
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        console::setColor(1);
                        cout << '@'; 
                        console::setColor(0);
                    } else {
                        bool isNote = false;
                        for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                        if (isNote) {
                            console::setColor(2); 
                            cout << '&';
                            console::setColor(0);
                        } else if (isShooter) {
                            console::setColor(3); 
                            cout << '!';
                            console::setColor(0);
                        } else if (maze[y][x] == '#') {
                            cout << '#';
                        } else {
                            cout << ' ';
                        }
                    }
                }
                cout << '\n';
            }
            // Clear input buffer to avoid ghost movement after game
            console::clearInputBuffer(); 
            continue; // 跳过本次移动逻辑
        }
        else if (doQuit) {
            console::setPos(0, MAZE_HEIGHT + 4);
            string filename = generateSaveFileName();
            if (saveMazeStateToFile(filename, state)) {
                cout << "Game saved to " << filename;
            } else {
                 cout << "Failed to save game.";
            }
            console::sleep(500);
            inMaze = false;
        }

        // 碰撞检测
        if (tryMove && nextX >= 0 && nextX < MAZE_WIDTH && nextY >= 0 && nextY < MAZE_HEIGHT) {
            if (maze[nextY][nextX] != '#') {
                // 擦除旧位置
                int oldX = playerX;
                int oldY = playerY;
                console::setPos(oldX, oldY + 2);
                cout << ' '; 

                playerX = nextX;
                playerY = nextY;

                // 绘制新位置
                console::setPos(playerX, playerY + 2);

                // 如果刚好走到了音符上面或者重绘，确保音符颜色正确，但现在角色覆盖它
                console::setColor(1);
                cout << '@';
                console::setColor(0);

                // 恢复覆盖的音符
                bool redrew = false;
                for (int i=0; i<3; i++) {
                    if (oldX == noteX[i] && oldY == noteY[i]) {
                        console::setPos(oldX, oldY + 2);
                        console::setColor(2);
                        cout << '&';
                        console::setColor(0);
                        redrew = true;
                        break;
                    }
                }
                if (!redrew) {
                    for (int i=0; i<3; i++) {
                        if (oldX == shooterX[i] && oldY == shooterY[i]) {
                            console::setPos(oldX, oldY + 2);
                            console::setColor(3);
                            cout << '!';
                            console::setColor(0);
                            break;
                        }
                    }
                }
            }
        }

        std::cout.flush(); // FLUSH is required on Linux, otherwise no screen updates!

        console::sleep(MOVE_DELAY_MS); // Control movement speed and yield CPU
    }

    // 恢复命令行位置到底部
    console::setPos(0, MAZE_HEIGHT + 3);

    // Clear all unread characters from standard input stream buffer before returning
    console::clearInputBuffer();
}