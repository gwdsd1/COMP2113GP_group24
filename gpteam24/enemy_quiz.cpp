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
    inline void clear() {
#if defined(_WIN32)
        system("cls");
#else
        std::cout << "\x1b[2J\x1b[H";
#endif
    }

    inline void sleepMs(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

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

#if !defined(_WIN32)
    struct LinuxTermGuard {
        termios orig{};
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

struct QuizQuestion {
    string question;
    string A;
    string B;
    string C;
    string D;
    char answer;
};

static void showIntro() {
    quiz_console::clear();
    cout << "A professor suddenly appears from nowhere.\n";
    cout << "\"Leaving already?\" they ask.\n";
    cout << "\"Not until you answer a few questions.\"\n";
    quiz_console::sleepMs(1800);
}

static bool askOneQuestion(const QuizQuestion& q, int index, int total) {
    while (true) {
        quiz_console::clear();

        cout << "=== Escape Trial: Main Building ===\n";
        cout << "Question " << index << " / " << total << "\n\n";
        cout << q.question << "\n\n";
        cout << "A. " << q.A << "\n";
        cout << "B. " << q.B << "\n";
        cout << "C. " << q.C << "\n";
        cout << "D. " << q.D << "\n\n";
        cout << "Press A / B / C / D\n";

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

bool startEnemyQuiz() {
#if !defined(_WIN32)
    quiz_console::LinuxTermGuard termGuard;
#endif

    showIntro();

    vector<QuizQuestion> bank = {
        {
            "A poster at the entrance of Main Building asks: What is the special ability of a group project?",
            "Boosting teamwork",
            "Improving friendship",
            "Turning one person into the entire team",
            "Making everyone more productive",
            'C'
        },
        {
            "At 11:59 PM, a cursed screen inside Main Building lights up. What is most likely to appear?",
            "A shooting star",
            "Inner peace",
            "A newly uploaded assignment",
            "Free time",
            'C'
        },
        {
            "A ghost in the tutorial room whispers: What is the real purpose of a tutorial?",
            "To deepen understanding",
            "To encourage discussion",
            "To reveal how little you actually know",
            "To let you leave early",
            'C'
        },
        {
            "A shadowy professor blocks the staircase and asks: What is the strongest crowd-control skill used by professors?",
            "Taking attendance",
            "Surprise quiz",
            "\"Can someone answer this?\"",
            "\"You can read this by yourself.\"",
            'C'
        },
        {
            "A notice on the wall flashes red: Which enemy attack is the most deadly?",
            "\"Please submit by Friday\"",
            "\"This is optional\"",
            "\"This will not be on the exam\"",
            "\"You should already know this\"",
            'D'
        },
        {
            "Near the library corner, the building asks: What is the true purpose of reading week?",
            "To read",
            "To rest",
            "To realize how behind you are",
            "To discover new hobbies",
            'C'
        },
        {
            "A locked classroom door asks: What has many keys but still cannot open the Main Building door?",
            "A piano",
            "A keycard",
            "A keyboard",
            "A janitor",
            'A'
        },
        {
            "The elevator hums and asks: What goes up every semester but never comes down?",
            "Your age",
            "Your workload",
            "Your attendance",
            "Your pen count",
            'B'
        },
        {
            "A cracked mirror in the corridor asks: What gets bigger the more you take away from it?",
            "A notebook",
            "A lecture hall",
            "A backpack",
            "A hole",
            'D'
        },
        {
            "At the final exit, Main Building gives one last riddle: What belongs to you, but your group members use it more than you do?",
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