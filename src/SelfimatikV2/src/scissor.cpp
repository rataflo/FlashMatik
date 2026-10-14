#include "scissor.h"
#include "parameters.h"

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
    tmc.rms_current(1900);       // 1.9A RMS (moteur 2.1A) : couple de coupe maximal
    tmc.microsteps(0);          // PAS COMPLETS (MRES=8) - comme l'ancienne version fonctionnelle
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(true);   // SpreadCycle pour meilleures performances vitesse
    tmc.tbl(1);                 // Blank time (1-2 pour vitesse)
    tmc.hend(2);                // Hysteresis end (0-7, plus bas = plus rapide)
    tmc.hstrt(3);               // Hysteresis start (0-7)

    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(parameters.params.scissorSpeed);
    stepper.setAcceleration(parameters.params.scissorAccel);
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
        stepper.moveTo(parameters.params.scissorStepOpened);
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
        stepper.moveTo((-parameters.params.scissorStepOpened) - 50);
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