#pragma once

// A23 (TMR): GPIO15/16/17 each have 10k to GND. T-2CAN leaves them floating.
enum Board { BOARD_UNKNOWN, BOARD_T2CAN, BOARD_TMR };
static Board board = BOARD_UNKNOWN;

static void boardDetect() {
  uint8_t bits[2] = {};
  bool stable = true;
  for (uint8_t pull = 0; pull < 2; ++pull) {
    for (int pin = 15; pin <= 17; ++pin)
      pinMode(pin, pull ? INPUT_PULLDOWN : INPUT_PULLUP);
    delayMicroseconds(2000);
    for (uint8_t sample = 0; sample < 8; ++sample) {
      uint8_t value = 0;
      for (int pin = 15; pin <= 17; ++pin)
        value |= (digitalRead(pin) == HIGH) << (pin - 15);
      if (sample && value != bits[pull]) stable = false;
      bits[pull] = value;
      delayMicroseconds(200);
    }
  }
  for (int pin = 15; pin <= 17; ++pin) pinMode(pin, INPUT);
  board = !stable || bits[1] != 0 ? BOARD_UNKNOWN :
          bits[0] == 0 ? BOARD_TMR : bits[0] == 7 ? BOARD_T2CAN : BOARD_UNKNOWN;
  if (board == BOARD_TMR) {
    // Deselect all XL2515s and hold all transceivers in standby before SPI.
    const int pins[] = {10, 9, 8, 18, 14, 21};
    for (int pin : pins) {
      digitalWrite(pin, HIGH);
      pinMode(pin, OUTPUT);
    }
  }
}
