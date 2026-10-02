#pragma once

#include <Arduino.h>

// Non-blocking driver for an active buzzer or LED on a GPIO pin.
// A negative pin disables it, so the rest of the code needs no special cases.
class Buzzer {
public:
  explicit Buzzer(int pin) : pin_(pin) {}

  void begin();
  void beep(uint8_t count, uint16_t onMs, uint16_t offMs);
  void update();  // call from loop()

private:
  void write(bool on);

  int pin_;
  uint8_t remainingToggles_ = 0;
  uint16_t onMs_ = 0;
  uint16_t offMs_ = 0;
  uint32_t lastToggleAt_ = 0;
  bool on_ = false;
};