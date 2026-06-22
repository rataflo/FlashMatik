#ifndef SCISSOR_H
#define SCISSOR_H

#include <digitalWriteFast.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "constants.h"
#include "tests.h"

class Scissor {
public:
    Scissor();
    void init();
    void cutPaper();
    void close();
    bool isEndMove();
    void resetMove();

private:
    TMC2208Stepper tmc;
    AccelStepper stepper;
    bool bMoving;
    bool bEndMove;
};

extern Scissor scissor; // Declare the global instance of Parameters

#endif
