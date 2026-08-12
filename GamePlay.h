#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <Arduboy2.h>

extern Arduboy2 arduboy;

enum RuleSet {
  RULE_VANILLA,
  RULE_NO_LIFTS,
  RULE_SECOND_CHANCE,
  RULE_EXTRA_CHAOS
};

void resetGame();
void updateAndDrawGame(RuleSet currentRules);
int getWinner();

#endif