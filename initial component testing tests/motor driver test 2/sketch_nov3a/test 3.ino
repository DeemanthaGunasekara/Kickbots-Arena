void setup() {
  pinMode (8,OUTPUT);
  pinMode(9,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(6,OUTPUT);

  // put your setup code here, to run once:

}

void loop() {
  digitalWrite(8,HIGH);
  digitalWrite(9,LOW);
  digitalWrite(7,HIGH);
  digitalWrite(6,LOW);
  delay(1000);
  digitalWrite(8,LOW);
  digitalWrite(9,HIGH);
  digitalWrite(7,LOW);
  digitalWrite(6,HIGH);

  delay(1000);
  
  // put your main code here, to run repeatedly:

}
