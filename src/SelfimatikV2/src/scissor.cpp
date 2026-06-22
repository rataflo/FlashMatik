#include "scissor.h"

Scissor scissor;

Scissor::Scissor()
    : tmc(SCISSOR_RX, SCISSOR_TX, R_SENSE),
    stepper(1, SCISSOR_PIN_STP, SCISSOR_PIN_DIR) {}

void Scissor::init() {
    debug("initScissor", String("debut"));
    bMoving = false;
    bEndMove = false;
    pinModeFast(SCISSOR_ENDSTOP_PIN, INPUT);

    pinMode(SCISSOR_PIN_ENABLE, OUTPUT);
    pinMode(SCISSOR_PIN_STP, OUTPUT);
    pinMode(SCISSOR_PIN_DIR, OUTPUT);
    pinMode(SCISSOR_RX, INPUT);
    pinMode(SCISSOR_TX, OUTPUT);

    digitalWrite(SCISSOR_PIN_ENABLE, LOW);

    tmc.begin();
    tmc.toff(5);                 // Enables driver in software
    tmc.rms_current(1900);        // Set motor RMS current
    tmc.microsteps(0);          // Set microsteps to 1/16th
    tmc.pwm_autoscale(false);
    tmc.en_spreadCycle(true);   // SpreadCycle pour meilleures performances vitesse
    tmc.tbl(2);                 // Blank time (1-2 pour vitesse)
    tmc.hend(6);                // Hysteresis end (0-7, plus bas = plus rapide)
    tmc.hstrt(5);               // Hysteresis start (0-7)

    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(SCISSOR_SPEED);
    stepper.setAcceleration(SCISSOR_ACCEL);
    do {
        close();
    } while (!bEndMove);
    resetMove();
}

void Scissor::cutPaper() {
    if(!bMoving){
        debug("cutPaper", String("begin"));
        digitalWrite(SCISSOR_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.moveTo(SCISSOR_STEP_OPENED);
        bMoving = true;
    }
    if (stepper.distanceToGo() != 0) {
        stepper.run();
    } else {
        digitalWrite(SCISSOR_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}

void Scissor::close() {
    if(!bMoving){
        debug("closeScissor", String("begin"));
        digitalWrite(SCISSOR_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.moveTo((-SCISSOR_STEP_OPENED) - 50);
        bMoving = true;
    }
    if (digitalReadFast(SCISSOR_ENDSTOP_PIN)) {
        stepper.run();
    } else {
        Serial.println(stepper.currentPosition());
        stepper.stop();
        digitalWrite(SCISSOR_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}

bool Scissor::isEndMove(){
    return bEndMove;
}

void Scissor::resetMove(){
    bEndMove = false;
    bMoving = false;
}