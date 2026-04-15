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

    // 清空残留输入，避免刚进入问答时自动选中第一题答案
    inline void clearInputBuffer() {
        getInput();
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

// 开场剧情：老师突然出现并开始 cold-call
static void showIntro() {
    quiz_console::clear();
    cout << "A professor suddenly appears from nowhere.\n";
    cout << "\"Leaving already?\" they ask.\n";
    cout << "\"Not until you answer a few questions.\"\n";

 
    quiz_console::sleepMs(4000);


    quiz_console::clearInputBuffer();
}

static bool askOneQuestion(const QuizQuestion& q, int index, int total) {
   
    quiz_console::clearInputBuffer();

    // 只在进入题目时清屏并绘制一次
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

bool startEnemyQuiz() {
#if !defined(_WIN32)
    quiz_console::LinuxTermGuard termGuard;
#endif

    showIntro();

    // 10 道题，每次随机抽 3 道
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