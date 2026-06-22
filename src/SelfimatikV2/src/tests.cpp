#include "tests.h"


void debug(String functionName, String varValue){
  
#ifdef DEBUG_MODE
  Serial.println(functionName + ":" + varValue);
  #endif
}
void debug(String functionName, float varValue){
  
#ifdef DEBUG_MODE
  Serial.println(functionName + ":" + String(varValue, 1));
  #endif
}
void debug(String functionName, char varValue){
  
#ifdef DEBUG_MODE
  Serial.println(functionName + ":" + varValue);
  #endif
}
void debug(String functionName, int varValue){
  
#ifdef DEBUG_MODE
  Serial.println(functionName + ":" + varValue);
  #endif
}

void debug(String functionName, boolean varValue){
  #ifdef DEBUG_MODE
    Serial.println(functionName + ":" + (varValue? "true" : "false"));
  #endif
}
