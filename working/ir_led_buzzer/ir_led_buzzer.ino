/*
  IR Break Beam + LED Strip + Buzzer Controller
  Hardware:
    - Arduino Nano
    - 2x IR break beam sensors (active LOW when beam broken)
    - 2x WS2812B LED strips, 90 LEDs each
    - Piezo buzzer (passive)
    - 5V external power supply

  Pin assignments:
    D2  - IR Sensor 1 (INPUT_PULLUP)
    D3  - IR Sensor 2 (INPUT_PULLUP)
    D5  - LED Strip 1 data  ← changed from D6
    D6  - LED Strip 2 data  ← changed from D7
    D9  - Piezo buzzer

  IMPORTANT: Update your wiring to match the new pin assignments above.
*/

#include <FastLED.h>

// ── Pin definitions ─────────────────────────────────────
#define IR1_PIN       2
#define IR2_PIN       3
#define LED1_PIN      5     // Strip 1 data — D5
#define LED2_PIN      6     // Strip 2 data — D6
#define BUZZER_PIN    9

// ── LED strip config ─────────────────────────────────────
#define NUM_LEDS      90
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB

CRGB strip1[NUM_LEDS];
CRGB strip2[NUM_LEDS];

// ── Non-blocking buzzer ──────────────────────────────────
unsigned long buzzerStopTime = 0;
bool buzzerActive = false;

void startBeep(int freq, int durationMs) {
  tone(BUZZER_PIN, freq);
  buzzerStopTime = millis() + durationMs;
  buzzerActive = true;
}

void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerStopTime) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

// ── State machine ────────────────────────────────────────
enum State { STARTUP_RAINBOW, WHITE_IDLE, ALERT };
State currentState = STARTUP_RAINBOW;
unsigned long stateStart = 0;
uint8_t rainbowHue = 0;

// Track which sensor triggered so ALERT stays consistent
bool alertStrip1Red = false;

// ── Helpers ──────────────────────────────────────────────
void showBoth() {
  // Show strip1, brief pause, then strip2 — prevents data line crosstalk
  FastLED[0].showLeds(FastLED.getBrightness());
  delayMicroseconds(50);
  FastLED[1].showLeds(FastLED.getBrightness());
}

void fillBoth(CRGB color) {
  fill_solid(strip1, NUM_LEDS, color);
  fill_solid(strip2, NUM_LEDS, color);
}

// ────────────────────────────────────────────────────────
void setup() {
  pinMode(IR1_PIN, INPUT_PULLUP);
  pinMode(IR2_PIN, INPUT_PULLUP);

  // Register strips separately so we can show them independently
  FastLED.addLeds<LED_TYPE, LED1_PIN, COLOR_ORDER>(strip1, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, LED2_PIN, COLOR_ORDER>(strip2, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(180);

  // Startup: clear both strips
  fillBoth(CRGB::Black);
  showBoth();

  stateStart = millis();
}

// ────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();
  updateBuzzer();

  bool ir1Broken = (digitalRead(IR1_PIN) == LOW);
  bool ir2Broken = (digitalRead(IR2_PIN) == LOW);

  switch (currentState) {

    // ── STARTUP: rolling rainbow for 3 seconds ──────────
    case STARTUP_RAINBOW: {
      fill_rainbow(strip1, NUM_LEDS, rainbowHue, 3);
      fill_rainbow(strip2, NUM_LEDS, rainbowHue + 128, 3);
      showBoth();
      rainbowHue += 2;
      delay(20);

      if (now - stateStart >= 3000) {
        fillBoth(CRGB::White);
        showBoth();
        startBeep(880, 300);
        currentState = WHITE_IDLE;
        stateStart = now;
      }
      break;
    }

    // ── IDLE: both white, watching sensors ───────────────
    case WHITE_IDLE: {
      fillBoth(CRGB::White);
      showBoth();

      if (ir1Broken || ir2Broken) {
        // Lock in which strip goes red at the moment of trigger
        alertStrip1Red = ir1Broken;

        if (alertStrip1Red) {
          fill_solid(strip1, NUM_LEDS, CRGB::Red);
          fill_solid(strip2, NUM_LEDS, CRGB::Black);
        } else {
          fill_solid(strip1, NUM_LEDS, CRGB::Black);
          fill_solid(strip2, NUM_LEDS, CRGB::Red);
        }
        showBoth();
        startBeep(1200, 300);
        currentState = ALERT;
        stateStart = now;
      }
      break;
    }

    // ── ALERT: hold state until beam clears ─────────────
    case ALERT: {
      // Use the locked-in alertStrip1Red — do NOT re-read sensors for display
      // This prevents flickering if the sensor briefly re-triggers
      if (alertStrip1Red) {
        fill_solid(strip1, NUM_LEDS, CRGB::Red);
        fill_solid(strip2, NUM_LEDS, CRGB::Black);
      } else {
        fill_solid(strip1, NUM_LEDS, CRGB::Black);
        fill_solid(strip2, NUM_LEDS, CRGB::Red);
      }
      showBoth();

      // Return to white once both beams are clear
      if (!ir1Broken && !ir2Broken) {
        fillBoth(CRGB::White);
        showBoth();
        currentState = WHITE_IDLE;
        stateStart = now;
      }
      break;
    }
  }
}
