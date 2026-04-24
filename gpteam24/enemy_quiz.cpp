#include "enemy_quiz.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <cctype>
#include <ctime>

#if defined(_WIN32)
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

using namespace std;

namespace quiz_console {
    // What it does: Clears the terminal screen.
    // Inputs: None.
    // Outputs: None.
    inline void clear() {
#if defined(_WIN32)
        system("cls");
#else
        std::cout << "\x1b[2J\x1b[H";
#endif
    }

    // What it does: Sleeps for a specified duration in milliseconds.
    // Inputs: ms is the sleep duration in milliseconds.
    // Outputs: None.
    inline void sleepMs(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // What it does: Reads latest keyboard input in non-blocking mode and normalizes letters to uppercase.
    // Inputs: None.
    // Outputs: Returns the latest detected key character, or 0 if no valid key is found.
    inline char getInput() {
        char lastChar = 0;
#if defined(_WIN32)
        while (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                _getch();
            } else {
                lastChar = static_cast<char>(std::toupper(ch));
            }
        }
#else
        char buf[64];
        ssize_t n;
        while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < n; i++) {
                if (std::isalpha(static_cast<unsigned char>(buf[i]))) {
                    lastChar = static_cast<char>(std::toupper(buf[i]));
                }
            }
        }
#endif
        return lastChar;
    }

    // What it does: Flushes pending input to avoid accidental key carryover.
    // Inputs: None.
    // Outputs: None.
    inline void clearInputBuffer() {
        getInput();
    }

#if !defined(_WIN32)
    struct LinuxTermGuard {
        termios orig{};
        bool active = false;
        int origFlags = 0;

        // What it does: Enables raw non-blocking terminal mode for quiz input.
        // Inputs: None.
        // Outputs: Constructs a guard that applies terminal settings.
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

        // What it does: Restores original terminal mode when guard is destroyed.
        // Inputs: None.
        // Outputs: None.
        ~LinuxTermGuard() {
            if (active) {
                tcsetattr(STDIN_FILENO, TCSANOW, &orig);
                fcntl(STDIN_FILENO, F_SETFL, origFlags);
            }
        }
    };
#endif
}

struct QuizQuestion {
    string question;
    string A;
    string B;
    string C;
    string D;
    char answer;
};

// What it does: Shows the intro narrative before the quiz starts.
// Inputs: None.
// Outputs: None.
static void showIntro() {
    quiz_console::clear();
    cout << "A professor suddenly appears from nowhere.\n";
    cout << "\n";
    cout << "\"Leaving already?\" they ask.\n";
    cout << "\n";
    cout << "\"Not until you answer a few questions.\"\n";


    quiz_console::sleepMs(4000);


    quiz_console::clearInputBuffer();
}

// What it does: Displays one question, waits for A/B/C/D input, and returns correctness.
// Inputs: q is the question data; index is current question number; total is total questions.
// Outputs: Returns true if the selected answer is correct, otherwise false.
static bool askOneQuestion(const QuizQuestion& q, int index, int total) {
    quiz_console::clearInputBuffer();

    // Clear and render once when entering this question.
    quiz_console::clear();
    cout << "=== Escape Trial: Main Building ===\n";
    cout << "Question " << index << " / " << total << "\n\n";
    cout << q.question << "\n\n";
    cout << "A. " << q.A << "\n";
    cout << "B. " << q.B << "\n";
    cout << "C. " << q.C << "\n";
    cout << "D. " << q.D << "\n\n";
    cout << "Press A / B / C / D\n";


    while (true) {
        char in = quiz_console::getInput();

        if (in == 'A' || in == 'B' || in == 'C' || in == 'D') {
            bool correct = (in == q.answer);

            cout << "\nYou chose: " << in << "\n";
            if (correct) {
                cout << "The professor nods reluctantly.\n";
                cout << "\"Fine. You may pass.\"\n";
            } else {
                cout << "\"You clearly need another tutorial,\" the professor says.\n";
            }

            quiz_console::sleepMs(1400);
            return correct;
        }

        quiz_console::sleepMs(16);
    }
}

// What it does: Runs the enemy quiz mini-game and returns pass/fail result.
// Inputs: None.
// Outputs: Returns true if the player answers at least 2 of 3 questions correctly; otherwise false.
bool startEnemyQuiz() {
#if !defined(_WIN32)
    quiz_console::LinuxTermGuard termGuard;
#endif

    showIntro();

    // Question bank: randomly select 3 questions from 10.
    vector<QuizQuestion> bank = {
    {
        "What is the special ability of a group project?",
        "Boosting teamwork",
        "Improving friendship",
        "Turning one person into the entire team",
        "Making everyone more productive",
        'C'
    },
    {
        "At 11:59 PM, what is most likely to appear?",
        "A shooting star",
        "Inner peace",
        "A newly uploaded assignment",
        "Free time",
        'C'
    },
    {
        "What is the real purpose of a tutorial?",
        "To deepen understanding",
        "To encourage discussion",
        "To reveal how little you actually know",
        "To let you leave early",
        'C'
    },
    {
        "What is the strongest crowd-control skill used by professors?",
        "Taking attendance",
        "Surprise quiz",
        "\"Can someone answer this?\"",
        "\"You can read this by yourself.\"",
        'C'
    },
    {
        "Which enemy attack is the most deadly?",
        "\"Please submit by Friday\"",
        "\"This is optional\"",
        "\"This will not be on the exam\"",
        "\"You should already know this\"",
        'D'
    },
    {
        "What is the true purpose of reading week?",
        "To read",
        "To rest",
        "To realize how behind you are",
        "To discover new hobbies",
        'C'
    },
    {
        "What has many keys but still cannot open the Main Building door?",
        "A piano",
        "A keycard",
        "A keyboard",
        "A janitor",
        'A'
    },
    {
        "What goes up every semester but never comes down?",
        "Your age",
        "Your workload",
        "Your attendance",
        "Your pen count",
        'B'
    },
    {
        "What gets bigger the more you take away from it?",
        "A notebook",
        "A lecture hall",
        "A backpack",
        "A hole",
        'D'
    },
    {
        "What belongs to you, but your group members use it more than you do?",
        "Your bag",
        "Your notes",
        "Your name",
        "Your water bottle",
        'C'
    }
};

    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::shuffle(bank.begin(), bank.end(), rng);

    const int totalQuestions = 3;
    int correctCount = 0;

    for (int i = 0; i < totalQuestions; i++) {
        if (askOneQuestion(bank[i], i + 1, totalQuestions)) {
            correctCount++;
        }
    }

    quiz_console::clear();
    cout << "=== Trial Result ===\n\n";
    cout << "You answered " << correctCount << " / " << totalQuestions << " correctly.\n\n";

    bool win = (correctCount >= 2);
    if (win) {
        cout << "You survive the professor's cold-call and slip past them.\n";
        cout << "The corridor ahead is finally clear.\n";
    } else {
        cout << "You fail the sudden questioning.\n";
        cout << "Main Building tightens its grip around you.\n";
    }

    quiz_console::sleepMs(1800);
    return win;
}
