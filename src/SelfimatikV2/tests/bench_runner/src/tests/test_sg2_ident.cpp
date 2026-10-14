// =====================================================================
// test_sg2_ident — identification des bits StallGuard2 du TMC2226
//
// Pourquoi ce test :
//   La lib pilote le 2226 via la classe TMC2208Stepper. Le 2226 est un
//   mono-moteur : son bit d'événement SG2 devrait être à la position
//   "s2vsa" du 2208, mais on ne parie pas — on vérifie sur la puce.
//
// Protocole (axe = SCISSOR, chariot saisissable à la main) :
//   HOME      : recadrage sur la butée (position connue)
//   LIBRE     : 3 s de déplacement à 100 pas complets/s — NE PAS BLOQUER
//   BLOQUEZ   : prompt serial ; le chariot continue d'insister (+40 pas)
//   RELACHEZ  : prompt serial après 8 s de stall
//   FIN       : après 6 s de retour au calme
//
// Chaque 100 ms, UNE SEULE lecture brute de DRV_STATUS est faite et tous
// les bits sont extraits de CE LECTURE UNIQUE : les bits SG2 sont
// read-clear (remis à zéro par la lecture), multiplier les lectures
// effacerait l'événement avant qu'on ne l'observe.
//
// Livrables attendus :
//   1) quel bit (s2vsa / s2vsb) s'arme au stall sur le TMC2226 ?
//   2) le bit est-il sticky ou remis à zéro par la lecture ?
//   3) s2ga / s2gb : fiables comme 2e signal, ou trop nerveux ?
//   4) état baseline de ot()
// =====================================================================
#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "constants.h"   // pins + R_SENSE, partagés avec le firmware principal
#include "test_sg2_ident.h"

// ===== Axe testé (v1 : SCISSOR uniquement — voir README du runner) =====
static TMC2208Stepper sg2_tmc(SCISSOR_RX, SCISSOR_TX, R_SENSE);
static AccelStepper  sg2_stp(1, SCISSOR_PIN_STP, SCISSOR_PIN_DIR);

#define SG2_SPEED          100   // pas complets/s — lent, facile à saisir
#define SG2_TRAVEL_FREE    150   // course libre depuis le home (< 220 = course machine)
#define SG2_TRAVEL_PUSH     40   // insistance pendant le stall (total 190 < 220)

// Positions des bits dans le payload 32 bits de DRV_STATUS (MSB first),
// d'après la structure DRV_STATUS_t de la lib TMCStepper.
#define BIT_OT    (1UL << 30)
#define BIT_S2GA  (1UL << 29)
#define BIT_S2GB  (1UL << 28)
#define BIT_S2VSA (1UL << 27)
#define BIT_S2VSB (1UL << 26)
#define BIT_STST  1UL

static const char*  phase = "";
static unsigned long tPhase = 0, tStart = 0, lastPrint = 0;
static unsigned long sgEvents = 0;

// --- Home : identique à scissor_bench (butée à l'ETAT BAS) ---
// Retourne true si OK, false si butée non trouvée (test déjà terminé).
static bool homeScissor() {
    digitalWrite(SCISSOR_PIN_ENABLE, LOW);     // moteur ON
    pinMode(SCISSOR_ENDSTOP_PIN, INPUT_PULLUP);
    digitalWrite(SCISSOR_PIN_DIR, HIGH);       // direction home

    unsigned long t0 = millis();
    while (digitalRead(SCISSOR_ENDSTOP_PIN) != LOW) {
        if (millis() - t0 > 15000) {
            Serial.println(F("ERREUR HOME : butée non trouvée, STOP."));
            finishTest();                      // teardown + inactive
            return false;
        }
        digitalWrite(SCISSOR_PIN_STP, !digitalRead(SCISSOR_PIN_STP));
        delayMicroseconds(1000);              // 1000 pas/s vers la butée
    }
    digitalWrite(SCISSOR_PIN_ENABLE, HIGH);   // coupure franche à la butée
    return true;
}

void test_sg2_ident_setup() {
    pinMode(SCISSOR_PIN_STP, OUTPUT);
    pinMode(SCISSOR_PIN_DIR, OUTPUT);
    pinMode(SCISSOR_PIN_ENABLE, OUTPUT);

    // Init TMC IDENTIQUE au firmware principal (scissor.cpp) :
    // le comportement SG2 dépend du courant/mode.
    sg2_tmc.begin();
    sg2_tmc.toff(5);
    sg2_tmc.rms_current(1900);       // = IRUN max (1,77 A) sur R_SENSE 0,11
    sg2_tmc.microsteps(0);           // pas complets
    sg2_tmc.en_spreadCycle(true);
    sg2_tmc.tbl(1);
    sg2_tmc.hend(2);
    sg2_tmc.hstrt(3);
    delay(200);

    Serial.println(F("HOME..."));
    if (!homeScissor()) return;     // home en erreur => test déjà terminé

    digitalWrite(SCISSOR_PIN_ENABLE, LOW);
    sg2_stp.setMaxSpeed(SG2_SPEED);
    sg2_stp.setAcceleration(300);
    sg2_stp.setCurrentPosition(0);
    sg2_stp.moveTo(SG2_TRAVEL_FREE);

    tStart = millis();
    phase = "LIBRE";
    tPhase = millis();
    sgEvents = 0;
    lastPrint = 0;
    Serial.println(F("PHASE LIBRE - 3s - NE PAS BLOQUER"));
}

void test_sg2_ident_loop() {
    sg2_stp.run();
    unsigned long now = millis();

    // ---- print @ 10 Hz : UNE lecture brute de DRV_STATUS par cycle ----
    if (now - lastPrint >= 100) {
        lastPrint = now;
        uint32_t sr = sg2_tmc.DRV_STATUS();
        if (sr & (BIT_S2VSA | BIT_S2VSB)) sgEvents++;

        // (pas de print en chaine : sur AVR print() retourne size_t)
        Serial.print("[");
        Serial.print(now - tStart);
        Serial.print("ms] ");
        Serial.print(phase);
        Serial.print(" s2vsa=");
        Serial.print((sr & BIT_S2VSA) ? 1 : 0);
        Serial.print(" s2vsb=");
        Serial.print((sr & BIT_S2VSB) ? 1 : 0);
        Serial.print(" s2ga=");
        Serial.print((sr & BIT_S2GA) ? 1 : 0);
        Serial.print(" s2gb=");
        Serial.print((sr & BIT_S2GB) ? 1 : 0);
        Serial.print(" ot=");
        Serial.print((sr & BIT_OT) ? 1 : 0);
        Serial.print(" cs=");
        Serial.print((sr >> 11) & 0x1F);
        Serial.print(" stst=");
        Serial.print(sr & BIT_STST);
        Serial.print(" #evt=");
        Serial.println(sgEvents);
    }

    // ---- machine à phases ----
    unsigned long el = now - tPhase;
    if (phase[0] == 'L' && el > 3000) {
        phase = "BLOQUEZ";
        tPhase = now;
        sg2_stp.moveTo(sg2_stp.currentPosition() + SG2_TRAVEL_PUSH);
        Serial.println(F(">>> BLOQUEZ LE CHARIOT SCISSOR MAINTENANT <<<"));
    } else if (phase[0] == 'B' && el > 8000) {
        phase = "RELACHEZ";
        tPhase = now;
        sg2_stp.stop();
        Serial.println(F(">>> RELACHEZ LE CHARIOT MAINTENANT <<<"));
    } else if (phase[0] == 'R' && el > 6000) {
        Serial.print("TEST SG2 SCISSOR FINI - total evenements SG2 : ");
        Serial.println(sgEvents);
        finishTest();
    }
}

void test_sg2_ident_teardown() {
    sg2_stp.stop();
    pinMode(SCISSOR_PIN_ENABLE, OUTPUT);
    digitalWrite(SCISSOR_PIN_ENABLE, HIGH);   // moteur coupe
    Serial.println(F("MOTEUR SCISSOR COUPE"));
}
