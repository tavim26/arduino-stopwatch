#pragma once

#include <Arduino.h>

// All time values are in milliseconds.
// Elapsed times are computed as differences of millis(), which stay correct
// across the ~49.7-day millis() rollover.

// Accumulating stopwatch with start/stop (pause) support.
class Stopwatch {
public:
  void start();
  void stop();
  void reset();

  bool isRunning() const { return running_; }
  uint32_t elapsedMs() const;

private:
  uint32_t accumulatedMs_ = 0;
  uint32_t startedAt_ = 0;
  bool running_ = false;
};

// Fixed-capacity list of lap split times (total elapsed time at each lap).
class LapList {
public:
  static constexpr size_t kCapacity = 99;

  bool add(uint32_t splitMs);  
  void clear() { count_ = 0; }

  size_t count() const { return count_; }
  uint32_t at(size_t index) const { return splits_[index]; }

private:
  uint32_t splits_[kCapacity] = {};
  size_t count_ = 0;
};

// Countdown timer with pause/resume.
class Countdown {
public:
  void setDuration(uint32_t durationMs);  
  bool start();                           
  void stop();                            
  void reset();                           

  // Call from loop(). Returns true exactly once, when the countdown hits zero.
  bool update();

  bool isRunning() const { return clock_.isRunning(); }
  bool isFinished() const { return finished_; }
  uint32_t durationMs() const { return durationMs_; }
  uint32_t remainingMs() const;

private:
  Stopwatch clock_;
  uint32_t durationMs_ = 0;
  bool finished_ = false;
};

// Counts how many fixed-length intervals have elapsed.
class IntervalCounter {
public:
  void start(uint32_t intervalMs);  
  void pause();
  bool resume();                    
  void reset();

  // Call from loop(). Returns true when a new interval has completed.
  bool update();

  bool isRunning() const { return clock_.isRunning(); }
  uint32_t intervalMs() const { return intervalMs_; }
  uint32_t elapsedMs() const { return clock_.elapsedMs(); }
  uint32_t count() const;

private:
  Stopwatch clock_;
  uint32_t intervalMs_ = 0;
  uint32_t reportedCount_ = 0;
};