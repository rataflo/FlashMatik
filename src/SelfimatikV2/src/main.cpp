/*
 * Selfimatik program.
 * Run on Atmega 2560. 
 * 
 * (c) Flo Gales 2023 : rataflo@free.fr
 * Licence cc-by-nc-sa : https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Libraries : 
 *  digitalWriteFast : https://github.com/NicksonYap/digitalWriteFast
 *  Steppers control: http://www.airspayce.com/mikem/arduino/AccelStepper/index.html
 *  Arduino menu library: https://github.com/neu-rah/ArduinoMenu
 *  Led matrix: https://github.com/wayoda/LedControl
 *  EEPROM management : https://github.com/thijse/Arduino-EEPROMEx
 *  Timer Three: https://github.com/PaulStoffregen/TimerThree
 */

#include <digitalWriteFast.h>
#include "constants.h"
#include "parameters.h"
#include "dev.h"
#include "scissor.h"
#include "shutter.h"
#include "paper.h"
#include "lcd.h"
#include "tests.h"
#include "Paper.h"
#include "Scissor.h"
#include <Adafruit_NeoPixel.h>


// Work variables
byte stepTakeShot = 0;
byte nbTotExp = 0;
byte numExp = 0;
byte numFrame = 0;


PhotoState photoState = IDLE_PHOTO;
DevState devState = IDLE_DEV;
DevState olDevState = IDLE_DEV;

Adafruit_NeoPixel preflashStrip(NUMPIXELS, PREFLASH_PIN, NEO_GRB + NEO_KHZ800);

unsigned long photoStartTime = 0;
unsigned long devStartTime = 0;
unsigned long preFlashStartTime = 0;

//Arduino serial communication
unsigned long lastQuery = 0;
bool doneSent = false;
bool photoRequested = false;

void manageStepsTakeShot();
void steppersTask();
void handlePhotoState();
void handleDevState();
void emergencyShutdown(byte errorCode);

const char* getDevStateName(DevState state) {
    switch(state) {
        case IDLE_DEV: return "IDLE_DEV";
        case ROTATE: return "ROTATE";
        case DOWN: return "DOWN";
        case UP: return "UP";
        case AGITATE_UP: return "AGITATE_UP";
        case AGITATE_DOWN: return "AGITATE_DOWN";
        case WAIT_PAPER: return "WAIT_PAPER";
        case DOWN_ROT: return "DOWN_ROT";
        case ROT_EXIT: return "ROT_EXIT";
        case UP_EXIT: return "UP_EXIT";
        case DRIP: return "DRIP";
        case WAIT_EXIT: return "WAIT_EXIT";
        case DOWN_FINISH: return "DOWN_FINISH";
        case UP_FINISH: return "UP_FINISH";
        case SECOND_EXPOSURE: return "SECOND_EXPOSURE";
        case MANUAL_DOWN: return "MANUAL_DOWN";
        case MANUAL_UP: return "MANUAL_UP";
        default: return "UNKNOWN";
    }
}

void setup() {
    pinModeFast(FLASH_PIN, OUTPUT);
    pinModeFast(EXIT_PIN, OUTPUT);
    pinModeFast(SECOND_EXPOSURE_PIN, OUTPUT);
    digitalWriteFast(FLASH_PIN, LOW);
    digitalWriteFast(EXIT_PIN, LOW);
    digitalWriteFast(SECOND_EXPOSURE_PIN, LOW);

    Serial.begin(115200);
    parameters.loadParameters();

    // Flash test
    //delay(3000);
    //digitalWriteFast(FLASH_PIN, HIGH);
    //delay(20);
    //digitalWriteFast(FLASH_PIN, LOW);
    
    initLCD();
    // init pre flash leds.
    preflashStrip.begin();
    preflashStrip.clear();
    //preflashStrip.setBrightness(10);
    preflashStrip.show();

    //Test pre flash
    //for(byte i=0; i<NUMPIXELS; i++){
    //    preflashStrip.setPixelColor(i, 0, 255, 0); // All LEDs red
    //}
    //preflashStrip.show();
    
    shutter.initLedMatrix();
    dev.init();

    
    printStartup("init Shutter");
    shutter.init();
    shutter.showCross();
    printStartup("init Scissor");
    scissor.init();
    printStartup("init Paper");
    paper.init();

    shutter.displayNumber(0);
    printStartup("init OK");
    debug("init","OK");

    /*#ifdef SIMUL_MODE
        photoState = COUNTDOWN;
        shutter.setNumFrame(0);
        photoRequested = false;
    #endif*/
    
    //scissor.cutPaper();
    //scissor.close();
}

void loop() {
    handlePhotoState();
    handleDevState();
    //manageStepsTakeShot();
    if(photoState == IDLE_PHOTO && devState == IDLE_DEV){
        checkMenu();
    }
    //dev.manageDryer();
}

void handlePhotoState() {
    switch (photoState) {
        case IDLE_PHOTO:
            if (!photoRequested && millis() - lastQuery > 500) {
                lastQuery = millis();
                if (Serial.available()) {
                    String reply = Serial.readStringUntil('\n');

                    if (reply == "DOWN") {
                        if (photoState == IDLE_PHOTO && devState == IDLE_DEV) {
                            Serial.println("DOWN_START");
                            devState = MANUAL_DOWN;
                        } else {
                            Serial.println("DOWN_NOT_READY");
                        }
                    }
                    else if (reply == "UP") {
                        if (photoState == IDLE_PHOTO && devState == IDLE_DEV) {
                            Serial.println("UP_START");
                            devState = MANUAL_UP;
                        } else {
                            Serial.println("UP_NOT_READY");
                        }
                    }
                    else if (reply == "START") {
                        Serial.println("DONE");
                        photoRequested = true;
                        shutter.showArrowDown();
                        #ifdef SIMUL_MODE
                            photoState = COUNTDOWN;
                            shutter.setNumFrame(0);
                            photoRequested = false;
                        #endif
                    }
                }else{
                    Serial.println("QUERY");
                }
            }
            if (photoRequested && !digitalReadFast(START_BTN_PIN)) { // Bouton de démarrage pressé
                Serial.println("Starting photo process...");
                parameters.params.totStrip = parameters.params.totStrip + 1;
                parameters.params.userCount1 = parameters.params.userCount1 + 1;
                parameters.params.userCount2 = parameters.params.userCount2 + 1;
                parameters.updateParameters();
                
                photoState = COUNTDOWN;
                shutter.setNumFrame(0);
                photoRequested = false;
            }
            break;

        case PAPER_NEXT_SHOT:
            if(!paper.isEndMove()){
                paper.movePaperNextShot();
            }else{
                paper.resetMove();
                photoState = COUNTDOWN;
            }
            
            break;

        case COUNTDOWN:
            shutter.showCountdown();
            if(shutter.getCountDown() == 0){
                photoState = PREFLASH_START;
            }
            break;

        case TAKE_PHOTO:
            shutter.fromage(); // Cheeeeeese
            if (shutter.isEndMove()) {
                if(shutter.getNumFrame() < 3){
                    shutter.setNumFrame(shutter.getNumFrame() + 1);
                    photoState = PAPER_NEXT_SHOT;
                }else{
                    shutter.showCross();
                    photoState = WAIT_DEV;
                }
                shutter.resetMove();
            }
            break;

        case PAPER_CUT:
            if(!paper.isEndMove()){
                paper.movePaperCutPos(); // Déplacer le papier pour la découpe
            }else{
                paper.resetMove();
                photoState = CUT;
            }
            break;
        
        case CUT:
            //#ifndef SIMUL_MODE
                if(!scissor.isEndMove()){
                    //digitalWriteFast(ROT_PIN_ENABLE, HIGH); //free rotation to help paper positioning
                    scissor.cutPaper(); // découper le papier
                }else{
                    scissor.resetMove();
                    photoState = CLOSE_SCISSOR;
                }
                break;
            //#endif
            /*#ifdef SIMUL_MODE
                photoState = PAPER_REWIND;
            #endif*/


        case CLOSE_SCISSOR:
            if(!scissor.isEndMove()){
                scissor.close(); // Refermer les ciseaux
            }else{
                scissor.resetMove();
                photoState = PAPER_OUT;
            }
            break;
        
        case PAPER_OUT:
            if(!paper.isEndMove()){
                paper.movePaperOut(); // Sortie du papier
            }else{
                //digitalWriteFast(ROT_PIN_ENABLE, LOW);
                paper.resetMove();
                photoState = PAPER_REWIND;
                devState = DOWN;
            }
            break;

        case PAPER_REWIND:
            if(!paper.isEndMove()){
                paper.movePaperFirstShot(); // Rembobine le papier
            }else{
                paper.resetMove();
                shutter.displayNumber(0);
                photoState = IDLE_PHOTO;
            }
            break;
        
        case PREFLASH_START:
            if(RED_TIME > 0){
                preFlashStartTime = millis();
                preflashStrip.setPixelColor(1, 8, 0, 0); // Red
                preflashStrip.setPixelColor(4, 8, 0, 0);
                preflashStrip.show();
            }
            photoState = RED;
            break;
        case RED:
            if(RED_TIME == 0 || millis() - preFlashStartTime > RED_TIME){  
                preflashStrip.setPixelColor(1, 0, 0, 0);
                preflashStrip.setPixelColor(4, 0, 0, 0);
                if(GREEN_TIME > 0){
                    preFlashStartTime = millis();
                    preflashStrip.setPixelColor(1, 0, 4, 0); // Green
                    preflashStrip.setPixelColor(4, 0, 4, 0); // Green
                    preflashStrip.show();
                    
                }
                photoState = GREEN;
            }
            break;
        case GREEN:
            if (GREEN_TIME == 0 || millis() - preFlashStartTime > GREEN_TIME) { 
                preflashStrip.setPixelColor(1, 0, 0, 0);
                preflashStrip.setPixelColor(4, 0, 0, 0);
                if(BLUE_TIME > 0){
                    preFlashStartTime = millis();
                    preflashStrip.setPixelColor(1, 0, 0, 2); // bleu
                    preflashStrip.setPixelColor(4, 0, 0, 2); // bleu
                    preflashStrip.show();
                }
                photoState = BLUE;
            }
            break;
        case BLUE:
            if (BLUE_TIME == 0 || millis() - preFlashStartTime > BLUE_TIME) {
                preflashStrip.setPixelColor(1, 0, 0, 0); 
                preflashStrip.setPixelColor(4, 0, 0, 0); // All LEDs red
                preflashStrip.clear();
                preflashStrip.show();
                photoState = TAKE_PHOTO;
            }
            break;
        case WAIT_DEV:
            break;
    }
}

void handleDevState() {

    if(devState != olDevState){
        Serial.println(getDevStateName(devState));
        olDevState = devState;
    }
    
    switch (devState) {
        case IDLE_DEV:
            if(photoState == WAIT_DEV){
                dev.openCarrier();
                devState = WAIT_PAPER;
            }
            break;
        case ROTATE:
            if(!dev.isRotEndMove()){ 
                dev.rotate(photoState == WAIT_DEV);
            }else{
                dev.resetMove();
                if(dev.isPair() && photoState == WAIT_DEV){
                    devState = WAIT_PAPER;
                }else{
                    // check si on dois continuer ou pas
                    if(dev.isDevFinished()){
                        devState = DOWN_FINISH;
                    }else{
                        devState = DOWN;
                    }
                }
            }
            break;
        case DOWN:
            if(dev.servoFinished()){
                if(!dev.isYEndMove()){
                    dev.down(dev.isPair() ? -Y_PAIR_DISTANCE : -Y_IMPAIR_DISTANCE, false);
                }else{
                    // if paper in tank 2 switch on light for second exposure.
                    if(dev.secondExposureNeeded()){
                        digitalWriteFast(SECOND_EXPOSURE_PIN, HIGH);
                    }
                    dev.resetMove();
                    devState = AGITATE_UP;
                }
            }
            break;
        case AGITATE_UP:
            if(dev.getAgitateStart() == 0){
                dev.setAgitateStart(millis());
            }

            if(!dev.isYEndMove()){
                dev.agitate(true);
            }else{
                dev.resetMove();
                devState = AGITATE_DOWN;
            }
            break;
        case AGITATE_DOWN:
            if(!dev.isYEndMove()){
                dev.agitate(false);
            }else{
                dev.resetMove();
                unsigned long nbSec = (unsigned long)(dev.isPair() ? parameters.params.tankPair : parameters.params.tankImpair);
                unsigned long duration = (unsigned long)nbSec * (unsigned long)1000;
                unsigned long currentMillis = millis();
                if (currentMillis - dev.getAgitateStart() > duration) {
                    dev.setAgitateStart(0);
                    devState = UP;
                } else{
                    devState = AGITATE_UP;
                }
                
            }
            break;
        case UP:
            if(!dev.isYEndMove()){
                digitalWriteFast(SECOND_EXPOSURE_PIN, LOW);
                dev.up(false, dev.exitNeeded() ? Y_IMPAIR_DISTANCE : Y_DISTANCE, false);
            }else{
                dev.resetMove();
                
                if(dev.exitNeeded()){
                    devState = ROT_EXIT;
                }else{
                    devState = DRIP;
                }
            }
            break;

        case WAIT_PAPER:
            if(dev.servoFinished() && photoState == WAIT_DEV){
              photoState = PAPER_CUT;  
            } else if(photoState != WAIT_DEV && photoState != PAPER_CUT && photoState != CUT && photoState != CLOSE_SCISSOR && photoState != PAPER_OUT){
                devState = DOWN;
                //photoState = PAPER_REWIND;
            }
            break;

        case DOWN_FINISH:
            if(dev.servoFinished()){
                if(!dev.isYEndMove()){
                    dev.down(-Y_EXIT_DISTANCE);
                }else{
                    dev.resetMove();
                    devState = UP_FINISH;
                }
            }
            break;

        case UP_FINISH:
            if(!dev.isYEndMove()){
                dev.up(false, Y_EXIT_DISTANCE);
            }else{
                dev.resetMove();
                devState = IDLE_DEV;
            }
            break;

        case DOWN_ROT:
            if(!dev.isYEndMove()){
                dev.down(-Y_EXIT_DISTANCE);
            }else{
                dev.resetMove();
                //check if paper in carrier at pos 9 so exit.
                if(dev.exitNeeded()){
                    devState = ROT_EXIT;
                }else{
                    devState = ROTATE;
                }
            }
            break;


        case ROT_EXIT:
            if(!dev.isRotEndMove()){
                dev.rotateExit();
            }else{
                dev.resetMove();
                devState = UP_EXIT;
            }
            break;

        case UP_EXIT:
            if(!dev.isYEndMove()){
                dev.up(true, Y_EXIT_DISTANCE+20);
            }else{
                dev.resetMove();
                devState = WAIT_EXIT;
            }
            break;

        case WAIT_EXIT:
            if(dev.isDripFinished()){
                devState = ROTATE;
            }
            break;

        case DRIP:
            if(dev.isDripFinished()){
                if(dev.isPair()){
                    devState = DOWN_ROT;
                }else{
                    devState = ROTATE;
                }
            }
            break;
        case MANUAL_DOWN:
            if(!dev.isYEndMove()){
                dev.down( -Y_PAIR_DISTANCE, true);
            }else{
                dev.resetMove();
                devState = IDLE_DEV;
            }
            break;

        case MANUAL_UP:
            if(!dev.isYEndMove()){
                dev.up(false, Y_DISTANCE, true);
            }else{
                dev.resetMove();
                devState = IDLE_DEV; 
            }
            break;
    }
}


