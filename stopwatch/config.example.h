#pragma once

// Copy this file to config.h and adjust the values.
// config.h is ignored by git, so your real credentials never get committed.

// Wi-Fi access point created by the ESP32
constexpr char AP_SSID[]     = "ESP32-Stopwatch";
constexpr char AP_PASSWORD[] = "change-me-please";   // WPA2: 8-63 characters

// GPIO for an active buzzer or LED that signals timer/interval events.
// Set to -1 to disable.
constexpr int BUZZER_PIN = -1;

// Redirect every DNS query to the stopwatch page, so phones open it
// automatically after joining the network.
constexpr bool CAPTIVE_PORTAL = true;