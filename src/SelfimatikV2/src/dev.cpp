#include "dev.h"

Dev dev;

Dev::Dev()
    : startDryer(0),
    tmcY(Y_RX, Y_TX, R_SENSE),
    tmcRot(ROT_RX, ROT_TX, R_SENSE),
    stepperRot(1, ROT_PIN_STP, ROT_PIN_DIR),
    stepperY(1, Y_PIN_STP, Y_PIN_DIR){  
    agitateStart = 0;
    servoStart = 0;
    dripStart = 0;
    for(int i = 0; i < 6; i++) {
        carriers[i].bOpen = false;
        carriers[i].tankPos = i * 2;
    }
    optoFound= false;
}

void Dev::init() {
    pinMode(DRYER_PIN, OUTPUT);
    pinMode(EXIT_PIN, OUTPUT);
    servoArm.attach(SERVO_PIN);
    servoArm.write(SERVO_POS_IDLE);

    pinModeFast(OPTO_PIN, INPUT);

    pinModeFast(Y_ENDSTOP_PIN, INPUT);
    pinMode(Y_PIN_ENABLE, OUTPUT);
    pinMode(Y_PIN_STP, OUTPUT);
    pinMode(Y_PIN_DIR, OUTPUT);
    pinMode(Y_RX, INPUT);
    pinMode(Y_TX, OUTPUT);
    pinMode(Y_BRAKE_PIN, OUTPUT);

    pinMode(ROT_PIN_ENABLE, OUTPUT);
    pinMode(ROT_PIN_STP, OUTPUT);
    pinMode(ROT_PIN_DIR, OUTPUT);
    pinMode(ROT_RX, INPUT);
    pinMode(ROT_TX, OUTPUT);

    // Setup TMC
    digitalWrite(Y_PIN_ENABLE, LOW);
    digitalWrite(Y_BRAKE_PIN, HIGH);
    delay(100);
    tmcY.begin();
    tmcY.toff(5);                 // Enables driver in software
    tmcY.rms_current(1200);        // Set motor RMS current avant 1500.
    tmcY.microsteps(0);          // Set microsteps to half step
    tmcY.pwm_autoscale(true);
    tmcY.en_spreadCycle(true);   // SpreadCycle pour meilleures performances vitesse
    tmcY.tbl(1);                 // Blank time (1-2 pour vitesse)
    tmcY.hend(2);                // Hysteresis end (0-7, plus bas = plus rapide)
    tmcY.hstrt(3);               // Hysteresis start (0-7)

    digitalWrite(ROT_PIN_ENABLE, LOW);
    delay(100);
    tmcRot.begin();
    tmcRot.toff(5);                 // Enables driver in software
    tmcRot.rms_current(800);        // Set motor RMS current
    tmcRot.microsteps(4);          // Set microsteps to 1/16th
    tmcRot.pwm_autoscale(true);
    tmcRot.en_spreadCycle(false);   // SpreadCycle pour meilleures performances vitesse
    tmcRot.tbl(2);                 // Blank time (1-2 pour vitesse)
    tmcRot.hend(4);                // Hysteresis end (0-7, plus bas = plus rapide)
    //tmcRot.hstrt(3);               // Hysteresis start (0-7)
    digitalWrite(ROT_PIN_ENABLE, HIGH);

    stepperRot.setCurrentPosition(0);
    stepperRot.setMaxSpeed(ROT_SPEED);
    stepperRot.setAcceleration(ROT_ACCEL);
    stepperY.setCurrentPosition(0);
    

    printStartup("init Y");
    initY();
    printStartup("init Rot");
    initRot();
    
    delay(1000);
    servoArm.detach();
}

void Dev::initY() {
    debug("initY", String("debut"));
    enableY();
    stepperY.setCurrentPosition(0);
    stepperY.setMaxSpeed(INIT_SPEED);
    stepperY.setAcceleration(INIT_ACCEL);
    stepperY.moveTo(Y_DISTANCE+50);

    while (digitalReadFast(Y_ENDSTOP_PIN)) {
        stepperY.run();
    }
    stepperY.stop();
    stepperY.run();
    Serial.println(stepperY.currentPosition());
    stepperY.setCurrentPosition(0);
    stepperY.setMaxSpeed(Y_SPEED);
    stepperY.setAcceleration(Y_ACCEL);
    disableY();
    resetMove();
}

void Dev::initRot() {
    debug("initRot", String("debut"));
    
    // Activation du moteur
    digitalWriteFast(ROT_PIN_ENABLE, LOW);
    stepperRot.setCurrentPosition(0);
    stepperRot.moveTo(X_ROTATE_IMPAIR * 2); // Lance la course
    
    bool optoFound = digitalReadFast(OPTO_PIN);
    while (!optoFound && stepperRot.distanceToGo() != 0) {
        stepperRot.run();
        optoFound = digitalReadFast(OPTO_PIN);
    }
    if(!optoFound){
        shutter.emergencyShutdown(1);
    }
    stepperRot.stop();
    stepperRot.run();
    stepperRot.setCurrentPosition(0);
    stepperRot.moveTo(NB_STEP_CENTER_ARM);
    while (stepperRot.distanceToGo() != 0) {
        stepperRot.run();
    }
    digitalWriteFast(ROT_PIN_ENABLE, HIGH);
    stepperRot.setCurrentPosition(0);
    
    // Finalisation
    bPair = true;
    resetMove();
}



void Dev::rotate(bool waitForPaper){
    if(!bRotMoving){
        debug("rotate", String("begin"));
        digitalWrite(ROT_PIN_ENABLE, LOW);
        stepperRot.setCurrentPosition(0);
        stepperRot.moveTo(bPair ? X_ROTATE_PAIR : X_ROTATE_IMPAIR);
        bRotMoving = true;
        // check if arm need to be opened or closed.
        if(!bPair){
            for(int i = 0; i < 6; i++){
                if(carriers[i].tankPos == 11){
                    if(!waitForPaper && carriers[i].bOpen == 1){
                        servoArm.attach(SERVO_PIN);
                        servoArm.write(SERVO_POS_CLOSE_BEGIN);
                    } else if(waitForPaper && carriers[i].bOpen == 0){
                        servoArm.attach(SERVO_PIN);
                        servoArm.write(SERVO_POS_OPEN_BEGIN);
                    }
                }
            }
        }

    }

    if(stepperRot.distanceToGo() != 0){
        if (!bPair && !optoFound) {
            optoFound = digitalReadFast(OPTO_PIN);
            if(!optoFound){
                stepperRot.run();
            }else{
                stepperRot.stop();
                stepperRot.run();
                stepperRot.setCurrentPosition(0); 
                stepperRot.moveTo(NB_STEP_CENTER_ARM);
            }
        } else {
            stepperRot.run();
        }
    }else{
        if(!bPair && !optoFound){
            shutter.emergencyShutdown(1);
        }
        digitalWrite(ROT_PIN_ENABLE, HIGH);
        stepperRot.setCurrentPosition(0);
        bPair = !bPair;
        for(int i = 0; i < 6; i++){
            carriers[i].tankPos = carriers[i].tankPos + 1 > 11 ? 0 : carriers[i].tankPos + 1;
        }

        // close or open arm
        if(bPair){
            for(int i = 0; i < 6; i++){
                if(carriers[i].tankPos == 0){
                    if(!waitForPaper && carriers[i].bOpen == 1){
                        closeCarrier();
                    } else if(waitForPaper && carriers[i].bOpen == 0){
                        openCarrier();
                    }
                }
            }
        }

        bRotMoving = false;
        bRotEndMove = true;
        optoFound = false;
    }
}

void Dev::rotateExit(){
    if(!bRotMoving){
        debug("rotateExit", String("begin"));
        digitalWrite(ROT_PIN_ENABLE, LOW);
        stepperRot.setCurrentPosition(0);
        stepperRot.moveTo(NB_STEP_ROT_EXIT);
        bRotMoving = true;
    }

    if (stepperRot.distanceToGo() != 0) {
        Serial.println(stepperRot.distanceToGo());
        stepperRot.run();
    }else{
        stepperRot.setCurrentPosition(0);
        digitalWrite(ROT_PIN_ENABLE, HIGH);
        bRotMoving = false;
        bRotEndMove = true;
    }
}

void Dev::down(long nbSteps) {
    if(servoStart != 0){
        return;
    }
    if(!bYMoving){
        debug("down", String("begin"));
        enableY();
        stepperY.setCurrentPosition(0);
        stepperY.setMaxSpeed(Y_DOWN_SPEED);
        stepperY.setAcceleration(Y_DOWN_ACCEL);
        stepperY.moveTo(nbSteps);
        //stepperY.setSpeed(-Y_SPEED);
        bYMoving = true;
    }
    if (stepperY.distanceToGo() != 0) {
        stepperY.run();
        // put servo in idle pos when Y is in the middle
        if(stepperY.distanceToGo() == nbSteps / 2){
            servoArm.attach(SERVO_PIN);
            servoArm.write(SERVO_POS_IDLE);
        }
    } else {
        stepperY.setCurrentPosition(0);
        debug("down", String("end"));
        servoArm.detach();
        bYMoving = false;
        bYEndMove = true;
    }
}

void Dev::up(bool bActivateExit, long nbSteps) {

    if(!bYMoving){
        debug("up", String("begin"));
        Serial.println(nbSteps);
        enableY();
        stepperY.setCurrentPosition(0);
        stepperY.setMaxSpeed(Y_SPEED);
        stepperY.setAcceleration(Y_ACCEL);
        stepperY.moveTo(nbSteps);
        bYMoving = true;

        if(bActivateExit){
            digitalWriteFast(EXIT_PIN, HIGH);
        }
    }

    if (!digitalReadFast(Y_ENDSTOP_PIN) || stepperY.distanceToGo() == 0) {
        stepperY.stop();
        stepperY.run();
        stepperY.setCurrentPosition(0);
        if(bActivateExit){
            digitalWriteFast(EXIT_PIN, LOW);
        }
        disableY();
        bYMoving = false;
        bYEndMove = true;
        debug("up", String("end"));
    }else if(stepperY.distanceToGo() != 0){
        stepperY.run();
    }
}

void Dev::agitate(bool bUp) {

    if(!bYMoving){
        //debug("agitate", String("begin"));
        //enableY();
        stepperY.setCurrentPosition(0);
        stepperY.setMaxSpeed(Y_AGITATE_SPEED);
        stepperY.setAcceleration(Y_AGITATE_ACCEL);
        if(bUp){
            stepperY.moveTo(Y_AGITATE_STEPS);
        }else{
            stepperY.moveTo(-Y_AGITATE_STEPS);
        }
        bYMoving = true;
    }
    if (stepperY.distanceToGo() != 0) {
        stepperY.run();
    } else {
        stepperY.setCurrentPosition(0);
        bYMoving = false;
        bYEndMove = true;
    }
}

void Dev::drip()
{
    if(dripStart == 0){
        dripStart = millis();
    }
}

void Dev::openCarrier(){
    debug("openCarrier", String("begin"));
    digitalWrite(ROT_PIN_ENABLE, LOW);
    servoStart = millis();
    servoArm.attach(SERVO_PIN);
    servoArm.write(SERVO_POS_OPEN_END);
     for(int i = 0; i < 6; i++){
        if(carriers[i].tankPos == 0){
            carriers[i].bOpen = 1;
        }
    }
}

void Dev::closeCarrier(){
    debug("closeCarrier", String("begin"));
    digitalWrite(ROT_PIN_ENABLE, LOW);
    servoStart = millis();
    servoArm.attach(SERVO_PIN);
    servoArm.write(SERVO_POS_CLOSE_END);
     for(int i = 0; i < 6; i++){
        if(carriers[i].tankPos == 0){
            carriers[i].bOpen = 0;
        }
    }
}

void Dev::manageDryer() {
    unsigned long currentMillis = millis();
    if(startDryer != 0 && currentMillis - startDryer > DRYER_TIME){
        digitalWriteFast(DRYER_PIN, HIGH);
        startDryer = 0;
    }
}

void Dev::paperDelivered(){
    for(int i = 0; i < 6; i++){
        if(carriers[i].tankPos == 0){
            carriers[i].bOpen = 1;
        }
    }
}

void Dev::enableY() {
    digitalWrite(Y_PIN_ENABLE, LOW);
    digitalWrite(Y_BRAKE_PIN, HIGH);
    digitalWrite(ROT_PIN_ENABLE, LOW);
    
}

void Dev::disableY() {
    digitalWrite(Y_BRAKE_PIN, LOW);
    digitalWrite(Y_PIN_ENABLE, HIGH);
    digitalWrite(ROT_PIN_ENABLE, HIGH);
}

bool Dev::isRotMoving(){
    return bRotMoving;
}

bool Dev::isRotEndMove(){
    return bRotEndMove;
}

bool Dev::isYMoving(){
    return bYMoving;
}

bool Dev::isYEndMove(){
    return bYEndMove;
}

void Dev::resetMove(){
    bRotEndMove = false;
    bRotMoving = false;
    bYEndMove = false;
    bYMoving = false;
}

unsigned long Dev::getAgitateStart(){
    return agitateStart;
}

void Dev::setAgitateStart(unsigned long start){
    agitateStart = start;
}   

bool Dev::isPair(){
    return bPair;
}

bool Dev::isDevFinished(){
    for(int i = 0; i < 6; i++){
        if(carriers[i].bOpen == 1){
            return false;
        }
    }
    return true;
}

bool Dev::exitNeeded(){
    for(int i = 0; i < 6; i++){
        if(carriers[i].tankPos == 11 && carriers[i].bOpen == 1){
            return true;
        }
    }
    return false;
}

bool Dev::secondExposureNeeded(){
    for(int i = 0; i < 6; i++){
        if(carriers[i].tankPos == 2 && carriers[i].bOpen == 1){
            return true;
        }
    }
    return false;
}


bool Dev::servoFinished(){
    unsigned long currentMillis = millis();
    if(servoStart != 0 && currentMillis - servoStart > SERVO_TIME){
        servoStart = 0;
        servoArm.detach();
        digitalWrite(ROT_PIN_ENABLE, HIGH);
        return true;
    } else if(servoStart == 0){
        digitalWrite(ROT_PIN_ENABLE, HIGH);
        return true;
    }
    return false;
}

bool Dev::isDripFinished(){
    unsigned long currentMillis = millis();
    if(dripStart == 0){
        dripStart = currentMillis;
        return false;
    }else if(dripStart != 0 && currentMillis - dripStart > parameters.params.driptTime * 1000){
        dripStart = 0;
        return true;
    }
    return false;
}

int Dev::fastAnalogReadMega(uint8_t pin) {
    if (pin < A0 || pin > A15) return 0;  // Vérification de la broche

    uint8_t analog_pin = pin - A0;       // Conversion en numéro interne (0-15)

    // Réinitialisation des registres ADC pour éviter des conflits
    ADMUX  = (1 << REFS0);               // Référence AVcc (5V)
    ADCSRB = (analog_pin & 0x08) ? (1 << MUX5) : 0;  // Gestion de MUX5 pour A8-A15
    ADMUX  |= (analog_pin & 0x07);       // Configuration MUX3:0

    // Configuration du prescaler (division par 128 pour une conversion stable)
    ADCSRA = (1 << ADEN)  |  // Activation ADC
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler 128

    ADCSRA |= (1 << ADSC);    // Lancement conversion
    while (ADCSRA & (1 << ADSC));  // Attente fin conversion

    return ADC;  // Retourne la valeur (équivalent à ADCL | (ADCH << 8))
}