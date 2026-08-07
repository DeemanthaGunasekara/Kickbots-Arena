#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

Servo myServo;

// Data structure received from transmitter
struct DataPacket {
  int x;
  int y;
  bool button;
};

DataPacket data;

// Motor driver pins (L298N)
const int ENA = 5;
const int IN1 = 2;
const int IN2 = 4;
const int IN3 = 7;
const int IN4 = 8;
const int ENB = 6;

// Servo
const int servoPin = 3;
bool prevButton = false;

void setup() {
  Serial.begin(9600);

  // Setup radio
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  // Motor setup
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Servo setup
  myServo.attach(servoPin);
  myServo.write(0); // Start position

  Serial.println("Receiver Ready");
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    handleMovement();
    handleServo();
  }
}

// ✅ Smooth 8-directional movement with differential drive
void handleMovement() {
  int xValue = data.x;
  int yValue = data.y;

  // Invert X axis if needed
  int xMapped = map(xValue, 0, 1023, 512, -512);
  int yMapped = map(yValue, 0, 1023, -512, 512);

  // Deadzone
  int deadZone = 100;
  if (abs(xMapped) < deadZone) xMapped = 0;
  if (abs(yMapped) < deadZone) yMapped = 0;

  // Differential motor control
  int leftSpeed = yMapped + xMapped;
  int rightSpeed = yMapped - xMapped;

  leftSpeed = constrain(leftSpeed, -512, 512);
  rightSpeed = constrain(rightSpeed, -512, 512);

  // Left motor direction
  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  // Right motor direction
  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  // Apply PWM
  analogWrite(ENA, map(abs(leftSpeed), 0, 512, 0, 255));
  analogWrite(ENB, map(abs(rightSpeed), 0, 512, 0, 255));
}

// ✅ Servo kicker – goes to 90° then back to 0° once per press
void handleServo() {
  if (data.button && !prevButton) {
    myServo.write(90);
    delay(300);  // allow servo to reach position
    myServo.write(0);
  }
  prevButton = data.button;
}
