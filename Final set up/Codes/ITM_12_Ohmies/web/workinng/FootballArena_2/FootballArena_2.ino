/*
 * Football Arena Controller — FINAL VERSION
 * Hardware: ESP32 + 2x IR Break Beam Sensors + 2x LED Strips (90 bulbs each) + Active Piezo Buzzer
 *
 * PIN LAYOUT:
 *   IR Sensor 1 (Player 1 goal)  -> GPIO 26
 *   IR Sensor 2 (Player 2 goal)  -> GPIO 27
 *   LED Strip 1 (Player 1 side)  -> GPIO 5
 *   LED Strip 2 (Player 2 side)  -> GPIO 18
 *   Piezo Buzzer                 -> GPIO 19
 *
 * WIRING:
 *   IR Sensors : VCC -> 3.3V, GND -> GND, OUT -> GPIO
 *   LED Strips : External 5V PSU -> Strip VCC
 *                External GND + ESP32 GND joined together -> Strip GND
 *                470Ω resistor on data line (GPIO -> Strip DIN)
 *   Buzzer     : GPIO 19 -> positive leg, GND -> negative leg
 *                HIGH = ON (active buzzer)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>

// ---- WiFi Credentials ----
const char* ssid     = "REDMI 15C";
const char* password = "123456789";

// ---- Pin Definitions ----
#define IR_SENSOR_1   26
#define IR_SENSOR_2   27
#define LED_STRIP_1    5
#define LED_STRIP_2   18
#define BUZZER_PIN    19

// ---- LED Config ----
#define NUM_LEDS        90
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define LED_BRIGHTNESS  80        // Keep low until external PSU is wired properly

CRGB strip1[NUM_LEDS];
CRGB strip2[NUM_LEDS];

// ---- Web Server ----
WebServer server(80);

// ---- Game State ----
struct GameState {
  bool          active     = false;
  bool          paused     = false;
  int           timeLeft   = 0;
  int           score1     = 0;
  int           score2     = 0;
  String        name1      = "Player 1";
  String        name2      = "Player 2";
  int           totalTime  = 0;
  unsigned long lastTick   = 0;
  unsigned long pauseEnd   = 0;
  int           goalWinner = 0;
};

GameState game;

// ---- IR Edge Detection ----
bool sensor1Clear = true;
bool sensor2Clear = true;

// ---- LED task state ----
// We drive LEDs from a separate FreeRTOS task to avoid WiFi interference
enum LedMode {
  LED_OFF,
  LED_RAINBOW,
  LED_WHITE,
  LED_GOAL_1,    // strip1 red, strip2 off
  LED_GOAL_2,    // strip2 red, strip1 off
  LED_WIN_1,     // strip1 green, strip2 red
  LED_WIN_2,     // strip1 red, strip2 green
  LED_DRAW       // both yellow
};

volatile LedMode ledMode   = LED_RAINBOW;  // rainbow on power-on
volatile uint8_t rainbowHue = 0;

// ---- Buzzer (non-blocking) ----
bool          buzzerActive = false;
unsigned long buzzerEnd    = 0;

// HIGH = ON for this hardware
void buzzerOn()  { digitalWrite(BUZZER_PIN, HIGH); }
void buzzerOff() { digitalWrite(BUZZER_PIN, LOW);  }

void startBuzzer(unsigned long ms) {
  buzzerOn();
  buzzerActive = true;
  buzzerEnd    = millis() + ms;
}

void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerEnd) {
    buzzerOff();
    buzzerActive = false;
  }
}

// =========================================================
//  LED Task — runs on Core 0, WiFi runs on Core 1
//  This prevents FastLED timing from being disrupted by WiFi
// =========================================================
void ledTask(void* param) {
  FastLED.addLeds<LED_TYPE, LED_STRIP_1, COLOR_ORDER>(strip1, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, LED_STRIP_2, COLOR_ORDER>(strip2, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(strip1, NUM_LEDS, CRGB::Black);
  fill_solid(strip2, NUM_LEDS, CRGB::Black);
  FastLED.show();

  for (;;) {
    switch (ledMode) {
      case LED_RAINBOW:
        for (int i = 0; i < NUM_LEDS; i++) {
          strip1[i] = CHSV(rainbowHue + (i * 256 / NUM_LEDS), 255, 200);
          strip2[i] = CHSV(rainbowHue + (i * 256 / NUM_LEDS), 255, 200);
        }
        FastLED.show();
        rainbowHue++;
        vTaskDelay(20 / portTICK_PERIOD_MS);
        break;

      case LED_WHITE:
        fill_solid(strip1, NUM_LEDS, CRGB::White);
        fill_solid(strip2, NUM_LEDS, CRGB::White);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_GOAL_1:
        fill_solid(strip1, NUM_LEDS, CRGB::Red);
        fill_solid(strip2, NUM_LEDS, CRGB::Black);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_GOAL_2:
        fill_solid(strip2, NUM_LEDS, CRGB::Red);
        fill_solid(strip1, NUM_LEDS, CRGB::Black);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_WIN_1:
        fill_solid(strip1, NUM_LEDS, CRGB::Green);
        fill_solid(strip2, NUM_LEDS, CRGB::Red);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_WIN_2:
        fill_solid(strip1, NUM_LEDS, CRGB::Red);
        fill_solid(strip2, NUM_LEDS, CRGB::Green);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_DRAW:
        fill_solid(strip1, NUM_LEDS, CRGB::Yellow);
        fill_solid(strip2, NUM_LEDS, CRGB::Yellow);
        FastLED.show();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        break;

      case LED_OFF:
      default:
        fill_solid(strip1, NUM_LEDS, CRGB::Black);
        fill_solid(strip2, NUM_LEDS, CRGB::Black);
        FastLED.show();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        break;
    }
  }
}

// =========================================================
//  CORS
// =========================================================
void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// =========================================================
//  API: POST /api/start
// =========================================================
void handleStart() {
  addCORS();
  if (server.method() == HTTP_OPTIONS) { server.send(204); return; }
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  game.name1      = doc["name1"] | "Player 1";
  game.name2      = doc["name2"] | "Player 2";
  game.totalTime  = doc["time"]  | 60;
  game.timeLeft   = game.totalTime;
  game.score1     = 0;
  game.score2     = 0;
  game.active     = true;
  game.paused     = false;
  game.lastTick   = millis();
  game.goalWinner = 0;

  sensor1Clear = true;
  sensor2Clear = true;

  // Both strips white when game starts
  ledMode = LED_WHITE;

  // Beep to signal game start
  startBuzzer(500);

  server.send(200, "application/json", "{\"status\":\"started\"}");
  Serial.println("Game started!");
}

// =========================================================
//  API: POST /api/stop
// =========================================================
void handleStop() {
  addCORS();
  game.active  = false;
  game.paused  = false;
  buzzerOff();
  buzzerActive = false;
  ledMode      = LED_RAINBOW;   // back to rainbow when stopped
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

// =========================================================
//  API: GET /api/state
// =========================================================
void handleState() {
  addCORS();
  StaticJsonDocument<256> doc;
  doc["active"]   = game.active;
  doc["paused"]   = game.paused;
  doc["timeLeft"] = game.timeLeft;
  doc["score1"]   = game.score1;
  doc["score2"]   = game.score2;
  doc["name1"]    = game.name1;
  doc["name2"]    = game.name2;
  doc["total"]    = game.totalTime;
  doc["winner"]   = game.goalWinner;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleOptions() {
  addCORS();
  server.send(204);
}

// =========================================================
//  Goal trigger
// =========================================================
void triggerGoal(int player) {
  if (game.paused || !game.active) return;

  if (player == 1) {
    game.score1++;
    sensor1Clear = false;
    ledMode      = LED_GOAL_1;   // strip1 red, strip2 off
  } else {
    game.score2++;
    sensor2Clear = false;
    ledMode      = LED_GOAL_2;   // strip2 red, strip1 off
  }

  startBuzzer(3000);             // buzzer on for 3 seconds
  game.paused     = true;
  game.goalWinner = player;
  game.pauseEnd   = millis() + 3000;

  Serial.printf("GOAL! Player %d  |  Score: %d - %d\n", player, game.score1, game.score2);
}

// =========================================================
//  Resume after 3s pause
// =========================================================
void resumeGame() {
  game.paused     = false;
  game.goalWinner = 0;
  sensor1Clear    = true;
  sensor2Clear    = true;
  ledMode         = LED_WHITE;   // both strips white
  startBuzzer(200);              // short beep on resume
  Serial.println("Game resumed.");
}

// =========================================================
//  Game over
// =========================================================
void endGame() {
  game.active = false;
  game.paused = false;

  if (game.score1 > game.score2)      ledMode = LED_WIN_1;
  else if (game.score2 > game.score1) ledMode = LED_WIN_2;
  else                                ledMode = LED_DRAW;

  // 3 victory beeps
  for (int i = 0; i < 3; i++) {
    buzzerOn();  delay(300);
    buzzerOff(); delay(200);
  }

  Serial.printf("Game Over!  %s %d - %d %s\n",
    game.name1.c_str(), game.score1, game.score2, game.name2.c_str());
}

// =========================================================
//  SETUP
// =========================================================
void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR_1, INPUT_PULLUP);
  pinMode(IR_SENSOR_2, INPUT_PULLUP);
  pinMode(BUZZER_PIN,  OUTPUT);
  buzzerOff();

  // Start LED task on Core 0 (WiFi uses Core 1)
  xTaskCreatePinnedToCore(
    ledTask,     // function
    "LEDTask",   // name
    4096,        // stack size
    NULL,        // params
    1,           // priority
    NULL,        // handle
    0            // core 0
  );

  // Brief buzzer beep on boot
  buzzerOn(); delay(200); buzzerOff();

  // WiFi
  Serial.printf("Connecting to %s", ssid);
  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWiFi failed — check SSID/password.");
  }

  server.on("/api/start",  HTTP_POST,    handleStart);
  server.on("/api/start",  HTTP_OPTIONS, handleOptions);
  server.on("/api/stop",   HTTP_POST,    handleStop);
  server.on("/api/stop",   HTTP_OPTIONS, handleOptions);
  server.on("/api/state",  HTTP_GET,     handleState);
  server.on("/api/state",  HTTP_OPTIONS, handleOptions);
  server.begin();

  Serial.println("Server ready.");
  Serial.println(WiFi.localIP());
}

// =========================================================
//  LOOP — game logic only, LEDs handled by ledTask
// =========================================================
void loop() {
  server.handleClient();
  updateBuzzer();

  if (!game.active) return;

  unsigned long now = millis();

  // Goal celebration pause
  if (game.paused) {
    if (now >= game.pauseEnd) resumeGame();
    return;
  }

  // Countdown tick
  if (now - game.lastTick >= 1000) {
    game.lastTick = now;
    if (game.timeLeft > 0) {
      game.timeLeft--;
      if (game.timeLeft == 0) {
        endGame();
        return;
      }
    }
  }

  // IR sensor polling with edge detection
  bool s1 = (digitalRead(IR_SENSOR_1) == LOW);
  bool s2 = (digitalRead(IR_SENSOR_2) == LOW);

  if (!s1) sensor1Clear = true;
  if (!s2) sensor2Clear = true;

  if (s1 && sensor1Clear) triggerGoal(1);
  if (s2 && sensor2Clear) triggerGoal(2);
}
