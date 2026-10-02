// ESP32 Web Stopwatch
// Creates a Wi-Fi access point and serves a web page with a chronometer a countdown timer and an interval counter.

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#if __has_include("config.h")
  #include "config.h"
#else
  #warning "config.h not found - using config.example.h. Copy it to config.h and set your own credentials."
  #include "config.example.h"
#endif

#include "buzzer.h"
#include "timekeeping.h"
#include "web_handlers.h"

static_assert(sizeof(AP_PASSWORD) - 1 >= 8 && sizeof(AP_PASSWORD) - 1 <= 63,
              "AP_PASSWORD must be 8-63 characters long (WPA2 requirement)");

static const byte DNS_PORT = 53;
static const uint32_t DEFAULT_TIMER_MS = 5UL * 60 * 1000;

WebServer server(80);
DNSServer dnsServer;
AppState app;
Buzzer buzzer(BUZZER_PIN);

static void restartAfterError(const char* message) 
{
  Serial.println(message);
  Serial.println("Restarting in 5 seconds...");
  delay(5000);
  ESP.restart();  
}

void setup() 
{
  Serial.begin(115200);
  buzzer.begin();

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) 
  {
    restartAfterError("Error: could not start the Wi-Fi access point.");
  }

  const IPAddress ip = WiFi.softAPIP();
  Serial.printf("Access point \"%s\" started. Open http://%s/\n", AP_SSID, ip.toString().c_str());

  if (CAPTIVE_PORTAL) 
  {
    dnsServer.start(DNS_PORT, "*", ip);
  }

  app.timer.setDuration(DEFAULT_TIMER_MS);

  registerRoutes(server, app);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() 
{
  if (CAPTIVE_PORTAL) 
  {
    dnsServer.processNextRequest();
  }
  server.handleClient();

  // Time-based events are handled here, independent of any browser.
  if (app.timer.update()) 
  {
    Serial.println("Timer finished.");
    buzzer.beep(3, 400, 200);
  }
  if (app.interval.update()) 
  {
    buzzer.beep(1, 120, 0);
  }
  buzzer.update();
}