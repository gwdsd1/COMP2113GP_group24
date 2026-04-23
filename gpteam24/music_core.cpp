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

struct Stage {
    std::string id;          // Stage ID.
    std::string singer;      // Artist name.
    std::string music;       // Song title.
    std::string chartPath;   // Chart file path.
    std::string musicPath;   // Music file path.
};

// Built-in stage list.
static const std::vector<Stage> stages = {
    { "1", "Yorushika", "Paddle", "charts/yorushika_paddle.chart", "music/yorushika_paddle.mp3" },
    { "2", "ZUTOMAYO", "Justice", "charts/zutomayo_justice.chart", "music/zutomayo_justice.mp3" },
	{ "3", "r-906" , "manimani","charts/r-906_manimani.chart","music/r-906_manimani.mp3"},
	{"4","n-buna","because summer will end","charts/n-buna_because_summer_will_end.chart","music/n-buna_because_summer_will_end.mp3"},
};

// ANSI terminal helper utilities.
namespace term {
    static const char* CSI = "\x1b[";

    // What it does: Hides terminal cursor.
    // Inputs: None.
    // Outputs: None.
    void hideCursor() { std::cout << CSI << "?25l"; }

    // What it does: Shows terminal cursor.
    // Inputs: None.
    // Outputs: None.
    void showCursor() { std::cout << CSI << "?25h"; }

    // What it does: Clears terminal screen.
    // Inputs: None.
    // Outputs: None.
    void clearScreen() { std::cout << CSI << "2J"; }

    // What it does: Moves cursor to home position.
    // Inputs: None.
    // Outputs: None.
    void moveHome() { std::cout << CSI << "H"; }

    // What it does: Resets terminal text attributes.
    // Inputs: None.
    // Outputs: None.
    void resetColor() { std::cout << CSI << "0m"; }

    // What it does: Moves cursor to row/column.
    // Inputs: r is row index, c is column index.
    // Outputs: None.
    void moveTo(int r, int c) { std::cout << CSI << r << ";" << c << "H"; }

    // What it does: Sets blue text color.
    // Inputs: None.
    // Outputs: None.
    void setBlue() { std::cout << CSI << "34m"; }

    // What it does: Sets bright blue text color.
    // Inputs: None.
    // Outputs: None.
    void setBrightBlue() { std::cout << CSI << "94m"; }

    // What it does: Sets red text color.
    // Inputs: None.
    // Outputs: None.
    void setRed() { std::cout << CSI << "91m"; }

    // What it does: Sets yellow text color.
    // Inputs: None.
    // Outputs: None.
    void setYellow() { std::cout << CSI << "93m"; }

    // What it does: Sets green text color.
    // Inputs: None.
    // Outputs: None.
    void setGreen() { std::cout << CSI << "92m"; }

    // What it does: Sets bright cyan text color.
    // Inputs: None.
    // Outputs: None.
    void setBrightCyan() { std::cout << CSI << "96m"; }

    // What it does: Sets note highlight style used for falling notes.
    // Inputs: None.
    // Outputs: None.
    void setNoteStyle() { std::cout << CSI << "1;97;44m"; }

#if defined(_WIN32)
    // What it does: Enables ANSI VT processing on Windows console.
    // Inputs: None.
    // Outputs: Returns true if VT mode is enabled, otherwise false.
    bool enableVT() {
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

        // What it does: Switches Linux terminal to raw non-blocking mode.
        // Inputs: None.
        // Outputs: None.
        void enterRaw() {
            if (!isatty(STDIN_FILENO)) return;
            if (tcgetattr(STDIN_FILENO, &orig) == -1) return;

            termios raw = orig;
            raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo.
            raw.c_cc[VMIN] = 0;               // Non-blocking read.
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return;

            // Set stdin to non-blocking mode.
            origFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, origFlags | O_NONBLOCK);
            ok = true;
        }

        // What it does: Restores original Linux terminal mode.
        // Inputs: None.
        // Outputs: None.
        void restore() {
            if (!ok) return;
            tcsetattr(STDIN_FILENO, TCSANOW, &orig);
            fcntl(STDIN_FILENO, F_SETFL, origFlags);
            ok = false;
        }
    };
#endif
} // namespace term

// Input state that records keys pressed in current frame.
struct Input {
    array<bool, 256> justPressed{};

    // What it does: Clears current-frame input flags.
    // Inputs: None.
    // Outputs: None.
    void clear() { justPressed.fill(false); }

    // What it does: Marks one key as pressed in this frame.
    // Inputs: ch is key code.
    // Outputs: None.
    void pushChar(unsigned char ch) {
        if (ch < justPressed.size()) justPressed[ch] = true;
    }

    // What it does: Checks whether a key is marked pressed in this frame.
    // Inputs: ch is key character.
    // Outputs: Returns true if key is pressed, otherwise false.
    bool pressed(char ch) const {
        unsigned char u = static_cast<unsigned char>(toupper(static_cast<unsigned char>(ch)));
        return u < justPressed.size() ? justPressed[u] : false;
    }
};

#if defined(_WIN32)
// What it does: Polls Windows keyboard input into Input buffer.
// Inputs: in is mutable input buffer.
// Outputs: None.
void pollInput(Input& in) {
    in.clear();
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            // Extended key prefix: consume scan code and ignore.
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
// What it does: Polls Linux keyboard input into Input buffer.
// Inputs: in is mutable input buffer.
// Outputs: None.
void pollInput(Input& in) {
    in.clear();
    unsigned char buf[64];
    while (true) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;
        for (ssize_t i = 0; i < n; ++i) {
            unsigned char c = buf[i];
            // Convert letters to uppercase; ignore unsupported escape sequences.
            if (isalpha(c)) c = static_cast<unsigned char>(toupper(c));
            in.pushChar(c);
        }
    }
}
#endif

// Core gameplay note data.
struct Note {
    int lane = 0;      // 0..5 -> S D F | J K L.
    double y = 0.0;    // Vertical position.
    bool dead = false; // Whether note should be removed.
};

// Floating judge-feedback info.
struct JudgeFeedback {
    std::string text = "";
    int lane = -1;
    double timer = 0.0;
    int color = 0; // 0=none, 1=Perfect, 2=Good, 3=Miss.
};

struct Game {
    // Screen size.
    int H = 28;
    int W = 64;

    // Lane X positions.
    int lanes = 6;
    int laneX[6] = { 10, 18, 26, 38, 46, 54 };

    // Judge line Y.
    int judgeY = 24;

    // Judge windows in rows.
    int perfectWindow = 1;
    int goodWindow = 2;

    // Note speed and spawn timing controls.
    double speedRowsPerSec = 15.0;
    double spawnInterval = 0.6;
    double spawnTimer = 0.0;

    // Runtime score state.
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int life = 100;

    // Judge statistics.
    int perfectCount = 0;
    int goodCount = 0;
    int missCount = 0;

    vector<Note> notes;

    // Judge feedback overlay.
    JudgeFeedback feedback;
    array<double, 6> laneHitTimer{}; // Lane flash timers.

    // Lane key mapping for 6 lanes.
    array<char, 6> laneKeys = { 'S','D','F','J','K','L' };

    // Chart mode state.
    bool useChart = false;
    ChartData chart;
    size_t nextNoteIndex = 0;
    double gameTime = 0.0;  // Elapsed gameplay time in seconds.

    // Song-end state.
    bool chartFinished = false;    // All chart notes have been consumed.
    double finishTimer = 0.0;      // Delay timer after song end.
    static constexpr double FINISH_DELAY = 1.0; // End delay in seconds.
    bool gameOver = false;         // Life-based game-over flag.

    // What it does: Loads chart data and resets chart-related runtime state.
    // Inputs: filepath is chart file path.
    // Outputs: Returns true if chart loads successfully, otherwise false.
    bool loadChart(const std::string& filepath) {
        if (chart.loadFromFile(filepath)) {
            useChart = true;
            nextNoteIndex = 0;
            gameTime = 0.0;
            chartFinished = false;
            finishTimer = 0.0;
            gameOver = false;

            // Keep gameplay speed at default regardless of BPM.
            speedRowsPerSec = 15.0;

            return true;
        }
        return false;
    }

    // What it does: Checks whether all chart notes are spawned and all active notes are cleared.
    // Inputs: None.
    // Outputs: Returns true if no notes remain, otherwise false.
    bool allNotesCleared() const {
        return nextNoteIndex >= chart.notes.size() && notes.empty();
    }

    // What it does: Determines whether gameplay should end.
    // Inputs: None.
    // Outputs: Returns true if game-over or chart end delay is complete, otherwise false.
    bool shouldEnd() const {
        return gameOver || (chartFinished && finishTimer >= FINISH_DELAY);
    }

    // What it does: Updates game logic (time, spawn, movement, judgment, miss handling).
    // Inputs: dt is delta time in seconds; input is frame key state.
    // Outputs: None.
    void update(double dt, const Input& input) {
        // Update gameplay time.
        gameTime += dt;

        // Accumulate finish timer after chart clears.
        if (chartFinished) {
            finishTimer += dt;
        }

        // Update judge feedback timer.
        if (feedback.timer > 0) {
            feedback.timer -= dt;
            if (feedback.timer <= 0) feedback.text = "";
        }

        // Update lane flash timers.
        for (int i = 0; i < lanes; ++i) {
            if (laneHitTimer[i] > 0) laneHitTimer[i] -= dt;
        }

        // 1) Spawn notes.
        if (useChart) {
            // Spawn notes in advance so they reach judge line at note time.
            double spawnAheadTime = (judgeY / speedRowsPerSec);

            while (nextNoteIndex < chart.notes.size()) {
                const auto& chartNote = chart.notes[nextNoteIndex];
                double adjustedTime = chartNote.time + chart.offset;

                // Spawn when spawn time is reached.
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
        }

        // 2) Move notes.
        for (auto it = notes.begin(); it != notes.end(); ) {
            it->y += speedRowsPerSec * dt;
            ++it;
        }

        // 3) Judge input for each lane.
        for (int li = 0; li < lanes; ++li) {
            if (input.pressed(laneKeys[li])) {
                laneHitTimer[li] = 0.15;

                int bestIdx = -1;
                int bestDist = 1000000000;
                for (int i = 0; i < (int)notes.size(); ++i) {
                    auto& n = notes[i];
                    if (n.dead || n.lane != li) continue;
                    int dist = (int)std::lround(n.y) - judgeY;
                    int ad = std::abs(dist);
                    if (ad < bestDist) { bestDist = ad; bestIdx = i; }
                }
                if (bestIdx >= 0) {
                    auto& n = notes[bestIdx];
                    int dist = (int)std::lround(n.y) - judgeY;
                    int ad = std::abs(dist);
                    // Perfect timing hit
                    if (ad <= perfectWindow) {
                        score += 10; combo++; maxCombo = std::max(maxCombo, combo);
                        perfectCount++;
                        n.dead = true;
                        // Show Perfect feedback.
                        feedback.text = "PERFECT!";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 1;
                    }
                    // Good timing hit
                    else if (ad <= goodWindow) {
                        score += 5;  combo++; maxCombo = std::max(maxCombo, combo);
                        goodCount++;
                        n.dead = true;
                        // Show Good feedback.
                        feedback.text = "GOOD";
                        feedback.lane = li;
                        feedback.timer = 0.5;
                        feedback.color = 2;
                    }
                    else {
                        // Missed the timing window
                    }
                }
            }
        }

        // 4) Handle misses.
        for (auto& n : notes) {
            if (!n.dead && n.y > judgeY + goodWindow + 1) {
                n.dead = true;
                combo = 0;
                missCount++;
                life = std::max(0, life - 2);
                if (life <= 0) gameOver = true;
                // Show Miss feedback.
                feedback.text = "MISS";
                feedback.lane = n.lane;
                feedback.timer = 0.5;
                feedback.color = 3;
            }
        }

        // 5) Remove dead/out-of-screen notes.
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [this](const Note& n) { return n.dead || n.y > (double)(H - 1); }),
            notes.end());
    }

    // What it does: Renders full game UI and note field to terminal.
    // Inputs: None.
    // Outputs: None.
    void render() {
        term::moveHome();

        // Render HUD.
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

        // Judge statistics.
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

        // Song-end countdown display.
        if (chartFinished) {
            std::cout << "    ";
            term::setYellow();
            int remaining = (int)std::ceil(FINISH_DELAY - finishTimer);
            if (remaining < 0) remaining = 0;
            std::cout << "Song finished! Ending in " << remaining << "s...";
            term::resetColor();
        }
        std::cout << "\n";

        // Canvas.
        static std::vector<std::string> canvas;
        canvas.assign(H, std::string(W, ' '));

        // Lane lines and hit flash effects.
        for (int i = 0; i < lanes; ++i) {
            int x = laneX[i];
            for (int y = 1; y < H - 2; ++y) {
                if (x >= 0 && x < W) {
                    canvas[y][x] = (laneHitTimer[i] > 0) ? '!' : '|';
                }
            }
        }

        // Judge line.
        if (judgeY >= 0 && judgeY < H) {
            for (int x = 0; x < W; ++x) canvas[judgeY][x] = '-';
        }

        // Notes.
        for (auto& n : notes) {
            int x = laneX[n.lane];
            int y = (int)std::lround(n.y);
            if (y >= 0 && y < H && x >= 0 && x < W) {
                canvas[y][x] = '@';
            }
        }

        // Judge feedback text.
        if (feedback.timer > 0 && feedback.lane >= 0 && feedback.lane < lanes) {
            int x = laneX[feedback.lane];
            int y = judgeY - 3;
            if (y >= 0 && y < H) {
                int textLen = (int)feedback.text.length();
                int startX = std::max(0, x - textLen / 2);
                for (int i = 0; i < textLen && startX + i < W; ++i) {
                    canvas[y][startX + i] = feedback.text[i];
                }
            }
        }

        // Lane key labels.
        if (H - 2 >= 0 && H - 2 < (int)canvas.size()) {
            for (int i = 0; i < lanes; ++i) {
                int x = laneX[i];
                if (x >= 0 && x < W) {
                    canvas[H - 2][x] = laneKeys[i];
                }
            }
        }

        // Quit hint on the last line.
        if (H - 1 >= 0 && H - 1 < (int)canvas.size()) {
            std::string hint = "(Q=quit)";
            int startX = W - (int)hint.size() - 1;
            if (startX > 0) {
                for (int i = 0; i < (int)hint.size() && startX + i < W; ++i) {
                    canvas[H - 1][startX + i] = hint[i];
                }
            }
        }

        // Draw canvas.
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < (int)canvas[y].size(); ++x) {
                char ch = canvas[y][x];
                if (ch == '@') {
                    term::setNoteStyle();
                    std::cout << ch;
                    term::resetColor();
                } else if (ch == '!') {
                    term::setBrightCyan();
                    std::cout << ch;
                    term::resetColor();
                } else if (feedback.timer > 0 && y == judgeY - 3) {
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
                        if (feedback.color == 1) term::setGreen();
                        else if (feedback.color == 2) term::setYellow();
                        else if (feedback.color == 3) term::setRed();
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

// What it does: Displays pre-game countdown animation.
// Inputs: None.
// Outputs: None.
void showCountdown() {
    const int countdown[] = {3, 2, 1};

    for (int num : countdown) {
        term::clearScreen();
        term::moveHome();

        // Center position for countdown box.
        int centerRow = 14;
        int centerCol = 32;

        // Draw countdown frame.
        term::moveTo(centerRow - 2, centerCol - 5);
        term::setYellow();
        std::cout << "+---------+";

        term::moveTo(centerRow - 1, centerCol - 5);
        std::cout << "|         |";

        term::moveTo(centerRow, centerCol - 5);
        term::setRed();
        std::cout << "|    " << num << "    |";

        term::moveTo(centerRow + 1, centerCol - 5);
        std::cout << "|         |";

        term::moveTo(centerRow + 2, centerCol - 5);
        std::cout << "+---------+";
        term::resetColor();
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    // Show "GO!".
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

// What it does: Displays post-game result panel with score and judge statistics.
// Inputs: game is final game state to display.
// Outputs: None.
void showGameOver(const Game& game) {
    term::clearScreen();
    term::moveHome();

    int centerRow = 10;
    int centerCol = 32;

    // Draw GAME OVER title box.
    term::moveTo(centerRow, centerCol - 10);
    term::setRed();
    std::cout << "+--------------------+"; // Top border

    term::moveTo(centerRow + 1, centerCol - 10);
    std::cout << "|                    |"; // Empty middle

    term::moveTo(centerRow + 2, centerCol - 10);
    term::setYellow();
    std::cout << "|   GAME  OVER!      |"; // "GAME OVER" message

    term::moveTo(centerRow + 3, centerCol - 10);
    term::setRed();
    std::cout << "|                    |"; // Empty middle

    term::moveTo(centerRow + 4, centerCol - 10);
    std::cout << "+--------------------+"; // Bottom border

    // Show final statistics.
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

// What it does: Runs the core rhythm game (stage select, gameplay loop, and result screen).
// Inputs: None.
// Outputs: Returns final score achieved in this run.
int startMusicGameInternal() {
    std::srand((unsigned)std::time(nullptr));

#if defined(_WIN32)
    term::enableVT(); // Enable ANSI VT processing.
#else
    term::TermiosGuard tg; // Enable raw mode and restore automatically on exit.
#endif

    term::hideCursor();
    term::clearScreen();
    term::moveHome();

    Game game;
    // Use global MusicManager and do not create a local music player.

    // Stage-selection mode.
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

        // Flush stdin to discard leftover keys from previous state.
#if defined(_WIN32)
        while (_kbhit()) _getch();
#else
        // On Linux, non-blocking clear stdin buffer to avoid stale keystrokes.
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

        // Find selected stage.
        for (const auto& s : stages) {
            if (s.id == stageInput) { selected = &s; break; }
        }

        if (selected) {
            term::setYellow();
            std::cout << "\nLoading stage: " << selected->singer << " - " << selected->music << "\n";
            term::resetColor();

            // Auto-load chart.
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

                // Auto-load music using MusicManager.
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

    // Show countdown.
    showCountdown();

    // Start music when countdown ends.
    MusicManager::getPlayer().play();

    Input input;

    // Fixed-step logic: 120Hz update, ~60FPS render.
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

        // Read and accumulate input this frame.
        Input tempInput;
        pollInput(tempInput);
        for (int i = 0; i < 256; ++i) {
            if (tempInput.justPressed[i]) input.justPressed[i] = true;
        }
        if (input.pressed('Q')) running = false;

        // Fixed-step logic update.
        while (acc >= dt) {
            game.update(dt, input);
            input.clear();
            acc -= dt;
        }

        // Chart mode: exit after all notes are cleared and FINISH_DELAY passes.
        if (game.useChart) {
            // Exit after notes are cleared for a short delay (independent of audio playback end).
            if (!game.chartFinished && game.allNotesCleared()) {
                game.chartFinished = true;
                game.finishTimer = 0.0;
            }
            if (game.shouldEnd()) {
                running = false;
            }
        }

        // Render at about 60 FPS.
        if (renderAcc >= (1.0 / 60.0)) {
            game.render();
            renderAcc = 0.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Stop current music to avoid leaking into outer scenes.
    MusicManager::stop();

    // Show final result screen.
    showGameOver(game);

    term::resetColor();
    term::showCursor();
    term::clearScreen();
    term::moveHome();

    // Return final score.
    return game.score;
}
