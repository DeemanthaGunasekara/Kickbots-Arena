// MASTER - Joystick Sender

// Define Joystick Pins
const int pinSW = 2;   // Joystick Button
const int pinVRx = A0; // Joystick X-axis
const int pinVRy = A1; // Joystick Y-axis

// Variables to store joystick values
int xValue = 0;
int yValue = 0;
int swState = 0;
int lastSwState = HIGH; // Assume button is not pressed initially

void setup() {
  // Initialize Joystick Button Pin
  pinMode(pinSW, INPUT_PULLUP);
  
  // Start Serial Communication for HC-05
  // Make sure the baud rate matches your HC-05's baud rate (often 38400 or 9600)
  Serial.begin(38400);
}

void loop() {
  // Read the joystick values
  xValue = analogRead(pinVRx);
  yValue = analogRead(pinVRy);
  swState = digitalRead(pinSW);

  // Map the analog values (0-1023) to a smaller range (e.g., 0-255) for easier transmission
  // You can also send the raw values if you prefer.
  int xMapped = map(xValue, 0, 1023, 0, 255);
  int yMapped = map(yValue, 0, 1023, 0, 255);

  // Send data as a single string separated by commas
  // Format: "X,Y,Button\n"
  Serial.print(xMapped);
  Serial.print(",");
  Serial.print(yMapped);
  Serial.print(",");
  Serial.print(swState);
  Serial.println(); // Sends a newline character (\n) to mark the end of the message

  // Small delay to avoid flooding the communication
  delay(50);
}
