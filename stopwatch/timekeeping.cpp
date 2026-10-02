#include "timekeeping.h"

// ---------- Stopwatch ----------

void Stopwatch::start() {
  if (running_) return;
  startedAt_ = millis();
  running_ = true;
}

void Stopwatch::stop() {
  if (!running_) return;
  accumulatedMs_ += millis() - startedAt_;  // kept in ms: no truncation per pause
  running_ = false;
}

void Stopwatch::reset() {
  running_ = false;
  accumulatedMs_ = 0;
}

uint32_t Stopwatch::elapsedMs() const {
  return running_ ? accumulatedMs_ + (millis() - startedAt_) : accumulatedMs_;
}

// ---------- LapList ----------

bool LapList::add(uint32_t splitMs) {
  if (count_ >= kCapacity) return false;
  splits_[count_++] = splitMs;
  return true;
}

// ---------- Countdown ----------

void Countdown::setDuration(uint32_t durationMs) {
  durationMs_ = durationMs;
  reset();
}

bool Countdown::start() {
  if (durationMs_ == 0) return false;
  if (finished_) reset();  // pressing Start after it finished restarts it
  clock_.start();
  return true;
}

void Countdown::stop() {
  clock_.stop();
}

void Countdown::reset() {
  clock_.reset();
  finished_ = false;
}

bool Countdown::update() {
  if (clock_.isRunning() && clock_.elapsedMs() >= durationMs_) {
    clock_.stop();
    finished_ = true;
    return true;
  }
  return false;
}

uint32_t Countdown::remainingMs() const {
  const uint32_t elapsed = clock_.elapsedMs();
  // Compare before subtracting: unsigned subtraction would underflow.
  return elapsed >= durationMs_ ? 0 : durationMs_ - elapsed;
}

// ---------- IntervalCounter ----------

void IntervalCounter::start(uint32_t intervalMs) {
  intervalMs_ = intervalMs;
  reportedCount_ = 0;
  clock_.reset();
  clock_.start();
}

void IntervalCounter::pause() {
  clock_.stop();
}

bool IntervalCounter::resume() {
  if (intervalMs_ == 0) return false;
  clock_.start();
  return true;
}

void IntervalCounter::reset() {
  clock_.reset();
  intervalMs_ = 0;
  reportedCount_ = 0;
}

uint32_t IntervalCounter::count() const {
  // Derived from elapsed time, so no intervals are missed
  // even if nobody is polling the web page.
  return intervalMs_ == 0 ? 0 : clock_.elapsedMs() / intervalMs_;
}

bool IntervalCounter::update() {
  const uint32_t current = count();
  if (current > reportedCount_) {
    reportedCount_ = current;
    return true;
  }
  return false;
}