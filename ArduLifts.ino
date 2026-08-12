#include <Arduboy2.h>
#include "GameStates.h"
#include "GamePlay.h"
#include "Sprites.h"
#include "Sound.h"

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

GameState currentState = STATE_SPLASH;
RuleSet selectedRules = RULE_VANILLA;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);

  unsigned long dynamicSeed = analogRead(A0) + analogRead(A1) + micros();
  randomSeed(dynamicSeed);
}

void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();
  arduboy.clear();

  switch (currentState) {
    case STATE_SPLASH:
      updateSplash();
      break;
    case STATE_TITLE:
      updateTitleScreen();
      break;
    case STATE_MAIN_MENU:
      updateMainMenu();
      break;
    case STATE_OPTIONS:
      updateOptions();
      break;
    case STATE_GAME:
      updateAndDrawGame(selectedRules);
      break;
    case STATE_RESULTS:
      updateResults();
      break;
  }

  arduboy.display();
}