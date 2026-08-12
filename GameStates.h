#ifndef GAMESTATES_H
#define GAMESTATES_H

#include <Arduboy2.h>

extern Arduboy2 arduboy;

enum GameState {
  STATE_SPLASH,
  STATE_TITLE,
  STATE_MAIN_MENU,
  STATE_OPTIONS,
  STATE_GAME,
  STATE_RESULTS
};

extern GameState currentState;

void updateSplash();
void updateTitleScreen();
void updateMainMenu();
void updateOptions();
void updateResults();

#endif