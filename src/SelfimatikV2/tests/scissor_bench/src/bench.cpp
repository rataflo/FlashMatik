/*
 * Banc test de l'axe scissor (16 µsteps, MRES=4).
 *
 * Procedure :
 *  1. Initialisation TMC identique au firmware (16µ, 1.6A RMS), lecture MRES.
 *  2. Home sur butee.
 *  3. Pour chaque reglage (vitesse, accel) : ouverture complete chrono-metree,
 *     retour home sur buttee.
 *
 * Mettre une feuille a chaque cycle et juger la qualite de la coupe.
 * 'r' = relancer le balayage. A la fin : chariot au home (ferme).
 * Reflasher le firmware principal apres le test !
 */
#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

#define R_SENSE 0.11f
#define SCISSOR_PIN_STP 46
#define SCISSOR_PIN_DIR 48
#define SCISSOR_ENDSTOP_PIN 15
#define SCISSOR_PIN_ENABLE A8
#define SCISSOR_RX A11
#define SCISSOR_TX 42

#define STROKE 220 // pas complets (valeur machine)

TMC2208Stepper tmc(SCISSOR_RX, SCISSOR_TX, R_SENSE);
AccelStepper stepper(1, SCISSOR_PIN_STP, SCISSOR_PIN_DIR);

// Reglages a tester : vitesse (µsteps/s), accel (µsteps/s²)
struct Setting { const char* tag; long v; long a; };
Setting settings[] = {
    {"A  8000/ 9000",  8000,  9000},  // valeur ancienne fonctionnelle (~460ms)
    {"B  8000/15000",  8000, 15000},
    {"C  8000/25000",  8000, 25000},
    {"D  8000/40000",  8000, 40000}, // limite mecanique ?
};
const int NB_SETTINGS = sizeof(settings) / sizeof(Setting);

void enableMotor(bool on) {
    digitalWrite(SCISSOR_PIN_ENABLE, on ? LOW : HIGH);
}

// Retour home sur buttee (comme Scissor::close()).
void homeScissor() {
    enableMotor(true);
    stepper.setMaxSpeed(2500);
    stepper.setAcceleration(12000);
    stepper.moveTo(-2 * STROKE); // marge au dela du home
    // La butee est ACTIVE quand le chariot est au home (niveau bas).
    int guard = 0;
    while (stepper.distanceToGo() != 0 && guard++ < 100000) {
        stepper.run();
        if (!digitalRead(SCISSOR_ENDSTOP_PIN)) {
            stepper.stop();
            break;
        }
        delayMicroseconds(200);
    }
    enableMotor(false);
}

void runSweep() {
    Serial.println(F("=== SWEEP COUPE ==="));
    for (int i = 0; i < NB_SETTINGS; i++) {
        homeScissor(); // laisse le moteur DESACTIVE a la fin
        delay(300);

        // REACTIVER le moteur avant l'ouverture (home l'avait coupe)
        enableMotor(true);
        stepper.setMaxSpeed(settings[i].v);
        stepper.setAcceleration(settings[i].a);
        stepper.setCurrentPosition(0);
        stepper.moveTo(STROKE);

        unsigned long t0 = micros();
        int guard = 0;
        while (stepper.distanceToGo() != 0 && guard++ < 100000) {
            stepper.run();
            delayMicroseconds(100);
        }
        unsigned long dt = micros() - t0;
        enableMotor(false);

        Serial.print(settings[i].tag);
        Serial.print(F("  ->  ouverture en "));
        Serial.print(dt / 1000);
        Serial.print(F(" ms  [butte="));
        Serial.print(digitalRead(SCISSOR_ENDSTOP_PIN));
        Serial.println(F("]"));
        delay(500);
    }
    homeScissor();
    Serial.println(F("FIN SWEEP - chariot au home."));
}

void setup() {
    Serial.begin(115200);
    pinMode(SCISSOR_PIN_ENABLE, OUTPUT);
    pinMode(SCISSOR_PIN_STP, OUTPUT);
    pinMode(SCISSOR_PIN_DIR, OUTPUT);
    pinMode(SCISSOR_ENDSTOP_PIN, INPUT);
    pinMode(SCISSOR_RX, INPUT);
    pinMode(SCISSOR_TX, OUTPUT);
    enableMotor(false);

    delay(1000);
    tmc.begin();
    tmc.toff(5);
    tmc.rms_current(1900);
    tmc.microsteps(8);   // pas complets
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(true);
    tmc.tbl(1);
    tmc.hend(2);
    tmc.hstrt(3);
    delay(500);

    Serial.println(F("=== BANC SCISSOR ==="));
    Serial.print(F("MRES lu = "));
    Serial.print(tmc.microsteps());
    Serial.println(F("  (8 = pas complets)"));

    homeScissor();
    runSweep();
    Serial.println(F("Taper 'r' pour re-lancer le balayage."));
}

void loop() {
    static bool running = false;
    if (Serial.available()) {
        char c = Serial.read();
        if ((c == 'r' || c == 'R') && !running) {
            homeScissor();
            runSweep();
        }
    }
}
