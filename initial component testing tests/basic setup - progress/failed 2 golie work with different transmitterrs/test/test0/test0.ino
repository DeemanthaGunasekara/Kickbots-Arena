#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ===== NRF24 =====
RF24 radio(9, 10); // CE, CSN
const byte pipe1[6] = "CTRL1"; // Controller 1
const byte pipe2[6] = "CTRL2"; // Controller 2

// ===== STEPPER MOTORS =====
#define STEP_PIN1 2
#define DIR_PIN1  3
#define STEP_PIN2 4
#define DIR_PIN2  5

// ===== SPEED SETTINGS =====
#define MAX_SPEED 800
#define MIN_SPEED 2000
#define MAX_POSITION 2000
#define ACCELERATION 8  // microseconds per loop for ramp
#define DEADZONE_LOW 470
#define DEADZONE_HIGH 550

// ===== DATA STRUCTURE =====
struct DataPacket {
  int x;          // car joystick X
  int y;          // car joystick Y
  bool button;    // kicker
  int x2;         // goalkeeper joystick X
  bool goalieBtn; // goalkeeper center button
};

DataPacket data;

// ===== MOTOR STATE =====
struct StepperState {
  long position = 0;
  long targetPosition = 0;

  bool motorEnabled = false;
  bool motorDirection = HIGH;

  bool lastGoalieBtn = false;
  unsigned long lastStepTime = 0;
  unsigned long lastSignalTime = 0;

  float currentDelay = MIN_SPEED;
  float targetDelay = MIN_SPEED;
};

StepperState motor1;
StepperState motor2;

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

  // Read all available packets
  while (radio.available(&pipeNum)) {
    radio.read(&data, sizeof(data));

    // Debug: which controller sent data
    Serial.print("Pipe: "); Serial.print(pipeNum);
    Serial.print("  Joystick x2: "); Serial.println(data.x2);

    // Map pipe to correct motor
    if (pipeNum == 0) {
      motor1.lastSignalTime = millis();
      updateTarget(motor1, data.x2, data.goalieBtn);
    } else if (pipeNum == 1) {
      motor2.lastSignalTime = millis();
      updateTarget(motor2, data.x2, data.goalieBtn);
    }
  }

  // Safety: stop motors if signal lost
  if (millis() - motor1.lastSignalTime > 200) motor1.motorEnabled = false;
  if (millis() - motor2.lastSignalTime > 200) motor2.motorEnabled = false;

  // Move motors
  moveMotor(motor1, STEP_PIN1, DIR_PIN1);
  moveMotor(motor2, STEP_PIN2, DIR_PIN2);

  // Debug: motor positions
  Serial.print("Motor1 Pos: "); Serial.print(motor1.position);
  Serial.print("  Motor2 Pos: "); Serial.println(motor2.position);

  delay(50); // small delay for readability
}

// ===== UPDATE TARGET POSITION =====
void updateTarget(StepperState &motor, int joystickValue, bool button) {
  // Single press centering
  if (button && !motor.lastGoalieBtn) motor.targetPosition = 0;
  motor.lastGoalieBtn = button;

  // Deadzone: if in deadzone and no centering, hold current position
  if (joystickValue > DEADZONE_LOW && joystickValue < DEADZONE_HIGH && !button) {
    motor.targetPosition = motor.position; // hold position
    return;
  }

  // Map joystick to target position outside deadzone
  if (joystickValue <= DEADZONE_LOW) {
    float norm = (DEADZONE_LOW - joystickValue) / (float)DEADZONE_LOW;
    motor.targetPosition = -MAX_POSITION * norm;
  } else if (joystickValue >= DEADZONE_HIGH) {
    float norm = (joystickValue - DEADZONE_HIGH) / (1023.0 - DEADZONE_HIGH);
    motor.targetPosition = MAX_POSITION * norm;
  }
}

// ===== MOVE MOTOR WITH ACCELERATION =====
void moveMotor(StepperState &motor, int stepPin, int dirPin) {
  long error = motor.targetPosition - motor.position;

  if (abs(error) < 2) {
    motor.motorEnabled = false;
    return;
  }

  motor.motorEnabled = true;
  motor.motorDirection = (error > 0) ? LOW : HIGH;
  digitalWrite(dirPin, motor.motorDirection);

  // Map distance to speed
  float speedFactor = min(abs(error) / (float)MAX_POSITION, 1.0);
  motor.targetDelay = MIN_SPEED - (MIN_SPEED - MAX_SPEED) * speedFactor;

  // Ramp currentDelay toward targetDelay
  if (motor.currentDelay > motor.targetDelay) motor.currentDelay -= ACCELERATION;
  else if (motor.currentDelay < motor.targetDelay) motor.currentDelay += ACCELERATION;

  // Step the motor
  if (micros() - motor.lastStepTime >= motor.currentDelay) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(4);
    digitalWrite(stepPin, LOW);

    motor.position += (motor.motorDirection == HIGH) ? -1 : 1;
    motor.lastStepTime = micros();
  }
}