#include "music_game.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

// 声明实际的音游入口函数（返回最终分数）
int startMusicGameInternal();

// 存储音游结果：true表示通过（分数>=7000）
static bool musicGamePassed = false;

// 获取音游结果的函数
bool getMusicGameResult() {
    return musicGamePassed;
}

inline void mg_clear_screen() {
#if defined(_WIN32)
    system("cls");
#else
    std::cout << "\x1b[2J\x1b[H";
#endif
}

// 修改后的startMusicGame，添加分数判断
bool startMusicGame() {
    mg_clear_screen();
    cout << "\n\nYou seem to interrupt a music lesson...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    mg_clear_screen();
    cout << "\n\nNow it's time for you to play some music...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 重置结果
    musicGamePassed = false;

    // 进入真正的音游部分，获取返回的分数
    int finalScore = startMusicGameInternal();

    // 返回迷宫前清屏
    mg_clear_screen();
    
    // 检查分数 - 分数>=6000为通过
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
