#include <AccelStepper.h>
#include <MultiStepper.h>
#include "src/DirectIO/DirectIO.h"

#define SCISSOR_PIN_STP 46
#define SCISSOR_PIN_DIR 48
#define SCISSOR_ENDSTOP_PIN 15
#define SCISSOR_PIN_ENABLE 62
#define SCISSOR_M0 22
#define SCISSOR_M1 39

#define SCISSOR_SPEED 3000
#define SCISSOR_ACCEL 9000
#define SCISSOR_STEP_OPENED 300 // Number of step to fully open the scissor.

AccelStepper scissor(1, SCISSOR_PIN_STP, SCISSOR_PIN_DIR);
Input<SCISSOR_ENDSTOP_PIN> endstopScissor(true);
Output<SCISSOR_PIN_ENABLE> enableScissor;

void setup() {
  Serial.begin(115200);
  pinMode(SCISSOR_M0, OUTPUT);
  pinMode(SCISSOR_M1, OUTPUT);
  digitalWrite(SCISSOR_M0, HIGH);
  digitalWrite(SCISSOR_M1, LOW);
  enableScissor.write(LOW);
  scissor.setCurrentPosition(0);
  scissor.setMaxSpeed(SCISSOR_SPEED);
  scissor.setAcceleration(SCISSOR_ACCEL);

  
}

void loop() {
  unsigned long currTime = millis();
  closeScissor();
  Serial.println(millis() - currTime);
  delay(2000);
  cutPaper();
  delay(2000);

}

void cutPaper() {
  enableScissor.write(LOW);
  scissor.moveTo(SCISSOR_STEP_OPENED);

  while(scissor.distanceToGo() != 0){
    scissor.run();
  }
  enableScissor.write(HIGH);
}

void closeScissor() {
  enableScissor.write(LOW);
  scissor.moveTo((-SCISSOR_STEP_OPENED)-50); 
  bool bCloseScissor = !endstopScissor.read();
  while(!bCloseScissor){
    scissor.run();
    bCloseScissor = !endstopScissor.read();
  }
  scissor.stop();
  scissor.setCurrentPosition(0);
  scissor.run();
  enableScissor.write(HIGH);
}
