#include <Arduino.h>
#include <AccelStepper.h>

#define PIN_STP 8
#define PIN_DIR 9
#define PIN_ENABLE 7
#define PIN_M1 5
#define PIN_M0 6
#define PIN_LED 13
#define PIN_START 4

#define NB_STEPS -14000
#define MAX_SPEED 2400
#define ACCEL 2600

AccelStepper stepper(1, PIN_STP, PIN_DIR);

void setup() {

  pinMode(PIN_START, INPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_M0, HIGH);
  digitalWrite(PIN_M1, HIGH);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCEL);
  delay(1000); // Wait for selfimatik board to start unless activate automatically exit.
}

void loop() {
  
    if(digitalRead(PIN_START) == HIGH){
      digitalWrite(PIN_ENABLE, LOW);
      digitalWrite(PIN_LED, HIGH);
      
      stepper.moveTo(NB_STEPS);

      while (stepper.distanceToGo() != 0) { 
          stepper.run();
      }

      stepper.stop();
      stepper.setCurrentPosition(0);
      stepper.run();
      
      digitalWrite(PIN_LED, LOW);
      digitalWrite(PIN_ENABLE, HIGH);
    }

}