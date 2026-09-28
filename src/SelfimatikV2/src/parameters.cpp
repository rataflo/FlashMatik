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

    if( params.checkCode < 4) {
        params.checkCode = 4;
        params.nbStepCenterArm = NB_STEP_CENTER_ARM;
        params.nbStepExit = NB_STEP_ROT_EXIT;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if(params.checkCode < 5) {
        // C0: nouveaux champs calibration, valeurs par defaut = anciennes macros en dur
        params.checkCode = 5;
        params.rotSpeed = ROT_SPEED;
        params.rotAccel = ROT_ACCEL;
        params.rotStepPair = X_ROTATE_PAIR;
        params.rotStepImpair = X_ROTATE_IMPAIR;
        params.manSpeed = 500;
        params.manAccel = 100;
        params.ySpeed = Y_SPEED;
        params.yAccel = Y_ACCEL;
        params.initSpeed = INIT_SPEED;
        params.initAccel = INIT_ACCEL;
        params.yDownSpeed = Y_DOWN_SPEED;
        params.yDownAccel = Y_DOWN_ACCEL;
        params.yAgitateSpeed = Y_AGITATE_SPEED;
        params.yAgitateAccel = Y_AGITATE_ACCEL;
        params.yDistance = Y_DISTANCE;
        params.yPairDistance = Y_PAIR_DISTANCE;
        params.yImpairDistance = Y_IMPAIR_DISTANCE;
        params.yExitDistance = Y_EXIT_DISTANCE;
        params.shutterSpeed = SHUTTER_SPEED;
        params.shutterAccel = SHUTTER_ACCEL;
        params.shutterStepRev = SHUTTER_STEP_REVOL;
        params.flashTime = 20;
        params.paperSpeed = PAPER_SPEED;
        params.paperAccel = PAPER_ACCEL;
        params.paperOutSpeed = PAPER_OUT_SPEED;
        params.paperOutAccel = PAPER_OUT_ACCEL;
        params.scissorSpeed = SCISSOR_SPEED;
        params.scissorAccel = SCISSOR_ACCEL;
        params.scissorStepOpened = SCISSOR_STEP_OPENED;
        params.servoPosIdle = SERVO_POS_IDLE;
        params.servoPosOpenBegin = SERVO_POS_OPEN_BEGIN;
        params.servoPosOpenEnd = SERVO_POS_OPEN_END;
        params.servoPosCloseBegin = SERVO_POS_CLOSE_BEGIN;
        params.servoPosCloseEnd = SERVO_POS_CLOSE_END;
        params.servoTime = SERVO_TIME;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }
}

void ParametersHandler::updateParameters() {
    EEPROM.updateBlock(EEPROM_ADRESS, params);
}
