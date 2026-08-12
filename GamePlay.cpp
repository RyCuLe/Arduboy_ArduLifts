#include "GamePlay.h"
#include "GameStates.h"
#include "Sprites.h"
#include "Sound.h"

const int TOTAL_TILES = 35;
const int GRID_COLS = 5;
const int GRID_ROWS = 7;
const int TILE_W = 20;
const int TILE_H = 20;
const int TILE_STEP = 19;

struct LiftShaft {
  int lowerTile;
  int upperTile;
};

const int NUM_LIFTS = 5;
static LiftShaft lifts[NUM_LIFTS] = {
  {  1, 11 },
  {  3, 13 },
  {  7, 32 },
  { 15, 25 },
  { 20, 30 }
};

enum LiftDirection { LIFT_UP, LIFT_DOWN };
static LiftDirection currentLiftDir = LIFT_UP;

static int activePlayer = 0;
static bool isVsCPU = true;
static int playerPos[2] = {0, 0};
static bool skipTurn[2] = {false, false};
static int winningPlayer = -1;

enum TurnPhase {
  PHASE_NEW_TURN,
  PHASE_ROLL,
  PHASE_SECOND_CHANCE,
  PHASE_MOVE,
  PHASE_CHAOS,
  PHASE_LIFT_ENTER,
  PHASE_LIFT_EXIT,
  PHASE_CHANGE_DIRECTION,
  PHASE_WIN
};

static TurnPhase currentPhase = PHASE_NEW_TURN;

static int activeLiftIdx = -1;
static int activeLiftStartTile = -1;
static int activeLiftTargetTile = -1;
static unsigned long liftPhaseTimer = 0;
static int liftAnimFrame = 0;

static unsigned long turnDelayTimer = 0;
const unsigned long TURN_TRANSITION_DELAY_MS = 1000;

static unsigned long winTimer = 0;
const unsigned long WIN_DELAY_MS = 2000;

static int currentRoll = 1;
static int shuffleTicks = 0;
static unsigned long lastShuffleTime = 0;
static bool flashState = true;
static int flashTicks = 0;
static bool usedSecondChance = false;

static int drawnCard = -1;

static const char* chaosCardNames[9] = {
  "PLUS 3 SPACES",
  "MINUS 3 SPACES",
  "PLUS 6 SPACES",
  "MINUS 6 SPACES",
  "SWAP PLACES",
  "PULL OPPONENT",
  "BACK TO START",
  "OPP TO START",
  "ROLL AGAIN!"
};

static int moveTarget = 0;
static int moveStartPos = 0;
static unsigned long lastMoveTime = 0;
static unsigned long cpuWaitTimer = 0;

int getWinner() {
  return winningPlayer;
}

static int getLiftIdx(int tile) {
  for (int i = 0; i < NUM_LIFTS; i++) {
    if (lifts[i].lowerTile == tile || lifts[i].upperTile == tile) return i;
  }
  return -1;
}

static void applyChaosCard(int cardIdx, int player, int opponent) {
  switch (cardIdx) {
    case 0: 
      if (playerPos[player] + 3 <= TOTAL_TILES - 1) playerPos[player] += 3;
      break;
    case 1: 
      playerPos[player] = max(0, playerPos[player] - 3); 
      break;
    case 2: 
      if (playerPos[player] + 6 <= TOTAL_TILES - 1) playerPos[player] += 6;
      break;
    case 3: 
      playerPos[player] = max(0, playerPos[player] - 6); 
      break;
    case 4: {
      int temp = playerPos[player];
      playerPos[player] = playerPos[opponent];
      playerPos[opponent] = temp;
      break;
    }
    case 5: playerPos[opponent] = playerPos[player]; break;
    case 6: playerPos[player] = 0; break;
    case 7: playerPos[opponent] = 0; break;
    case 8: 
      currentPhase = PHASE_NEW_TURN;
      break;
  }
}

static void triggerWin(int winner) {
  winningPlayer = winner;
  winTimer = millis();
  sound.tones(sfxWin);
  currentPhase = PHASE_WIN;
}

void resetGame() {
  playerPos[0] = 0;
  playerPos[1] = 0;
  skipTurn[0] = false;
  skipTurn[1] = false;
  activePlayer = 0;
  winningPlayer = -1;
  currentLiftDir = LIFT_UP;
  activeLiftIdx = -1;
  activeLiftStartTile = -1;
  activeLiftTargetTile = -1;
  winTimer = 0;
  turnDelayTimer = 0;
  cpuWaitTimer = 0;
  currentPhase = PHASE_NEW_TURN;
}

static int getCameraY() {
  int targetTile = playerPos[activePlayer];
  if (currentPhase == PHASE_LIFT_ENTER) {
    targetTile = activeLiftStartTile;
  } else if (currentPhase == PHASE_LIFT_EXIT) {
    targetTile = activeLiftTargetTile;
  }

  int playerRow = targetTile / GRID_COLS;

  int targetRow = playerRow;
  if (targetRow < 1) targetRow = 1;
  if (targetRow > GRID_ROWS - 2) targetRow = GRID_ROWS - 2;

  return (targetRow * TILE_STEP) - 22; 
}

static void getTileScreenPos(int tileIndex, int cameraY, int &outX, int &outY) {
  int row = tileIndex / GRID_COLS;
  int col = tileIndex % GRID_COLS;

  if (row % 2 == 0) {
    col = (GRID_COLS - 1) - col;
  }

  outX = 16 + (col * TILE_STEP);          
  outY = 43 - (row * TILE_STEP) + cameraY; 
}

void updateAndDrawGame(RuleSet currentRules) {
  bool isCPU = (isVsCPU && activePlayer == 1);
  int cameraY = getCameraY();

  switch (currentPhase) {
    case PHASE_NEW_TURN:
      if (skipTurn[activePlayer]) {
        skipTurn[activePlayer] = false;
        activePlayer = (activePlayer == 0) ? 1 : 0;
      } else {
        if (!isCPU) {
          if (arduboy.justPressed(A_BUTTON)) {
            shuffleTicks = 0;
            currentPhase = PHASE_ROLL;
          }
        } else {
          if (cpuWaitTimer == 0) cpuWaitTimer = millis();
          if (millis() - cpuWaitTimer >= 1000) {
            cpuWaitTimer = 0;
            shuffleTicks = 0;
            currentPhase = PHASE_ROLL;
          }
        }
      }
      break;

    case PHASE_ROLL:
      if (shuffleTicks < 10) {
        if (millis() - lastShuffleTime >= 100) {
          lastShuffleTime = millis();
          currentRoll = random(1, 7);
          shuffleTicks++;
        }
      } else if (flashTicks < 6) {
        if (millis() - lastShuffleTime >= 150) {
          lastShuffleTime = millis();
          flashState = !flashState;
          flashTicks++;
        }
      } else {
        flashState = true;
        flashTicks = 0;

        if (currentRules == RULE_SECOND_CHANCE && !usedSecondChance) {
          currentPhase = PHASE_SECOND_CHANCE;
        } else if (currentRules == RULE_EXTRA_CHAOS && currentRoll == 6) {
          drawnCard = random(0, 9);
          cpuWaitTimer = millis();
          currentPhase = PHASE_CHAOS;
        } else {
          moveStartPos = playerPos[activePlayer];
          
          if (playerPos[activePlayer] + currentRoll <= TOTAL_TILES - 1) {
            moveTarget = playerPos[activePlayer] + currentRoll;
          } else {
            moveTarget = playerPos[activePlayer];
          }
          currentPhase = PHASE_MOVE;
        }
      }
      break;

    case PHASE_SECOND_CHANCE:
      if (!isCPU) {
        if (arduboy.justPressed(A_BUTTON)) {
          usedSecondChance = false;
          moveStartPos = playerPos[activePlayer];
          if (playerPos[activePlayer] + currentRoll <= TOTAL_TILES - 1) {
            moveTarget = playerPos[activePlayer] + currentRoll;
          } else {
            moveTarget = playerPos[activePlayer];
          }
          currentPhase = PHASE_MOVE;
        } else if (arduboy.justPressed(B_BUTTON)) {
          usedSecondChance = true;
          shuffleTicks = 0;
          currentPhase = PHASE_ROLL;
        }
      } else {
        bool wouldOvershoot = (playerPos[activePlayer] + currentRoll > TOTAL_TILES - 1);
        if (wouldOvershoot || currentRoll <= 3) {
          usedSecondChance = true;
          shuffleTicks = 0;
          currentPhase = PHASE_ROLL;
        } else {
          usedSecondChance = false;
          moveStartPos = playerPos[activePlayer];
          moveTarget = playerPos[activePlayer] + currentRoll;
          currentPhase = PHASE_MOVE;
        }
      }
      break;

    case PHASE_MOVE:
      if (millis() - lastMoveTime >= 300) {
        lastMoveTime = millis();
        if (playerPos[activePlayer] < moveTarget) {
          playerPos[activePlayer]++;
          sound.tones(sfxStep);
        } else {
          if (playerPos[activePlayer] >= TOTAL_TILES - 1) {
            triggerWin(activePlayer);
          } else {
            bool didMoveThisTurn = (playerPos[activePlayer] > moveStartPos);

            int liftIdx = getLiftIdx(playerPos[activePlayer]);
            bool canUseLift = false;
            int targetTile = -1;

            if (didMoveThisTurn && liftIdx != -1 && currentRules != RULE_NO_LIFTS) {
              if (currentLiftDir == LIFT_UP && playerPos[activePlayer] == lifts[liftIdx].lowerTile) {
                canUseLift = true;
                targetTile = lifts[liftIdx].upperTile;
              } else if (currentLiftDir == LIFT_DOWN && playerPos[activePlayer] == lifts[liftIdx].upperTile) {
                canUseLift = true;
                targetTile = lifts[liftIdx].lowerTile;
              }
            }

            if (canUseLift) {
              activeLiftIdx = liftIdx;
              activeLiftStartTile = playerPos[activePlayer];
              activeLiftTargetTile = targetTile;
              
              sound.tones(sfxLift);
              liftPhaseTimer = millis();
              liftAnimFrame = 0;
              currentPhase = PHASE_LIFT_ENTER;
            } else {
              turnDelayTimer = millis();
              currentPhase = PHASE_CHANGE_DIRECTION;
            }
          }
        }
      }
      break;

    case PHASE_CHAOS:
      if ((!isCPU && arduboy.justPressed(A_BUTTON)) || 
          (isCPU && millis() - cpuWaitTimer >= 2000) ||
          (arduboy.justPressed(A_BUTTON))) {
        cpuWaitTimer = 0;
        int opponent = (activePlayer == 0) ? 1 : 0;
        applyChaosCard(drawnCard, activePlayer, opponent);

        if (playerPos[activePlayer] >= TOTAL_TILES - 1) {
          triggerWin(activePlayer);
        } else if (currentPhase != PHASE_NEW_TURN) {
          turnDelayTimer = millis();
          currentPhase = PHASE_CHANGE_DIRECTION;
        }
      }
      break;

    case PHASE_LIFT_ENTER: {
      unsigned long elapsed = millis() - liftPhaseTimer;

      if (elapsed < 200) {
        liftAnimFrame = 0;
      } else if (elapsed < 600) {
        liftAnimFrame = (elapsed - 200) / 100;
      } else if (elapsed < 1000) {
        liftAnimFrame = 3;
      } else if (elapsed < 1400) {
        liftAnimFrame = 3 - ((elapsed - 1000) / 100);
      } else if (elapsed < 1600) {
        liftAnimFrame = 0;
      } else {
        playerPos[activePlayer] = activeLiftTargetTile;
        liftPhaseTimer = millis();
        currentPhase = PHASE_LIFT_EXIT;
      }
      break;
    }

    case PHASE_LIFT_EXIT: {
      unsigned long elapsed = millis() - liftPhaseTimer;

      if (elapsed < 200) {
        liftAnimFrame = 0;
      } else if (elapsed < 600) {
        liftAnimFrame = (elapsed - 200) / 100;
      } else if (elapsed < 1000) {
        liftAnimFrame = 3;
      } else if (elapsed < 1400) {
        liftAnimFrame = 3 - ((elapsed - 1100) / 100);
      } else if (elapsed < 1600) {
        liftAnimFrame = 0;
      } else {
        activeLiftIdx = -1;
        activeLiftStartTile = -1;
        activeLiftTargetTile = -1;

        if (playerPos[activePlayer] >= TOTAL_TILES - 1) {
          triggerWin(activePlayer);
        } else {
          turnDelayTimer = millis();
          currentPhase = PHASE_CHANGE_DIRECTION;
        }
      }
      break;
    }

    case PHASE_CHANGE_DIRECTION:
      if (millis() - turnDelayTimer >= TURN_TRANSITION_DELAY_MS) {
        usedSecondChance = false;
        activePlayer = (activePlayer == 0) ? 1 : 0;
        cpuWaitTimer = 0;

        if (activePlayer == 0) {
          currentLiftDir = (currentLiftDir == LIFT_UP) ? LIFT_DOWN : LIFT_UP;
        }

        currentPhase = PHASE_NEW_TURN;
      }
      break;

    case PHASE_WIN:
      if (millis() - winTimer >= WIN_DELAY_MS) {
        currentState = STATE_RESULTS;
      }
      break;
  }

  bool inLiftSequence = (currentPhase == PHASE_LIFT_ENTER || currentPhase == PHASE_LIFT_EXIT);

  for (int i = 0; i < TOTAL_TILES; i++) {
    int drawX, drawY;
    getTileScreenPos(i, cameraY, drawX, drawY);

    if (drawY >= -TILE_H && drawY <= 64) {
      arduboy.drawRect(drawX, drawY, TILE_W, TILE_H, WHITE);

      bool isStartTile  = (i == activeLiftStartTile);
      bool isTargetTile = (i == activeLiftTargetTile);

      if (inLiftSequence && (isStartTile || isTargetTile) && currentRules != RULE_NO_LIFTS) {
        int frameToDraw = 0;
        if (currentPhase == PHASE_LIFT_ENTER && isStartTile) {
          frameToDraw = liftAnimFrame;
        } else if (currentPhase == PHASE_LIFT_EXIT && isTargetTile) {
          frameToDraw = liftAnimFrame;
        }

        arduboy.fillRect(drawX + 1, drawY + 1, 18, 18, BLACK);
        arduboy.drawBitmap(drawX, drawY, LiftAnim[frameToDraw], 20, 20, WHITE);
      } else {
        bool isLiftTile = (getLiftIdx(i) != -1 && currentRules != RULE_NO_LIFTS);

        if (isLiftTile) {
          char arrowChar = (currentLiftDir == LIFT_UP) ? '^' : 'v';
          uint8_t arrowW = getCustomCharWidth(arrowChar);
          int alignX = (drawX + 18) - arrowW;

          drawCustomChar(alignX, drawY + 2, arrowChar);
        } else {
          uint8_t numW = getCustomNumWidth(i + 1);
          int alignX = (drawX + 18) - numW;

          drawCustomNum(alignX, drawY + 2, i + 1);
        }
      }

      if (playerPos[0] == i && !(activePlayer == 0 && inLiftSequence)) {
        arduboy.drawBitmap(drawX + 2, drawY + 11, CounterHeart, 7, 7, WHITE);
      }

      if (playerPos[1] == i && !(activePlayer == 1 && inLiftSequence)) {
        arduboy.drawBitmap(drawX + 11, drawY + 11, CounterCircle, 7, 7, WHITE);
      }
    }
  }

  arduboy.drawBitmap(1, 16, activePlayer == 0 ? TurnHeartYes  : TurnHeartNo,  14, 14, WHITE);
  arduboy.drawBitmap(1, 34, activePlayer == 1 ? TurnCircleYes : TurnCircleNo, 14, 14, WHITE);

  if (currentPhase == PHASE_ROLL || currentPhase == PHASE_SECOND_CHANCE || currentPhase == PHASE_MOVE || inLiftSequence) {
    if (flashState) {
      arduboy.drawBitmap(114, 26, Dice[currentRoll - 1], 12, 12, WHITE);
    }
  } else if (currentPhase == PHASE_NEW_TURN && (!isVsCPU || activePlayer == 0)) {
    arduboy.drawBitmap(114, 26, ButtonPressA, 12, 12, WHITE);
  }

  if (currentPhase == PHASE_CHAOS && drawnCard != -1) {
    int boxW = 100;
    int boxH = 28;
    int boxX = (128 - boxW) / 2;
    int boxY = (64 - boxH) / 2;

    arduboy.fillRect(boxX, boxY, boxW, boxH, BLACK);
    arduboy.drawRect(boxX, boxY, boxW, boxH, WHITE);
    arduboy.drawFastHLine(boxX + 2, boxY + 13, boxW - 4, WHITE);

    const char* headerText = "CHAOS CARD!";
    uint8_t headerW = getCustomStringWidth(headerText);
    drawCustomString(boxX + (boxW - headerW) / 2, boxY + 4, headerText);

    const char* cardText = chaosCardNames[drawnCard];
    uint8_t cardW = getCustomStringWidth(cardText);
    drawCustomString(boxX + (boxW - cardW) / 2, boxY + 17, cardText);
  }
}