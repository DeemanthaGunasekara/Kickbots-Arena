#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

struct JoystickData {
  int x;
  int y;
};

JoystickData data;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();
  Serial.println("Transmitter ready");
}

void loop() {
  data.x = analogRead(A0); // X-axis
  data.y = analogRead(A1); // Y-axis

  // Send data structure
  radio.write(&data, sizeof(data));

  Serial.print("Sent -> X:");
  Serial.print(data.x);
  Serial.print("  Y:");
  Serial.println(data.y);

  delay(100);
}
