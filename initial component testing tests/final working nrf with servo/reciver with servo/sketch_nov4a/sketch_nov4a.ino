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
int servoAngle = 0;
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

void handleMovement() {
  int xValue = data.x;
  int yValue = data.y;

  // Map joystick values
  int xMapped = map(xValue, 0, 1023, -512, 512);
  int yMapped = map(yValue, 0, 1023, -512, 512);

  // Deadzone
  int deadZone = 100;
  if (abs(xMapped) < deadZone) xMapped = 0;
  if (abs(yMapped) < deadZone) yMapped = 0;

  // Motor control
  if (yMapped > 100) {
    // Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, map(yMapped, 0, 512, 0, 255));
    analogWrite(ENB, map(yMapped, 0, 512, 0, 255));
  } 
  else if (yMapped < -100) {
    // Backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, map(abs(yMapped), 0, 512, 0, 255));
    analogWrite(ENB, map(abs(yMapped), 0, 512, 0, 255));
  } 
  else if (xMapped > 100) {
    // Right
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, map(xMapped, 0, 512, 0, 255));
    analogWrite(ENB, map(xMapped, 0, 512, 0, 255));
  } 
  else if (xMapped < -100) {
    // Left
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, map(abs(xMapped), 0, 512, 0, 255));
    analogWrite(ENB, map(abs(xMapped), 0, 512, 0, 255));
  } 
  else {
    // Stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

void handleServo() {
  if (data.button && !prevButton) {
    // Toggle servo angle between 0° and 90°
    servoAngle = (servoAngle == 0) ? 90 : 0;
    myServo.write(servoAngle);
    delay(200); // Debounce
  }
  prevButton = data.button;
}
