#include <array>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <conio.h>   // _kbhit, _getch
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#include "ChartLoader.h"
#include "MusicManager.h"

using namespace std;

// -------------------- 关卡数据 --------------------
struct Stage {
    std::string id;          // 关卡编号
    std::string singer;      // 歌手
    std::string music;       // 曲名
    std::string chartPath;   // 曲谱文件路径
    std::string musicPath;   // 音乐文件路径
};

//在此添加关卡
static const std::vector<Stage> stages = {
    { "1", "Yorushika", "Paddle", "charts/yorushika_paddle.chart", "music/yorushika_paddle.mp3" },
    { "2", "ZUTOMAYO", "Justice", "charts/zutomayo_justice.chart", "music/zutomayo_justice.mp3" },
	{ "3", "r-906" , "manimani","charts/r-906_manimani.chart","music/r-906_manimani.mp3"},
	{"4","n-buna","because summer will end","charts/n-buna_because_summer_will_end.chart","music/n-buna_because_summer_will_end.mp3"},
};

// -------------------- 终端工具（ANSI） --------------------
namespace term {
    static const char* CSI = "\x1b[";

    void hideCursor() { std::cout << CSI << "?25l"; }
    void showCursor() { std::cout << CSI << "?25h"; }
    void clearScreen() { std::cout << CSI << "2J"; }
    void moveHome() { std::cout << CSI << "H"; }
    void resetColor() { std::cout << CSI << "0m"; }
    void moveTo(int r, int c) { std::cout << CSI << r << ";" << c << "H"; }
    void setBlue() { std::cout << CSI << "34m"; }  // 蓝色
    void setBrightBlue() { std::cout << CSI << "94m"; }  // 亮蓝色
    void setRed() { std::cout << CSI << "91m"; }  // 亮红色
    void setYellow() { std::cout << CSI << "93m"; }  // 亮黄色
    void setGreen() { std::cout << CSI << "92m"; }  // 亮绿色
    void setBrightCyan() { std::cout << CSI << "96m"; }  // 亮青色
    void setNoteStyle() { std::cout << CSI << "1;97;44m"; }  // 加粗白色文字 + 蓝色背景

#if defined(_WIN32)
    bool enableVT() {
        // 启用 Windows 10+ 的虚拟终端处理（ANSI）
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return false;
        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) return false;
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        return SetConsoleMode(hOut, mode) != 0;
    }
#else
    struct TermiosGuard {
        termios orig{};
        bool ok = false;

        TermiosGuard() { enterRaw(); }
        ~TermiosGuard() { restore(); }

        void enterRaw() {
            if (!isatty(STDIN_FILENO)) return;
            if (tcgetattr(STDIN_FILENO, &orig) == -1) return;

            termios raw = orig;
            raw.c_lflag &= ~(ICANON | ECHO);  // 非规范 + 关闭回显
            raw.c_cc[VMIN] = 0;              // 非阻塞
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return;

            // 设 stdin 非阻塞
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
            ok = true;
        }
        void restore() {
            if (!ok) return;
            tcsetattr(STDIN_FILENO, TCSANOW, &orig);
            ok = false;
        }
    };
#endif
} // namespace term

// -------------------- 输入（仅记录"按下边沿"） --------------------
struct Input {
    array<bool, 256> justPressed{};

    void clear() { justPressed.fill(false); }

    void pushChar(unsigned char ch) {
        if (ch < justPressed.size()) justPressed[ch] = true;
    }
    bool pressed(char ch) const {
        unsigned char u = static_cast<unsigned char>(toupper(static_cast<unsigned char>(ch)));
        return u < justPressed.size() ? justPressed[u] : false;
    }
};

#if defined(_WIN32)
void pollInput(Input& in) {
    in.clear();
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            // 扩展键（方向键等），再次 _getch() 可取扫描码，这里忽略
            (void)_getch();
        }
        else {
            unsigned char c = static_cast<unsigned char>(ch);
            if (isalpha(c)) c = static_cast<unsigned char>(toupper(c));
            in.pushChar(c);
        }
    }
}
#else
void pollInput(Input& in) {
    in.clear();
    unsigned char buf[64];
    while (true) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; ++i) {
            unsigned char c = buf[i];
            // 这里忽略方向键的 ESC 序列，仅处理字母键
            if (isalpha(c)) c = static_cast<unsigned char>(toupper(c));
            in.pushChar(c);
        }
    }
}
#endif

// -------------------- 游戏数据结构 --------------------
struct Note {
    int lane = 0;     // 0..5 -> S D F | J K L
    double y = 0.0;   // 0 顶部，向下增大
    bool dead = false; // 命中或超时
};

// 判定反馈信息
struct JudgeFeedback {
    std::string text = "";
    int lane = -1;
    double timer = 0.0;
    int color = 0; // 0=无, 1=绿色(Perfect), 2=黄色(Good), 3=红色(Miss)
};

struct Game {
    // 屏幕尺寸（行、列）
    int H = 28;
    int W = 64;

    // 6 轨 X 坐标（列）
    int lanes = 6;
    int laneX[6] = { 10, 18, 26, 38, 46, 54 };

    // 判定线 Y（行）
    int judgeY = 24;

    // 判定窗口（单位：行）
    int perfectWindow = 0; // ±0 行
    int goodWindow = 1; // ±1 行

    // 下落速度与产生频率
    double speedRowsPerSec = 15.0;
    double spawnInterval = 0.6;
    double spawnTimer = 0.0;

    // 分数、连击、生命
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int life = 100;
    
    // 判定统计
    int perfectCount = 0;
    int goodCount = 0;
    int missCount = 0;

    vector<Note> notes;
    
    // 判定反馈
    JudgeFeedback feedback;
    array<double, 6> laneHitTimer{}; // 按键高亮计时器

    // 键位映射（6 轨）：S D F | J K L
    array<char, 6> laneKeys = { 'S','D','F','J','K','L' };
    
    // 曲谱模式
    bool useChart = false;
    ChartData chart;
    size_t nextNoteIndex = 0;
    double gameTime = 0.0;  // 游戏时间（秒）
    
    // 曲谱结束检测
    bool chartFinished = false;    // 所有音符已处理完毕
    double finishTimer = 0.0;      // 结束后的倒计时（秒）
    static constexpr double FINISH_DELAY = 1.0; // 结束后等待1秒
    
    // 加载曲谱
    bool loadChart(const std::string& filepath) {
        if (chart.loadFromFile(filepath)) {
            useChart = true;
            nextNoteIndex = 0;
            gameTime = 0.0;
            chartFinished = false;
            finishTimer = 0.0;
            
            // 根据BPM调整速度
            speedRowsPerSec = 15.0;
            
            return true;
        }
        return false;
    }
    
    // 所有音符是否已生成且屏幕上无存活音符
    bool allNotesCleared() const {
        return nextNoteIndex >= chart.notes.size() && notes.empty();
    }

    // 检查是否应结束游戏
    bool shouldEnd() const {
        return chartFinished && finishTimer >= FINISH_DELAY;
    }

    void update(double dt, const Input& input) {
        // 更新游戏时间
        gameTime += dt;
        
        // 如果曲谱已结束，累计倒计时
        if (chartFinished) {
            finishTimer += dt;
        }
        
        // 更新判定反馈计时器
        if (feedback.timer > 0) {
            feedback.timer -= dt;
            if (feedback.timer <= 0) feedback.text = "";
        }
        
        // 更新轨道高亮计时器
        for (int i = 0; i < lanes; ++i) {
            if (laneHitTimer[i] > 0) laneHitTimer[i] -= dt;
        }
        
        // 1) 生成音符
        if (useChart) {
            // 曲谱模式：根据时间生成音符
            double spawnAheadTime = (judgeY / speedRowsPerSec); // 提前生成时间

            while (nextNoteIndex < chart.notes.size()) {
                const auto& chartNote = chart.notes[nextNoteIndex];
                double adjustedTime = chartNote.time + chart.offset;

                // 检查是否到达生成时间
                if (gameTime >= adjustedTime - spawnAheadTime) {
                    Note n;
                    n.lane = chartNote.lane;
                    n.y = 0.0;
                    notes.push_back(n);
                    nextNoteIndex++;
                } else {
                    break;
                }
            }

            // 注意：曲谱结束检测已移至主循环，结合音乐播放状态判断
        }

        // 2) 掉落
        for (auto it = notes.begin(); it != notes.end(); ) {
            it->y += speedRowsPerSec * dt;
            ++it;
        }

        // 3) 判定（仅按下边沿）
        for (int li = 0; li < lanes; ++li) {
            if (input.pressed(laneKeys[li])) {
                laneHitTimer[li] = 0.15; // 设置轨道高亮时长
                
                int bestIdx = -1;
                int bestDist = 1000000000; // 将 1e9 改为整数字面量，避免 double 到 int 的转换警告
                for (int i = 0; i < (int)notes.size(); ++i) {
                    auto& n = notes[i];
                    if (n.dead || n.lane != li) continue;
                    int dist = (int)std::lround(n.y) - judgeY; // 正=已过线
                    int ad = std::abs(dist);
                    if (ad < bestDist) { bestDist = ad; bestIdx = i; }
                }
                if (bestIdx >= 0) {
                    auto& n = notes[bestIdx];
                    int dist = (int)std::lround(n.y) - judgeY;
                    int ad = std::abs(dist);
                    if (ad <= perfectWindow) {
                        score += 10; combo++; maxCombo = std::max(maxCombo, combo);
                        perfectCount++;
                        n.dead = true;
                        // 显示 Perfect 反馈
                        feedback.text = "PERFECT!";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 1;
                    }
                    else if (ad <= goodWindow) {
                        score += 5;  combo++; maxCombo = std::max(maxCombo, combo);
                        goodCount++;
                        n.dead = true;
                        // 显示 Good 反馈
                        feedback.text = "GOOD";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 2;
                    }
                    else {
                        // 偏离过大，忽略本次击打（不扣分）
                    }
                }
            }
        }

        // 4) 超时 Miss
        for (auto& n : notes) {
            if (!n.dead && n.y > judgeY + goodWindow + 1) {
                n.dead = true;
                combo = 0;
                missCount++;
                life = std::max(0, life - 2);
                // 显示 Miss 反馈
                feedback.text = "MISS";
                feedback.lane = n.lane;
                feedback.timer = 0.5;
                feedback.color = 3;
            }
        }

        // 5) 清理（修正后的 erase-remove）
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [this](const Note& n) { return n.dead || n.y > (double)(H - 1); }),
            notes.end());
    }

    void render() {
        term::moveHome();

        // 顶部 HUD
        std::cout << "Console Rhythm 6L  |  ";
        if (useChart) {
            term::setBrightCyan();
            std::cout << "[" << chart.title << "]";
            term::resetColor();
            std::cout << "  ";
        }
        std::cout << "Score: "
            << score << "   Combo: " << combo
            << "   MaxCombo: " << maxCombo
            << "   Life: " << life << "\n";
        
        // 判定统计行
        term::setGreen();
        std::cout << "Perfect: " << perfectCount;
        term::resetColor();
        std::cout << "  ";
        term::setYellow();
        std::cout << "Good: " << goodCount;
        term::resetColor();
        std::cout << "  ";
        term::setRed();
        std::cout << "Miss: " << missCount;
        term::resetColor();
        
        // 曲谱结束倒计时提示
        if (chartFinished) {
            std::cout << "    ";
            term::setYellow();
            int remaining = (int)std::ceil(FINISH_DELAY - finishTimer);
            if (remaining < 0) remaining = 0;
            std::cout << "Song finished! Ending in " << remaining << "s...";
            term::resetColor();
        }
        std::cout << "\n";

        // 画布
        static std::vector<std::string> canvas;
        canvas.assign(H, std::string(W, ' '));

        // 轨道（带高亮效果）
        for (int i = 0; i < lanes; ++i) {
            int x = laneX[i];
            for (int y = 1; y < H - 2; ++y) {
                if (x >= 0 && x < W) {
                    // 如果轨道被按下，使用特殊字符
                    canvas[y][x] = (laneHitTimer[i] > 0) ? '!' : '|';
                }
            }
        }

        // 判定线
        if (judgeY >= 0 && judgeY < H) {
            for (int x = 0; x < W; ++x) canvas[judgeY][x] = '-';
        }

        // 音符
        for (auto& n : notes) {
            int x = laneX[n.lane];
            int y = (int)std::lround(n.y);
            if (y >= 0 && y < H && x >= 0 && x < W) {
                canvas[y][x] = '@';  // 使用 @ 符号，更加醒目
            }
        }
        
        // 判定反馈显示
        if (feedback.timer > 0 && feedback.lane >= 0 && feedback.lane < lanes) {
            int x = laneX[feedback.lane];
            int y = judgeY - 3; // 在判定线上方显示
            if (y >= 0 && y < H) {
                // 在中心位置显示判定文字
                int textLen = (int)feedback.text.length();
                int startX = std::max(0, x - textLen / 2);
                for (int i = 0; i < textLen && startX + i < W; ++i) {
                    canvas[y][startX + i] = feedback.text[i];
                }
            }
        }

        // 键位提示 - 对齐到每个轨道下方
        if (H - 2 >= 0 && H - 2 < (int)canvas.size()) {
            for (int i = 0; i < lanes; ++i) {
                int x = laneX[i];
                if (x >= 0 && x < W) {
                    canvas[H - 2][x] = laneKeys[i];
                }
            }
        }
        
        // 退出提示放在最后一行
        if (H - 1 >= 0 && H - 1 < (int)canvas.size()) {
            std::string hint = "(Q=quit)";
            int startX = W - (int)hint.size() - 1;
            if (startX > 0) {
                for (int i = 0; i < (int)hint.size() && startX + i < W; ++i) {
                    canvas[H - 1][startX + i] = hint[i];
                }
            }
        }
        

        // 输出
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < (int)canvas[y].size(); ++x) {
                char ch = canvas[y][x];
                if (ch == '@') {
                    term::setNoteStyle();  // 设置醒目的音符样式（白字蓝底）
                    std::cout << ch;
                    term::resetColor();      // 重置颜色
                } else if (ch == '!') {
                    // 高亮的轨道
                    term::setBrightCyan();
                    std::cout << ch;
                    term::resetColor();
                } else if (feedback.timer > 0 && y == judgeY - 3) {
                    // 判定文字区域，根据判定类型着色
                    bool isJudgeText = false;
                    if (feedback.lane >= 0 && feedback.lane < lanes) {
                        int textX = laneX[feedback.lane];
                        int textLen = (int)feedback.text.length();
                        int startX = std::max(0, textX - textLen / 2);
                        if (x >= startX && x < startX + textLen) {
                            isJudgeText = true;
                        }
                    }
                    
                    if (isJudgeText) {
                        if (feedback.color == 1) term::setGreen();      // Perfect
                        else if (feedback.color == 2) term::setYellow(); // Good
                        else if (feedback.color == 3) term::setRed();    // Miss
                        std::cout << ch;
                        term::resetColor();
                    } else {
                        std::cout << ch;
                    }
                } else {
                    std::cout << ch;
                }
            }
            std::cout.put('\n');
        }
        std::cout.flush();
    }
};

// -------------------- 倒计时功能 --------------------
void showCountdown() {
    const int countdown[] = {3, 2, 1};
    
    for (int num : countdown) {
        term::clearScreen();
        term::moveHome();
        
        // 计算屏幕中央位置
        int centerRow = 14;
        int centerCol = 32;
        
        // 绘制大号数字效果
        term::moveTo(centerRow - 2, centerCol - 5);
        term::setYellow();
        std::cout << "═══════════";
        
        term::moveTo(centerRow - 1, centerCol - 5);
        std::cout << "║         ║";
        
        term::moveTo(centerRow, centerCol - 5);
        term::setRed();
        std::cout << "║    " << num << "    ║";
        
        term::moveTo(centerRow + 1, centerCol - 5);
        term::setYellow();
        std::cout << "║         ║";
        
        term::moveTo(centerRow + 2, centerCol - 5);
        std::cout << "═══════════";
        
        term::resetColor();
        std::cout.flush();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    // 显示 "GO!"
    term::clearScreen();
    term::moveHome();
    
    int centerRow = 14;
    int centerCol = 32;
    
    term::moveTo(centerRow - 2, centerCol - 5);
    term::setGreen();
    std::cout << "═══════════";
    
    term::moveTo(centerRow - 1, centerCol - 5);
    std::cout << "║         ║";
    
    term::moveTo(centerRow, centerCol - 5);
    std::cout << "║   GO!   ║";
    
    term::moveTo(centerRow + 1, centerCol - 5);
    std::cout << "║         ║";
    
    term::moveTo(centerRow + 2, centerCol - 5);
    std::cout << "═══════════";
    
    term::resetColor();
    std::cout.flush();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// -------------------- 游戏结束画面 --------------------
void showGameOver(const Game& game) {
    term::clearScreen();
    term::moveHome();
    
    int centerRow = 10;
    int centerCol = 32;
    
    // 显示 GAME OVER
    term::moveTo(centerRow, centerCol - 10);
    term::setRed();
    std::cout << "╔════════════════════╗";
    
    term::moveTo(centerRow + 1, centerCol - 10);
    std::cout << "║                    ║";
    
    term::moveTo(centerRow + 2, centerCol - 10);
    term::setYellow();
    std::cout << "║   GAME  OVER!      ║";
    
    term::moveTo(centerRow + 3, centerCol - 10);
    term::setRed();
    std::cout << "║                    ║";
    
    term::moveTo(centerRow + 4, centerCol - 10);
    std::cout << "╚════════════════════╝";
    
    // 显示最终统计
    term::moveTo(centerRow + 6, centerCol - 10);
    term::setBrightCyan();
    std::cout << "Final Score: " << game.score;
    
    term::moveTo(centerRow + 7, centerCol - 10);
    std::cout << "Max Combo: " << game.maxCombo;
    
    term::moveTo(centerRow + 9, centerCol - 10);
    term::setGreen();
    std::cout << "Perfect: " << game.perfectCount;
    
    term::moveTo(centerRow + 10, centerCol - 10);
    term::setYellow();
    std::cout << "Good: " << game.goodCount;
    
    term::moveTo(centerRow + 11, centerCol - 10);
    term::setRed();
    std::cout << "Miss: " << game.missCount;
    
    term::moveTo(centerRow + 13, centerCol - 10);
    term::resetColor();
    std::cout << "Thanks for playing!";
    
    term::resetColor();
    std::cout.flush();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}

// -------------------- 主程序 --------------------
void startMusicGameInternal() {
    std::srand((unsigned)std::time(nullptr));

#if defined(_WIN32)
    term::enableVT(); // 尝试启用 ANSI
#else
    term::TermiosGuard tg; // 进入 raw 模式，退出自动恢复
#endif

    term::hideCursor();
    term::clearScreen();
    term::moveHome();
    
    Game game;
    // 使用全局 MusicManager，不需要局部 musicPlayer
    
    // -------- 关卡模式 --------
    const Stage* selected = nullptr;
    while (!selected) {
        term::clearScreen();
        term::moveHome();

        term::setBrightCyan();
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║          Stage Select                  ║\n";
        std::cout << "╚════════════════════════════════════════╝\n\n";
        term::resetColor();

        for (size_t i = 0; i < stages.size(); ++i) {
            const auto& s = stages[i];
            term::setYellow();
            std::cout << "  [" << s.id << "] ";
            term::setBrightCyan();
            std::cout << s.singer;
            term::resetColor();
            std::cout << " - ";
            term::setGreen();
            std::cout << s.music << "\n";
            term::resetColor();
        }

        std::cout << "\n";
        term::setYellow();
        std::cout << "Enter stage number: ";
        term::resetColor();

        // Flush stdin buffer to throw away any stray keys left over from previous state
#if defined(_WIN32)
        while (_kbhit()) _getch();
#else
        // 在 Linux 下如果使用 termios，之前已在主循环 pollInput 处理过。
        // 若在此处不需要严格清空输入或可以用其他非阻塞方式，可暂时跳过。
        // 为确保兼容性，可以定义一个空操作或类似实现，此处因为使用了 stdin 输入，先简单屏蔽。
#endif
        std::cin.clear();

        std::string stageInput;
        std::getline(std::cin, stageInput);
        stageInput.erase(0, stageInput.find_first_not_of(" \t\r\n"));
        stageInput.erase(stageInput.find_last_not_of(" \t\r\n") + 1);

        // 查找对应关卡
        for (const auto& s : stages) {
            if (s.id == stageInput) { selected = &s; break; }
        }

        if (selected) {
            term::setYellow();
            std::cout << "\nLoading stage: " << selected->singer << " - " << selected->music << "\n";
            term::resetColor();

            // 自动加载曲谱
            if (!game.loadChart(selected->chartPath)) {
                term::setRed();
                std::cout << "? Failed to load chart: " << selected->chartPath << "\n";
                std::cout << "  Please select another stage...\n";
                term::resetColor();
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                selected = nullptr;
            } else {
                term::setGreen();
                std::cout << "? Chart loaded  (" << game.chart.notes.size() << " notes, BPM " << game.chart.bpm << ")\n";
                term::resetColor();

                // 自动加载音乐（使用 MusicManager）
                MusicManager::stop();
                if (MusicManager::getPlayer().load(selected->musicPath)) {
                    term::setGreen();
                    std::cout << "? Music loaded: " << selected->musicPath << "\n";
                    term::resetColor();
                } else {
                    term::setRed();
                    std::cout << "? Music not found: " << selected->musicPath << "\n";
                    std::cout << "  (game will run without music)\n";
                    term::resetColor();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            }
        } else {
            term::setRed();
            std::cout << "\n? Invalid stage number. Please try again...\n";
            term::resetColor();
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }

    term::hideCursor();
    term::clearScreen();

    // 显示倒计时
    showCountdown();

    // 倒计时结束后开始播放音乐
    MusicManager::getPlayer().play();

    Input input;

    // 固定逻辑步长 120Hz，渲染 ~60FPS
    const double dt = 1.0 / 120.0;
    auto now = []() { return std::chrono::steady_clock::now(); };
    auto prev = now();
    double acc = 0.0;
    double renderAcc = 0.0;
    bool running = true;

    while (running) {
        auto cur = now();
        double frame = std::chrono::duration<double>(cur - prev).count();
        prev = cur;
        acc += frame;
        renderAcc += frame;

        // 输入（非阻塞）
        pollInput(input);
        if (input.pressed('Q')) running = false;

        // 逻辑多步跑满
        while (acc >= dt) {
            game.update(dt, input);
            acc -= dt;
        }
        
        // 曲谱模式：音乐播放结束后再等待 FINISH_DELAY 退出
        if (game.useChart) {
            // 有音乐时以音乐结束为准，无音乐时以曲谱结束为准
            bool musicDone = MusicManager::isFinished();
            if (!game.chartFinished && game.allNotesCleared() && musicDone) {
                game.chartFinished = true;
                game.finishTimer = 0.0;
            }
            if (game.shouldEnd()) {
                running = false;
            }
        }

        // 渲染（约 60 FPS）
        if (renderAcc >= (1.0 / 60.0)) {
            game.render();
            renderAcc = 0.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 停止当前音乐，返回时会自动播放背景音乐
    MusicManager::stop();

    // 显示游戏结束画面
    showGameOver(game);
    
    term::resetColor();
    term::showCursor();
    term::clearScreen();
    term::moveHome();
}
