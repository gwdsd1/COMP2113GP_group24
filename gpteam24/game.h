#ifndef GAME_H
#define GAME_H

// What it does: Displays the game main menu and routes the user to selected actions.
// Inputs: None.
// Outputs: None.
void showMainMenu();

// What it does: Starts a brand-new game session.
// Inputs: None.
// Outputs: None.
void newGame();

// What it does: Loads a saved game and starts the maze using saved state.
// Inputs: None.
// Outputs: None.
void loadGame();

// What it does: Performs game quit flow and prints exit message.
// Inputs: None.
// Outputs: None.
void quitGame();

#endif // GAME_H
