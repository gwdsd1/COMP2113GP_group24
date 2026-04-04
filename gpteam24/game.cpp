#include "game.h"
#include "maze.h"
#include "MusicManager.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace std;

void showMainMenu() {
    // 播放主菜单背景音乐
    MusicManager::playBackgroundMusic("music/menu_bg.mp3");

    bool isRunning = true;
    while (isRunning) {
        system("cls"); // 清除屏幕(Windows平台适用)

        // 打印巨大字体的 ASCII Art
        cout << R"(
  ______  _____  _____          _____  ______ 
 |  ____|/ ____|/ ____|   /\   |  __ \|  ____|
 | |__  | (___ | |       /  \  | |__) | |__   
 |  __|  \___ \| |      / /\ \ |  ___/|  __|  
 | |____ ____) | |____ / ____ \| |    | |____ 
 |______|_____/ \_____/_/    \_\_|    |______|
                                              
  ______ _____   ____  __  __ 
 |  ____|  __ \ / __ \|  \/  |
 | |__  | |__) | |  | | \  / |
 |  __| |  _  /| |  | | |\/| |
 | |    | | \ \| |__| | |  | |
 |_|    |_|  \_\\____/|_|  |_|
                              
  _______ _    _ ______ 
 |__   __| |  | |  ____|
    | |  | |__| | |__   
    | |  |  __  |  __|  
    | |  | |  | | |____ 
    |_|  |_|  |_|______|
                        
  __  __          _____ _   _ 
 |  \/  |   /\   |_   _| \ | |
 | \  / |  /  \    | | |  \| |
 | |\/| | / /\ \   | | | . ` |
 | |  | |/ ____ \ _| |_| |\  |
 |_|  |_/_/    \_\_____|_| \_|
                              
  ____  _    _ _____ _      _____ _____ _   _  _____ 
 |  _ \| |  | |_   _| |    |  __ \_   _| \ | |/ ____|
 | |_) | |  | | | | | |    | |  | || | |  \| | |  __ 
 |  _ <| |  | | | | | |    | |  | || | | . ` | | |_ |
 | |_) | |__| |_| |_| |____| |__| || |_| |\  | |__| |
 |____/ \____/|_____|______|_____/_____|_| \_|\_____|
        )" << "\n\n";

        cout << "========================================================\n";
        cout << "                   1. New Game                          \n";
        cout << "                   2. Load Game                         \n";
        cout << "                   3. Quit                              \n";
        cout << "========================================================\n";
        cout << "\nPlease select an option (1-3): ";

        string choice;
        cin >> choice;

        if (choice == "1") {
            newGame();
        } else if (choice == "2") {
            loadGame();
        } else if (choice == "3") {
            quitGame();
            isRunning = false; // 退出主循环
        } else {
            cout << "Invalid choice! Press Enter to try again...\n";
            cin.ignore();
            cin.get();
        }
    }
}

void newGame() {
    system("cls");
    cout << "\n\nYou wake up in the Main Building of HKU, your mind is blank...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    MazeState state;
    startMaze(state, false);
}

void loadGame() {
    system("cls");
    cout << "\nLoading saved game...\n";

    MazeState state;
    if (loadMazeStateFromFile("save.txt", state)) {
        cout << "Save file loaded successfully!\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        startMaze(state, true);
    } else {
        cout << "No save file found.\n";
        cout << "\nPress Enter to return to main menu...";
        cin.ignore();
        cin.get();
    }
}

void quitGame() {
    cout << "\nExiting game... Goodbye!\n";
}
