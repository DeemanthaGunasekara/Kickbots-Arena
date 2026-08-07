#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00002";

// Updated DataPacket with goalkeeper button
struct DataPacket {
  int x;          // Joystick 1 X → car
  int y;          // Joystick 1 Y → car
  bool button;    // Joystick 1 Button → kicker
  int x2;         // Joystick 2 X → goalie
  bool goalieBtn; // Joystick 2 Button → return goalie to center
};

DataPacket data;

void setup() {
  pinMode(4, INPUT_PULLUP);  // Joystick 1 button → kicker
  pinMode(5, INPUT_PULLUP);  // Joystick 2 button → goalie center
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
}

void loop() {
  // Read joystick 1 for car
  data.x = analogRead(A0);
  data.y = analogRead(A1);

  // Read kicker button
  data.button = (digitalRead(4) == LOW);

  // Read joystick 2 X for goalie
  data.x2 = analogRead(A2);

  // Read goalie center button
  data.goalieBtn = (digitalRead(5) == LOW);

  // Send the packet
  radio.write(&data, sizeof(data));

  delay(20);
}