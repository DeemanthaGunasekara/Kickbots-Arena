#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);               
const byte address[6] = "00002";

// ===== MOTOR 1 =====
#define STEP_PIN1 2
#define DIR_PIN1  3

// ===== MOTOR 2 =====
#define STEP_PIN2 4
#define DIR_PIN2  5

// ===== SPEED SETTINGS =====
#define MAX_SPEED 800
#define MIN_SPEED 2000

// ===== TRAVEL LIMIT =====
#define MAX_POSITION 1500

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;
  bool goalieBtn;
};

DataPacket data;

unsigned long lastStepTime = 0;
unsigned long lastReceiveTime = 0;

int currentDelay = 1000;
bool motorDirection = HIGH;
bool motorEnabled = false;
bool signalReceived = false;

long currentPosition = 0;

// ===== CENTERING CONTROL =====
bool centeringActive = false;
bool lastGoalieBtn = false;

void setup() {

  pinMode(STEP_PIN1, OUTPUT);
  pinMode(DIR_PIN1, OUTPUT);

  pinMode(STEP_PIN2, OUTPUT);
  pinMode(DIR_PIN2, OUTPUT);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {

  // ===== RECEIVE RADIO =====
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastReceiveTime = millis();
    signalReceived = true;
  }

  // ===== SIGNAL LOSS SAFETY =====
  if (millis() - lastReceiveTime > 200) {
    motorEnabled = false;
    signalReceived = false;
  }

  if (signalReceived) {

    // ===== DETECT SINGLE BUTTON PRESS =====
    if (data.goalieBtn && !lastGoalieBtn) {
      centeringActive = true;
    }
    lastGoalieBtn = data.goalieBtn;

    // ===== CENTERING MODE =====
    if (centeringActive) {

      if (currentPosition > 0) {
        motorDirection = HIGH;
        motorEnabled = true;
      }
      else if (currentPosition < 0) {
        motorDirection = LOW;
        motorEnabled = true;
      }
      else {
        motorEnabled = false;
        centeringActive = false;   // Stop when centered
      }

      currentDelay = MAX_SPEED;

      digitalWrite(DIR_PIN1, motorDirection);
      digitalWrite(DIR_PIN2, motorDirection);
    }

    // ===== NORMAL JOYSTICK CONTROL =====
    else {

      int joystickValue = data.x2;

      // Deadzone
      if (joystickValue > 460 && joystickValue < 560) {
        motorEnabled = false;
      }
      else {

        // MOVE LEFT
        if (joystickValue < 460 && currentPosition > -MAX_POSITION) {
          motorDirection = HIGH;
          motorEnabled = true;

          float norm = (460 - joystickValue) / 460.0;
          norm = pow(norm, 0.33);
          currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
        }

        // MOVE RIGHT
        else if (joystickValue > 560 && currentPosition < MAX_POSITION) {
          motorDirection = LOW;
          motorEnabled = true;

          float norm = (joystickValue - 560) / (1023.0 - 560.0);
          norm = pow(norm, 0.33);
          currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
        }
        else {
          motorEnabled = false;
        }

        digitalWrite(DIR_PIN1, motorDirection);
        digitalWrite(DIR_PIN2, motorDirection);
      }
    }
  }

  // ===== STEP BOTH MOTORS =====
  if (motorEnabled && (micros() - lastStepTime >= currentDelay)) {

    digitalWrite(STEP_PIN1, HIGH);
    digitalWrite(STEP_PIN2, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN1, LOW);
    digitalWrite(STEP_PIN2, LOW);

    if (motorDirection == HIGH) currentPosition--;
    else currentPosition++;

    lastStepTime = micros();
  }
}