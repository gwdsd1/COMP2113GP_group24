#include "maze.h"
#include "music_game.h"
#include "shooter_game.h"
#include "MusicManager.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <conio.h>
#include <windows.h>
#include <time.h>
#include <fstream>

using namespace std;

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

    system("cls");
    cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // 首次绘制迷宫
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            if (x == playerX && y == playerY) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                cout << '@'; // 主角
                SetConsoleTextAttribute(hConsole, 7); // 恢复默认颜色
            } else {
                bool isNote = false;
                for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                bool isShooter = false;
                for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                if (isNote) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); // 金黄色
                    cout << '&'; // 音符符号
                    SetConsoleTextAttribute(hConsole, 7);
                } else if (isShooter) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); // 鲜红色
                    cout << '!'; // 弹幕关卡符号
                    SetConsoleTextAttribute(hConsole, 7);
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
    while (_kbhit()) { _getch(); }
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

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

            system("cls");
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                        cout << '@'; 
                        SetConsoleTextAttribute(hConsole, 7);
                    } else {
                        bool isNote = false;
                        for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                        if (isNote) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); 
                            cout << '&';
                            SetConsoleTextAttribute(hConsole, 7);
                        } else if (isShooter) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); 
                            cout << '!';
                            SetConsoleTextAttribute(hConsole, 7);
                        } else if (maze[y][x] == '#') {
                            cout << '#';
                        } else {
                            cout << ' ';
                        }
                    }
                }
                cout << '\n';
            }
            while (_kbhit()) _getch(); 
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

        // 显示或隐藏提示信息
        COORD hintPos = { 0, (short)(MAZE_HEIGHT + 2) };
        SetConsoleCursorPosition(hConsole, hintPos);
        if (nearNote) {
            cout << "Press E to interact...                                  ";
        } else {
            cout << "                                                        ";
        }

        int nextX = playerX;
        int nextY = playerY;
        bool tryMove = false;

        // Use GetAsyncKeyState for non-blocking, smooth continuous input
        if (GetAsyncKeyState('W') & 0x8000) { nextY--; tryMove = true; }
        else if (GetAsyncKeyState('S') & 0x8000) { nextY++; tryMove = true; }
        else if (GetAsyncKeyState('A') & 0x8000) { nextX--; tryMove = true; }
        else if (GetAsyncKeyState('D') & 0x8000) { nextX++; tryMove = true; }
        else if (nearNote && (GetAsyncKeyState('E') & 0x8000)) {
            // Prevent multiple rapid triggers
            Sleep(200); 

            startMusicGame();

            // Generate a new position for the music game entrance
            for (int i=0; i<3; i++) {
                do {
                    noteX[i] = rand() % MAZE_WIDTH;
                    noteY[i] = rand() % MAZE_HEIGHT;
                } while (maze[noteY[i]][noteX[i]] == '#' || (noteX[i] == playerX && noteY[i] == playerY));
            }

            // 重新绘制整个迷宫（从音乐游戏回来后）
            system("cls");
            cout << "Use W/A/S/D to move. Press Q to quit maze.\n\n";
            for (int y = 0; y < MAZE_HEIGHT; y++) {
                for (int x = 0; x < MAZE_WIDTH; x++) {
                    if (x == playerX && y == playerY) {
                        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                        cout << '@'; 
                        SetConsoleTextAttribute(hConsole, 7);
                    } else {
                        bool isNote = false;
                        for (int i=0; i<3; i++) { if (x == noteX[i] && y == noteY[i]) isNote = true; }
                        bool isShooter = false;
                        for (int i=0; i<3; i++) { if (x == shooterX[i] && y == shooterY[i]) isShooter = true; }

                        if (isNote) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); 
                            cout << '&';
                            SetConsoleTextAttribute(hConsole, 7);
                        } else if (isShooter) {
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY); 
                            cout << '!';
                            SetConsoleTextAttribute(hConsole, 7);
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
            while (_kbhit()) _getch(); 
            continue; // 跳过本次移动逻辑
        }
        else if (GetAsyncKeyState('Q') & 0x8000) {
            COORD msgPos = { 0, (short)(MAZE_HEIGHT + 4) };
            SetConsoleCursorPosition(hConsole, msgPos);
            if (saveMazeStateToFile("save.txt", state)) {
                cout << "Game saved to save.txt";
            } else {
                cout << "Failed to save game.";
            }
            Sleep(500);
            inMaze = false;
        }

        // 碰撞检测
        if (tryMove && nextX >= 0 && nextX < MAZE_WIDTH && nextY >= 0 && nextY < MAZE_HEIGHT) {
            if (maze[nextY][nextX] != '#') {
                // 擦除旧位置
                COORD oldPos = { (short)playerX, (short)(playerY + 2) };
                SetConsoleCursorPosition(hConsole, oldPos);
                cout << ' '; 

                playerX = nextX;
                playerY = nextY;

                // 绘制新位置
                COORD newPos = { (short)playerX, (short)(playerY + 2) };
                SetConsoleCursorPosition(hConsole, newPos);

                // 如果刚好走到了音符上面或者重绘，确保音符颜色正确，但现在角色覆盖它
                SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                cout << '@';
                SetConsoleTextAttribute(hConsole, 7);

                // 恢复覆盖的音符
                bool redrew = false;
                for (int i=0; i<3; i++) {
                    if (oldPos.X == noteX[i] && (oldPos.Y - 2) == noteY[i]) {
                        SetConsoleCursorPosition(hConsole, oldPos);
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                        cout << '&';
                        SetConsoleTextAttribute(hConsole, 7);
                        redrew = true;
                        break;
                    }
                }
                if (!redrew) {
                    for (int i=0; i<3; i++) {
                        if (oldPos.X == shooterX[i] && (oldPos.Y - 2) == shooterY[i]) {
                            SetConsoleCursorPosition(hConsole, oldPos);
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                            cout << '!';
                            SetConsoleTextAttribute(hConsole, 7);
                            break;
                        }
                    }
                }
            }
        }

        Sleep(MOVE_DELAY_MS); // Control movement speed and yield CPU
    }

    // 恢复命令行位置到底部
    COORD endPos = { 0, (short)(MAZE_HEIGHT + 3) };
    SetConsoleCursorPosition(hConsole, endPos);

    // Clear all unread characters from standard input stream buffer before returning
    while (_kbhit()) {
        _getch();
    }
    // Also clear the console input buffer explicitly
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}