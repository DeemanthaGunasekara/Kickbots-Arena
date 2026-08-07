#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// Stepper motor pins
#define STEP_PIN 2
#define DIR_PIN 3

// Motor configuration
#define STEPS_PER_REV 150
#define MAX_SPEED 800      // Minimum delay (fastest)
#define MIN_SPEED 2000     // Maximum delay (slowest)

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;       // Joystick 2 X for motor control
};

DataPacket data;
unsigned long lastStepTime = 0;
unsigned long lastReceiveTime = 0;
int currentDelay = 1000;
bool motorDirection = HIGH;
bool motorEnabled = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Arena Receiver Ready - Stepper Control");

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  // CHECK for radio data (non-blocking)
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    lastReceiveTime = millis();

    int joystickValue = data.x2;
    
    // Deadzone
    if (joystickValue > 412 && joystickValue < 612) {
      motorEnabled = false;
    } else {
      motorEnabled = true;
      
      if (joystickValue < 412) {
        motorDirection = LOW;
        currentDelay = map(constrain(joystickValue, 0, 412), 0, 412, MAX_SPEED, MIN_SPEED);
      } else {
        motorDirection = HIGH;
        currentDelay = map(constrain(joystickValue, 612, 1023), 612, 1023, MIN_SPEED, MAX_SPEED);
      }
      
      digitalWrite(DIR_PIN, motorDirection);
    }
  }

  // Stop motor if no signal for 500ms (safety)
  if (millis() - lastReceiveTime > 500) {
    motorEnabled = false;
  }

  // CONTINUOUSLY step the motor based on last received joystick value
  // This runs independently of radio reception!
  if (motorEnabled && (micros() - lastStepTime >= currentDelay)) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);
    lastStepTime = micros();
  }
}