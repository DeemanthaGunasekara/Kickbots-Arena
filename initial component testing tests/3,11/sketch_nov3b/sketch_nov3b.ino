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

int buzzerpin = 13;

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
  pinMode(buzzerpin, OUTPUT);
  
  Serial.begin(9600);
}

void loop() {
  xval = analogRead(joyx);
  yval = analogRead(joyy);
  buttonstate = digitalRead(buttonpin);

  Serial.print("X: ");
  Serial.print(xval);
  Serial.print(" Y: ");
  Serial.print(yval);

  // Stop motors initially
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(ENApin, 0);
  analogWrite(ENBpin, 0);

  // Use YOUR specific center values with wider dead zone
  int xCenter = 510;  // Your measured center
  int yCenter = 489;  // Your measured center
  int threshold = 50; // Increased dead zone to prevent false triggers

  // Joystick UP - Forward (Y value decreases when pushed up)
  if(yval < (yCenter - threshold)){
    int motorspeed = map(yval, yCenter - threshold, 0, 100, 255);
    motorspeed = constrain(motorspeed, 100, 255);
    
    // Both motors forward
    digitalWrite(in1, HIGH);  // Motor A forward
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);  // Motor B forward
    digitalWrite(in4, LOW);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print(" - Forward - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick DOWN - Backward (Y value increases when pushed down)
  else if(yval > (yCenter + threshold)){
    int motorspeed = map(yval, yCenter + threshold, 1023, 100, 255);
    motorspeed = constrain(motorspeed, 100, 255);
    
    // Both motors backward
    digitalWrite(in1, LOW);   // Motor A backward
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);   // Motor B backward
    digitalWrite(in4, HIGH);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print(" - Backward - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick LEFT - Turn Left (X value decreases when pushed left)
  else if(xval < (xCenter - threshold)){
    int motorspeed = map(xval, xCenter - threshold, 0, 150, 255);
    motorspeed = constrain(motorspeed, 150, 255);
    
    // Turn LEFT: Right motor forward, Left motor backward
    digitalWrite(in1, HIGH);  // Right motor (A) FORWARD
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);   // Left motor (B) BACKWARD  
    digitalWrite(in4, HIGH);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print(" - Left Turn - Speed: ");
    Serial.println(motorspeed);
  }
  // Joystick RIGHT - Turn Right (X value increases when pushed right)
  else if(xval > (xCenter + threshold)){
    int motorspeed = map(xval, xCenter + threshold, 1023, 150, 255);
    motorspeed = constrain(motorspeed, 150, 255);
    
    // Turn RIGHT: Right motor backward, Left motor forward
    digitalWrite(in1, LOW);   // Right motor (A) BACKWARD
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);  // Left motor (B) FORWARD
    digitalWrite(in4, LOW);
    
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);
    Serial.print(" - Right Turn - Speed: ");
    Serial.println(motorspeed);
  }
  // Dead zone - Stop (joystick centered)
  else {
    analogWrite(ENApin, 0);
    analogWrite(ENBpin, 0);
    Serial.println(" - Stop");
  }

  // Buzzer control
  if (buttonstate == HIGH) {
    digitalWrite(buzzerpin, LOW);
  }
  else {
    digitalWrite(buzzerpin, HIGH);
  }

  delay(50);
}