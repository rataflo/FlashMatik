/*
 * Diagnostic lecture seule du driver TMC scissor.
 * Lit MRES (microsteps effectif), GHOM, et quelques registers, SANS bouger
 * le moteur. Urtile pour savoir en combien de microsteps l'axe coupe reellement.
 */
#include <Arduino.h>
#include <TMCStepper.h>

#define R_SENSE 0.11f
#define SCISSOR_PIN_ENABLE A8
#define SCISSOR_RX A11
#define SCISSOR_TX 42

TMC2208Stepper tmc(SCISSOR_RX, SCISSOR_TX, R_SENSE);

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("=== DIAG TMC SCISSOR (lecture seule) ==="));

    pinMode(SCISSOR_PIN_ENABLE, OUTPUT);
    pinMode(SCISSOR_RX, INPUT);
    pinMode(SCISSOR_TX, OUTPUT);
    digitalWrite(SCISSOR_PIN_ENABLE, HIGH); // moteur desactive

    delay(1000); // laisse le driver se stabiliser
    tmc.begin();
    delay(500);

    Serial.println(F("Tentatives de lecture MRES (0=256,1=128,2=64,3=32,4=16,5=8,6=4,7=2,8=1):"));
    for (int i = 0; i < 5; i++) {
        Serial.print(F("  essai ")); Serial.print(i + 1);
        Serial.print(F(" : MRES=")); Serial.println(tmc.microsteps());
        delay(500);
    }
    Serial.print(F("VACTUAL = 0x")); Serial.println(tmc.VACTUAL(), HEX);
    Serial.print(F("DRV_STATUS = 0x")); Serial.println(tmc.DRV_STATUS(), HEX);
    Serial.println(F("FIN DIAG - reflasher le firmware principal."));
}

void loop() {
    delay(1000);
}
