#include <Servo.h>

int joyx = A0;
int joyy = A1;
int buttonpin = 2;

int xval;
int yval;
int buttonstate;

int ENApin = 10;
int in1 = 8;
int in2 = 9;
int in3 = 6;
int in4 = 7; 
int ENBpin = 5;

int servoPin = 13; // Using pin 13 for servo instead of buzzer
Servo myServo; // Create servo object

int servoPosition = 0; // Current servo position
bool servoMoved = false; // Flag to track if servo has been moved

void setup() {
  pinMode(joyx, INPUT);
  pinMode(joyy, INPUT);
  pinMode(buttonpin, INPUT_PULLUP);
  
  pinMode(ENApin, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(ENBpin, OUTPUT);
  
  myServo.attach(servoPin); // Attach servo to pin 13
  myServo.write(0); // Start at 0 degrees
  
  Serial.begin(9600);
  Serial.println("System Ready - Press joystick button to move servo to 180 degrees");
}

void loop() {
  xval = analogRead(joyx);
  yval = analogRead(joyy);
  buttonstate = digitalRead(buttonpin);

  // Motor control logic (same as before)
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(ENApin, 0);
  analogWrite(ENBpin, 0);

  int xCenter = 510;
  int yCenter = 489;
  int threshold = 50;

  // Joystick UP - Forward
  if(yval < (yCenter - threshold)){
    int motorspeed = map(yval, yCenter - threshold, 0, 100, 255);
    motorspeed = constrain(motorspeed, 100, 255);
    
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print("Forward - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick DOWN - Backward
  else if(yval > (yCenter + threshold)){
    int motorspeed = map(yval, yCenter + threshold, 1023, 100, 255);
    motorspeed = constrain(motorspeed, 100, 255);
    
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print("Backward - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick LEFT - Turn Left
  else if(xval < (xCenter - threshold)){
    int motorspeed = map(xval, xCenter - threshold, 0, 150, 255);
    motorspeed = constrain(motorspeed, 150, 255);
    
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print("Left Turn - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick RIGHT - Turn Right
  else if(xval > (xCenter + threshold)){
    int motorspeed = map(xval, xCenter + threshold, 1023, 150, 255);
    motorspeed = constrain(motorspeed, 150, 255);
    
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print("Right Turn - Speed: ");
    Serial.println(motorspeed);
  }
  // Dead zone - Stop
  else {
    Serial.println("Stop");
  }

  // Servo control with joystick button
  if (buttonstate == LOW) { // Button pressed (LOW with INPUT_PULLUP)
    if (!servoMoved) {
      // Move servo to 180 degrees
      myServo.write(180);
      servoPosition = 180;
      servoMoved = true;
      Serial.println("Servo moved to 180 degrees");
      delay(500);
        myServo.write(0);
        // servoPosition = 0;
        servoMoved = true;
      delay(300); // Debounce delay
    }
  } else {
    // Button released - reset the flag
    servoMoved = false;
  }

  delay(50);
}