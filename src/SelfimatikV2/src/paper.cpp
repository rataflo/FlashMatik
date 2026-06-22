#include "paper.h"


PaperHandler paper;

PaperHandler::PaperHandler()
    : tmc(PAPER_RX, PAPER_TX, R_SENSE),
    stepper(1, PAPER_PIN_STP, PAPER_PIN_DIR){}

void PaperHandler::movePaperFirstShot() {
    if(!bMoving){ // start rewind.
        debug("movePaperFirstShot", String("begin"));
        digitalWriteFast(PAPER_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.setMaxSpeed(PAPER_SPEED);
        stepper.setAcceleration(PAPER_ACCEL);
        stepper.moveTo(-99999);
        bMoving = true;
        bRewindStep2 = false;
    }else{
        bool bSwitch = false;
        if(!bRewindStep2){
            bSwitch = digitalReadFast(PAPER_SWITCH1_PIN);
        }
        // first rewind to reach the opto endstop
        if(!bRewindStep2 && bSwitch){
            stepper.run();
        } else if (!bRewindStep2 && !bSwitch) { // init move after opto endstop reached
            // Init second move after opto endstop reached
            stepper.setCurrentPosition(0);
            stepper.moveTo(-DELTA_FIRST_SHOT);
            bRewindStep2 = true;
        } else if(bRewindStep2 && stepper.distanceToGo() != 0){
            // second rewind to reach the deltaFirstShot position
            stepper.run();
        } else if(bRewindStep2 && stepper.distanceToGo() == 0){
            // rewind finished
            digitalWriteFast(PAPER_PIN_ENABLE, HIGH);
            bMoving = false;
            bEndMove = true;
            bRewindStep2 = false;
        }
    }
}

void PaperHandler::init() {
    debug("initPaper", String("begin"));
    pinModeFast(PAPER_SWITCH1_PIN, INPUT);
    pinMode(PAPER_PIN_ENABLE, OUTPUT);
    pinMode(PAPER_PIN_STP, OUTPUT);
    pinMode(PAPER_PIN_DIR, OUTPUT);
    pinMode(PAPER_RX, INPUT);
    pinMode(PAPER_TX, OUTPUT);

    digitalWrite(PAPER_PIN_ENABLE, LOW);

    delay(100);
    tmc.begin();
    tmc.toff(5);                 // Enables driver in software
    tmc.rms_current(1500);        // Set motor RMS current
    tmc.microsteps(4);          // Set microsteps to 1/16th
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(false);   // SpreadCycle pour meilleures performances vitesse
    tmc.tbl(1);                 // Blank time (1-2 pour vitesse)
    tmc.hend(2);                // Hysteresis end (0-7, plus bas = plus rapide)
    tmc.hstrt(3);               // Hysteresis start (0-7)

    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(PAPER_SPEED);
    stepper.setAcceleration(PAPER_ACCEL);
    stepper.moveTo(1200);

    while (!digitalReadFast(PAPER_SWITCH1_PIN) && stepper.distanceToGo() != 0) {
        stepper.run();
    }

    if (!digitalReadFast(PAPER_SWITCH1_PIN)) {
        digitalWrite(PAPER_PIN_ENABLE, HIGH);
        while (!digitalReadFast(PAPER_SWITCH1_PIN));
    }
    digitalWrite(PAPER_PIN_ENABLE, LOW);
    stepper.setCurrentPosition(0);
    stepper.moveTo(-99999);

    while (digitalReadFast(PAPER_SWITCH1_PIN)) {
        stepper.run();
    }
    stepper.stop();
    stepper.setCurrentPosition(0);
    stepper.moveTo(-DELTA_FIRST_SHOT);

    while (stepper.distanceToGo() != 0) {
        stepper.run();
    }
    digitalWrite(PAPER_PIN_ENABLE, HIGH);

    resetMove();
}


void PaperHandler::movePaperNextShot() {
    if(!bMoving){
        debug("movePaperNextShot", String("begin"));
        digitalWrite(PAPER_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.setMaxSpeed(PAPER_OUT_SPEED);
        stepper.setAcceleration(PAPER_OUT_ACCEL);
        stepper.moveTo(NB_STEP_PAPER_ONE_SHOT);
        bMoving = true;
    }
    if (stepper.distanceToGo() != 0) {
        stepper.run();
    } else {
        digitalWrite(PAPER_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}

void PaperHandler::movePaperPreviousShot() {
    debug("movePaperPreviousShot", String("begin"));
    digitalWrite(PAPER_PIN_ENABLE, LOW);
    stepper.setCurrentPosition(0);
    stepper.moveTo(parameters.params.nbStepOneShot);

    while (stepper.distanceToGo() != 0) {
        stepper.run();
    }
    digitalWrite(PAPER_PIN_ENABLE, HIGH);
}

void PaperHandler::movePaperOut() {
    if(!bMoving){
        debug("movePaperOut", String("begin"));
        digitalWrite(PAPER_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.setMaxSpeed(PAPER_OUT_SPEED);
        stepper.setAcceleration(PAPER_OUT_ACCEL);
        stepper.moveTo(NB_STEP_PAPER_OUT);
        bMoving = true;
    }
    if (stepper.distanceToGo() != 0) {
        stepper.run();
    } else {
        digitalWrite(PAPER_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}


void PaperHandler::movePaperCutPos() {
    if(!bMoving){
        debug("movePaperCutPos", String("begin"));
        digitalWrite(PAPER_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.setMaxSpeed(PAPER_OUT_SPEED);
        stepper.setAcceleration(PAPER_OUT_ACCEL);
        stepper.moveTo(NB_STEP_PAPER_CUT);
        bMoving = true;
    }
    if (stepper.distanceToGo() != 0) {
        stepper.run();
    } else {
        digitalWrite(PAPER_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}

bool PaperHandler::isEndMove(){
    return bEndMove;
}

void PaperHandler::resetMove(){
    bEndMove = false;
    bMoving = false;
}