#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include <ArduboyTones.h>

extern ArduboyTones sound;

const uint16_t song[] PROGMEM = {
  NOTE_C6, 256, NOTE_E6, 256, NOTE_G6, 256, NOTE_E6, 256,
  NOTE_D6, 256, NOTE_F6, 256, NOTE_A6, 256, NOTE_F6, 256,
  NOTE_E6, 256, NOTE_G6, 256, NOTE_C6, 256, NOTE_G6, 256,
  NOTE_F6, 256, NOTE_D6, 256, NOTE_C6, 256,
  NOTE_REST, 256,
  TONES_REPEAT
};

const uint16_t sfxStep[] PROGMEM = {
  NOTE_C5, 15, NOTE_G5, 20, TONES_END
};

const uint16_t sfxLift[] PROGMEM = {
  NOTE_C3, 40, NOTE_G2, 50, NOTE_REST, 30,
  NOTE_C4, 70, NOTE_E4, 70, NOTE_G4, 70,
  NOTE_C6, 120,
  TONES_END
};

const uint16_t sfxWin[] PROGMEM = {
  NOTE_C5, 80, NOTE_E5, 80, NOTE_G5, 80, NOTE_C6, 160, NOTE_G5, 80, NOTE_C6, 320, TONES_END
};

#endif