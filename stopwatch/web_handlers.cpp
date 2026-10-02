#include "web_handlers.h"

#include <WiFi.h>
#include "index_html.h"

namespace {

constexpr uint32_t kMaxSeconds = 99UL * 3600 + 59 * 60 + 59;  // 99:59:59

void sendError(WebServer& server, int code, const char* message) {
  server.send(code, "text/plain", message);
}

void appendBool(String& json, bool value) {
  json += value ? "true" : "false";
}

String buildStateJson(const AppState& app) {
  String json;
  json.reserve(320 + app.laps.count() * 11);

  json += "{\"chrono\":{\"running\":";
  appendBool(json, app.chrono.isRunning());
  json += ",\"elapsed\":";
  json += app.chrono.elapsedMs();
  json += ",\"laps\":[";
  for (size_t i = 0; i < app.laps.count(); ++i) {
    if (i > 0) json += ',';
    json += app.laps.at(i);
  }

  json += "]},\"timer\":{\"running\":";
  appendBool(json, app.timer.isRunning());
  json += ",\"finished\":";
  appendBool(json, app.timer.isFinished());
  json += ",\"duration\":";
  json += app.timer.durationMs();
  json += ",\"remaining\":";
  json += app.timer.remainingMs();

  json += "},\"interval\":{\"running\":";
  appendBool(json, app.interval.isRunning());
  json += ",\"interval\":";
  json += app.interval.intervalMs();
  json += ",\"elapsed\":";
  json += app.interval.elapsedMs();
  json += "}}";

  return json;
}

void sendState(WebServer& server, const AppState& app) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buildStateJson(app));
}

// Validates ?seconds=N (integer, 1..359999). Sends a 400 and returns false on error.
bool readSecondsArg(WebServer& server, uint32_t& outMs) {
  const String raw = server.arg("seconds");  // "" if missing

  // toInt() silently returns 0 for garbage, so check the digits explicitly.
  bool valid = raw.length() > 0 && raw.length() <= 6;
  for (unsigned int i = 0; valid && i < raw.length(); ++i) {
    valid = isDigit(raw[i]);
  }

  const uint32_t seconds = valid ? strtoul(raw.c_str(), nullptr, 10) : 0;
  if (seconds < 1 || seconds > kMaxSeconds) {
    sendError(server, 400, "'seconds' must be an integer between 1 and 359999");
    return false;
  }

  outMs = seconds * 1000UL;
  return true;
}

// POST endpoint that runs `action` and replies with the new state.
// The action returns false when it has already sent an error response.
template <typename Action>
void onAction(WebServer& server, AppState& app, const char* uri, Action action) {
  server.on(uri, HTTP_POST, [&server, &app, action]() {
    if (action()) sendState(server, app);
  });
}

}  // namespace

void registerRoutes(WebServer& server, AppState& app) {
  server.on("/", HTTP_GET, [&server]() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // Read-only: computes the state, never modifies it.
  server.on("/api/state", HTTP_GET, [&server, &app]() {
    sendState(server, app);
  });

  // ----- Chronometer -----
  onAction(server, app, "/api/chrono/start", [&app]() {
    app.chrono.start();
    return true;
  });
  onAction(server, app, "/api/chrono/stop", [&app]() {
    app.chrono.stop();
    return true;
  });
  onAction(server, app, "/api/chrono/reset", [&app]() {
    app.chrono.reset();
    app.laps.clear();
    return true;
  });
  onAction(server, app, "/api/chrono/lap", [&server, &app]() {
    if (!app.chrono.isRunning()) {
      sendError(server, 409, "Chronometer is not running");
      return false;
    }
    if (!app.laps.add(app.chrono.elapsedMs())) {
      sendError(server, 409, "Lap limit reached - reset the chronometer");
      return false;
    }
    return true;
  });

  // ----- Timer -----
  onAction(server, app, "/api/timer/set", [&server, &app]() {
    uint32_t durationMs;
    if (!readSecondsArg(server, durationMs)) return false;
    app.timer.setDuration(durationMs);
    return true;
  });
  onAction(server, app, "/api/timer/start", [&server, &app]() {
    if (!app.timer.start()) {
      sendError(server, 409, "Set a duration first");
      return false;
    }
    return true;
  });
  onAction(server, app, "/api/timer/stop", [&app]() {
    app.timer.stop();
    return true;
  });
  onAction(server, app, "/api/timer/reset", [&app]() {
    app.timer.reset();
    return true;
  });

  // ----- Interval counter -----
  onAction(server, app, "/api/interval/start", [&server, &app]() {
    uint32_t intervalMs;
    if (!readSecondsArg(server, intervalMs)) return false;
    app.interval.start(intervalMs);
    return true;
  });
  onAction(server, app, "/api/interval/pause", [&app]() {
    app.interval.pause();
    return true;
  });
  onAction(server, app, "/api/interval/resume", [&server, &app]() {
    if (!app.interval.resume()) {
      sendError(server, 409, "Start the interval counter first");
      return false;
    }
    return true;
  });
  onAction(server, app, "/api/interval/reset", [&app]() {
    app.interval.reset();
    return true;
  });

  // Unknown API paths get a real 404; anything else (captive-portal checks
  // from phones, mistyped URLs) is redirected to the main page.
  server.onNotFound([&server]() {
    if (server.uri().startsWith("/api/")) {
      sendError(server, 404, "Unknown endpoint");
      return;
    }
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
}