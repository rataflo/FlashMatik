#include <Servo.h>

// Start position
#define SERVO_POS1 41
// Position when turning to tank 5
#define SERVO_POS2 160

#define SERVO_PIN 11
Servo servo;

int pos = 0;
void setup() {
  // put your setup code here, to run once:
// servo arm position
  servo.attach(SERVO_PIN);
  servo.write(SERVO_POS1);  
}

void loop() {
  //positions test
  //for (pos = SERVO_POS1; pos <= SERVO_POS2; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo.write(SERVO_POS1);              // tell servo to go to position in variable 'pos'
    //delay(15);                       // waits 15ms for the servo to reach the position
  //}
  delay(2000);
  //for (pos = SERVO_POS2; pos >= SERVO_POS1; pos -= 1) { // goes from 180 degrees to 0 degrees
    servo.write(SERVO_POS2);              // tell servo to go to position in variable 'pos'
    //delay(15);                       // waits 15ms for the servo to reach the position
  //}
  delay(2000);
}
