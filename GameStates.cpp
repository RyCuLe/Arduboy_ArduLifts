#include "GameStates.h"
#include "GamePlay.h"
#include "Sprites.h"
#include "Sound.h"

extern RuleSet selectedRules;

static unsigned long splashStartTime = 0;
static int titleLiftY = -20;
static int mainMenuCursor = 0;
static int optionsCursor = 0;
static int resultsCursor = 0;

void updateSplash() {
  if (splashStartTime == 0) {
    splashStartTime = millis();
  }

  unsigned long elapsedTime = millis() - splashStartTime;
  int currentFrame = (elapsedTime / 120) % 4;

  arduboy.drawBitmap(43, 12, Splash[currentFrame], 41, 39, WHITE);

  if (elapsedTime >= 3000 || arduboy.justPressed(A_BUTTON)) {
    splashStartTime = 0;
    currentState = STATE_TITLE;
  }
}

void updateTitleScreen() {
  if (!sound.playing()) {
    sound.tones(song);
  }

  arduboy.fillRect(13, 0, 2, 64, WHITE);

  titleLiftY++;
  if (titleLiftY > 64) {
    titleLiftY = -20;
  }

  arduboy.fillRect(4, titleLiftY, 20, 20, BLACK);
  arduboy.drawBitmap(4, titleLiftY, LiftAnim[0], 20, 20, WHITE);
  arduboy.drawBitmap(32, 8, TitleArduLifts, 90, 10, WHITE);

  if ((millis() / 500) % 2 == 0) {
    const char* prompt = "PRESS A TO START";
    uint8_t textW = getCustomStringWidth(prompt);
    drawCustomString(32 + (90 - textW) / 2, 48, prompt);
  }

  if (arduboy.justPressed(A_BUTTON)) {
    randomSeed(micros());
    currentState = STATE_MAIN_MENU;
  }
}

void updateMainMenu() {
  if (!sound.playing()) {
    sound.tones(song);
  }

  const char* header = "MAIN MENU";
  uint8_t headW = getCustomStringWidth(header);
  drawCustomString((128 - headW) / 2, 4, header);
  arduboy.drawFastHLine(14, 12, 100, WHITE);

  const char* menuItems[2] = {
    "PLAY",
    "RULE SET"
  };

  for (int i = 0; i < 2; i++) {
    int itemY = 24 + (i * 16);
    uint8_t itemW = getCustomStringWidth(menuItems[i]);
    int itemX = (128 - itemW) / 2;

    if (i == mainMenuCursor) {
      arduboy.drawRect(itemX - 6, itemY - 2, itemW + 12, 9, WHITE);
    }
    
    drawCustomString(itemX, itemY, menuItems[i]);
  }

  if (arduboy.justPressed(UP_BUTTON))   mainMenuCursor = 0;
  if (arduboy.justPressed(DOWN_BUTTON)) mainMenuCursor = 1;

  if (arduboy.justPressed(A_BUTTON)) {
    if (mainMenuCursor == 0) {
      sound.noTone();
      resetGame();
      currentState = STATE_GAME;
    } else {
      currentState = STATE_OPTIONS;
    }
  }

  if (arduboy.justPressed(B_BUTTON)) {
    currentState = STATE_TITLE;
  }
}

void updateOptions() {
  const char* header = "RULE SET";
  uint8_t headW = getCustomStringWidth(header);
  drawCustomString((128 - headW) / 2, 4, header);
  arduboy.drawFastHLine(14, 12, 100, WHITE);

  const char* rulesetNames[4] = {
    "1  VANILLA",
    "2  NO LIFTS",
    "3  2ND CHANCE",
    "4  EXTRA CHAOS"
  };

  for (int i = 0; i < 4; i++) {
    int itemY = 18 + (i * 11);

    if (i == optionsCursor) {
      arduboy.drawRect(18, itemY - 2, 92, 9, WHITE);
    }

    drawCustomString(24, itemY, rulesetNames[i]);

    if (selectedRules == (RuleSet)i) {
      drawCustomString(98, itemY, "X");
    }
  }

  if (arduboy.justPressed(UP_BUTTON) && optionsCursor > 0) optionsCursor--;
  if (arduboy.justPressed(DOWN_BUTTON) && optionsCursor < 3) optionsCursor++;

  if (arduboy.justPressed(A_BUTTON)) {
    selectedRules = (RuleSet)optionsCursor;
  }

  if (arduboy.justPressed(B_BUTTON)) {
    currentState = STATE_MAIN_MENU;
  }
}

void updateResults() {
  const char* winText = (getWinner() == 0) ? "P1 WINS!" : "P2 WINS!";
  uint8_t winW = getCustomStringWidth(winText);
  drawCustomString((128 - winW) / 2, 6, winText);
  arduboy.drawFastHLine(14, 14, 100, WHITE);

  const char* menuItems[2] = {
    "PLAY AGAIN",
    "MAIN MENU"
  };

  for (int i = 0; i < 2; i++) {
    int itemY = 28 + (i * 16);
    uint8_t itemW = getCustomStringWidth(menuItems[i]);
    int itemX = (128 - itemW) / 2;

    if (i == resultsCursor) {
      arduboy.drawRect(itemX - 6, itemY - 2, itemW + 12, 9, WHITE);
    }

    drawCustomString(itemX, itemY, menuItems[i]);
  }

  if (arduboy.justPressed(UP_BUTTON))   resultsCursor = 0;
  if (arduboy.justPressed(DOWN_BUTTON)) resultsCursor = 1;

  if (arduboy.justPressed(A_BUTTON)) {
    if (resultsCursor == 0) {
      sound.noTone();
      resetGame();
      currentState = STATE_GAME;
    } else {
      currentState = STATE_MAIN_MENU;
    }
  }
}