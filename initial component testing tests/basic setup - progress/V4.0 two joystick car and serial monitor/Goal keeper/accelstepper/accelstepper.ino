#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <AccelStepper.h>

#define STEP_PIN 2
#define DIR_PIN 3

RF24 radio(9, 10);
const byte address[6] = "00001";

// AccelStepper setup
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

struct DataPacket {
  int x;
  int y;
  bool button;
  int x2;
};

DataPacket data;

int centerValue = 512;
int deadzone = 80;

void setup() {
  Serial.begin(9600);
  Serial.println("Arena Receiver Ready");

  // Stepper settings
  stepper.setMaxSpeed(2000);     // maximum step speed
  stepper.setAcceleration(3000); // acceleration for smooth motion

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    int xValue = data.x2;
    int deviation = xValue - centerValue;

    if (abs(deviation) > deadzone) {
      // direction and speed
      long speed = map(abs(deviation), deadzone, 512, 0, 2000);
      speed = constrain(speed, 0, 2000);

      if (deviation > 0) {
        stepper.setSpeed(speed);   // rotate forward
      } else {
        stepper.setSpeed(-speed);  // rotate backward
      }
    }
    else {
      stepper.setSpeed(0); // stop smoothly
    }
  }

  stepper.runSpeed(); // non-blocking, smooth motion
}
