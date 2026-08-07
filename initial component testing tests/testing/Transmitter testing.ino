/* Transmitter code
 * Install the NRF24 library to your IDE
 * Upload this code to the Arduino UNO, NANO, Pro mini (5V,16MHz)
 * Connect a NRF24 module to it:
 
    NRF Module --> Arduino UNO,NANO
    
    GND    ->   GND
    Vcc    ->   3.3V
    CE     ->   D9
    CSN    ->   D10
    CLK    ->   D13
    MOSI   ->   D11
    MISO   ->   D12

  
*/



#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// RF24 setup
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// Joystick pins
const int xPin = A1;
const int yPin = A0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening(); // Transmitter mode
}

void loop() {
  int xAxis = analogRead(xPin); // 0–1023
  int yAxis = analogRead(yPin); // 0–1023

  // Construct message: "590,620"
  String message = String(xAxis) + "," + String(yAxis);

  // Send data as C-style string
  radio.write(message.c_str(), message.length() + 1);
}
