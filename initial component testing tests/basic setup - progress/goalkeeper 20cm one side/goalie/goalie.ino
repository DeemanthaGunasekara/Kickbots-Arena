#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);               // CE, CSN pins
const byte address[6] = "00002";

#define STEP_PIN 2                // Step input to A4988
#define DIR_PIN 3                 // Direction input to A4988

#define MAX_SPEED 400             // fastest step delay (µs)
#define MIN_SPEED 1500            // slowest step delay (µs)

#define MAX_POSITION 200        // ±10 cm travel at 1/4 microstepping

struct DataPacket {
  int x;          // Car joystick X (ignored here)
  int y;          // Car joystick Y (ignored here)
  bool button;    // Car kicker button (ignored here)
  int x2;         // Goalkeeper joystick X
  bool goalieBtn; // Goalkeeper "return to center" button
};

DataPacket data;

unsigned long lastStepTime = 0;
unsigned long lastReceiveTime = 0;

int currentDelay = 1000;
bool motorDirection = HIGH;
bool motorEnabled = false;
bool signalReceived = false;

long currentPosition = 0;         // Step counter for software limits

// For edge detection
bool lastGoalieBtn = false;
bool returningToCenter = false;

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  // Read radio data
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastReceiveTime = millis();
    signalReceived = true;
  }

  // Stop motor if no signal for 200 ms
  if (millis() - lastReceiveTime > 200) {
    motorEnabled = false;
    signalReceived = false;
  }

  // Detect single button press (rising edge)
  if (signalReceived) {
    if (data.goalieBtn && !lastGoalieBtn) {
      returningToCenter = true; // start moving to center
    }
    lastGoalieBtn = data.goalieBtn;
  }

  // Return to center logic
  if (returningToCenter) {
    motorEnabled = true;

    if (currentPosition > 0) motorDirection = HIGH;  // Move left
    else if (currentPosition < 0) motorDirection = LOW; // Move right
    else {
      motorEnabled = false;
      returningToCenter = false; // reached center
    }

    currentDelay = MAX_SPEED; // Max speed for return
    digitalWrite(DIR_PIN, motorDirection);
  }
  // Normal joystick movement
  else if (signalReceived) {
    int joystickValue = data.x2;

    // Deadzone
    if (joystickValue > 460 && joystickValue < 560) {
      motorEnabled = false;
    } else {
      // Move LEFT
      if (joystickValue < 460 && currentPosition > -MAX_POSITION) {
        motorDirection = HIGH;
        motorEnabled = true;

        float norm = (460 - joystickValue) / 460.0;
        norm = pow(norm, 0.33);
        currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
      }
      // Move RIGHT
      else if (joystickValue > 560 && currentPosition < MAX_POSITION) {
        motorDirection = LOW;
        motorEnabled = true;

        float norm = (joystickValue - 560) / (1023 - 560.0);
        norm = pow(norm, 0.33);
        currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
      }
      else motorEnabled = false;

      digitalWrite(DIR_PIN, motorDirection);
    }
  }

  // Step motor if enabled
  if (motorEnabled && (micros() - lastStepTime >= currentDelay)) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);

    if (motorDirection == HIGH) currentPosition--;
    else if (motorDirection == LOW) currentPosition++;

    lastStepTime = micros();
  }
}