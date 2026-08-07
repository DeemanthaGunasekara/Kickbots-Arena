#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// Pins
#define JOY_X A0
#define JOY_Y A1
#define KICKER_BTN 4
#define GOALIE_X A2
#define GOALIE_BTN 5

struct DataPacket {
  int x;          // Car joystick X
  int y;          // Car joystick Y
  bool button;    // Kicker
  int x2;         // Goalkeeper joystick X
  bool goalieBtn; // Goalkeeper center button
};

DataPacket data;

void setup() {
  pinMode(KICKER_BTN, INPUT_PULLUP);
  pinMode(GOALIE_BTN, INPUT_PULLUP);

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
}

void loop() {
  // Car joystick
  data.x = analogRead(JOY_X);
  data.y = analogRead(JOY_Y);

  // Kicker button
  data.button = (digitalRead(KICKER_BTN) == LOW);

  // Goalkeeper joystick & center
  data.x2 = analogRead(GOALIE_X);
  data.goalieBtn = (digitalRead(GOALIE_BTN) == LOW);

  radio.write(&data, sizeof(data));

  delay(20);
}