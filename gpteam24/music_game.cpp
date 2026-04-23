#include "music_game.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

// What it does: Declares the internal rhythm-game entry point that returns final score.
// Inputs: None.
// Outputs: Returns the final score achieved in the rhythm game.
int startMusicGameInternal();

// Stores the latest rhythm-game result.
static bool musicGamePassed = false;

// What it does: Returns the latest cached result of the rhythm game.
// Inputs: None.
// Outputs: Returns true if the latest run passed, otherwise false.
bool getMusicGameResult() {
    return musicGamePassed;
}

// What it does: Clears the terminal screen.
// Inputs: None.
// Outputs: None.
inline void mg_clear_screen() {
#if defined(_WIN32)
    system("cls");
#else
    std::cout << "\x1b[2J\x1b[H";
#endif
}

// What it does: Runs the music mini-game flow including intro, gameplay, and pass/fail judgment.
// Inputs: None.
// Outputs: Returns true if final score is at least 6000, otherwise false.
bool startMusicGame() {
    mg_clear_screen();
    cout << "\n\nYou seem to interrupt a music lesson...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    mg_clear_screen();
    cout << "\n\nNow it's time for you to play some music...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Reset cached result.
    musicGamePassed = false;

    // Enter the core rhythm gameplay and get final score.
    int finalScore = startMusicGameInternal();

    // Clear screen before returning to maze.
    mg_clear_screen();

    // Score >= 6000 means pass.
    if (finalScore >= 6000) {
        musicGamePassed = true;
        cout << "\n\nWELL DONE!!----Applause filled the room...now you can leave the room...\n" << std::flush;
    } else {
        musicGamePassed = false;
        cout << "\n\nNOT BAD...----But you need more practice...now you can leave the room...\n" << std::flush;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    mg_clear_screen();

    return musicGamePassed;
}
