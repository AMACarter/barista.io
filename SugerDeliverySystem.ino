///////////////////////////////////////////////
////////////// SMART BARISTA //////////////////
////////// SUGAR DELIVERY SYSTEM //////////////
///////////////////////////////////////////////

#include <Servo.h>

const byte servoPin = 9;      // pin used to command the servo motor
Servo servo;

void gateClose() {          //gate system closed
  servo.write(180);
  delay(175);
  servo.write(90);
}

/**
   release a ration of food
*/
void gateOpen() {
  servo.write(0);
  delay(175);
  servo.write(90);
}

void setup() {
  Serial.begin(9600);
  servo.attach(servoPin);
  Serial.println("Sugar System Ready");
}

void loop() {
  Serial.println("Waiting...");
  // Insert waiting command from server
  gateOpen();
  delay(150);
  gateClose();
}
