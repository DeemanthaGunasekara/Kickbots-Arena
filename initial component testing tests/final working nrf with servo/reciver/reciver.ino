#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

struct JoystickData {
  int x;
  int y;
};

JoystickData data;

// Motor pins
const int ENA = 5;  // PWM (left motor speed)
const int IN1 = 2;
const int IN2 = 4;
const int IN3 = 7;
const int IN4 = 8;
const int ENB = 6;  // PWM (right motor speed)

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Serial.println("Receiver ready");
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    int xValue = data.x;
    int yValue = data.y;

    Serial.print("X:");
    Serial.print(xValue);
    Serial.print("  Y:");
    Serial.println(yValue);

    int xCentered = xValue - 512;
    int yCentered = yValue - 512;

    int deadzone = 100; // small neutral range

    int leftSpeed = 0, rightSpeed = 0;

    if (abs(yCentered) < deadzone && abs(xCentered) < deadzone) {
      stopMotors();
      return;
    }

    if (yCentered > deadzone) {        // Forward
      leftSpeed  = map(yCentered, deadzone, 512, 0, 255);
      rightSpeed = map(yCentered, deadzone, 512, 0, 255);
      forward();
    } 
    else if (yCentered < -deadzone) {  // Backward
      leftSpeed  = map(yCentered, -deadzone, -512, 0, 255);
      rightSpeed = map(yCentered, -deadzone, -512, 0, 255);
      backward();
    }

    // Turning adjustment (X-axis)
    if (xCentered > deadzone) {        // Right turn
      rightSpeed = rightSpeed / 2;     // slow right motor
    } 
    else if (xCentered < -deadzone) {  // Left turn
      leftSpeed = leftSpeed / 2;       // slow left motor
    }

    analogWrite(ENA, constrain(leftSpeed, 0, 255));
    analogWrite(ENB, constrain(rightSpeed, 0, 255));
  }
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
