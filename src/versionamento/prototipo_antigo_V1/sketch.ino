// Definition of the LEDState class
class LEDState {
public:
  // Constructor of the LEDState class
  LEDState(int ledPin) : pin(ledPin), state(LOW) {
    pinMode(pin, OUTPUT);     // Set the pin as an output
    digitalWrite(pin, state); // Set the initial state (LOW)
  }

  // Method to toggle the LED state
  void toggle() {
    state = !state;          // Toggle the current state
    digitalWrite(pin, state); // Update the LED with the new state
  }

private:
  int pin;    // LED pin
  int state;  // LED state (LOW or HIGH)
};

// Definition of the button pins
#define BUTTON_PIN1 2
#define BUTTON_PIN2 17
#define BUTTON_PIN3 5

int aux = 0; // Auxiliary variable

// Creating LEDState objects to control the LEDs
LEDState led1(15);
LEDState led2(4);
LEDState led3(16);

// Initialization function (executed once at the beginning)
void setup() {
  Serial.begin(115200);   // Initialize serial communication for debugging
  pinMode(BUTTON_PIN1, INPUT_PULLUP); // Set button 1 as an input with an internal pull-up resistor
  pinMode(BUTTON_PIN2, INPUT_PULLUP); // Set button 2 as an input with an internal pull-up resistor
  pinMode(BUTTON_PIN3, INPUT_PULLUP); // Set button 3 as an input with an internal pull-up resistor
}

// Main function (executed repeatedly)
void loop() {
  if (digitalRead(BUTTON_PIN1) == LOW) {
    led1.toggle();       // Toggle the state of LED 1
    delay(1000);         // Wait for 1 second
    aux++;               // Increment the auxiliary variable
    Serial.println(aux); // Print the value of aux to the serial port
    Serial.println("Local com superlotação"); // Print a message to the serial port
  }

  if (digitalRead(BUTTON_PIN2) == LOW) {
    led2.toggle();       // Toggle the state of LED 2
    delay(1000);         // Wait for 1 second
    Serial.println("Local com leve lotação"); // Print a message to the serial port
  }

  if (digitalRead(BUTTON_PIN3) == LOW) {
    led3.toggle();       // Toggle the state of LED 3
    delay(1000);         // Wait for 1 second
    Serial.println("Local sem lotação"); // Print a message to the serial port
  }
}
