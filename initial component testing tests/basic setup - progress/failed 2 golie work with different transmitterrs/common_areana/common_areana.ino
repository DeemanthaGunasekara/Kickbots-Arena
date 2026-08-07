
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ===== NRF24 =====
RF24 radio(9, 10); // CE, CSN
// Controller 1
const byte address[6] = "CTRL1";

// Controller 2
const byte address[6] = "CTRL2"; // Controller 2

// ===== STEPPER MOTORS =====
#define STEP_PIN1 2
#define DIR_PIN1  3
#define STEP_PIN2 4
#define DIR_PIN2  5

// ===== SPEED SETTINGS =====
#define MAX_SPEED 800
#define MIN_SPEED 2000
#define MAX_POSITION 2000

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;
  bool goalieBtn;
};

DataPacket data;

// ===== MOTOR STATE =====
struct StepperState {
  long position = 0;
  bool motorEnabled = false;
  bool motorDirection = HIGH;
  bool centeringActive = false;
  bool lastGoalieBtn = false;
  unsigned long lastStepTime = 0;
  unsigned long currentDelay = MIN_SPEED;
};

StepperState motor1; // pipe 00001
StepperState motor2; // pipe 00002

unsigned long lastReceiveTime = 0;

void setup() {
  Serial.begin(9600);

  pinMode(STEP_PIN1, OUTPUT);
  pinMode(DIR_PIN1, OUTPUT);
  pinMode(STEP_PIN2, OUTPUT);
  pinMode(DIR_PIN2, OUTPUT);

  // NRF24 setup
  radio.begin();
  radio.openReadingPipe(1, pipe1);
  radio.openReadingPipe(2, pipe2);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  Serial.println("Goalkeeper Receiver Ready");
}

void loop() {
  uint8_t pipeNum;
  if (radio.available(&pipeNum)) {
    radio.read(&data, sizeof(data));
    lastReceiveTime = millis();

    // Map pipe to correct motor
    if (pipeNum == 1) processMotor(motor1, data.x2, data.goalieBtn, STEP_PIN1, DIR_PIN1);
    else if (pipeNum == 2) processMotor(motor2, data.x2, data.goalieBtn, STEP_PIN2, DIR_PIN2);
  }

  // Safety: stop motors if signal lost
  if (millis() - lastReceiveTime > 200) {
    motor1.motorEnabled = false;
    motor2.motorEnabled = false;
  }

  // Step motors
  stepMotor(motor1, STEP_PIN1);
  stepMotor(motor2, STEP_PIN2);
}

// ===== PROCESS MOTOR INPUT =====
void processMotor(StepperState &motor, int joystickValue, bool button, int stepPin, int dirPin) {
  // Single press centering
  if (button && !motor.lastGoalieBtn) motor.centeringActive = true;
  motor.lastGoalieBtn = button;

  if (motor.centeringActive) {
    if (motor.position > 0) motor.motorDirection = HIGH;
    else if (motor.position < 0) motor.motorDirection = LOW;
    else motor.centeringActive = false;

    motor.motorEnabled = motor.centeringActive;
    motor.currentDelay = MAX_SPEED;
    digitalWrite(dirPin, motor.motorDirection);
  }
  else {
    // Deadzone
    if (joystickValue > 460 && joystickValue < 560) motor.motorEnabled = false;
    else {
      if (joystickValue < 460 && motor.position > -MAX_POSITION) {
        motor.motorDirection = HIGH;
        motor.motorEnabled = true;
        float norm = (460 - joystickValue) / 460.0;
        norm = pow(norm, 0.33);
        motor.currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
      }
      else if (joystickValue > 560 && motor.position < MAX_POSITION) {
        motor.motorDirection = LOW;
        motor.motorEnabled = true;
        float norm = (joystickValue - 560) / (1023.0 - 560.0);
        norm = pow(norm, 0.33);
        motor.currentDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * norm;
      }
      else motor.motorEnabled = false;
      digitalWrite(dirPin, motor.motorDirection);
    }
  }
}

// ===== STEP MOTOR =====
void stepMotor(StepperState &motor, int stepPin) {
  if (!motor.motorEnabled) return;

  if (micros() - motor.lastStepTime >= motor.currentDelay) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(stepPin, LOW);

    if (motor.motorDirection == HIGH) motor.position--;
    else motor.position++;

    motor.lastStepTime = micros();
  }
}