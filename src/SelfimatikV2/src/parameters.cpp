#include "parameters.h"

ParametersHandler parameters;

storage ParametersHandler::getParameters() {
    return params;
}

void ParametersHandler::loadParameters() {
    EEPROM.readBlock(EEPROM_ADRESS, params);
    // Check verif code, if not correct init eeprom.
    //params.checkCode=0;
    if (params.checkCode < 1) {
        params.checkCode = 1;
        params.totStrip = 0;
        params.nbStepOneShot = NB_STEP_PAPER_ONE_SHOT;
        params.nbStepPaperOut = NB_STEP_PAPER_OUT;
        params.deltaFirstShot = DELTA_FIRST_SHOT;
        params.userMode1 = false;
        params.expTime = 1;
        params.bulbTime = 1;
        params.bflashOn = true;
        params.nbExp = 1;
        params.bDefineEachShot = false;
        params.tankPair = TANK_TIME;
        params.tankImpair = TANK_TIME_IMPAIR;
        params.driptTime = DRIP_TIME;
        params.shotExpTimes[0] = 1;
        params.shotBulbTimes[0] = 1;
        params.shotFlashOn[0] = true;
        params.shotNbExps[0] = 1;

        params.shotExpTimes[1] = 1;
        params.shotBulbTimes[1] = 1;
        params.shotFlashOn[1] = true;
        params.shotNbExps[1] = 1;

        params.shotExpTimes[2] = 1;
        params.shotBulbTimes[2] = 1;
        params.shotFlashOn[2] = true;
        params.shotNbExps[2] = 1;

        params.shotExpTimes[3] = 1;
        params.shotBulbTimes[3] = 1;
        params.shotFlashOn[3] = true;
        params.shotNbExps[3] = 1;

        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 2) {
        params.checkCode = 2;
        params.redTime = RED_TIME;
        params.greenTime = GREEN_TIME;
        params.blueTime = BLUE_TIME;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 3) {
        params.checkCode = 3;
        params.nbStepOneShot = NB_STEP_PAPER_ONE_SHOT;
        params.nbStepPaperOut = NB_STEP_PAPER_OUT;
        params.deltaFirstShot = DELTA_FIRST_SHOT;
        params.nbStepPaperCut = NB_STEP_PAPER_CUT;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }
}

void ParametersHandler::updateParameters() {
    EEPROM.updateBlock(EEPROM_ADRESS, params);
}
