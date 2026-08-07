#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00002"; // change to 00002 for Car 2

Servo myServo;

// Data from transmitter
struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;         // ignored by car
  bool goalieBtn; // ignored by car
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

  myServo.attach(servoPin);
  myServo.write(0);

  Serial.println("Car Receiver Ready");
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

  int xMapped = map(xValue, 0, 1023, -512, 512);
  int yMapped = map(yValue, 0, 1023, 512, -512); // inverted to fix forward/backward

  int deadZone = 100;
  if (abs(xMapped) < deadZone) xMapped = 0;
  if (abs(yMapped) < deadZone) yMapped = 0;

  int leftSpeed = yMapped + xMapped;
  int rightSpeed = yMapped - xMapped;

  leftSpeed = constrain(leftSpeed, -512, 512);
  rightSpeed = constrain(rightSpeed, -512, 512);

  setMotor(ENA, IN1, IN2, leftSpeed);
  setMotor(ENB, IN3, IN4, rightSpeed);
}

void setMotor(int EN, int IN1, int IN2, int speedVal) {
  if (speedVal > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (speedVal < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
  analogWrite(EN, map(abs(speedVal), 0, 512, 0, 255));
}

void handleServo() {
  if (data.button && !prevButton) {
    myServo.write(90);
    delay(250);
    myServo.write(0);
  }
  prevButton = data.button;
}