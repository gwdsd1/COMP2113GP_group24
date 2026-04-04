#include "shooter_game.h"
#include "MusicManager.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <windows.h>
#include <conio.h>
#include <cstdlib>

using namespace std;

// structure for enemies
struct ShooterEnemy {
    double x;
    double y;
    string text;
    int hp;
    int maxHp;
    bool active;
    double hitFlashTimer;
};

void startShooterGame() {
    // 播放射击游戏背景音乐
    MusicManager::playBackgroundMusic("music/shooter_bg.mp3");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Enable VT for ANSI
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // 1. Transition effect
    system("cls");
    cout << "\x1b[91m"; // Bright Red
    cout << "\n\nYOU SUDDENLY FEEL A HUGE SHOCK ...THAT IS......\n";
    this_thread::sleep_for(chrono::milliseconds(2000));
    cout << "ASSIGNMENTS......TUTORIALS......GROUP PROJECTS......PRESENTATIONS......MIDTERMS......FINALS......\n";
    this_thread::sleep_for(chrono::milliseconds(2000));
    cout << "YOU KNOW YOU HAVE TO FACE ALL OF THESE......\n";
    this_thread::sleep_for(chrono::milliseconds(2000));
    cout << "\x1b[0m"; // Reset

    // 2. Countdown
    for (int i = 3; i >= 1; i--) {
        system("cls");
        cout << "\x1b[93m\n\n\n\t\t\t" << i << "\x1b[0m\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
    system("cls");
    cout << "\x1b[92m\n\n\n\t\t\tGO!\x1b[0m\n";
    this_thread::sleep_for(chrono::milliseconds(500));

    // Clear buffer
    while (_kbhit()) _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    // 3. Game settings
    const int W = 64;
    const int H = 24;
    double playerX = W / 2.0;
    double playerY = H - 2.0;

    int score = 0;
    auto startTime = chrono::steady_clock::now();

    string enemyTypes[] = {"A", "T", "GP", "P", "M", "F"};
    vector<ShooterEnemy> enemies;

    bool running = true;
    double spawnTimer = 0.0;
    double lastDt = 0.016;
    
    // Hide cursor
    cout << "\x1b[?25l";

    while (running) {
        auto frameStart = chrono::steady_clock::now();

        auto current_time = chrono::steady_clock::now();
        double elapsedSeconds = chrono::duration<double>(current_time - startTime).count();
        if (elapsedSeconds >= 20.0) {
            running = false;
        }

        // Input
        if (GetAsyncKeyState('A') & 0x8000) { playerX -= 40.0 * lastDt; }
        if (GetAsyncKeyState('D') & 0x8000) { playerX += 40.0 * lastDt; }
        if (GetAsyncKeyState('W') & 0x8000) { playerY -= 30.0 * lastDt; }
        if (GetAsyncKeyState('S') & 0x8000) { playerY += 30.0 * lastDt; }
        
        if (playerX < 0) playerX = 0;
        if (playerX >= W) playerX = W - 1;
        if (playerY < H/2) playerY = H/2; 
        if (playerY >= H) playerY = H - 1;

        // Spawn logic
        spawnTimer -= lastDt;
        if (spawnTimer <= 0) {
            ShooterEnemy e;
            int typeIdx = rand() % 6;
            e.text = enemyTypes[typeIdx];
            e.x = rand() % (W - e.text.length() - 1);
            e.y = 0;
            e.maxHp = e.hp = 8 + rand() % 5; // Approx 5 hits (since laser does 2 per frame hit, 10 HP = 5 hits)
            e.active = true;
            e.hitFlashTimer = 0.0;
            enemies.push_back(e);
            spawnTimer = 0.3 + (rand() % 40) / 100.0; // Faster spawn 0.3s - 0.7s
        }

        int pxi = (int)playerX;
        int pyi = (int)playerY;

        // Calculate max laser length based on 2-second growth time
        int maxLaserLen = pyi;
        double growthFactor = min(1.0, elapsedSeconds / 2.0);
        int currentLaserLen = (int)(maxLaserLen * growthFactor);
        int laserTopY = pyi - currentLaserLen;

        // Update enemies & collision
        for (auto& e : enemies) {
            if (e.active) {
                e.y += 4.0 * lastDt; // Much slower drop speed
                if (e.y > H) e.active = false; 

                if (e.hitFlashTimer > 0) {
                    e.hitFlashTimer -= lastDt;
                }

                // hit scan logic (wide laser / wide hitbox)
                int ex = (int)e.x;
                int len = e.text.length();
                // Player's laser at 'pxi', hit if player is under and within expanded bounds AND the laser reached the enemy
                if (pxi >= ex - 1 && pxi <= ex + len && pyi > e.y && e.y >= laserTopY) {
                    e.hp -= 2; 
                    e.hitFlashTimer = 0.1; // Flash yellow for 100ms
                    if (e.hp <= 0) {
                        e.active = false;
                        score += 50;
                    }
                }
            }
        }

        // Render buffer
        vector<string> buffer(H, string(W, ' '));
        vector<vector<int>> colorBuffer(H, vector<int>(W, 0)); // 0=default, 1=red, 2=blue, 3=yellow

        // Player
        if (pyi >= 0 && pyi < H && pxi >= 0 && pxi < W) {
            buffer[pyi][pxi] = '^'; // player char (blue)
            colorBuffer[pyi][pxi] = 2; // blue
        }

        // Laser
        for (int y = pyi - 1; y >= laserTopY && y >= 0; y--) {
            buffer[y][pxi] = '|';
            colorBuffer[y][pxi] = 2; // blue
        }

        // Apply enemies to buffer
        for (auto& e : enemies) {
            if (e.active) {
                int ey = (int)e.y;
                int ex = (int)e.x;
                if (ey >= 0 && ey < H) {
                    for (size_t i=0; i<e.text.length() && ex+i < W; i++) {
                        buffer[ey][ex+i] = e.text[i];
                        colorBuffer[ey][ex+i] = (e.hitFlashTimer > 0) ? 3 : 1; // 3=yellow, 1=red
                    }
                }
            }
        }

        // Output logic (ANSI)
        string frameOut = "\x1b[H"; // Move home
        int lastColor = 0;
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                int col = colorBuffer[y][x];
                if (col != lastColor) {
                    if (col == 0) frameOut += "\x1b[0m"; // Reset
                    else if (col == 1) frameOut += "\x1b[91m"; // Red
                    else if (col == 2) frameOut += "\x1b[94m"; // Blue
                    else if (col == 3) frameOut += "\x1b[93m"; // Yellow
                    lastColor = col;
                }
                frameOut += buffer[y][x];
            }
            if (y < H - 1) frameOut += "\n";
        }
        if (lastColor != 0) frameOut += "\x1b[0m";
        frameOut += "\nTime left: " + to_string((int)(20.0 - elapsedSeconds)) + "s  | Score: " + to_string(score) + "      \n";

        cout << frameOut;

        // Framing
        this_thread::sleep_for(chrono::milliseconds(16));
        auto frameEnd = chrono::steady_clock::now();
        lastDt = chrono::duration<double>(frameEnd - frameStart).count();
        if (lastDt < 0.001) lastDt = 0.016; 
    }

    // End transition
    system("cls");
    cout << "\x1b[96m\n\nYOU FINALLY HANDLED ALL OF THESE......\x1b[0m\n";
    this_thread::sleep_for(chrono::milliseconds(2000));
    
    // Clear buffer again
    cout << "\x1b[?25h"; // Show cursor
    SetConsoleMode(hConsole, mode); // Restore orig mode
    while (_kbhit()) _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}
