/* Receiver code
 * Install the NRF24 library to your IDE
 * Upload this code to the Arduino UNO, NANO, Pro mini (5V,16MHz)
 * Connect a NRF24 module to it:
 
    NRF Module --> Arduino UNO,NANO
    
    GND    ->   GND
    Vcc    ->   3.3V
    CE     ->   D9
    CSN    ->   D10
    CLK    ->   D13
    MOSI   ->   D11
    MISO   ->   D12

  
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// RF24 setup
RF24 radio(9, 10);  // CE, CSN pins
const byte address[6] = "00001";

// Motor A (Left)
#define enA 5
#define in1 8
#define in2 7
// Motor B (Right)
#define enB 6
#define in3 4
#define in4 3

char receivedData[32] = "";    // Buffer for receiving data
int xAxis = 590, yAxis = 590;  // Safe default neutral joystick values
int motorSpeedA = 0, motorSpeedB = 0;

unsigned long lastSignalTime = 0;
const unsigned long SIGNAL_TIMEOUT = 500;  // ms

void setup() {
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));

    char *xStr = strtok(receivedData, ",");
    char *yStr = strtok(NULL, ",");

    if (xStr && yStr) {
      xAxis = atoi(xStr);
      yAxis = atoi(yStr);
      lastSignalTime = millis();
    }
  }

  if (millis() - lastSignalTime > SIGNAL_TIMEOUT) {
    stopMotors();
    return;
  }

  handleJoystick(xAxis, yAxis);
  motorControl(motorSpeedA, motorSpeedB);
}

void handleJoystick(int x, int y) {
  motorSpeedA = 0;
  motorSpeedB = 0;

  bool forward = false;
  bool backward = false;
  bool turning = false;

  if (y > 600) forward = true;
  else if (y < 500) backward = true;

  if (x < 500 || x > 600) turning = true;

  if (forward && !turning) {
    setMotorDir(true, true);
    motorSpeedA = map(y, 600, 1023, 0, 255);
    motorSpeedB = map(y, 600, 1023, 0, 255);
  } else if (backward && !turning) {
    setMotorDir(false, false);
    motorSpeedA = map(y, 500, 0, 0, 255);
    motorSpeedB = map(y, 500, 0, 0, 255);
  } else if (turning && !forward && !backward) {
    if (x < 500) {
      // Turn RIGHT: Left forward, Right backward
      setMotorDir(true, false);
      int xMapped = map(x, 500, 0, 0, 255);
      motorSpeedA = xMapped;
      motorSpeedB = xMapped;
    } else {
      // Turn LEFT: Left backward, Right forward
      setMotorDir(false, true);
      int xMapped = map(x, 600, 1023, 0, 255);
      motorSpeedA = xMapped;
      motorSpeedB = xMapped;
    }
  } else if (turning && (forward || backward)) {
    int baseSpeed = forward ? map(y, 600, 1023, 0, 255) : map(y, 500, 0, 0, 255);
    int turnAdjust = (x < 500) ? map(x, 500, 0, 0, baseSpeed) : map(x, 600, 1023, 0, baseSpeed);

    //turn+motion
    if (forward) {
      setMotorDir(true, true);
      motorSpeedA = (x < 500) ? baseSpeed - turnAdjust : baseSpeed;
      motorSpeedB = (x > 600) ? baseSpeed - turnAdjust : baseSpeed;
    } else {
      setMotorDir(false, false);
      motorSpeedA = (x < 500) ? baseSpeed - turnAdjust : baseSpeed;
      motorSpeedB = (x > 600) ? baseSpeed - turnAdjust : baseSpeed;
    }
  }

  motorSpeedA = constrain(motorSpeedA, 0, 255);
  motorSpeedB = constrain(motorSpeedB, 0, 255);
  
  if (motorSpeedA < 70) motorSpeedA = 0;
  if (motorSpeedB < 70) motorSpeedB = 0;
}

// Set motor directions individually
void setMotorDir(bool leftFwd, bool rightFwd) {
  digitalWrite(in1, leftFwd ? LOW : HIGH);
  digitalWrite(in2, leftFwd ? HIGH : LOW);

  digitalWrite(in3, rightFwd ? LOW : HIGH);
  digitalWrite(in4, rightFwd ? HIGH : LOW);
}

// Apply speed via PWM
void motorControl(int speedA, int speedB) {
  analogWrite(enA, speedA);
  analogWrite(enB, speedB);
}

// Stop motors safely
void stopMotors() {
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
