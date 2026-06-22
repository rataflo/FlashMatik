#ifndef SHUTTER_H
#define SHUTTER_H

#include <digitalWriteFast.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "constants.h"
#include "tests.h"
#include "parameters.h"

class Shutter {
public:
    Shutter();
    void init();
    void takeShot();
    void test();
    void fromage();

    // LED Matrix methods
    void initLedMatrix();
    void displayNumber(byte numero);
    void showCountdown();
    void showArrowDown();
    void showCross();
    void showError();
    void clearLedMatrix();
    byte getCountDown();
    byte getNumFrame();
    void setNumFrame(byte num);
    void setLedMatrixIntensity(uint8_t intensity);

    bool isEndMove();
    void resetMove();
    void emergencyShutdown(byte errorCode);

private:
    TMC2208Stepper tmc;
    AccelStepper stepper;
    bool bCloseShutter;
    bool bMoving = false;
    bool bEndMove = false;
    bool flashTriggered = false;
    unsigned long flashStartMillis = 0;
    //MD_MAX72XX ledMatrix = MD_MAX72XX(MD_MAX72XX::FC16_HW, LED_MATRIX_SDI_PIN, LED_MATRIX_SCL_PIN, LED_MATRIX_CS_PIN, 1);
    MD_MAX72XX ledMatrix;
    unsigned long prevousMillisCountdown;
    byte countDown;
    byte numFrame;

    // LED Matrix images
    static const byte IMAGES[][8];
    static const byte ARROWDOWN[8];
    static const byte CROSS[8];
    static const byte ERROR[8];
    static const int IMAGES_LEN;
};

extern Shutter shutter;

#endif
