# ESP32 Web Stopwatch

A stopwatch, countdown timer and interval counter running on an ESP32 and controlled from any phone or laptop browser. The ESP32 creates its own Wi-Fi network and serves the web interface, so no internet connection or app is needed.

## Features

- **Chronometer** with lap times (lap duration and total split)
- **Countdown timer** with presets, custom duration (up to 99:59:59), pause/resume, and sound/vibration alert when it ends
- **Interval counter** that counts fixed-length intervals, with progress bar and pause/resume
- Smooth display with hundredths of a second, synced with the ESP32 every second
- Timekeeping runs on the ESP32 and continues even when no browser is connected
- Captive portal: the page opens automatically after joining the network
- Optional buzzer or LED on a GPIO pin for timer and interval events
- Responsive layout with dark mode

## Hardware

- Any ESP32 development board
- *(Optional)* active buzzer or LED connected to a GPIO pin and GND

## Project Structure

```
stopwatch/
├── stopwatch.ino        # Setup, main loop, Wi-Fi access point
├── config.example.h     # Configuration template (copy to config.h)
├── timekeeping.h/.cpp   # Stopwatch, Countdown, IntervalCounter, LapList
├── web_handlers.h/.cpp  # HTTP routes and JSON API
├── buzzer.h/.cpp        # Non-blocking buzzer/LED driver
└── index_html.h         # Web interface (HTML, CSS, JS)
```

The timekeeping logic is independent of the web layer: it only works with `millis()` differences, while the web handlers translate HTTP requests into calls on these classes.

## Getting Started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and add the **esp32 by Espressif Systems** package from the Boards Manager.
2. Clone the repository:
```bash
   git clone https://github.com/tavim26/esp32-web-stopwatch.git
```
3. Copy `stopwatch/config.example.h` to `stopwatch/config.h` and set your own values:
```cpp
   constexpr char AP_SSID[]     = "ESP32-Stopwatch";
   constexpr char AP_PASSWORD[] = "your-password";   // 8-63 characters
   constexpr int  BUZZER_PIN    = -1;                // GPIO number, or -1 to disable
```
4. Open `stopwatch/stopwatch.ino`, select your ESP32 board and port, then upload.

## Usage

1. Connect to the Wi-Fi network created by the ESP32.
2. The interface opens automatically. If it doesn't, go to `http://192.168.4.1`.
3. Keyboard shortcuts on desktop: **Space** starts/stops the chronometer, **L** records a lap.

## API

All actions are `POST` requests and return the updated state as JSON.

| Endpoint | Description |
|---|---|
| `GET /api/state` | Current state of all components |
| `/api/chrono/start` · `stop` · `reset` · `lap` | Chronometer control |
| `/api/timer/set?seconds=N` | Set timer duration (1–359999) |
| `/api/timer/start` · `stop` · `reset` | Timer control |
| `/api/interval/start?seconds=N` | Start interval counter |
| `/api/interval/pause` · `resume` · `reset` | Interval counter control |

## License

This project is licensed under the [MIT License](LICENSE).
