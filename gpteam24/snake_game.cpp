#include "snake_game.h"
#include "MusicManager.h"
#include <iostream>
#include <deque>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <string>
#include <algorithm>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

// ========== 贪吃蛇专用控制台工具 ==========
namespace snake_con {

inline void setColor(int code) {
#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    switch (code) {
        case 1: SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY); break;
        case 2: SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); break;
        case 3: SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY); break;
        case 4: SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY); break;
        case 5: SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); break;
        default: SetConsoleTextAttribute(h, 7); break;
    }
#else
    switch (code) {
        case 1: std::cout << "\x1b[92m"; break;
        case 2: std::cout << "\x1b[93m"; break;
        case 3: std::cout << "\x1b[91m"; break;
        case 4: std::cout << "\x1b[95m"; break;
        case 5: std::cout << "\x1b[96m"; break;
        default: std::cout << "\x1b[0m"; break;
    }
#endif
}

inline void setPos(int x, int y) {
#if defined(_WIN32)
    COORD p = {(short)x, (short)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), p);
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
    char last = 0;
#if defined(_WIN32)
    while (_kbhit()) {
        int c = _getch();
        if (c == 0 || c == 224) _getch();
        else last = (char)std::toupper(c);
    }
#else
    char buf[64];
    ssize_t n;
    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; ++i) {
            if (buf[i] == '\x1b' && i + 2 < n && buf[i + 1] == '[') {
                switch (buf[i + 2]) {
                    case 'A': last = 'W'; break;
                    case 'B': last = 'S'; break;
                    case 'C': last = 'D'; break;
                    case 'D': last = 'A'; break;
                }
                i += 2;
            } else if (isalpha(buf[i])) {
                last = (char)std::toupper(buf[i]);
            }
        }
    }
#endif
    return last;
}

inline void clearInput() { getInput(); }
inline void flush() { std::cout.flush(); }
inline void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#if !defined(_WIN32)
struct TermGuard {
    termios orig{};
    int flags = 0;
    bool ok = false;
    TermGuard() {
        if (!isatty(STDIN_FILENO)) return;
        tcgetattr(STDIN_FILENO, &orig);
        termios t = orig;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        ok = true;
    }
    ~TermGuard() {
        if (ok) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig);
            fcntl(STDIN_FILENO, F_SETFL, flags);
        }
    }
};
#endif

} // namespace snake_con

// ========== 游戏常量 ==========
static const int BW  = 42;
static const int BH  = 22;
static const int HDR = 2;
static const int TARGET_SCORE = 150;   // ← 新增：目标分数

struct SC { int x, y; };

static bool onBody(const std::deque<SC>& s, int x, int y) {
    for (auto& c : s)
        if (c.x == x && c.y == y) return true;
    return false;
}

static SC randFree(const std::deque<SC>& s, const std::vector<SC>& occ) {
    SC p;
    for (int att = 0; att < 8000; ++att) {
        p.x = 1 + rand() % (BW - 2);
        p.y = 1 + rand() % (BH - 2);
        if (onBody(s, p.x, p.y)) continue;
        bool hit = false;
        for (auto& o : occ)
            if (o.x == p.x && o.y == p.y) { hit = true; break; }
        if (!hit) return p;
    }
    return {1, 1};
}

// ========== 底部状态栏绘制 ==========                           // ← 新增函数
static void drawStatusBar(int score, bool boosted) {
    using namespace snake_con;

    // 第一行：分数和目标
    setPos(0, BH + HDR);
    setColor(2);
    std::cout << " Score: " << score << " / " << TARGET_SCORE << "    ";
    setColor(0);

    // 第二行：加速提示（如果有的话）
    setPos(0, BH + HDR + 1);
    if (boosted) {
        setColor(5);
        std::cout << " >> SPEED BOOST! <<              ";
        setColor(0);
    } else {
        std::cout << "                                  ";
    }
}

// ========== 全量绘制 ==========
static void fullDraw(const std::deque<SC>& snake, const SC& food,
                     bool poisonOn, const SC& poison,
                     bool speedOn,  const SC& speedPot,
                     int score, bool boosted) {                    // ← 加了 boosted 参数
    using namespace snake_con;
    clear();
    setPos(0, 0);
    std::cout << "=== Snake Game ===   Target: " << TARGET_SCORE << "      [Q Quit]";
    setPos(0, 1);
    std::cout << "WASD Move  |  * Food(+10)  |  X Poison(-5)  |  ^ Speed(x2)";

    for (int y = 0; y < BH; ++y)
        for (int x = 0; x < BW; ++x) {
            setPos(x, y + HDR);
            if (y == 0 || y == BH - 1 || x == 0 || x == BW - 1)
                std::cout << '#';
            else
                std::cout << ' ';
        }

    for (size_t i = 0; i < snake.size(); ++i) {
        setPos(snake[i].x, snake[i].y + HDR);
        setColor(1);
        std::cout << (i == 0 ? '@' : 'o');
        setColor(0);
    }

    setPos(food.x, food.y + HDR);
    setColor(2); std::cout << '*'; setColor(0);

    if (poisonOn) {
        setPos(poison.x, poison.y + HDR);
        setColor(3); std::cout << 'X'; setColor(0);
    }
    if (speedOn) {
        setPos(speedPot.x, speedPot.y + HDR);
        setColor(5); std::cout << '^'; setColor(0);
    }

    drawStatusBar(score, boosted);                                // ← 新增
    flush();
}

// ==================== 主入口 ====================
void startSnakeGame() {
    using namespace snake_con;

    MusicManager::playBackgroundMusic("music/snake_bg.mp3");

#if !defined(_WIN32)
    TermGuard tg;
#endif

    srand((unsigned)time(nullptr));

    std::deque<SC> snake;
    int cx = BW / 2, cy = BH / 2;
    snake.push_back({cx,     cy});
    snake.push_back({cx - 1, cy});
    snake.push_back({cx - 2, cy});

    int dir   = 0;
    int score = 0;
    int eatCnt = 0;
    bool alive = true;
    bool won   = false;                                           // ← 新增：是否达成目标

    const int BASE_MS = 150;
    int  curDelay = BASE_MS;
    bool boosted  = false;
    auto boostEnd = std::chrono::steady_clock::now();

    std::vector<SC> occ;
    SC food = randFree(snake, occ);

    SC poison  {-1, -1}; bool poisonOn = false;
    SC speedPot{-1, -1}; bool speedOn  = false;

    fullDraw(snake, food, poisonOn, poison, speedOn, speedPot, score, boosted);
    clearInput();

    // =====================  游戏主循环  =====================
    while (alive) {

        // --- 检查加速是否到期 ---
        if (boosted && std::chrono::steady_clock::now() >= boostEnd) {
            boosted  = false;
            curDelay = BASE_MS;
            drawStatusBar(score, boosted);                        // ← 改用 drawStatusBar
        }

        // --- 读取输入 ---
        char k = getInput();
        if (k == 'Q') { alive = false; break; }
        if (k == 'W' && dir != 1) dir = 3;
        else if (k == 'S' && dir != 3) dir = 1;
        else if (k == 'A' && dir != 0) dir = 2;
        else if (k == 'D' && dir != 2) dir = 0;

        // --- 计算新蛇头 ---
        SC nh = snake.front();
        switch (dir) {
            case 0: nh.x++; break;
            case 1: nh.y++; break;
            case 2: nh.x--; break;
            case 3: nh.y--; break;
        }

        // --- 撞墙检测 ---
        if (nh.x <= 0 || nh.x >= BW - 1 || nh.y <= 0 || nh.y >= BH - 1) break;

        // --- 撞自己检测 ---
        if (onBody(snake, nh.x, nh.y)) break;

        // --- 移动蛇 ---
        snake.push_front(nh);

        bool ateFood   = (nh.x == food.x     && nh.y == food.y);
        bool atePoison = (poisonOn && nh.x == poison.x   && nh.y == poison.y);
        bool ateSpeed  = (speedOn  && nh.x == speedPot.x && nh.y == speedPot.y);

        if (ateFood) {
            score += 10;
            eatCnt++;

            // ---- 检查是否达到目标分数 ----                      // ← 新增
            if (score >= TARGET_SCORE) {
                won = true;
                alive = false;
                drawStatusBar(score, boosted);
                flush();
                break;
            }

            occ.clear();
            if (poisonOn) occ.push_back(poison);
            if (speedOn)  occ.push_back(speedPot);
            food = randFree(snake, occ);
            setPos(food.x, food.y + HDR);
            setColor(2); std::cout << '*'; setColor(0);

            if (eatCnt % 2 == 0 && !poisonOn) {
                occ.clear(); occ.push_back(food);
                if (speedOn) occ.push_back(speedPot);
                poison = randFree(snake, occ);
                poisonOn = true;
                setPos(poison.x, poison.y + HDR);
                setColor(3); std::cout << 'X'; setColor(0);
            }

            if (eatCnt % 3 == 0 && !speedOn) {
                occ.clear(); occ.push_back(food);
                if (poisonOn) occ.push_back(poison);
                speedPot = randFree(snake, occ);
                speedOn = true;
                setPos(speedPot.x, speedPot.y + HDR);
                setColor(5); std::cout << '^'; setColor(0);
            }
        }

        if (atePoison) {
            poisonOn = false;
            score = std::max(0, score - 5);
            for (int i = 0; i < 2 && (int)snake.size() > 1; ++i) {
                SC t = snake.back(); snake.pop_back();
                setPos(t.x, t.y + HDR); std::cout << ' ';
            }
        }

        if (ateSpeed) {
            speedOn  = false;
            boosted  = true;
            curDelay = BASE_MS / 2;
            boostEnd = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        }

        if (!ateFood && (int)snake.size() > 1) {
            SC t = snake.back(); snake.pop_back();
            setPos(t.x, t.y + HDR); std::cout << ' ';
        }

        setPos(nh.x, nh.y + HDR);
        setColor(1); std::cout << '@'; setColor(0);

        if ((int)snake.size() > 1) {
            setPos(snake[1].x, snake[1].y + HDR);
            setColor(1); std::cout << 'o'; setColor(0);
        }

        // ---- 更新底部状态栏 ----                               // ← 改用 drawStatusBar
        drawStatusBar(score, boosted);

        flush();
        sleepMs(curDelay);
    }

    // =====================  游戏结束  =====================      // ← 整段重写
    clear();
    std::cout << "\n\n";

    if (won) {
        // 达成目标
        std::cout << "    ==========================================\n";
        std::cout << "       M I S S I O N   A C C O M P L I S H E D\n";
        std::cout << "       Final Score : " << score << " / " << TARGET_SCORE << "\n";
        std::cout << "    ==========================================\n\n";
        std::cout << "    Returning to maze...\n";
    } else {
        // 死亡或主动退出
        std::cout << "    ================================\n";
        std::cout << "          G A M E   O V E R\n";
        std::cout << "       Final Score : " << score << " / " << TARGET_SCORE << "\n";
        std::cout << "    ================================\n\n";
        std::cout << "    Returning to maze...\n";
    }

    flush();
    clearInput();
    sleepMs(2000);      // ← 停留2秒后自动返回迷宫，不再等按键
    clearInput();
}