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

    if (params.checkCode < 6) {
        // C6: spider 7 bras - nouvelles etapes rotation mesurees au banc
        // (tests/src/test_rotation.cpp : T=4940 pas/tour, S/2=353 pas/position)
        params.checkCode = 6;
        params.rotStepPair = X_ROTATE_PAIR;
        params.rotStepImpair = X_ROTATE_IMPAIR;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 7) {
        // C7: axe scissor passe de 256 µsteps (MRES=0) a 16 µsteps (MRES=4).
        // Stroke et dynamique rescalees : 240 @256µ = 15 µsteps @16µ.
        params.checkCode = 7;
        params.scissorSpeed = SCISSOR_SPEED;
        params.scissorAccel = SCISSOR_ACCEL;
        params.scissorStepOpened = SCISSOR_STEP_OPENED;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 8) {
        // C8: retour aux PAS COMPLETS (MRES=8 force dans scissor.cpp), comme l'ancienne
        // version fonctionnelle. 240 pas complets = passage complet (1.2 tour moteur).
        // Les valeurs C7 (16µ) etait une erreur de rescaling - le chariot a peine bouge.
        params.checkCode = 8;
        params.scissorSpeed = SCISSOR_SPEED;
        params.scissorAccel = SCISSOR_ACCEL;
        params.scissorStepOpened = SCISSOR_STEP_OPENED;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 9) {
        // C9: stroke scissor corrigee a 220 pas complets (valeur de reference machine).
        params.checkCode = 9;
        params.scissorStepOpened = SCISSOR_STEP_OPENED;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    if (params.checkCode < 10) {
        // C10: bande papier 20.0 cm au lieu de 20.4 (mesure machine): x20/20.4.
        // 5441 + 2*1824 + 3706 = 12795 pas (etais 13050 pour 20.4cm).
        params.checkCode = 10;
        params.nbStepOneShot = NB_STEP_PAPER_ONE_SHOT;
        params.nbStepPaperCut = NB_STEP_PAPER_CUT;
        params.nbStepPaperOut = NB_STEP_PAPER_OUT;
        EEPROM.writeBlock(EEPROM_ADRESS, params);
    }

    dumpParameters();
}

void ParametersHandler::dumpParameters() {
    Serial.println(F("--- PARAMETRES EEPROM ---"));
    Serial.print(F("checkCode: ")); Serial.println(params.checkCode);
    Serial.print(F("totStrip: ")); Serial.println(params.totStrip);
    Serial.print(F("userCount1: ")); Serial.println(params.userCount1);
    Serial.print(F("userCount2: ")); Serial.println(params.userCount2);
    Serial.print(F("tankPair: ")); Serial.println(params.tankPair);
    Serial.print(F("tankImpair: ")); Serial.println(params.tankImpair);
    Serial.print(F("driptTime: ")); Serial.println(params.driptTime);
    Serial.print(F("nbStepOneShot: ")); Serial.println(params.nbStepOneShot);
    Serial.print(F("nbStepPaperCut: ")); Serial.println(params.nbStepPaperCut);
    Serial.print(F("nbStepPaperOut: ")); Serial.println(params.nbStepPaperOut);
    Serial.print(F("deltaFirstShot: ")); Serial.println(params.deltaFirstShot);
    Serial.print(F("agitateSteps: ")); Serial.println(params.agitateSteps);
    Serial.print(F("nbStepCenterArm: ")); Serial.println(params.nbStepCenterArm);
    Serial.print(F("nbStepExit: ")); Serial.println(params.nbStepExit);
    Serial.print(F("userMode1: ")); Serial.println(params.userMode1);
    Serial.print(F("expTime: ")); Serial.println(params.expTime);
    Serial.print(F("bulbTime: ")); Serial.println(params.bulbTime);
    Serial.print(F("redTime: ")); Serial.println(params.redTime);
    Serial.print(F("greenTime: ")); Serial.println(params.greenTime);
    Serial.print(F("blueTime: ")); Serial.println(params.blueTime);
    Serial.print(F("rotSpeed: ")); Serial.println(params.rotSpeed);
    Serial.print(F("rotAccel: ")); Serial.println(params.rotAccel);
    Serial.print(F("rotStepPair: ")); Serial.println(params.rotStepPair);
    Serial.print(F("rotStepImpair: ")); Serial.println(params.rotStepImpair);
    Serial.print(F("manSpeed: ")); Serial.println(params.manSpeed);
    Serial.print(F("manAccel: ")); Serial.println(params.manAccel);
    Serial.print(F("ySpeed: ")); Serial.println(params.ySpeed);
    Serial.print(F("yAccel: ")); Serial.println(params.yAccel);
    Serial.print(F("initSpeed: ")); Serial.println(params.initSpeed);
    Serial.print(F("initAccel: ")); Serial.println(params.initAccel);
    Serial.print(F("yDownSpeed: ")); Serial.println(params.yDownSpeed);
    Serial.print(F("yDownAccel: ")); Serial.println(params.yDownAccel);
    Serial.print(F("yAgitateSpeed: ")); Serial.println(params.yAgitateSpeed);
    Serial.print(F("yAgitateAccel: ")); Serial.println(params.yAgitateAccel);
    Serial.print(F("yDistance: ")); Serial.println(params.yDistance);
    Serial.print(F("yPairDistance: ")); Serial.println(params.yPairDistance);
    Serial.print(F("yImpairDistance: ")); Serial.println(params.yImpairDistance);
    Serial.print(F("yExitDistance: ")); Serial.println(params.yExitDistance);
    Serial.print(F("shutterSpeed: ")); Serial.println(params.shutterSpeed);
    Serial.print(F("shutterAccel: ")); Serial.println(params.shutterAccel);
    Serial.print(F("shutterStepRev: ")); Serial.println(params.shutterStepRev);
    Serial.print(F("flashTime: ")); Serial.println(params.flashTime);
    Serial.print(F("paperSpeed: ")); Serial.println(params.paperSpeed);
    Serial.print(F("paperAccel: ")); Serial.println(params.paperAccel);
    Serial.print(F("paperOutSpeed: ")); Serial.println(params.paperOutSpeed);
    Serial.print(F("paperOutAccel: ")); Serial.println(params.paperOutAccel);
    Serial.print(F("scissorSpeed: ")); Serial.println(params.scissorSpeed);
    Serial.print(F("scissorAccel: ")); Serial.println(params.scissorAccel);
    Serial.print(F("scissorStepOpened: ")); Serial.println(params.scissorStepOpened);
    Serial.print(F("servoPosIdle: ")); Serial.println(params.servoPosIdle);
    Serial.print(F("servoPosOpenBegin: ")); Serial.println(params.servoPosOpenBegin);
    Serial.print(F("servoPosOpenEnd: ")); Serial.println(params.servoPosOpenEnd);
    Serial.print(F("servoPosCloseBegin: ")); Serial.println(params.servoPosCloseBegin);
    Serial.print(F("servoPosCloseEnd: ")); Serial.println(params.servoPosCloseEnd);
    Serial.print(F("servoTime: ")); Serial.println(params.servoTime);
    Serial.print(F("bflashOn: ")); Serial.println(params.bflashOn);
    Serial.print(F("bDefineEachShot: ")); Serial.println(params.bDefineEachShot);
    Serial.print(F("nbExp: ")); Serial.println(params.nbExp);

    Serial.println(F("--- FIN PARAMETRES ---"));
    // Attendre que le dernier octet soit VRAIMENT émis sur l'UART0.
    // Une TX UART0 encore en cours perturbe l'init suivante (driver Y mort).
    // delay(200) marchait aussi mais avec ~15 ms de marge ; TXC0 est exact.
    while (!(UCSR0A & (1 << TXC0))) { }
}

void ParametersHandler::updateParameters() {
    EEPROM.updateBlock(EEPROM_ADRESS, params);
}
