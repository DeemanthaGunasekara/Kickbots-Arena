#include <SPI.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint8_t payload[1];

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setRetries(15, 15);
  radio.setPayloadSize(1);
  radio.openWritingPipe(pipes[0]);
  radio.stopListening(); // important for TX mode
}

void loop() {
  payload[0] = 1; // example data
  bool ok = radio.write(payload, 1);

  Serial.print("Status: ");
  Serial.println(ok); // prints 1 if success, 0 if fail

  delay(200);
}
