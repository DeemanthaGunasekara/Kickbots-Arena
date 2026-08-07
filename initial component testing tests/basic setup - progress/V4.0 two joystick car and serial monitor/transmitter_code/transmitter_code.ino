#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

struct DataPacket {
  int x;        // Joystick 1 X
  int y;        // Joystick 1 Y
  bool button;  // Joystick 1 Button
  int x2;       // Joystick 2 X  ✅ NEW
};

DataPacket data;

void setup() {
  pinMode(4, INPUT_PULLUP); // Joystick 1 button (active LOW)
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
}

void loop() {
  data.x = analogRead(A0);       // Joystick 1 X
  data.y = analogRead(A1);       // Joystick 1 Y
  data.button = (digitalRead(4) == LOW);  // True when pressed
  data.x2 = analogRead(A2);      // Joystick 2 X ✅ NEW

  radio.write(&data, sizeof(data));
  delay(20);
}
