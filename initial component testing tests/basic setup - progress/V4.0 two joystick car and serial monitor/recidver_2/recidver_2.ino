#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001"; // Same as transmitter

struct DataPacket {
  int x;        // Joystick 1 X (ignored)
  int y;        // Joystick 1 Y (ignored)
  bool button;  // Joystick 1 button (ignored)
  int x2;       // Joystick 2 X ✅ used here
};

DataPacket data;

void setup() {
  Serial.begin(9600);
  Serial.println("Arena Receiver Ready (Serial Monitor Test)");

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    // Print only the second joystick X value
    Serial.print("Joystick 2 X: ");
    Serial.println(data.x2);
  }
}
