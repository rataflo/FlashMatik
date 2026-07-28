#include "shutter.h"

// Static variables
const uint8_t Shutter::IMAGES[][8] = {
    // Chiffre 0
    {0x00, 0x64, 0x62, 0x02, 0x02, 0x62, 0x64, 0x00},
    // Chiffre 1
    {0x00, 0x01, 0x01, 0x7F, 0x7F, 0x11, 0x01, 0x00},
    // Chiffre 2
    {0x00, 0x31, 0x79, 0x49, 0x45, 0x67, 0x23, 0x00},
    // Chiffre 3
    {0x00, 0x36, 0x7F, 0x49, 0x49, 0x63, 0x22, 0x00},
    // Chiffre 4
    {0x00, 0x04, 0x7F, 0x7F, 0x24, 0x14, 0x0C, 0x00},
    // Chiffre 5
    {0x00, 0x4E, 0x5F, 0x51, 0x51, 0x73, 0x72, 0x00}
};

const uint8_t Shutter::ARROWDOWN[8] = {
    0x08, 0x0C, 0xFE, 0xFF, 0xFE, 0x0C, 0x08, 0x00
};

const uint8_t Shutter::CROSS[8] = {
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

const uint8_t Shutter::ERROR[8] = {
    0x00, 0x61, 0x62, 0x02,
    0x02, 0x62, 0x61, 0x00
};

const int Shutter::IMAGES_LEN = sizeof(IMAGES) / 8;

// Define the global instance of Shutter
Shutter shutter;

uint8_t buffer[8];  // Stocke les 8 lignes de la matrice

Shutter::Shutter()
    : tmc(SHUTTER_RX, SHUTTER_TX, R_SENSE),
      stepper(1, SHUTTER_PIN_STP, SHUTTER_PIN_DIR),
      bCloseShutter(false),
      bMoving(false),
      bEndMove(false),
      flashTriggered(false),
      flashStartMillis(0),
      ledMatrix(MD_MAX72XX::GENERIC_HW, LED_MATRIX_CS_PIN, 1),
      prevousMillisCountdown(0),
      countDown(0) {}

void Shutter::init() {
    debug("initShutter", String("debut"));
    
    pinMode(LED_MATRIX_CS_PIN, OUTPUT);
    initLedMatrix();

    pinMode(SHUTTER_PIN_ENABLE, OUTPUT);
    pinModeFast(SHUTTER_ENDSTOP_PIN, INPUT);
    pinMode(SHUTTER_PIN_STP, OUTPUT);
    pinMode(SHUTTER_PIN_DIR, OUTPUT);
    pinMode(SHUTTER_RX, INPUT);
    pinMode(SHUTTER_TX, OUTPUT);
    

    digitalWrite(SHUTTER_PIN_ENABLE, LOW);
    stepper.setMaxSpeed(SHUTTER_SPEED);
    stepper.setAcceleration(SHUTTER_ACCEL);

    delay(100);
    tmc.begin();
    tmc.toff(5);                 // Enables driver in software
    tmc.rms_current(1600);        // Set motor RMS current
    tmc.microsteps(2);          // Set microsteps to 1/16th
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(true);   // SpreadCycle pour meilleures performances vitesse
    tmc.hend(2);                // Hysteresis end (0-7, plus bas = plus rapide)
    tmc.hstrt(3);               // Hysteresis start (0-7)

    int homing = 0;
    while (digitalReadFast(SHUTTER_ENDSTOP_PIN)) {
        stepper.moveTo(homing);
        stepper.run();
        homing++;
        delay(5);
    }
    //Serial.println(homing);
    stepper.stop();
    stepper.setCurrentPosition(0);
    stepper.run();
    
    bCloseShutter = true;
    numFrame = 0;
    digitalWrite(SHUTTER_PIN_ENABLE, HIGH);
}

void Shutter::test() {
    debug("takeShot", String("begin"));
    digitalWrite(SHUTTER_PIN_ENABLE, LOW);
    stepper.setMaxSpeed(SHUTTER_SPEED);
    stepper.setAcceleration(SHUTTER_ACCEL);
    stepper.setCurrentPosition(0);
    stepper.moveTo(100);

    while (stepper.currentPosition() > SHUTTER_STEP_REVOL / 2) {
        stepper.run();
    }
    stepper.stop();
    stepper.setCurrentPosition(0);
    stepper.run();
    digitalWrite(SHUTTER_PIN_ENABLE, HIGH);
    debug("takeShot", String("end"));
}

void Shutter::fromage() {
    if(!bMoving){
        debug("fromage", String("begin"));
        digitalWrite(SHUTTER_PIN_ENABLE, LOW);
        stepper.setCurrentPosition(0);
        stepper.moveTo(SHUTTER_STEP_REVOL);
        bMoving = true;
    }
    if (stepper.distanceToGo() != 0) {
        stepper.run();
        //TODO: geestion du flash
        if(stepper.currentPosition() == SHUTTER_STEP_REVOL / 2){
            flashStartMillis = millis();
            digitalWriteFast(FLASH_PIN, HIGH);
        }
        unsigned long currentMillis = millis();
        if(currentMillis - flashStartMillis >= 20){
            if(!flashTriggered){
                digitalWriteFast(FLASH_PIN, LOW);
            }
        }
        // Check microswitch
        if(stepper.distanceToGo() < 50 && !digitalReadFast(SHUTTER_ENDSTOP_PIN)){
             debug("shutsteps", String(stepper.currentPosition()));
            stepper.stop();
            stepper.setCurrentPosition(0);
            stepper.run();
        }
        
    } else {
        digitalWrite(SHUTTER_PIN_ENABLE, HIGH);
        bMoving = false;
        bEndMove = true;
    }
}

// LED Matrix methods
void Shutter::initLedMatrix() {
    SPI.begin();
    
    // CONFIGURATION SPI MAXIMUM VITESSE :
    SPI.setClockDivider(SPI_CLOCK_DIV2);  // 8 MHz sur Mega 16MHz
    SPI.setDataMode(SPI_MODE0);           // Mode le plus rapide
    SPI.setBitOrder(MSBFIRST);            // Ordre MSB

    ledMatrix.begin();
    ledMatrix.control(MD_MAX72XX::INTENSITY, 1);
    ledMatrix.control(MD_MAX72XX::SCANLIMIT, 7);  // Toutes les lignes
    ledMatrix.control(MD_MAX72XX::DECODE, MD_MAX72XX::OFF); // Désactive BCD
    //ledMatrix.transform(MD_MAX72XX::TFLR); // Flip Left to Right
    ledMatrix.clear();
}

/*void Shutter::displayNumber(byte numero) {
    for (int row = 7; row >= 0; row--) {
        byte rowData = IMAGES[numero][7-row];
        for (int col = 0; col < 8; col++) {
            bool pixelOn = (rowData >> (7 - col)) & 0x01;
            ledMatrix.setPoint(row, col, pixelOn);
        }
    }
}*/

void Shutter::displayNumber(byte numero) {
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // Désactive les mises à jour
    for (byte row = 0; row < 8; row++) {
        ledMatrix.setRow(row, IMAGES[numero][row]);
    }
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);   // Rafraîchit la matrice
}

void Shutter::showCountdown() {
    if(countDown == 0){
        countDown = 5;
        displayNumber(countDown);
        prevousMillisCountdown = millis();
    }else{
        unsigned long currentMillis = millis();
        if (currentMillis - prevousMillisCountdown >= 1000) {
            prevousMillisCountdown = currentMillis;
            countDown = countDown > 0 ? countDown - 1 : 0;
            displayNumber(countDown);
        }
    }
    
}

void Shutter::showArrowDown() {
    /*for (int row = 0; row < 8; row++) {
        byte rowData = ARROWDOWN[row];
        for (int col = 0; col < 8; col++) {
            bool pixelOn = (rowData >> (7 - col)) & 0x01;
            ledMatrix.setPoint(row, col, pixelOn);
        }
    }*/

    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // Désactive les mises à jour
    for (byte row = 0; row < 8; row++) {
        ledMatrix.setRow(row, ARROWDOWN[row]);
    }
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);  // Rafraîchit la matrice
}

void Shutter::showCross() {
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // Désactive les mises à jour
    for (byte row = 0; row < 8; row++) {
        ledMatrix.setRow(row, CROSS[row]);
    }
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);   // Rafraîchit la matrice
}

void Shutter::showError() {
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // Désactive les mises à jour
    for (byte row = 0; row < 8; row++) {
        ledMatrix.setRow(row, ERROR[row]);
    }
    ledMatrix.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);   // Rafraîchit la matrice
}

void Shutter::clearLedMatrix() {
    ledMatrix.clear();
}

byte Shutter::getCountDown() {
    return countDown;
}

byte Shutter::getNumFrame() {
    return numFrame;
}

void Shutter::setNumFrame(byte num) {
    numFrame = num;
}

bool Shutter::isEndMove() {
    return bEndMove;
}

void Shutter::resetMove() {
    bEndMove = false;
    bMoving = false;
}

void Shutter::emergencyShutdown(byte errorCode){
    Serial.println("ERROR " + String(errorCode));
    // turn off all motors and servos
    digitalWrite(ROT_PIN_ENABLE, HIGH);
    digitalWrite(Y_PIN_ENABLE, HIGH);
    digitalWrite(Y_BRAKE_PIN, LOW);
    showError();
    while(1){
        // infinite loop to stop the program
    }
}