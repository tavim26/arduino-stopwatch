#pragma once

#include <WebServer.h>
#include "timekeeping.h"

// Everything the web interface can read or control.
struct AppState {
  Stopwatch chrono;
  LapList laps;
  Countdown timer;
  IntervalCounter interval;
};

// Registers the page, the JSON API and the captive-portal fallback.
// `server` and `app` must outlive the server (use globals).
void registerRoutes(WebServer& server, AppState& app);