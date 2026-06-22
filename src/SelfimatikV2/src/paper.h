#ifndef PAPER_H
#define PAPER_H

#include <digitalWriteFast.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "constants.h"
#include "parameters.h"
#include "tests.h"


class PaperHandler {
public:
    PaperHandler();
    void movePaperFirstShot();
    void init();
    void movePaperNextShot();
    void movePaperPreviousShot();
    void movePaperOut();
    void movePaperCutPos();
    bool isEndMove();
    void resetMove();

private:
    bool bMoving;
    bool bEndMove;
    bool bRewindStep2;
    TMC2208Stepper tmc;
    AccelStepper stepper;
};

extern PaperHandler paper; // Declare the global instance of PaperHandler

#endif
