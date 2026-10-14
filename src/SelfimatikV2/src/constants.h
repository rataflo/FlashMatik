#ifndef constants_h
#define constants_h

#include "Arduino.h"

// uncomment to activate debug with serial output.
#define DEBUG_MODE
// uncomment to activate simulation mode (take shot, no paper cut, dev).
//#define SIMUL_MODE


//UART TMC
#define R_SENSE 0.11f // TMC2226

//PINS
#define ROT_PIN_STP A6
#define ROT_PIN_DIR A7
#define ROT_PIN_ENABLE A2
#define ROT_RX            A10
#define ROT_TX            A5

#define Y_PIN_STP A0
#define Y_PIN_DIR A1
#define Y_ENDSTOP_PIN 3
#define Y_PIN_ENABLE 38
#define Y_BRAKE_PIN 7
#define Y_RX            A9 
#define Y_TX            40

#define PAPER_PIN_STP 36
#define PAPER_PIN_DIR 34
#define PAPER_SWITCH1_PIN 18
#define PAPER_PIN_ENABLE 30
#define PAPER_RX 12
#define PAPER_TX 20

#define SHUTTER_PIN_STP 26
#define SHUTTER_PIN_DIR 28
#define SHUTTER_ENDSTOP_PIN 14
#define SHUTTER_PIN_ENABLE 24
#define SHUTTER_RX A12
#define SHUTTER_TX 44

#define SCISSOR_PIN_STP 46
#define SCISSOR_PIN_DIR 48
#define SCISSOR_ENDSTOP_PIN 15
#define SCISSOR_PIN_ENABLE A8
#define SCISSOR_RX A11
#define SCISSOR_TX 42

#define LED_MATRIX_SDI_PIN 51 //52//12 // SDI = DIN
//#define LED_MATRIX_CS_PIN 50 //51 //11 // CS
#define LED_MATRIX_CS_PIN 5 //51 //11 // CS
#define LED_MATRIX_SCL_PIN 52 //50//10 // SCL = CLOCK

#define FLASH_PIN 45
#define START_BTN_PIN 19
#define OPTO_PIN 6

#define SERVO_PIN 11

#define EXIT_PIN 2

#define SECOND_EXPOSURE_PIN 8

#define PREFLASH_PIN 4
#define NUMPIXELS 6

#define LCD_PINS_RS        16
#define LCD_PINS_ENABLE    17
#define LCD_PINS_D4        23
#define LCD_PINS_D5        25
#define LCD_PINS_D6        27
#define LCD_PINS_D7        29
#define LCD_HEIGHT        4
#define LCD_WIDTH          20

// Encoder
#define BTN_EN1            31
#define BTN_EN2            33
#define BTN_ENC            35


#define DRYER_PIN 12

#define SERVO_POS_IDLE 10
#define SERVO_POS_OPEN_BEGIN 10
#define SERVO_POS_OPEN_END 125
#define SERVO_POS_CLOSE_BEGIN 140
#define SERVO_POS_CLOSE_END 35
#define SERVO_TIME 1500

#define ROT_SPEED 300
#define ROT_ACCEL 300
// 7 bras / 14 tanks : valeurs mesurees au banc le 2026-07-31 (tests/src/test_rotation.cpp).
// T = 4940 pas/tour, S = T/7 = 705.7 pas/bras, S/2 = 352.86 pas/position.
// rotStepPair = S/2 arrondi. rotStepImpair > S - NB_STEP_CENTER_ARM - rotStepPair
// (= 337 max sur l'ecart mesure 694..715) avec marge, arret sur l'opto a chaque tour impair.
#define X_ROTATE_PAIR 353
#define X_ROTATE_IMPAIR 400 //420

#define Y_SPEED 2800 //3000 //4000 //8000
#define Y_ACCEL 800 //1000 //10000
#define Y_DISTANCE 1555//3075 //6150
#define Y_PAIR_DISTANCE 1537//3075 //6150
#define Y_IMPAIR_DISTANCE 1450//2900 //5800
#define Y_EXIT_DISTANCE 102//205 //410
#define Y_AGITATE_SPEED 3000//4000 //8000
#define Y_AGITATE_ACCEL 1000//1000 //10000
#define Y_AGITATE_STEPS 175//350 //700
#define Y_DOWN_SPEED 3000//4000 //8000
#define Y_DOWN_ACCEL 1500//1000 //10000

#define SHUTTER_SPEED 800//1450
#define SHUTTER_ACCEL 400//1450 // 1450 => 1s, 5500 => 1/2s, 22000 => 1/4s, 30000 => max, 191ms less than 1/8s
#define SHUTTER_STEP_REVOL 215 // Number of step for a full rotation of the shutter

// Scissor : axe pilote en PAS COMPLETS (TMC MRES=8, force explicitement dans scissor.cpp)
// pour le couple maximal. Moteur NEMA34 2.1A/65Ncm, train 40T->20T (x2 vitesse sur la courroie).
// 220 pas complets = 1.1 tour moteur = ~2.2 tours de courroie : valeur de reference machine.
// La coupe est limitee en accel : pic ~ sqrt(a*d) = 1400 pas/s, duree ~400ms.
#define SCISSOR_SPEED 8000 // pas complets/s
#define SCISSOR_ACCEL 9000 // pas complets/s^2
#define SCISSOR_STEP_OPENED 220 // pas complets pour ouvrir la cisaille

#define NB_STEP_PAPER_ONE_SHOT 1824 // Number of step to move to another shot. Previous: 1860 (bande 20.0cm: x20/20.4), 1240 (x3/2 gear 20T/60T)
#define NB_STEP_PAPER_CUT 3706 // Number of step to move paper to cut position. Previous: 3780 (bande 20.0cm: x20/20.4), 2520
#define NB_STEP_PAPER_OUT 5441 // Number of step to move out paper. Previous: 5550 (bande 20.0cm: x20/20.4), 3700, 1285
#define PAPER_SPEED 1000
#define PAPER_ACCEL 800
#define PAPER_OUT_SPEED 1000
#define PAPER_OUT_ACCEL 1500
#define DELTA_FIRST_SHOT 1440 //Delta in step to do after paper reach opto 1. Previous: 960 (x3/2 gear 20T/60T), -80

#define INIT_SPEED 1000 //4000
#define INIT_ACCEL 400 //5000

#define DRYER_TIME 10000

#define TANK_TIME 25 //250000 // Default time in ms in tank.
#define TANK_TIME_IMPAIR 20 //20000 // Default time in ms in tank.
#define DRIP_TIME 5 // Default time in ms for drip.

#define NB_STEP_ROT_EXIT 20
#define NB_STEP_CENTER_ARM 25

//Pre flash times (valeurs par defaut, reglables EEPROM)
#define RED_TIME 100
#define GREEN_TIME 25
#define BLUE_TIME 12

// EEPROM data & work variables
#define EEPROM_ADRESS 0
struct storage {
  byte checkCode = 0;
  int totStrip = 0;
  int userCount1 = 0;
  int userCount2 = 0;

  int tankPair = TANK_TIME;
  int tankImpair = TANK_TIME_IMPAIR;
  int driptTime = DRIP_TIME;
  int nbStepOneShot = NB_STEP_PAPER_ONE_SHOT;
  int nbStepPaperCut = NB_STEP_PAPER_CUT;
  int nbStepPaperOut = NB_STEP_PAPER_OUT;
  int deltaFirstShot = DELTA_FIRST_SHOT;

  int agitateSteps = Y_AGITATE_STEPS;

  int nbStepCenterArm = NB_STEP_CENTER_ARM;
  int nbStepExit = NB_STEP_ROT_EXIT;

  bool userMode1 = false;

  int expTime = 1;
  int bulbTime = 1;

  int redTime = RED_TIME;
  int greenTime = GREEN_TIME; 
  int blueTime = BLUE_TIME;

  // C0: valeurs moteurs/calibration (checkCode v5), remplacent les macros en dur
  int rotSpeed = ROT_SPEED;
  int rotAccel = ROT_ACCEL;
  int rotStepPair = X_ROTATE_PAIR;
  int rotStepImpair = X_ROTATE_IMPAIR;
  int manSpeed = 500;
  int manAccel = 100;

  int ySpeed = Y_SPEED;
  int yAccel = Y_ACCEL;
  int initSpeed = INIT_SPEED;
  int initAccel = INIT_ACCEL;
  int yDownSpeed = Y_DOWN_SPEED;
  int yDownAccel = Y_DOWN_ACCEL;
  int yAgitateSpeed = Y_AGITATE_SPEED;
  int yAgitateAccel = Y_AGITATE_ACCEL;
  int yDistance = Y_DISTANCE;
  int yPairDistance = Y_PAIR_DISTANCE;
  int yImpairDistance = Y_IMPAIR_DISTANCE;
  int yExitDistance = Y_EXIT_DISTANCE;

  int shutterSpeed = SHUTTER_SPEED;
  int shutterAccel = SHUTTER_ACCEL;
  int shutterStepRev = SHUTTER_STEP_REVOL;
  int flashTime = 20;

  int paperSpeed = PAPER_SPEED;
  int paperAccel = PAPER_ACCEL;
  int paperOutSpeed = PAPER_OUT_SPEED;
  int paperOutAccel = PAPER_OUT_ACCEL;

  int scissorSpeed = SCISSOR_SPEED;
  int scissorAccel = SCISSOR_ACCEL;
  int scissorStepOpened = SCISSOR_STEP_OPENED;

  int servoPosIdle = SERVO_POS_IDLE;
  int servoPosOpenBegin = SERVO_POS_OPEN_BEGIN;
  int servoPosOpenEnd = SERVO_POS_OPEN_END;
  int servoPosCloseBegin = SERVO_POS_CLOSE_BEGIN;
  int servoPosCloseEnd = SERVO_POS_CLOSE_END;
  int servoTime = SERVO_TIME;

  bool bflashOn = true;
  bool bDefineEachShot = false;
  byte nbExp = 1;

  int shotExpTimes[4]={1,1,1,1};
  int shotBulbTimes[4]={1,1,1,1};
  bool shotFlashOn[4]={true,true,true,true};
  int shotNbExps[4]={1,1,1,1};
};

enum PhotoState {
    IDLE_PHOTO,
    COUNTDOWN,
    TAKE_PHOTO,
    PAPER_NEXT_SHOT,
    WAIT_DEV,
    PAPER_CUT,
    CUT,
    CLOSE_SCISSOR,
    PAPER_OUT,
    PAPER_REWIND,
    PREFLASH_START,
    RED,
    GREEN,
    BLUE
};

enum DevState {
    IDLE_DEV,
    ROTATE,
    DOWN,
    UP,
    AGITATE_UP,
    AGITATE_DOWN,
    WAIT_PAPER,
    DOWN_ROT,
    ROT_EXIT,
    UP_EXIT,
    DRIP,
    WAIT_EXIT,
    DOWN_FINISH,
    UP_FINISH,
    SECOND_EXPOSURE,
    MANUAL_DOWN,
    MANUAL_UP,
    PAUSE,
    UNPAUSE
};

// Déclaration des variables globales (extern)
extern PhotoState photoState;
extern DevState devState;


#endif
