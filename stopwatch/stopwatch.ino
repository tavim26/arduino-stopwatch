#include <WiFi.h>
#include <WebServer.h>

void handleStartChronometer();
void handleStopChronometer();
void handleResetChronometer();

void handleStartTimer();
void handleStopTimer();
void handleResetTimer();

void handleStartLapCounter();
void handleResetLapCounter();
void handleTime();

// Configurare  Wi-Fi
const char* ssid = "StopWatch";     
const char* password = "12345678+"; 

WebServer server(80);

// Variabile pentru cronometru
unsigned long startChronometerTime = 0;
unsigned long elapsedChronometerTime = 0;
bool runningChronometer = false;

// Variabile pentru timer
unsigned long countdownTime = 0;
unsigned long startTimerTime = 0;
bool runningTimer = false;

// Variabile pentru lap counter
unsigned int lapCount = 0;
unsigned long lapStartTime = 0;
unsigned long lapInterval = 0; 
bool runningLapCounter = false;

String generateHTML() 
{
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 Stopwatch</title>";
  
  // CSS
  html += "<style>";
  html += "body { font-family: 'Arial', sans-serif; background-color: #f5f5f5; margin: 0; padding: 0; color: #333;}";
  html += "h1, h2 { text-align: center; color: #007bff; }";
  html += "h1 { margin-top: 20px; font-size: 24px; }";
  html += "h2 { margin-top: 10px; font-size: 20px; }";
  html += "p { font-size: 36px; text-align: center; margin: 10px; }";
  html += "button { padding: 15px 20px; margin: 10px 0; font-size: 18px; background-color: #007bff; border: none; color: white; border-radius: 5px; cursor: pointer; width: 100%; transition: background-color 0.3s; }";
  html += "button:hover { background-color: #0056b3; }"; 
  html += "input[type='number'] { font-size: 18px; padding: 10px; width: 100%; margin: 10px 0; border-radius: 5px; border: 1px solid #ccc; }";
  html += "select { font-size: 18px; padding: 10px; width: 100%; margin: 10px 0; border-radius: 5px; border: 1px solid #ccc; }";
  html += "div { padding: 20px; max-width: 400px; margin: auto; background-color: white; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); margin-top: 20px; }";
  
  // Optimizare mobile device
  html += "@media screen and (max-width: 600px) {";
  html += "  button { font-size: 16px; padding: 12px 15px; }";
  html += "  p { font-size: 30px; }";
  html += "  h1 { font-size: 22px; }";
  html += "  h2 { font-size: 18px; }";
  html += "}";

  html += "</style>";
  
  // Script pentru actualizarea cronometrului
  html += "<script>";
  html += "function updateTime() {";
  html += "  fetch('/time').then(response => response.json()).then(data => {";
  html += "    document.getElementById('chronometer').innerText = data.chronometer;";
  html += "    document.getElementById('timer').innerText = data.timer;";
  html += "    document.getElementById('laps').innerText = data.laps;";
  html += "    setTimeout(updateTime, 1000);";
  html += "  });";
  html += "}"; 
  html += "function sendRequest(path) { fetch(path); }";
  html += "window.onload = updateTime;";
  html += "</script>";
  
  html += "</head><body>";

  // Cronometru
  html += "<div>";
  html += "<h1>ESP32 Stopwatch</h1>";
  html += "<h2>Chronometer</h2>";
  html += "<p id='chronometer'>00:00:00</p>";
  html += "<button onclick=\"sendRequest('/startChronometer')\">Start</button>";
  html += "<button onclick=\"sendRequest('/stopChronometer')\">Stop</button>";
  html += "<button onclick=\"sendRequest('/resetChronometer')\">Reset</button>";
  html += "</div>";

  // Timer
  html += "<div>";
  html += "<h2>Timer</h2>";
  html += "<p id='timer'>00:00:00</p>";
  html += "<select id='timer_select'>";
  html += "  <option value='300'>5 Minutes</option>";
  html += "  <option value='600'>10 Minutes</option>";
  html += "  <option value='900'>15 Minutes</option>";
  html += "  <option value='1200'>20 Minutes</option>";
  html += "</select>";
  html += "<button onclick=\"sendRequest('/startTimer?time=' + document.getElementById('timer_select').value)\">Start Timer</button>";
  html += "<button onclick=\"sendRequest('/stopTimer')\">Stop Timer</button>";
  html += "<button onclick=\"sendRequest('/resetTimer')\">Reset Timer</button>";
  html += "</div>";

  // lap counter
  html += "<div>";
  html += "<h2>Lap Counter</h2>";
  html += "<p id='laps'>0</p>";
  html += "<label for='lap_interval'>Set Lap Interval (seconds):</label>";
  html += "<input type='number' id='lap_interval' min='1' step='1'>";
  html += "<button onclick=\"sendRequest('/startLapCounter?interval=' + document.getElementById('lap_interval').value)\">Start Lap Counter</button>";
  html += "<button onclick=\"sendRequest('/resetLapCounter')\">Reset Lap Counter</button>";
  html += "</div>";

  html += "</body></html>";
  return html;

}



// Formatare timp HH:MM:SS
String formatTime(unsigned long seconds) 
{
  unsigned long hrs = seconds / 3600;
  unsigned long mins = (seconds / 60) % 60;
  unsigned long secs = seconds % 60;

  char timeString[9];
  sprintf(timeString, "%02lu:%02lu:%02lu", hrs, mins, secs);

  return String(timeString);
}

// Gestionare timp
void handleTime() 
{
  unsigned long currentTime = millis();

  // Actualizare cronometru
  unsigned long displayChronometerTime = elapsedChronometerTime;
  if (runningChronometer) 
  {
    displayChronometerTime += (currentTime - startChronometerTime) / 1000;
  }

  // Actualizare timer
  unsigned long displayTimerTime = countdownTime;
  if (runningTimer) 
  {
    displayTimerTime = countdownTime - (currentTime - startTimerTime) / 1000;

    if (displayTimerTime <= 0) 
    {
      displayTimerTime = 0;
      runningTimer = false;  
    }
  }

  // Actualizare lap 
  unsigned long displayLapTime = 0;

  if (runningLapCounter) 
  {
    displayLapTime = (currentTime - lapStartTime) / 1000;

    if (lapInterval > 0 && displayLapTime >= lapInterval) 
    {
      lapCount++;
      lapStartTime = currentTime; 
    }
  }


  // Construirea  JSON
  String json = "{";
  json += "\"chronometer\":\"" + formatTime(displayChronometerTime) + "\",";
  json += "\"timer\":\"" + formatTime(displayTimerTime) + "\",";
  json += "\"laps\":\"" + String(lapCount) + "\"";
  json += "}";
  server.send(200, "application/json", json);

}


// Handle pentru Lap Counter
void handleStartLapCounter() 
{
  if (server.hasArg("interval")) 
  {
    lapInterval = server.arg("interval").toInt();

    if (lapInterval > 0) 
    {
      lapStartTime = millis();
      runningLapCounter = true;
      lapCount = 0;  
      server.send(200, "text/plain", "Lap counter started");

    } 
    else 
    {
      server.send(400, "text/plain", "Invalid interval");
    }
  } 
  else 
  {
    server.send(400, "text/plain", "Interval not provided");
  }
}

void handleResetLapCounter() 
{
  runningLapCounter = false;
  lapCount = 0;
  lapInterval = 0;
  server.send(200, "text/plain", "Lap counter reset");
}

// pentru cronomteru
void handleStartChronometer() 
{
  if (!runningChronometer) 
  {
    runningChronometer = true;
    startChronometerTime = millis();
  }
  server.send(200, "text/plain", "Chronometer started");
}

void handleStopChronometer() 
{
  if (runningChronometer) 
  {
    elapsedChronometerTime += (millis() - startChronometerTime) / 1000;
    runningChronometer = false;
  }
  server.send(200, "text/plain", "Chronometer stopped");
}

void handleResetChronometer() 
{
  runningChronometer = false;
  elapsedChronometerTime = 0;
  server.send(200, "text/plain", "Chronometer reset");

}

// functii timer
void handleStartTimer() 
{
  if (server.hasArg("time")) 
  {
    countdownTime = server.arg("time").toInt();
    startTimerTime = millis();
    runningTimer = true;

    server.send(200, "text/plain", "Timer started");

  } 
  else 
  {
    server.send(400, "text/plain", "Time not provided");
  }
}

void handleStopTimer() 
{
  if (runningTimer) 
  {
    countdownTime -= (millis() - startTimerTime) / 1000;
    runningTimer = false;
  }

  server.send(200, "text/plain", "Timer stopped");
}

void handleResetTimer() 
{
  runningTimer = false;
  countdownTime = 0;
  server.send(200, "text/plain", "Timer reset");
}

// setup
void setup() 
{
  Serial.begin(115200);

  if (WiFi.softAP(ssid, password)) 
  {
    Serial.println("Wi-Fi AP started!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

  } 
  else 
  {
    Serial.println("Error: Could not start Wi-Fi!");
    while (1);
  }

  // rute
  server.on("/", []() { server.send(200, "text/html", generateHTML()); });
  server.on("/time", handleTime);

  server.on("/startChronometer", handleStartChronometer);
  server.on("/stopChronometer", handleStopChronometer);
  server.on("/resetChronometer", handleResetChronometer);

  server.on("/startTimer", handleStartTimer);
  server.on("/stopTimer", handleStopTimer);
  server.on("/resetTimer", handleResetTimer);

  server.on("/startLapCounter", handleStartLapCounter);
  server.on("/resetLapCounter", handleResetLapCounter);

  server.begin();

  Serial.println("HTTP server started.");

 
}

// main loop
void loop() 
{
  server.handleClient();
}