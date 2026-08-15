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
#define HALL_PIN A4
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

#define SERVO_POS1 20
#define SERVO_POS2 147
#define SERVO_POS3 90

#define SERVO_POS_IDLE 10
#define SERVO_POS_OPEN_BEGIN 10
#define SERVO_POS_OPEN_END 128
#define SERVO_POS_CLOSE_BEGIN 140
#define SERVO_POS_CLOSE_END 27
#define SERVO_TIME 1500

#define ROT_SPEED 400
#define ROT_ACCEL 400
#define ROT_HALL_VALUE 530
#define X_ROTATE_PAIR 300
#define X_ROTATE_IMPAIR 320 //420

#define Y_SPEED 3000 //4000 //8000
#define Y_ACCEL 1000 //1000 //10000
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

//#define SCISSOR_SPEED 8000 //1500
//#define SCISSOR_ACCEL 10000 //2500
#define SCISSOR_SPEED 8000 //1500
#define SCISSOR_ACCEL 9000 //2500
//#define SCISSOR_STEP_OPENED 450 //850 // Number of step to fully open the scissor.
#define SCISSOR_STEP_OPENED 240 //850 // Number of step to fully open the scissor.

#define NB_STEP_PAPER_ONE_SHOT 1240 // Number of step to move to another shot.
#define NB_STEP_PAPER_CUT 2520 // Number of step to move paper to cut position
#define NB_STEP_PAPER_OUT 3700 // Number of step to move out paper.Previous:1285
#define PAPER_SPEED 1000
#define PAPER_ACCEL 800
#define PAPER_OUT_SPEED 1000
#define PAPER_OUT_ACCEL 1500
#define DELTA_FIRST_SHOT 960 //Delta in step to do after paper reach opto 1. Previous: -80
#define NB_STEP_CENTERING 0 //Delta in step to do for centering the paper carrier to the feed down.

#define INIT_SPEED 1000 //4000
#define INIT_ACCEL 400 //5000

#define WAIT_BETWEEN_SHOT 5000 // Wait between shot in ms.
#define DRYER_TIME 10000

#define TANK_TIME 25 //250000 // Default time in ms in tank.
#define TANK_TIME_IMPAIR 20 //20000 // Default time in ms in tank.
#define DRIP_TIME 5 // Default time in ms for drip.

#define NB_STEP_ROT_EXIT 20
#define NB_STEP_CENTER_ARM 25

//Pre flash times
#define RED_TIME 100
#define GREEN_TIME 25
#define BLUE_TIME 12

const int expTimes[3]={6000,2000,700};

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

  bool bflashOn = true;
  bool bDefineEachShot = false;
  byte nbExp = 1;

  int shotExpTimes[4]={1,1,1,1};
  int shotBulbTimes[4]={1,1,1,1};
  bool shotFlashOn[4]={true,true,true,true};
  int shotNbExps[4]={1,1,1,1};
};


#endif
