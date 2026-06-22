#include <TMCStepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>


#define SPIDER_EXIT_PIN_STP A0
#define SPIDER_EXIT_PIN_DIR A1
#define SPIDER_EXIT_PIN_ENABLE 38
#define Y_ENDSTOP_PIN 3
#define Y_DISTANCE 6150
#define FAN_PIN 9

#define Y_BRAKE_PIN 7
#define SPIDER_SW_RX            A9 // TMC2208/TMC2224 SoftwareSerial receive pin
#define SPIDER_SW_TX            40 // TMC2208/TMC2224 SoftwareSerial transmit pin
#define R_SENSE 0.11f // Match to your driver

TMC2208Stepper driver(SPIDER_SW_RX, SPIDER_SW_TX, R_SENSE);  

AccelStepper stepperExit(1, SPIDER_EXIT_PIN_STP, SPIDER_EXIT_PIN_DIR);

void initY() {
    stepperExit.setCurrentPosition(0);
    //stepperY.setSpeed(INIT_SPEED);
    stepperExit.moveTo(-Y_DISTANCE-50);

    while (digitalRead(Y_ENDSTOP_PIN)) {
        stepperExit.run();
    }
    stepperExit.stop();
    stepperExit.run();
    //Serial.println(stepperY.currentPosition());
    stepperExit.setCurrentPosition(0);
}

void setup() { //Setup runs once//
  Serial.begin(9600);
  pinMode(Y_BRAKE_PIN, OUTPUT);
  pinMode(SPIDER_EXIT_PIN_ENABLE, OUTPUT);
  pinMode(Y_ENDSTOP_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);

  driver.begin();
  driver.toff(5);                 // Enables driver in software
  driver.rms_current(1400);        // Set motor RMS current
  driver.microsteps(4);          // Set microsteps to 1/16th
  driver.pwm_autoscale(true);
  driver.en_spreadCycle(true);   // SpreadCycle pour meilleures performances vitesse
  
  // Réglages avancés pour vitesse
  driver.tbl(1);                 // Blank time (1-2 pour vitesse)
  driver.hend(2);                // Hysteresis end (0-7, plus bas = plus rapide)
  driver.hstrt(3);               // Hysteresis start (0-7)
  
  // Configuration AccelStepper optimisée
  stepperExit.setMaxSpeed(8000);    // Augmenté significativement
  stepperExit.setAcceleration(10000); // Accélération très rapide

  digitalWrite(Y_BRAKE_PIN, HIGH);
  digitalWrite(SPIDER_EXIT_PIN_ENABLE, LOW);
  initY();

}

void loop() { //Loop runs forever//

  
  stepperExit.setCurrentPosition(0);
  stepperExit.moveTo(Y_DISTANCE);

    while (stepperExit.distanceToGo() != 0) {
        stepperExit.run();
    }
    stepperExit.stop();
    stepperExit.run();
    stepperExit.setCurrentPosition(0);

  stepperExit.moveTo(-Y_DISTANCE);

    while (stepperExit.distanceToGo() != 0) {
        stepperExit.run();
    }
    stepperExit.stop();
    stepperExit.run();

}