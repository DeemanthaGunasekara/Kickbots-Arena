#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);
const byte address[6] = "00001";

#define STEP_PIN 2
#define DIR_PIN 3

#define MAX_SPEED 800
#define MIN_SPEED 2000

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;
};

DataPacket data;

unsigned long lastStepTime = 0;
unsigned long lastReceiveTime = 0;

int currentDelay = 1000;
bool motorDirection = HIGH;
bool motorEnabled = false;
bool signalReceived = false;

void setup() {
  Serial.begin(9600);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {

  // Check for radio data
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastReceiveTime = millis();
    signalReceived = true;
  }

  // ❗ If transmitter OFF or no signal for 200ms → STOP MOTOR
  if (millis() - lastReceiveTime > 200) {
    motorEnabled = false;
    signalReceived = false;
  }

  if (signalReceived) {
    int joystickValue = data.x2;

    // deadzone
    if (joystickValue > 460 && joystickValue < 560) {
      motorEnabled = false;
    } else {
      motorEnabled = true;

      if (joystickValue < 460) {
        motorDirection = HIGH;
        currentDelay = map(constrain(joystickValue, 0, 460), 0, 460, MAX_SPEED, MIN_SPEED);
      } else {
        motorDirection = LOW;
        currentDelay = map(constrain(joystickValue, 560, 1023), 560, 1023, MIN_SPEED, MAX_SPEED);
      }

      digitalWrite(DIR_PIN, motorDirection);
    }
  }

  // Run stepper
  if (motorEnabled && (micros() - lastStepTime >= currentDelay)) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);
    lastStepTime = micros();
  }
}
