#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ===== NRF =====
RF24 radio(9,10);
const byte pipe1[6] = "CTRL1";
const byte pipe2[6] = "CTRL2";

// ===== STEPPER PINS =====
#define STEP_PIN1 2
#define DIR_PIN1 3
#define STEP_PIN2 4
#define DIR_PIN2 5

// ===== LIMITS =====
#define MAX_POSITION 2000

#define MAX_SPEED 700
#define MIN_SPEED 2000

#define ACCELERATION 8

// ===== DATA =====
struct DataPacket{
  int x;
  int y;
  bool button;
  int x2;
  bool goalieBtn;
};

DataPacket data;

// ===== MOTOR STATE =====
struct StepperState{

  long position = 0;
  long targetPosition = 0;

  bool motorDirection = HIGH;
  bool motorEnabled = false;

  bool lastGoalieBtn = false;

  unsigned long lastStepTime = 0;

  float currentSpeed = MIN_SPEED;
  float targetSpeed = MIN_SPEED;

  unsigned long lastSignalTime = 0;

};

StepperState motor1;
StepperState motor2;

void setup(){

  Serial.begin(9600);

  pinMode(STEP_PIN1,OUTPUT);
  pinMode(DIR_PIN1,OUTPUT);
  pinMode(STEP_PIN2,OUTPUT);
  pinMode(DIR_PIN2,OUTPUT);

  radio.begin();
  radio.openReadingPipe(1,pipe1);
  radio.openReadingPipe(2,pipe2);

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);

  radio.startListening();

  Serial.println("Goalkeeper Acceleration Control Ready");

}

void loop(){

  uint8_t pipe;

  while(radio.available(&pipe)){

    radio.read(&data,sizeof(data));

    if(pipe==1){
      motor1.lastSignalTime = millis();
      updateTarget(motor1,data.x2,data.goalieBtn);
    }

    if(pipe==2){
      motor2.lastSignalTime = millis();
      updateTarget(motor2,data.x2,data.goalieBtn);
    }

  }

  if(millis() - motor1.lastSignalTime > 200)
    motor1.motorEnabled = false;

  if(millis() - motor2.lastSignalTime > 200)
    motor2.motorEnabled = false;

  moveMotor(motor1,STEP_PIN1,DIR_PIN1);
  moveMotor(motor2,STEP_PIN2,DIR_PIN2);

}

// ===== TARGET POSITION =====
void updateTarget(StepperState &motor,int joy,bool button){

  if(button && !motor.lastGoalieBtn)
    motor.targetPosition = 0;

  motor.lastGoalieBtn = button;

  if(joy > 470 && joy < 550)
    return;

  float normalized;

  if(joy < 470){

    normalized = (470 - joy) / 470.0;
    motor.targetPosition = -MAX_POSITION * normalized;

  }

  if(joy > 550){

    normalized = (joy - 550) / (1023.0 - 550.0);
    motor.targetPosition = MAX_POSITION * normalized;

  }

}

// ===== MOTOR CONTROL =====
void moveMotor(StepperState &motor,int stepPin,int dirPin){

  long error = motor.targetPosition - motor.position;

  if(abs(error) < 2){
    motor.motorEnabled = false;
    return;
  }

  motor.motorEnabled = true;

  if(error > 0)
    motor.motorDirection = LOW;
  else
    motor.motorDirection = HIGH;

  digitalWrite(dirPin,motor.motorDirection);

  float speedFactor = min(abs(error)/(float)MAX_POSITION,1.0);

  motor.targetSpeed = MIN_SPEED - (MIN_SPEED - MAX_SPEED)*speedFactor;

  // ===== ACCELERATION RAMP =====
  if(motor.currentSpeed > motor.targetSpeed)
    motor.currentSpeed -= ACCELERATION;

  else if(motor.currentSpeed < motor.targetSpeed)
    motor.currentSpeed += ACCELERATION;

  if(micros() - motor.lastStepTime >= motor.currentSpeed){

    digitalWrite(stepPin,HIGH);
    delayMicroseconds(4);
    digitalWrite(stepPin,LOW);

    if(motor.motorDirection == HIGH)
      motor.position--;
    else
      motor.position++;

    motor.lastStepTime = micros();

  }

}