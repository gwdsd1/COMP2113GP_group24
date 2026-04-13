#include "enemy_quiz.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cctype>

#if defined(_WIN32)
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

namespace quiz_console {
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
            if (ch == 0 || ch == 224) {
                _getch();
            } else {
                lastChar = (char)std::toupper(ch);
            }
        }
#else
        char buf[64];
        ssize_t n;
        while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
            for (ssize_t i = 0; i < n; i++) {
                if (std::isalpha((unsigned char)buf[i])) {
                    lastChar = (char)std::toupper(buf[i]);
                }
            }
        }
#endif
        return lastChar;
    }

    inline void sleepMs(int ms) {
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

struct QuizQuestion {
    std::string question;
    std::string A;
    std::string B;
    std::string C;
    char answer;
};

bool askOne(const QuizQuestion& q) {
    while (true) {
        quiz_console::clear();
        std::cout << q.question << "\n\n";
        std::cout << "A. " << q.A << "\n";
        std::cout << "B. " << q.B << "\n";
        std::cout << "C. " << q.C << "\n\n";
        std::cout << "Press A / B / C\n";

        char in = quiz_console::getInput();
        if (in == 'A' || in == 'B' || in == 'C') {
            bool ok = (in == q.answer);
            std::cout << "\nYou chose: " << in << "\n";
            std::cout << (ok ? "Correct!" : "Wrong!") << "\n";
            quiz_console::sleepMs(1200);
            return ok;
        }

        quiz_console::sleepMs(16);
    }
}

bool startEnemyQuiz() {
#if !defined(_WIN32)
    quiz_console::LinuxTermGuard termGuard;
#endif

    QuizQuestion qs[3] = {
        {
            "What has keys but can't open locks?",
            "A piano",
            "A door",
            "A map",
            'A'
        },
        {
            "Which number is bigger?",
            "19",
            "91",
            "Both same",
            'B'
        },
        {
            "What goes up but never comes down?",
            "Smoke",
            "Your age",
            "Balloon",
            'B'
        }
    };

    int correct = 0;
    for (int i = 0; i < 3; i++) {
        if (askOne(qs[i])) correct++;
    }

    quiz_console::clear();
    std::cout << "You got " << correct << " / 3 correct.\n";
    bool win = (correct >= 2);
    std::cout << (win ? "You escaped the monster!" : "The monster defeated you!") << "\n";
    quiz_console::sleepMs(1500);
    return win;
}