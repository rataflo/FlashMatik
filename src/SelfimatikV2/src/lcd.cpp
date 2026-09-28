#include "lcd.h"

using namespace Menu;
LiquidCrystal lcd(LCD_PINS_RS, LCD_PINS_ENABLE, LCD_PINS_D4, LCD_PINS_D5, LCD_PINS_D6, LCD_PINS_D7);

ClickEncoder clickEncoder(BTN_EN2, BTN_EN1, BTN_ENC,4);

ClickEncoderStream encStream(clickEncoder,1);
MENU_INPUTS(in, &encStream);
void timerIsr() {clickEncoder.service();}

result doAlert(eventMask e, prompt &item);

result saveParams(eventMask e, prompt &item){
  parameters.updateParameters();
  return proceed;
}

result showEvent(eventMask e,navNode& nav,prompt& item) {
  /*Serial.print("event: ");
  Serial.println(e);*/
  return proceed;
}

result eraseCounter1(eventMask e, prompt &item){
  parameters.params.userCount1 = 0;
  parameters.updateParameters();
  return proceed;
}

result eraseCounter2(eventMask e, prompt &item){
  parameters.params.userCount2 = 0;
  parameters.updateParameters();
  return proceed;
}

// --- Actions pour le contrôle manuel ---
result manualUpAction(eventMask e, prompt &item) {
    if (e == enterEvent) {
        if (photoState == IDLE_PHOTO && devState == IDLE_DEV) {
            Serial.println("MANUAL_UP from LCD");
            devState = MANUAL_UP;
        } 
    }
    return proceed;
}

result manualDownAction(eventMask e, prompt &item) {
    if (e == enterEvent) {
        if (photoState == IDLE_PHOTO && devState == IDLE_DEV) {
            Serial.println("MANUAL_DOWN from LCD");
            devState = MANUAL_DOWN;
        }
    }
    return proceed;
}

result manualDevAction(eventMask e, prompt &item) {
    if (e == enterEvent) {
        if (devState == PAUSE) {
            Serial.println("MANUAL_DEV from LCD");
            devState = UNPAUSE;
        }
    }
    return proceed;
}

MENU(menuData,"Data",showEvent,anyEvent,noStyle
  ,FIELD(parameters.params.totStrip,"Total"," ",0,0,0,0, doNothing ,noEvent, noStyle)
  ,FIELD(parameters.params.userCount1,"Counter 1","",0,0,0,0, doNothing ,noEvent, noStyle)
  ,OP("Erase counter 1", eraseCounter1, enterEvent)
  ,FIELD(parameters.params.userCount2,"Counter 2","",0,0,0,0, doNothing ,noEvent, noStyle)
  ,OP("Erase counter 2", eraseCounter2, enterEvent)
  ,EXIT("<Back")
);


MENU(menuTimes, "Times", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.tankPair,"Tank 1/3/5/../13","s",1,480,1,0,saveParams ,exitEvent , noStyle)
  ,FIELD(parameters.params.tankImpair,"Tank 2/4/../14","s",1,480,1,0,saveParams ,exitEvent , noStyle)
  ,FIELD(parameters.params.driptTime,"Drip time","s",5,480,1,0,saveParams ,exitEvent , noStyle)
  ,FIELD(parameters.params.redTime,"Red time","ms",0,1000,1,0,saveParams ,exitEvent , noStyle)
  ,FIELD(parameters.params.greenTime,"Green time","ms",0,1000,1,0,saveParams ,exitEvent , noStyle)
  ,FIELD(parameters.params.blueTime,"Blue time","ms",0,1000,1,0,saveParams ,exitEvent , noStyle)
  ,EXIT("<Back")
);


MENU(menuPaper, "Paper", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.nbStepOneShot,"One shot","",0,5000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.nbStepPaperCut,"Paper cut","",0,6000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.nbStepPaperOut,"Paper out","",0,6000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.deltaFirstShot,"Delta first","",0,1000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.paperSpeed,"Speed feed","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.paperAccel,"Accel feed","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.paperOutSpeed,"Speed out","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.paperOutAccel,"Accel out","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);

MENU(menuRotation, "Rotation", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.nbStepCenterArm,"Stp center","",-5000,5000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.nbStepExit,"Stp exit","",0,5000,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.rotStepPair,"Step pair","",100,2000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.rotStepImpair,"Step impair","",100,2000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.rotSpeed,"Speed rot","",50,2000,50,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.rotAccel,"Accel rot","",50,2000,50,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);


MENU(menuY, "Y axis", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.ySpeed,"Speed Y","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yAccel,"Accel Y","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yDownSpeed,"Speed down","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yDownAccel,"Accel down","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yAgitateSpeed,"Agit speed","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yAgitateAccel,"Agit accel","",100,10000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.agitateSteps,"Agit steps","",10,1000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yDistance,"Dist Y","",0,10000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yPairDistance,"Dist pair","",0,10000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yImpairDistance,"Dist impair","",0,10000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.yExitDistance,"Dist exit","",0,1000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.initSpeed,"Speed init","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.initAccel,"Accel init","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.manSpeed,"Manual speed","",10,1000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.manAccel,"Manual accel","",10,1000,10,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);


MENU(menuServo, "Servo", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.servoPosIdle,"Pos idle","deg",0,180,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.servoPosOpenBegin,"Open begin","deg",0,180,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.servoPosOpenEnd,"Open end","deg",0,180,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.servoPosCloseBegin,"Close begin","deg",0,180,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.servoPosCloseEnd,"Close end","deg",0,180,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.servoTime,"Time ms","ms",0,10000,100,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);


MENU(menuScissor, "Scissor", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.scissorSpeed,"Speed","",1000,20000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.scissorAccel,"Accel","",1000,20000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.scissorStepOpened,"Step open","",50,2000,10,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);


MENU(menuShutter, "Shutter", doNothing ,noEvent, noStyle
  ,FIELD(parameters.params.shutterSpeed,"Speed","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shutterAccel,"Accel","",100,5000,100,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shutterStepRev,"Step rev","",100,1000,10,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.flashTime,"Flash ms","",1,200,1,0,saveParams ,exitEvent, noStyle)
  ,EXIT("<Back")
);


MENU(menuSetup,"Setup",showEvent,anyEvent,noStyle
  ,SUBMENU(menuPaper)
  ,SUBMENU(menuRotation)
  ,SUBMENU(menuY)
  ,SUBMENU(menuShutter)
  ,SUBMENU(menuScissor)
  ,SUBMENU(menuServo)
  ,EXIT("<Back")
);

TOGGLE(parameters.params.userMode1,userMode1,"Dble exp 1: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);


SELECT(parameters.params.expTime, selExpTimes, "Exp time", saveParams, exitEvent, noStyle
  ,VALUE("1/4", 0, doNothing, noEvent)
  ,VALUE("1/2", 1, doNothing, noEvent)
  ,VALUE("1s", 2, doNothing, noEvent)
  ,VALUE("Bulb", 3, doNothing, noEvent)
);

MENU(menuModes,"User modes",showEvent,anyEvent,noStyle
  ,SUBMENU(userMode1)
  ,EXIT("<Back")
);

TOGGLE(parameters.params.bflashOn,flashOnOff,"Flashs: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);

TOGGLE(parameters.params.bDefineEachShot, defineOnOff, "Activate: ", doNothing, noEvent, noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);


SELECT(parameters.params.shotExpTimes[0], selExpTimes1, "Exp time", saveParams, exitEvent, noStyle
  ,VALUE("1/4", 0, doNothing, noEvent)
  ,VALUE("1/2", 1, doNothing, noEvent)
  ,VALUE("1s", 2, doNothing, noEvent)
  ,VALUE("Bulb", 3, doNothing, noEvent)
);

TOGGLE(parameters.params.shotFlashOn[0],flashOnOff1,"Flashs: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);

MENU(defineShot1,"Shot 1",showEvent,anyEvent,noStyle
  ,SUBMENU(selExpTimes1)
  ,FIELD(parameters.params.shotBulbTimes[0],"Bulb time","s",0,600,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shotNbExps[0],"Nb exp","",1,10,1,0,saveParams ,exitEvent, noStyle)
  ,SUBMENU(flashOnOff1)
  ,EXIT("<Back")
);

SELECT(parameters.params.shotExpTimes[1], selExpTimes2, "Exp time", saveParams, exitEvent, noStyle
  ,VALUE("1/4", 0, doNothing, noEvent)
  ,VALUE("1/2", 1, doNothing, noEvent)
  ,VALUE("1s", 2, doNothing, noEvent)
  ,VALUE("Bulb", 3, doNothing, noEvent)
);

TOGGLE(parameters.params.shotFlashOn[1],flashOnOff2,"Flashs: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);

MENU(defineShot2,"Shot 2",showEvent,anyEvent,noStyle
  ,SUBMENU(selExpTimes2)
  ,FIELD(parameters.params.shotBulbTimes[1],"Bulb time","s",0,600,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shotNbExps[1],"Nb exp","",1,10,1,0,saveParams ,exitEvent, noStyle)
  ,SUBMENU(flashOnOff2)
  ,EXIT("<Back")
);

SELECT(parameters.params.shotExpTimes[2], selExpTimes3, "Exp time", saveParams, exitEvent, noStyle
  ,VALUE("1/4", 0, doNothing, noEvent)
  ,VALUE("1/2", 1, doNothing, noEvent)
  ,VALUE("1s", 2, doNothing, noEvent)
  ,VALUE("Bulb", 3, doNothing, noEvent)
);

TOGGLE(parameters.params.shotFlashOn[2],flashOnOff3,"Flashs: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);

MENU(defineShot3,"Shot 3",showEvent,anyEvent,noStyle
  ,SUBMENU(selExpTimes3)
  ,FIELD(parameters.params.shotBulbTimes[2],"Bulb time","s",0,600,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shotNbExps[2],"Nb exp","",1,10,1,0,saveParams ,exitEvent, noStyle)
  ,SUBMENU(flashOnOff3)
  ,EXIT("<Back")
);

SELECT(parameters.params.shotExpTimes[3], selExpTimes4, "Exp time", saveParams, exitEvent, noStyle
  ,VALUE("1/4", 0, doNothing, noEvent)
  ,VALUE("1/2", 1, doNothing, noEvent)
  ,VALUE("1s", 2, doNothing, noEvent)
  ,VALUE("Bulb", 3, doNothing, noEvent)
);

TOGGLE(parameters.params.shotFlashOn[3],flashOnOff4,"Flashs: ",doNothing,noEvent,noStyle
  ,VALUE("On", true, saveParams, noEvent)
  ,VALUE("Off", false, saveParams, noEvent)
);

MENU(defineShot4,"Shot 4",showEvent,anyEvent,noStyle
  ,SUBMENU(selExpTimes4)
  ,FIELD(parameters.params.shotBulbTimes[3],"Bulb time","s",0,600,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.shotNbExps[3],"Nb exp","",1,10,1,0,saveParams ,exitEvent, noStyle)
  ,SUBMENU(flashOnOff4)
  ,EXIT("<Back")
);

MENU(menuDefineEachShot,"Define each shot",showEvent,anyEvent,noStyle
  ,SUBMENU(defineOnOff)
  ,SUBMENU(defineShot1)
  ,SUBMENU(defineShot2)
  ,SUBMENU(defineShot3)
  ,SUBMENU(defineShot4)
  ,EXIT("<Back")
);


MENU(menuShot,"Shot",showEvent,anyEvent,noStyle
  ,SUBMENU(selExpTimes)
  ,FIELD(parameters.params.bulbTime,"Bulb time","s",0,600,1,0,saveParams ,exitEvent, noStyle)
  ,FIELD(parameters.params.nbExp,"Nb exp","",1,10,1,0,saveParams ,exitEvent, noStyle)
  ,SUBMENU(flashOnOff)
  ,SUBMENU(menuDefineEachShot)
  ,SUBMENU(menuModes)
  ,EXIT("<Back")
);

// --- Menu de contrôle manuel ---
MENU(menuManual, "Manual Control", doNothing, noEvent, noStyle
    ,OP("Move UP", manualUpAction, enterEvent)
    ,OP("Move DOWN", manualDownAction, enterEvent)
    ,OP("/!\ Unpause DEV", manualDevAction, enterEvent)
    ,EXIT("<Back")
);

MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,SUBMENU(menuShot)
  ,SUBMENU(menuTimes)
  ,SUBMENU(menuData)
  ,SUBMENU(menuSetup)
  ,SUBMENU(menuManual)  
);

#define MAX_DEPTH 5

MENU_OUTPUTS(out, MAX_DEPTH
  ,LIQUIDCRYSTAL_OUT(lcd,{0,0,20,4})
  ,NONE
);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);//the navigation root object

/*result idle(menuOut& o,idleEvent e) {
  o.setCursor(0,0);
  o.print(F("suspended..."));
  return proceed;
}*/

void initLCD() {
  Serial.begin(115200);
  while(!Serial);
  Serial.flush();
  //encoder.begin();
  lcd.begin(20,4);
  //nav.idleTask=idle;
  //nav.showTitle = false;
  menuData[0].enabled=disabledStatus;
  menuData[1].enabled=disabledStatus;
  menuData[3].enabled=disabledStatus;
  menuShot[1].enabled = parameters.params.expTime != 3 ? disabledStatus : enabledStatus; 

  menuDefineEachShot[1].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[2].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[3].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[4].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 

  defineShot1[1].enabled = parameters.params.shotExpTimes[0] != 3 ? disabledStatus : enabledStatus; 
  defineShot2[1].enabled = parameters.params.shotExpTimes[1] != 3 ? disabledStatus : enabledStatus; 
  defineShot3[1].enabled = parameters.params.shotExpTimes[2] != 3 ? disabledStatus : enabledStatus; 
  defineShot4[1].enabled = parameters.params.shotExpTimes[3] != 3 ? disabledStatus : enabledStatus; 

  lcd.setCursor(0, 0);
  lcd.print("Selfimatik");
  lcd.setCursor(0, 1);
  lcd.print("V0.2");
  lcd.setCursor(0, 2);
  lcd.print("Startup...");
  Timer3.initialize(1000);
  Timer3.attachInterrupt(timerIsr);
  clickEncoder.setAccelerationEnabled(true);
}

void printStartup(String msg){
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(msg);
}

void idleOnLCD(){
  nav.idleOn();
}
void idleOffLCD(){
  nav.idleOff();
}

void checkMenu(){
  menuShot[1].enabled = parameters.params.expTime != 3 ? disabledStatus : enabledStatus; 

  menuDefineEachShot[1].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[2].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[3].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 
  menuDefineEachShot[4].enabled = parameters.params.bDefineEachShot ? enabledStatus : disabledStatus; 

  defineShot1[1].enabled = parameters.params.shotExpTimes[0] != 3 ? disabledStatus : enabledStatus; 
  defineShot2[1].enabled = parameters.params.shotExpTimes[1] != 3 ? disabledStatus : enabledStatus; 
  defineShot3[1].enabled = parameters.params.shotExpTimes[2] != 3 ? disabledStatus : enabledStatus; 
  defineShot4[1].enabled = parameters.params.shotExpTimes[3] != 3 ? disabledStatus : enabledStatus; 
  nav.poll();
}
