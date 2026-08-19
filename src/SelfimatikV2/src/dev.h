#ifndef DEV_H
#define DEV_H

#include <digitalWriteFast.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>
#include "constants.h"
#include "parameters.h"
#include "lcd.h"
#include "tests.h"
#include "shutter.h"

class Dev {
public:
    Dev();
    void init();
    void process();
    void manageDryer();
    void paperDelivered();
    void steppersTask();
    void openCarrier();
    void closeCarrier();
    void rotate(bool waitForPaper);
    void rotateExit();
    bool isRotMoving();
    bool isRotEndMove();
    bool isYMoving();
    bool isYEndMove();
    void resetMove();
    void down(long nbSteps, bool bManual = false);
    void up(bool bActivateExit, long nbSteps, bool bManual = false);
    void agitate(bool bUp);
    void drip();
    unsigned long getAgitateStart();
    void setAgitateStart(unsigned long start);
    bool isPair();
    bool isDevFinished();
    bool servoFinished();
    bool exitNeeded();
    bool isDripFinished();
    bool secondExposureNeeded();

    struct carrier {
        bool bOpen;
        byte tankPos;    // Actual (0-11)
    };
    carrier carriers[6];
private:
    void initRot();
    void initY();
    void enableY();
    void disableY();
    int fastAnalogReadMega(uint8_t pin);
    Servo servoArm;
    TMC2208Stepper tmcY;
    TMC2208Stepper tmcRot;
    AccelStepper stepperY;
    AccelStepper stepperRot;
    unsigned long startDryer = 0;
    unsigned long agitateStart = 0;
    unsigned long servoStart = 0;
    unsigned long dripStart = 0;
    bool bRotMoving;
    bool bRotEndMove;
    bool bYMoving;
    bool bYEndMove;
    bool bPair;
    bool bDripEnd;
    bool optoFound;

};

extern Dev dev; // Declare the global instance of PaperHandler

#endif