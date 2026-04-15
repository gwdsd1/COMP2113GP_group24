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

// -------------------- �ؿ����� --------------------
struct Stage {
    std::string id;          // �ؿ����
    std::string singer;      // ����
    std::string music;       // ����
    std::string chartPath;   // �����ļ�·��
    std::string musicPath;   // �����ļ�·��
};

//�ڴ����ӹؿ�
static const std::vector<Stage> stages = {
    { "1", "Yorushika", "Paddle", "charts/yorushika_paddle.chart", "music/yorushika_paddle.mp3" },
    { "2", "ZUTOMAYO", "Justice", "charts/zutomayo_justice.chart", "music/zutomayo_justice.mp3" },
	{ "3", "r-906" , "manimani","charts/r-906_manimani.chart","music/r-906_manimani.mp3"},
	{"4","n-buna","because summer will end","charts/n-buna_because_summer_will_end.chart","music/n-buna_because_summer_will_end.mp3"},
};

// -------------------- �ն˹��ߣ�ANSI�� --------------------
namespace term {
    static const char* CSI = "\x1b[";

    void hideCursor() { std::cout << CSI << "?25l"; }
    void showCursor() { std::cout << CSI << "?25h"; }
    void clearScreen() { std::cout << CSI << "2J"; }
    void moveHome() { std::cout << CSI << "H"; }
    void resetColor() { std::cout << CSI << "0m"; }
    void moveTo(int r, int c) { std::cout << CSI << r << ";" << c << "H"; }
    void setBlue() { std::cout << CSI << "34m"; }  // ��ɫ
    void setBrightBlue() { std::cout << CSI << "94m"; }  // ����ɫ
    void setRed() { std::cout << CSI << "91m"; }  // ����ɫ
    void setYellow() { std::cout << CSI << "93m"; }  // ����ɫ
    void setGreen() { std::cout << CSI << "92m"; }  // ����ɫ
    void setBrightCyan() { std::cout << CSI << "96m"; }  // ����ɫ
    void setNoteStyle() { std::cout << CSI << "1;97;44m"; }  // �Ӵְ�ɫ���� + ��ɫ����

#if defined(_WIN32)
    bool enableVT() {
        // ���� Windows 10+ �������ն˴�����ANSI��
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
        int origFlags = 0;

        TermiosGuard() { enterRaw(); }
        ~TermiosGuard() { restore(); }

        void enterRaw() {
            if (!isatty(STDIN_FILENO)) return;
            if (tcgetattr(STDIN_FILENO, &orig) == -1) return;

            termios raw = orig;
            raw.c_lflag &= ~(ICANON | ECHO);  // �ǹ淶 + �رջ���
            raw.c_cc[VMIN] = 0;              // ������
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return;

            // �� stdin ������
            origFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, origFlags | O_NONBLOCK);
            ok = true;
        }
        void restore() {
            if (!ok) return;
            tcsetattr(STDIN_FILENO, TCSANOW, &orig);
            fcntl(STDIN_FILENO, F_SETFL, origFlags);
            ok = false;
        }
    };
#endif
} // namespace term

// -------------------- ���루����¼"���±���"�� --------------------
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
            // ��չ����������ȣ����ٴ� _getch() ��ȡɨ���룬�������
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
            // ������Է������ ESC ���У���������ĸ��
            if (isalpha(c)) c = static_cast<unsigned char>(toupper(c));
            in.pushChar(c);
        }
    }
}
#endif

// -------------------- ��Ϸ���ݽṹ --------------------
struct Note {
    int lane = 0;     // 0..5 -> S D F | J K L
    double y = 0.0;   // 0 ��������������
    bool dead = false; // ���л�ʱ
};

// �ж�������Ϣ
struct JudgeFeedback {
    std::string text = "";
    int lane = -1;
    double timer = 0.0;
    int color = 0; // 0=��, 1=��ɫ(Perfect), 2=��ɫ(Good), 3=��ɫ(Miss)
};

struct Game {
    // ��Ļ�ߴ磨�С��У�
    int H = 28;
    int W = 64;

    // 6 �� X ���꣨�У�
    int lanes = 6;
    int laneX[6] = { 10, 18, 26, 38, 46, 54 };

    // �ж��� Y���У�
    int judgeY = 24;

    // �ж����ڣ���λ���У�
    int perfectWindow = 1; // ��1 ��
    int goodWindow = 2; // ��2 ��

    // �����ٶ������Ƶ��
    double speedRowsPerSec = 15.0;
    double spawnInterval = 0.6;
    double spawnTimer = 0.0;

    // ����������������
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int life = 100;
    
    // �ж�ͳ��
    int perfectCount = 0;
    int goodCount = 0;
    int missCount = 0;

    vector<Note> notes;
    
    // �ж�����
    JudgeFeedback feedback;
    array<double, 6> laneHitTimer{}; // ����������ʱ��

    // ��λӳ�䣨6 �죩��S D F | J K L
    array<char, 6> laneKeys = { 'S','D','F','J','K','L' };
    
    // ����ģʽ
    bool useChart = false;
    ChartData chart;
    size_t nextNoteIndex = 0;
    double gameTime = 0.0;  // ��Ϸʱ�䣨�룩
    
    // ���׽������
    bool chartFinished = false;    // ���������Ѵ������
    double finishTimer = 0.0;      // ������ĵ���ʱ���룩
    static constexpr double FINISH_DELAY = 1.0; // ������ȴ�1��
    bool gameOver = false;         // ������Ϸ�Ƿ����
    
    // ��������
    bool loadChart(const std::string& filepath) {
        if (chart.loadFromFile(filepath)) {
            useChart = true;
            nextNoteIndex = 0;
            gameTime = 0.0;
            chartFinished = false;
            finishTimer = 0.0;
            gameOver = false;
            
            // ����BPM�����ٶ�
            speedRowsPerSec = 15.0;
            
            return true;
        }
        return false;
    }
    
    // ���������Ƿ�����������Ļ���޴������
    bool allNotesCleared() const {
        return nextNoteIndex >= chart.notes.size() && notes.empty();
    }

    // ����Ƿ�Ӧ������Ϸ
    bool shouldEnd() const {
        return gameOver || (chartFinished && finishTimer >= FINISH_DELAY);
    }

    void update(double dt, const Input& input) {
        // ������Ϸʱ��
        gameTime += dt;
        
        // ��������ѽ������ۼƵ���ʱ
        if (chartFinished) {
            finishTimer += dt;
        }
        
        // �����ж�������ʱ��
        if (feedback.timer > 0) {
            feedback.timer -= dt;
            if (feedback.timer <= 0) feedback.text = "";
        }
        
        // ���¹��������ʱ��
        for (int i = 0; i < lanes; ++i) {
            if (laneHitTimer[i] > 0) laneHitTimer[i] -= dt;
        }
        
        // 1) ��������
        if (useChart) {
            // ����ģʽ������ʱ����������
            double spawnAheadTime = (judgeY / speedRowsPerSec); // ��ǰ����ʱ��

            while (nextNoteIndex < chart.notes.size()) {
                const auto& chartNote = chart.notes[nextNoteIndex];
                double adjustedTime = chartNote.time + chart.offset;

                // ����Ƿ񵽴�����ʱ��
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

            // ע�⣺���׽��������������ѭ����������ֲ���״̬�ж�
        }

        // 2) ����
        for (auto it = notes.begin(); it != notes.end(); ) {
            it->y += speedRowsPerSec * dt;
            ++it;
        }

        // 3) �ж��������±��أ�
        for (int li = 0; li < lanes; ++li) {
            if (input.pressed(laneKeys[li])) {
                laneHitTimer[li] = 0.15; // ���ù������ʱ��
                
                int bestIdx = -1;
                int bestDist = 1000000000; // �� 1e9 ��Ϊ���������������� double �� int ��ת������
                for (int i = 0; i < (int)notes.size(); ++i) {
                    auto& n = notes[i];
                    if (n.dead || n.lane != li) continue;
                    int dist = (int)std::lround(n.y) - judgeY; // ��=�ѹ���
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
                        // ��ʾ Perfect ����
                        feedback.text = "PERFECT!";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 1;
                    }
                    else if (ad <= goodWindow) {
                        score += 5;  combo++; maxCombo = std::max(maxCombo, combo);
                        goodCount++;
                        n.dead = true;
                        // ��ʾ Good ����
                        feedback.text = "GOOD";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 2;
                    }
                    else {
                        // ƫ����󣬺��Ա��λ��򣨲��۷֣�
                    }
                }
            }
        }

        // 4) ��ʱ Miss
        for (auto& n : notes) {
            if (!n.dead && n.y > judgeY + goodWindow + 1) {
                n.dead = true;
                combo = 0;
                missCount++;
                life = std::max(0, life - 2);
                if (life <= 0) gameOver = true;
                // ��ʾ Miss ����
                feedback.text = "MISS";
                feedback.lane = n.lane;
                feedback.timer = 0.5;
                feedback.color = 3;
            }
        }

        // 5) ������������� erase-remove��
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [this](const Note& n) { return n.dead || n.y > (double)(H - 1); }),
            notes.end());
    }

    void render() {
        term::moveHome();

        // ���� HUD
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
        
        // �ж�ͳ����
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
        
        // ���׽�������ʱ��ʾ
        if (chartFinished) {
            std::cout << "    ";
            term::setYellow();
            int remaining = (int)std::ceil(FINISH_DELAY - finishTimer);
            if (remaining < 0) remaining = 0;
            std::cout << "Song finished! Ending in " << remaining << "s...";
            term::resetColor();
        }
        std::cout << "\n";

        // ����
        static std::vector<std::string> canvas;
        canvas.assign(H, std::string(W, ' '));

        // �����������Ч����
        for (int i = 0; i < lanes; ++i) {
            int x = laneX[i];
            for (int y = 1; y < H - 2; ++y) {
                if (x >= 0 && x < W) {
                    // �����������£�ʹ�������ַ�
                    canvas[y][x] = (laneHitTimer[i] > 0) ? '!' : '|';
                }
            }
        }

        // �ж���
        if (judgeY >= 0 && judgeY < H) {
            for (int x = 0; x < W; ++x) canvas[judgeY][x] = '-';
        }

        // ����
        for (auto& n : notes) {
            int x = laneX[n.lane];
            int y = (int)std::lround(n.y);
            if (y >= 0 && y < H && x >= 0 && x < W) {
                canvas[y][x] = '@';  // ʹ�� @ ���ţ�������Ŀ
            }
        }
        
        // �ж�������ʾ
        if (feedback.timer > 0 && feedback.lane >= 0 && feedback.lane < lanes) {
            int x = laneX[feedback.lane];
            int y = judgeY - 3; // ���ж����Ϸ���ʾ
            if (y >= 0 && y < H) {
                // ������λ����ʾ�ж�����
                int textLen = (int)feedback.text.length();
                int startX = std::max(0, x - textLen / 2);
                for (int i = 0; i < textLen && startX + i < W; ++i) {
                    canvas[y][startX + i] = feedback.text[i];
                }
            }
        }

        // ��λ��ʾ - ���뵽ÿ������·�
        if (H - 2 >= 0 && H - 2 < (int)canvas.size()) {
            for (int i = 0; i < lanes; ++i) {
                int x = laneX[i];
                if (x >= 0 && x < W) {
                    canvas[H - 2][x] = laneKeys[i];
                }
            }
        }
        
        // �˳���ʾ�������һ��
        if (H - 1 >= 0 && H - 1 < (int)canvas.size()) {
            std::string hint = "(Q=quit)";
            int startX = W - (int)hint.size() - 1;
            if (startX > 0) {
                for (int i = 0; i < (int)hint.size() && startX + i < W; ++i) {
                    canvas[H - 1][startX + i] = hint[i];
                }
            }
        }
        

        // ���
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < (int)canvas[y].size(); ++x) {
                char ch = canvas[y][x];
                if (ch == '@') {
                    term::setNoteStyle();  // ������Ŀ��������ʽ���������ף�
                    std::cout << ch;
                    term::resetColor();      // ������ɫ
                } else if (ch == '!') {
                    // �����Ĺ��
                    term::setBrightCyan();
                    std::cout << ch;
                    term::resetColor();
                } else if (feedback.timer > 0 && y == judgeY - 3) {
                    // �ж��������򣬸����ж�������ɫ
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

// -------------------- ����ʱ���� --------------------
void showCountdown() {
    const int countdown[] = {3, 2, 1};
    
    for (int num : countdown) {
        term::clearScreen();
        term::moveHome();
        
        // ������Ļ����λ��
        int centerRow = 14;
        int centerCol = 32;
        
        // ���ƴ������Ч��
        term::moveTo(centerRow - 2, centerCol - 5);
        term::setYellow();
        std::cout << "+---------+";

        term::moveTo(centerRow - 1, centerCol - 5);
        std::cout << "|         |";

        term::moveTo(centerRow, centerCol - 5);
        term::setRed();
        std::cout << "|    " << num << "    |";

        term::moveTo(centerRow + 1, centerCol - 5);
        term::setYellow();
        std::cout << "|         |";

        term::moveTo(centerRow + 2, centerCol - 5);
        std::cout << "+---------+";
        
        term::resetColor();
        std::cout.flush();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
    
    // ��ʾ "GO!"
    term::clearScreen();
    term::moveHome();
    
    int centerRow = 14;
    int centerCol = 32;
    
    term::moveTo(centerRow - 2, centerCol - 5);
    term::setGreen();
    std::cout << "+---------+";

    term::moveTo(centerRow - 1, centerCol - 5);
    std::cout << "|         |";

    term::moveTo(centerRow, centerCol - 5);
    std::cout << "|   GO!   |";

    term::moveTo(centerRow + 1, centerCol - 5);
    std::cout << "|         |";

    term::moveTo(centerRow + 2, centerCol - 5);
    std::cout << "+---------+";
    
    term::resetColor();
    std::cout.flush();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// -------------------- ��Ϸ�������� --------------------
void showGameOver(const Game& game) {
    term::clearScreen();
    term::moveHome();
    
    int centerRow = 10;
    int centerCol = 32;
    
    // ��ʾ GAME OVER
    term::moveTo(centerRow, centerCol - 10);
    term::setRed();
    std::cout << "+--------------------+";

    term::moveTo(centerRow + 1, centerCol - 10);
    std::cout << "|                    |";

    term::moveTo(centerRow + 2, centerCol - 10);
    term::setYellow();
    std::cout << "|   GAME  OVER!      |";

    term::moveTo(centerRow + 3, centerCol - 10);
    term::setRed();
    std::cout << "|                    |";

    term::moveTo(centerRow + 4, centerCol - 10);
    std::cout << "+--------------------+";
    
    // ��ʾ����ͳ��
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
    
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

// -------------------- ������ --------------------
int startMusicGameInternal() {
    std::srand((unsigned)std::time(nullptr));

#if defined(_WIN32)
    term::enableVT(); // �������� ANSI
#else
    term::TermiosGuard tg; // ���� raw ģʽ���˳��Զ��ָ�
#endif

    term::hideCursor();
    term::clearScreen();
    term::moveHome();
    
    Game game;
    // ʹ��ȫ�� MusicManager������Ҫ�ֲ� musicPlayer
    
    // -------- �ؿ�ģʽ --------
    const Stage* selected = nullptr;
    while (!selected) {
        term::clearScreen();
        term::moveHome();

        term::setBrightCyan();
        std::cout << "+----------------------------------------+\n";
        std::cout << "|          Stage Select                  |\n";
        std::cout << "+----------------------------------------+\n\n";
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
        // �� Linux �¶������в����ı�׼���뻺�壬�Է�֮ǰ�Ĳ�����������
        {
            int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
            char ch;
            while(read(STDIN_FILENO, &ch, 1) > 0);
            fcntl(STDIN_FILENO, F_SETFL, oldf);
        }
#endif
        std::cout.flush();

        std::string stageInput;
        Input selIn;
        while (true) {
            pollInput(selIn);
            for (char c = '1'; c <= '9'; ++c) {
                if (selIn.pressed(c)) {
                    stageInput = std::string(1, c);
                    break;
                }
            }
            if (!stageInput.empty()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        std::cout << stageInput << "\n" << std::flush;

        // ���Ҷ�Ӧ�ؿ�
        for (const auto& s : stages) {
            if (s.id == stageInput) { selected = &s; break; }
        }

        if (selected) {
            term::setYellow();
            std::cout << "\nLoading stage: " << selected->singer << " - " << selected->music << "\n";
            term::resetColor();

            // �Զ���������
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

                // �Զ��������֣�ʹ�� MusicManager��
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

    // ��ʾ����ʱ
    showCountdown();

    // ����ʱ������ʼ��������
    MusicManager::getPlayer().play();

    Input input;

    // �̶��߼����� 120Hz����Ⱦ ~60FPS
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

        // ���루��������
        Input tempInput;
        pollInput(tempInput);
        for (int i = 0; i < 256; ++i) {
            if (tempInput.justPressed[i]) input.justPressed[i] = true;
        }
        if (input.pressed('Q')) running = false;

        // �߼��ಽ����
        while (acc >= dt) {
            game.update(dt, input);
            input.clear();
            acc -= dt;
        }
        
        // 曲谱模式：音符清除后等待 FINISH_DELAY 退出
        if (game.useChart) {
            // 音符全部清除后等待一定时间就退出（不依赖音乐是否播放完成）
            if (!game.chartFinished && game.allNotesCleared()) {
                game.chartFinished = true;
                game.finishTimer = 0.0;
            }
            if (game.shouldEnd()) {
                running = false;
            }
        }

        // ��Ⱦ��Լ 60 FPS��
        if (renderAcc >= (1.0 / 60.0)) {
            game.render();
            renderAcc = 0.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // ֹͣ��ǰ���֣�����ʱ���Զ����ű�������
    MusicManager::stop();

    // ��ʾ��Ϸ��������
    showGameOver(game);
    
    term::resetColor();
    term::showCursor();
    term::clearScreen();
    term::moveHome();
    
    // �������շ���
    return game.score;
}
