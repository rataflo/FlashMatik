/*
 * TEST ROTATION - Spider 7 bras / 14 tanks
 * -----------------------------------------
 * Mesure le nombre de pas entre deux lamelles (bras) successifs et en
 * deduit les valeurs fiables pour la rotation 14 positions :
 *   - T = pas par revolution
 *   - S = pas entre deux lamelles
 *   - rotStepPair   (deplacement pair, bac -> bac)        ~= S/2
 *   - rotStepImpair (deplacement impair, arrete par opto) >= S - 25 - P
 *
 * Principe (identique au firmware) :
 *   1. Home : tourne dans le sens + jusqu'a trouver la lamelle (opto HIGH),
 *      la laisser passer, puis avancer de NB_STEP_CENTER_ARM pas
 *      (position "centree", comme initRot()).
 *   2. Mesure ensuite les 7 intervalles lamelle -> lamelle suivante en
 *      detectant le front montant de l'opto a chaque fois, en se recentrant
 *      a chaque tour de lamelle comme le firmware.
 *
 * Seule l'axe ROT est pilotee. A la fin le bras est centre sur une lamelle
 * (memorisation des etats identique a la fin de initRot()).
 *
 * Le test attend 30 s avant de bouger (le temps de connecter le moniteur
 * serie 115200) ; un caractere serie lance le test immediatement.
 * A la fin : taper 'r' pour re-lancer.
 */

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

// --- Pins (identiques a constants.h) --------------------------------------
#define R_SENSE 0.11f          // TMC2226
#define ROT_PIN_STP A6
#define ROT_PIN_DIR A7
#define ROT_PIN_ENABLE A2
#define ROT_RX A10
#define ROT_TX A5
#define OPTO_PIN 6

// --- Parametres du test -----------------------------------------------------
#define NB_VANES 7             // 7 bras = 7 lamelles
#define NB_EDGE (NB_VANES + 1) // 8 fronts montants (1 home + 7 mesures)
#define NB_STEP_CENTER_ARM 25  // meme offset de centrage que le firmware
#define SEARCH_LIMIT 1500      // pas de recherche au home (>= 2*S attendu)
#define INTERVAL_LIMIT 2500    // garde-fou par intervalle mesure
#define VANE_PASS_LIMIT 2000   // garde-fou pour le passage d'une lamelle
#define TEST_SPEED 300         // pas/s (ROT_SPEED du firmware)
#define TEST_ACCEL 300         // ROT_ACCEL du firmware
#define WAIT_BEFORE_RUN 30000  // delai avant auto-lancee (ms)

TMC2208Stepper tmcRot(ROT_RX, ROT_TX, R_SENSE);
AccelStepper stepperRot(1, ROT_PIN_STP, ROT_PIN_DIR);

long vaneEdge[NB_EDGE] = {0};
bool bError = false;
String errorMessage = "";

bool opto() {
    return digitalRead(OPTO_PIN) == HIGH;
}

// Avance jusqu'au front montant (opto LOW -> HIGH). False si non trouve avant maxSteps.
bool findEdge(long& pos, long maxSteps) {
    stepperRot.moveTo(stepperRot.currentPosition() + maxSteps);
    while (!opto()) {
        stepperRot.run();
        if (stepperRot.distanceToGo() == 0) return false; // cible atteinte, lamelle non vue
    }
    stepperRot.stop();
    pos = stepperRot.currentPosition();
    return true;
}

// Avance tant que l'opto est HIGH (lamelle qui passe). False si dure trop longtemps.
bool passVane(long maxSteps) {
    stepperRot.moveTo(stepperRot.currentPosition() + maxSteps);
    while (opto()) {
        stepperRot.run();
        if (stepperRot.distanceToGo() == 0) return false;
    }
    stepperRot.stop();
    return true;
}

// Avance d'un nombre fixe de pas (comme le firmware apres detection de la lamelle).
void moveSteps(long n) {
    stepperRot.moveTo(stepperRot.currentPosition() + n);
    while (stepperRot.distanceToGo() != 0) {
        stepperRot.run();
    }
}

void halt(String msg) {
    bError = true;
    errorMessage = msg;
}

void runTest() {
    bError = false;
    Serial.println(F("=== TEST ROTATION 7 BRAS / 14 TANKS ==="));

    // 1. Home : trouver la premiere lamelle (comme initRot du firmware)
    Serial.println(F("[1/2] Recherche d'une lamelle (home)..."));
    long edge;
    if (opto()) {
        // deja sur une lamelle au demarrage : laisser passer d'abord
        if (!passVane(VANE_PASS_LIMIT)) { halt("lamelle trop large / opto bloque"); return; }
    }
    if (!findEdge(edge, SEARCH_LIMIT)) {
        halt("AUCUNE lamelle trouvee (opto HS ? sens de rotation ?)");
        return;
    }
    vaneEdge[0] = edge;
    Serial.println(F("Lamelle reference trouvee."));

    // Laisser la lamelle passer puis se centrer (+NB_STEP_CENTER_ARM), comme initRot()
    if (!passVane(VANE_PASS_LIMIT)) { halt("lamelle trop large / opto bloque"); return; }
    moveSteps(NB_STEP_CENTER_ARM);

    // 2. Mesurer les 7 intervalles lamelle -> lamelle
    Serial.println("[2/2] Mesure des " + String(NB_VANES) + " intervalles...");
    for (int i = 1; i < NB_EDGE; i++) {
        if (!findEdge(edge, INTERVAL_LIMIT)) {
            halt("lamelle " + String(i) + " non trouvee (ecart anormal, mecanique ?)");
            return;
        }
        vaneEdge[i] = edge;
        Serial.print(F("  lamelle "));
        Serial.print(i + 1);
        Serial.print(F(": front a step "));
        Serial.print(edge);
        Serial.print(F(" (intervalle = "));
        Serial.print(edge - vaneEdge[i - 1]);
        Serial.println(F(" pas)"));
        if (!passVane(VANE_PASS_LIMIT)) { halt("lamelle " + String(i + 1) + " trop large / opto bloque"); return; }
        moveSteps(NB_STEP_CENTER_ARM);
    }
    if (bError) { Serial.println("ERREUR: " + errorMessage); return; }

    // 3. Calculs et resultats
    // vaneEdge[7] est la lamelle reference un tour complet apres vaneEdge[0].
    long T = vaneEdge[NB_VANES] - vaneEdge[0];
    long minS = 100000000L, maxS = 0;
    Serial.println(F("Pas entre deux lamelles (bras) :"));
    for (int i = 0; i < NB_VANES; i++) {
        long s = vaneEdge[i + 1] - vaneEdge[i];
        Serial.print(F("  bras ")); Serial.print(i + 1); Serial.print(F(" -> ")); Serial.print(i + 2);
        Serial.print(F(" : ")); Serial.print(s);
        Serial.println(F(" pas"));
        minS = min(minS, s);
        maxS = max(maxS, s);
    }

    float Sfloat = (float)T / NB_VANES;                            // pas par bras (moyenne)
    long P = (long)lround(Sfloat / 2.0);                           // rotStepPair propose = S/2
    long oddRemaining = (long)lround(Sfloat) - NB_STEP_CENTER_ARM - P; // distance impaire reelle restante
    long Imin = oddRemaining + 40;                                 // marge 40 pas

    Serial.println(F(""));
    Serial.println(F("--- RESULTATS -------------------------------------"));
    Serial.print(F("Tour complet T = ")); Serial.print(T); Serial.println(F(" pas/revolution"));
    Serial.print(F("Pas/bras S = T/7 = ")); Serial.print(Sfloat, 2); Serial.println(F(" pas (51.4 degres)"));
    Serial.print(F("Pas/position = S/2 = ")); Serial.print(Sfloat / 2.0, 2); Serial.println(F(" pas (25.7 degres)"));
    Serial.print(F("Intervalle min=")); Serial.print(minS);
    Serial.print(F(" max=")); Serial.print(maxS);
    Serial.print(F(" delta=")); Serial.print(maxS - minS); Serial.println(F(" pas"));
    Serial.println(F("---------------------------------------------------"));
    Serial.println(F("VALEURS PROPOSEES POUR constants.h / EEPROM :"));
    Serial.print(F("  X_ROTATE_PAIR (rotStepPair) = ")); Serial.print(P);
    Serial.print(F("   [S/2]"));
    Serial.println();
    Serial.print(F("  rotStepImpair >= ")); Serial.print(Imin);
    Serial.print(F("   [S - ")); Serial.print(NB_STEP_CENTER_ARM);
    Serial.print(F(" - P = ")); Serial.print(oddRemaining);
    Serial.println(F(" + marge 40]"));
    Serial.print(F("  NB_STEP_CENTER_ARM = ")); Serial.println(NB_STEP_CENTER_ARM);
    Serial.println(F("  (inchange, a confirmer visuellement au banc)"));
    Serial.println(F("---------------------------------------------------"));
    Serial.println(F("FIN - spider centre sur lamelle (etat = fin de initRot)."));
    Serial.println(F("Reflasher le firmware principal, ou 'r' pour re-lancer le test."));
}

void setupRot() {
    pinMode(ROT_PIN_ENABLE, OUTPUT);
    pinMode(ROT_PIN_STP, OUTPUT);
    pinMode(ROT_PIN_DIR, OUTPUT);
    pinMode(ROT_RX, INPUT);
    pinMode(ROT_TX, OUTPUT);
    pinMode(OPTO_PIN, INPUT);

    // Identique a Dev::init() du firmware principal
    digitalWrite(ROT_PIN_ENABLE, LOW);
    delay(100);
    tmcRot.begin();
    tmcRot.toff(5);
    tmcRot.rms_current(800);
    tmcRot.microsteps(4);
    tmcRot.pwm_autoscale(true);
    tmcRot.en_spreadCycle(false);
    tmcRot.tbl(2);
    tmcRot.hend(4);

    stepperRot.setCurrentPosition(0);
    stepperRot.setMaxSpeed(TEST_SPEED);
    stepperRot.setAcceleration(TEST_ACCEL);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F(""));
    Serial.println(F("================================================"));
    Serial.println(F(" TEST ROTATION - spider 7 bras / 14 tanks"));
    Serial.println(" Le test se lance auto dans " + String(WAIT_BEFORE_RUN / 1000) + " s,");
    Serial.println(F(" ou immediatement si vous tapez une touche."));
    Serial.println(F("================================================"));

    setupRot();

    // Attente avant de bouger quoi que ce soit
    unsigned long t0 = millis();
    while (!Serial.available() && millis() - t0 < WAIT_BEFORE_RUN) {
        delay(100);
    }
    runTest();
}

void loop() {
    // A la fin : bras centre, moteur desactive (memorisation fin de initRot)
    digitalWrite(ROT_PIN_ENABLE, HIGH);

    static unsigned long lastKeyCheck = 0;
    if (millis() - lastKeyCheck > 500) {
        lastKeyCheck = millis();
        if (Serial.available()) {
            char c = (char)Serial.read();
            if (c == 'r' || c == 'R') {
                Serial.println(F("Relance du test..."));
                setupRot();
                runTest();
            } else {
                Serial.print(c);
            }
        }
    }
    delay(10);
}
