#include "game.h"
#include "maze.h"
#include "MusicManager.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

// What it does: Shows the main menu loop, handles user choices, and routes to game flows.
// Inputs: None.
// Outputs: None.
void showMainMenu() {
    // Play main menu background music.
    MusicManager::playBackgroundMusic("music/menu_bg.mp3");

    bool isRunning = true;
    while (isRunning) {
        #if defined(_WIN32)
        system("cls");
        #else
        std::cout << "\x1b[2J\x1b[H";
        #endif

        // Print large ASCII art title.
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
            isRunning = false; // Exit main loop.
        } else {
            cout << "Invalid choice! Press Enter to try again...\n";
            cin.ignore();
            cin.get();
        }
    }
}

// What it does: Starts a fresh maze run and restores menu music after maze ends.
// Inputs: None.
// Outputs: None.
void newGame() {
#if defined(_WIN32)
    system("cls");
#else
    std::cout << "\x1b[2J\x1b[H";
#endif
    cout << "\n\nYou wake up in the Main Building of HKU, your mind is blank...\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    MazeState state;
    startMaze(state, false);

    // Restore main menu music after leaving maze.
    MusicManager::playBackgroundMusic("music/menu_bg.mp3");
}

// What it does: Displays available save files, loads selected save state, and starts maze.
// Inputs: None.
// Outputs: None.
void loadGame() {
#if defined(_WIN32)
    system("cls");
#else
    std::cout << "\x1b[2J\x1b[H";
#endif
    cout << "\nLoading saved game...\n";

    vector<string> saveFiles = getSaveFiles();

    if (saveFiles.empty()) {
        cout << "No save files found.\n";
        cout << "\nPress Enter to return to main menu...";
        cin.ignore();
        cin.get();
        return;
    }

    cout << "\nAvailable save files:\n";
    for (int i = 0; i < saveFiles.size(); i++) {
        cout << i + 1 << ". " << saveFiles[i] << '\n';
    }

    cout << "\nChoose a save file number: ";
    int choice;
    cin >> choice;

    if (choice < 1 || choice > saveFiles.size()) {
        cout << "Invalid choice.\n";
        cout << "\nPress Enter to return to main menu...";
        cin.ignore();
        cin.get();
        return;
    }

    MazeState state;
    string selectedFile = saveFiles[choice - 1];

    if (loadMazeStateFromFile(selectedFile, state)) {
        cout << "\nSave file loaded successfully!\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        startMaze(state, true);
        // Restore main menu music after leaving maze.
        MusicManager::playBackgroundMusic("music/menu_bg.mp3");
    } else {
        cout << "\nFailed to load save file.\n";
        cout << "\nPress Enter to return to main menu...";
        cin.ignore();
        cin.get();
    }
}

// What it does: Executes quit flow message from the menu.
// Inputs: None.
// Outputs: None.
void quitGame() {
    cout << "\nExiting game... Goodbye!\n";
}
