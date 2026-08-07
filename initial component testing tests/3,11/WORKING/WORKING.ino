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
  // put your setup code here, to run once:
  pinMode(joyx , INPUT);
  pinMode(joyy , INPUT);


  pinMode(buttonpin, INPUT);
 
 pinMode(ENApin, OUTPUT);
 pinMode(in1, OUTPUT);
 pinMode(in2, OUTPUT);
 pinMode(in3, OUTPUT);
 pinMode(in4, OUTPUT);
 pinMode(ENBpin, OUTPUT);
pinMode(buzzerpin, OUTPUT);

}

void loop() {

  // put your main code here, to run repeatedly:
  xval = analogRead(joyx);
  yval= analogRead(joyy);
  buttonstate = digitalRead(buttonpin);

  //joystick up
  if(yval <= 485){
    int motorspeed = map( yval, 485, 0, 0, 255);
    digitalWrite(in1,HIGH);
    digitalWrite(in2,LOW);
    digitalWrite(in3,HIGH);
    digitalWrite(in4,LOW);
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);


  }
  // joystickdown
   if(yval >= 495){
    int motorspeed = map( yval, 495, 1023, 0, 255);
    digitalWrite(in1,LOW);
    digitalWrite(in2,HIGH);
    digitalWrite(in3,LOW);
    digitalWrite(in4,HIGH);
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, motorspeed);


  }
  // Jjoystick left
  if(xval <= 505){
    int motorspeed = map(xval, 505, 0, 0, 255);
    digitalWrite(in1,LOW);
    digitalWrite(in2,LOW);
    digitalWrite(in3,HIGH);
    digitalWrite(in4,LOW);
    analogWrite(ENApin, 0);
    analogWrite(ENBpin, motorspeed);
  }
// JOYSTICK RIGHT
  if(xval >= 515){
    int motorspeed = map(xval, 515, 1023, 0, 255);
    digitalWrite(in1,HIGH);
    digitalWrite(in2,LOW);
    digitalWrite(in3,LOW);
    digitalWrite(in4,LOW);
    analogWrite(ENApin, motorspeed);
    analogWrite(ENBpin, 0);
  }

  // dead zone
  if (yval > 485 && yval < 495 && xval > 505 && xval < 515){
    digitalWrite(in1,LOW);
    digitalWrite(in2,LOW);
    digitalWrite(in3,LOW);
    digitalWrite(in4,LOW);
  }

  //
  if (buttonpin == HIGH){
    digitalWrite(buzzerpin, LOW);
  }
  
  if (buttonpin == LOW){
    digitalWrite(buzzerpin, HIGH);
  }

}
