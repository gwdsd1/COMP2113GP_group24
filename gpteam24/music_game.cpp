#include "music_game.h"
#include "MusicPlayer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

// 声明实际的音游入口函数
void startMusicGameInternal();

void startMusicGame() {
    system("cls");
    cout << "\n\nYou seem to interrupt a music lesson...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    system("cls");
    cout << "\n\nNow it's time for you to play some music...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 进入真正的音游部分
    startMusicGameInternal();

    // 返回迷宫前清屏
    system("cls");
    cout << "\n\nWELL DONE!!----Applause filled the room...now you can leave the room...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    system("cls");
}