#ifndef lcd_h
#define lcd_h

#include <Wire.h>
#include <menu.h>
#include <menuIO/liquidCrystalOut.h>
#include <TimerThree.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/keyIn.h>
#include <menuIO/chainStream.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include "constants.h"
#include "parameters.h"

void initLCD();
void checkMenu();
void idleOnLCD();
void idleOffLCD();
void printStartup(String msg);
#endif