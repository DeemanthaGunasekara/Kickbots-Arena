/*
  ============================================================
  FOOTBALL ARENA CONTROLLER - ESP32 Firmware
  ============================================================
  Hardware:
    - ESP32 Dev Module
    - 2x IR Break-Beam Sensors (active LOW when beam is broken)
    - 2x WS2812B LED Strips (90 LEDs each) via FastLED
    - 3-pin Piezo Buzzer (passive, driven by PWM)

  Pin Assignments (change to match your wiring):
    IR_SENSOR_1   GPIO 34  (Player 1 goal sensor)
    IR_SENSOR_2   GPIO 35  (Player 2 goal sensor)
    LED_STRIP_1   GPIO 5   (Player 1 LED strip — Data pin)
    LED_STRIP_2   GPIO 18  (Player 2 LED strip — Data pin)
    BUZZER_PIN    GPIO 25  (PWM-capable pin)

  Dependencies (install via Arduino Library Manager):
    - FastLED  >= 3.6
    - ArduinoJson >= 6.x
    - ESPAsyncWebServer + AsyncTCP (install from GitHub)
  ============================================================
*/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// ── Pin config ──────────────────────────────────────────────
#define IR_SENSOR_1   34
#define IR_SENSOR_2   35
#define LED_STRIP_1   5
#define LED_STRIP_2   18
#define BUZZER_PIN    25
#define NUM_LEDS      90

// ── WiFi credentials ────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ── LED arrays ──────────────────────────────────────────────
CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];

// ── Web server & WebSocket ───────────────────────────────────
AsyncWebServer server(80);
AsyncWebSocket  ws("/ws");

// ── Game state ──────────────────────────────────────────────
enum GamePhase {
  PHASE_IDLE,       // Waiting for setup
  PHASE_RAINBOW,    // Rainbow intro (before countdown)
  PHASE_PLAYING,    // Normal play — both strips white
  PHASE_GOAL,       // Goal scored — 4-second celebration
  PHASE_RESUME,     // Brief resume flash + buzzer after goal
  PHASE_FINISHED    // Game over
};

GamePhase   phase          = PHASE_IDLE;
String      player1Name    = "Player 1";
String      player2Name    = "Player 2";
int         player1Score   = 0;
int         player2Score   = 0;
unsigned long gameDuration  = 0;   // in ms
unsigned long gameStartTime = 0;
unsigned long phaseStart    = 0;
int         lastGoalPlayer  = 0;   // 1 or 2

// Debounce
unsigned long lastGoal1Time = 0;
unsigned long lastGoal2Time = 0;
const unsigned long DEBOUNCE_MS = 500;

// Rainbow animation state
uint8_t rainbowHue = 0;

// ── Buzzer helpers ───────────────────────────────────────────
void buzzerTone(int freq, int dur) {
  ledcWriteTone(0, freq);
  delay(dur);
  ledcWriteTone(0, 0);
}

void buzzerGoal() {
  // Short triumphant beep sequence (non-blocking version uses timer below)
  ledcWriteTone(0, 1047); // C6
}

void buzzerResume() {
  ledcWriteTone(0, 800);
}

void buzzerOff() {
  ledcWriteTone(0, 0);
}

// ── LED helpers ──────────────────────────────────────────────
void setStrip(CRGB* strip, CRGB color) {
  fill_solid(strip, NUM_LEDS, color);
}

void rainbowStep() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds1[i] = CHSV(rainbowHue + (i * 256 / NUM_LEDS), 255, 200);
    leds2[i] = CHSV(rainbowHue + (i * 256 / NUM_LEDS), 255, 200);
  }
  FastLED.show();
  rainbowHue++;
}

// ── WebSocket broadcast ──────────────────────────────────────
void broadcastState() {
  StaticJsonDocument<256> doc;
  unsigned long remaining = 0;
  if (phase == PHASE_PLAYING || phase == PHASE_GOAL || phase == PHASE_RESUME) {
    unsigned long elapsed = millis() - gameStartTime;
    remaining = (elapsed < gameDuration) ? (gameDuration - elapsed) / 1000 : 0;
  }
  doc["phase"]         = (int)phase;
  doc["player1Name"]   = player1Name;
  doc["player2Name"]   = player2Name;
  doc["player1Score"]  = player1Score;
  doc["player2Score"]  = player2Score;
  doc["timeLeft"]      = remaining;
  doc["lastGoalPlayer"]= lastGoalPlayer;
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ── WebSocket event handler ──────────────────────────────────
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->opcode != WS_TEXT) return;

  String msg = String((char*)data).substring(0, len);
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;

  const char* cmd = doc["cmd"];
  if (!cmd) return;

  if (strcmp(cmd, "start") == 0) {
    player1Name   = doc["p1"]  | "Player 1";
    player2Name   = doc["p2"]  | "Player 2";
    gameDuration  = (unsigned long)(doc["time"] | 60) * 1000UL;
    player1Score  = 0;
    player2Score  = 0;
    lastGoalPlayer = 0;
    phase         = PHASE_RAINBOW;
    phaseStart    = millis();
    broadcastState();
  }

  if (strcmp(cmd, "reset") == 0) {
    phase = PHASE_IDLE;
    buzzerOff();
    setStrip(leds1, CRGB::Black);
    setStrip(leds2, CRGB::Black);
    FastLED.show();
    broadcastState();
  }
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // IR sensors — internal pull-up (beam broken = LOW)
  pinMode(IR_SENSOR_1, INPUT_PULLUP);
  pinMode(IR_SENSOR_2, INPUT_PULLUP);

  // Buzzer via LEDC
  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  // LED strips
  FastLED.addLeds<WS2812B, LED_STRIP_1, GRB>(leds1, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<WS2812B, LED_STRIP_2, GRB>(leds2, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(180);
  setStrip(leds1, CRGB::Black);
  setStrip(leds2, CRGB::Black);
  FastLED.show();

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Serve IP info
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/plain", WiFi.localIP().toString());
  });

  server.begin();
  Serial.println("Server started.");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {
  ws.cleanupClients();
  unsigned long now = millis();

  // ── RAINBOW phase: 3-second intro before game clock starts ──
  if (phase == PHASE_RAINBOW) {
    rainbowStep();
    delay(15);
    if (now - phaseStart >= 3000) {
      phase         = PHASE_PLAYING;
      gameStartTime = millis();
      phaseStart    = millis();
      setStrip(leds1, CRGB::White);
      setStrip(leds2, CRGB::White);
      FastLED.show();
      broadcastState();
    }
    return;
  }

  // ── PLAYING phase ────────────────────────────────────────────
  if (phase == PHASE_PLAYING) {
    // Check time
    unsigned long elapsed = now - gameStartTime;
    if (elapsed >= gameDuration) {
      phase = PHASE_FINISHED;
      buzzerOff();
      setStrip(leds1, CRGB::Black);
      setStrip(leds2, CRGB::Black);
      FastLED.show();
      broadcastState();
      return;
    }

    // Broadcast time every second
    static unsigned long lastBroadcast = 0;
    if (now - lastBroadcast >= 1000) {
      lastBroadcast = now;
      broadcastState();
    }

    // Check IR sensors (active LOW — beam broken = goal)
    bool s1 = (digitalRead(IR_SENSOR_1) == LOW);
    bool s2 = (digitalRead(IR_SENSOR_2) == LOW);

    if (s1 && (now - lastGoal1Time > DEBOUNCE_MS)) {
      lastGoal1Time  = now;
      player1Score++;
      lastGoalPlayer = 1;
      phase          = PHASE_GOAL;
      phaseStart     = now;
      // Player 1 scores: strip 1 → RED, strip 2 → OFF
      setStrip(leds1, CRGB::Red);
      setStrip(leds2, CRGB::Black);
      FastLED.show();
      buzzerGoal();
      broadcastState();
    } else if (s2 && (now - lastGoal2Time > DEBOUNCE_MS)) {
      lastGoal2Time  = now;
      player2Score++;
      lastGoalPlayer = 2;
      phase          = PHASE_GOAL;
      phaseStart     = now;
      // Player 2 scores: strip 2 → RED, strip 1 → OFF
      setStrip(leds1, CRGB::Black);
      setStrip(leds2, CRGB::Red);
      FastLED.show();
      buzzerGoal();
      broadcastState();
    }
    return;
  }

  // ── GOAL phase: 4 seconds of red + buzzer ────────────────────
  if (phase == PHASE_GOAL) {
    if (now - phaseStart >= 4000) {
      phase      = PHASE_RESUME;
      phaseStart = now;
      // Resume: both WHITE, short resume buzz
      setStrip(leds1, CRGB::White);
      setStrip(leds2, CRGB::White);
      FastLED.show();
      buzzerResume();
      broadcastState();
    }
    return;
  }

  // ── RESUME phase: 0.5 s resume beep then back to playing ─────
  if (phase == PHASE_RESUME) {
    if (now - phaseStart >= 500) {
      buzzerOff();
      phase = PHASE_PLAYING;
    }
    return;
  }

  // ── FINISHED phase: do nothing, web app shows winner ─────────
  if (phase == PHASE_FINISHED) {
    return;
  }
}
