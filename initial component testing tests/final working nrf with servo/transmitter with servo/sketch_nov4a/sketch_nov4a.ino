#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

struct DataPacket {
  int x;
  int y;
  bool button;
};

DataPacket data;

void setup() {
  pinMode(4, INPUT_PULLUP); // Joystick button (active LOW)
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
}

void loop() {
  data.x = analogRead(A0);
  data.y = analogRead(A1);
  data.button = (digitalRead(4) == LOW); // true when pressed
  radio.write(&data, sizeof(data));
  delay(20);
}
