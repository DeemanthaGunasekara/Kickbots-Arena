#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define STEP_PIN 2
#define DIR_PIN 3

RF24 radio(9, 10);
const byte address[6] = "00001";

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;
};

DataPacket data;

int centerValue = 512;  // expected joystick center
int deadzone = 80;

void setup() {
  Serial.begin(9600);
  Serial.println("Arena Receiver Ready");

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    int xValue = data.x2; // joystick value from controller

    int deviation = xValue - centerValue;

    if (abs(deviation) > deadzone) {
      // Direction control
      if (deviation > 0) digitalWrite(DIR_PIN, HIGH);
      else digitalWrite(DIR_PIN, LOW);

      // Map joystick deflection to step speed
      int speedDelay = map(abs(deviation), deadzone, 512, 800, 50);
      speedDelay = constrain(speedDelay, 50, 800);

      // One step pulse
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(speedDelay);
      digitalWrite(STEP_PIN, LOW);
      delayMicroseconds(speedDelay);

      Serial.print("Joystick: ");
      Serial.print(xValue);
      Serial.print(" | SpeedDelay: ");
      Serial.println(speedDelay);
    } 
    else {
      delay(5); // stop when joystick near center
    }
  }
}
