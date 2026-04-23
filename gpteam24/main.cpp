#include "game.h"
#include "MusicManager.h"

// What it does: Initializes audio, runs the main menu loop, and performs cleanup before process exit.
// Inputs: None.
// Outputs: Returns process exit code 0 on normal completion.
int main() {
    // Initialize the audio system.
    MusicManager::initialize();

    // Show the main menu.
    showMainMenu();

    // Release audio resources.
    MusicManager::cleanup();

    return 0;
}
