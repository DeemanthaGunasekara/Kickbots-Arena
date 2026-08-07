#define STEP_PIN 2
#define DIR_PIN 3

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  delay(2000);
}

void loop() {

  // Move 5 rotations in one direction
  digitalWrite(DIR_PIN, HIGH);
  for (int i = 0; i < 1000; i++) { // 1000 steps = 5 rotations
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }

  delay(1000);

  // Move 5 rotations in opposite direction
  digitalWrite(DIR_PIN, LOW);
  for (int i = 0; i < 1000; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }

  delay(1000);
}
