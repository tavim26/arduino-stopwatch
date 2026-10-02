#include "buzzer.h"

void Buzzer::begin() {
  if (pin_ < 0) return;
  pinMode(pin_, OUTPUT);
  write(false);
}

void Buzzer::beep(uint8_t count, uint16_t onMs, uint16_t offMs) {
  if (pin_ < 0 || count == 0) return;
  onMs_ = onMs;
  offMs_ = offMs;
  write(true);
  remainingToggles_ = count * 2 - 1;  // the first "on" already happened
  lastToggleAt_ = millis();
}

void Buzzer::update() {
  if (remainingToggles_ == 0) return;
  const uint32_t wait = on_ ? onMs_ : offMs_;
  if (millis() - lastToggleAt_ >= wait) {
    write(!on_);
    lastToggleAt_ = millis();
    remainingToggles_--;
  }
}

void Buzzer::write(bool on) {
  on_ = on;
  digitalWrite(pin_, on ? HIGH : LOW);
}