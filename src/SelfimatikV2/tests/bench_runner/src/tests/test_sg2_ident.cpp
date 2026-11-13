// =====================================================================
// test_sg2_ident — identification des bits StallGuard2 du TMC2226
// (axes Y et ROT — un seul cœur de test paramétré par configuration d'axe)
//
// Pourquoi ce test :
//   La lib pilote le 2226 via la classe TMC2208Stepper. Le 2226 est un
//   mono-moteur : son bit d'événement SG2 devrait être à la position
//   "s2vsa" du 2208, mais on ne parie pas — on vérifie sur la puce.
//   L'init TMC est REPRODUITE A L'IDENTIQUE du firmware principal
//   (dev.cpp) car le comportement SG2 dépend du courant/mode.
//
// Protocole (mêmes phases pour Y et ROT) :
//   HOME      : recadrage (Y = butée, ROT = opto) — identique à dev.cpp
//   LIBRE     : 3 s, déplacement lent (100 pas/s) — NE PAS BLOQUER
//   BLOQUEZ   : prompt serial ; l'axe pousse en continu 8 s (800 pas)
//   RELACHEZ  : prompt serial après 8 s de stall (moteur coupé)
//   FIN       : après 6 s de retour au calme
//
// Chaque 100 ms, UNE SEULE lecture brute de DRV_STATUS est faite et tous
// les bits sont extraits de CETTE LECTURE UNIQUE : les bits SG2 sont
// read-clear (remis à zéro par la lecture), multiplier les lectures
// effacerait l'événement avant qu'on ne l'observe.
//
// Livrables attendus (par axe) :
//   1) quel bit (s2vsa / s2vsb) s'arme au stall sur le TMC2226 ?
//   2) le bit est-il sticky ou remis à zéro par la lecture ?
//   3) s2ga / s2gb : fiables comme 2e signal, ou trop nerveux ?
//   4) état baseline de ot() et de cs (doit lire 31 = IRUN max)
// =====================================================================
#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "constants.h"   // pins + R_SENSE, partagés avec le firmware principal
#include "test_sg2_ident.h"

// ===== Configuration d'un axe (valeurs = dev.cpp, init Y / init Rot) =====
struct AxisCfg {
    const char*  name;
    bool         isY;          // sélection de l'instance TMC/stepper
    bool         homeEndstop;  // true = butée (Y), false = opto (ROT)
    uint8_t      pinHome;      // pin butée / opto
    int          homeSteps;    // course de recadrage (Y: 1605, ROT: 800)
    int          homeSpeed;    // pas/s pendant le home
    int          homeAccel;
    int          speed;        // pas/s en phase LIBRE/BLOQUEZ
    int          freeTravel;   // course libre depuis le home (signé)
    int          pushTravel;   // insistance pendant le stall (40)
    int          rmsCurrent;   // = dev.cpp
    int          microsteps;   // 0 = pas complets, 4 = 1/4 (mapping lib)
    bool         spreadCycle;
    int          tbl;
    int          hend;
    int8_t       hstrt;        // -1 = non configuré (ROT : valeur défaut)
    int          phaseAccel;   // accel. phases LIBRE/BLOQUEZ (pas/s²)
    int          topOffset;    // Y: pas entre depart BLOQUEZ et butee (1400) ; -1 = N/A (ROT)
};

static const AxisCfg CFG_Y = {
    "Y", true, true, Y_ENDSTOP_PIN,
    1605, 1000, 400, 2800,    // = yDistance+50 / initSpeed / initAccel / speed (= Y_SPEED real)
    -1400, +8400,             // libre -1400 (course max machine ~1555), puis push +8400 (3 s a 2800 pps) :
                              // remontee libre puis GRIND ~2.5 s contre la butee home (limite echauffement/OTpw)
                              // + 5 s de maintien (SG fige, sans echauffement additif)
    // ATTENTION BANC : SpreadCycle FORCE A FALSE ici (contrairement a dev.cpp)
    // car SG_RESULT n'est valide qu'en StealthChop — c'est le but du test.
    1200, 0, false, 1, 2, 3, 20000, 1400   // + topOffset (home = butee, LIBRE = -1400)
};

static const AxisCfg CFG_ROT = {
    "ROT", false, false, OPTO_PIN,
    800, 300, 300, 400,       // = rotStepImpair*2 / rotSpeed / rotAccel / speed
    1200, 3200,               // libre +1200 (~3 s a 400 pps), puis push +3200 (8 s a 400 pps)
    800, 4, false, 2, 4, -1, 300, -1   // = dev.cpp tmcRot (hstrt non configure) + phaseAccel + topOffset N/A
};

// ===== Instances (statiques, pins figées à la construction) =====
// TMCProbe : meme chose qu'un TMC2208Stepper mais expose available() (protected)
// pour le diagnostic de liaison (compter les octets qui arrivent sur RX).
class TMCProbe : public TMC2208Stepper {
public:
    TMCProbe(uint16_t rx, uint16_t tx, float rs) : TMC2208Stepper(rx, tx, rs) {}
    int rxCount() { return available(); }
    uint32_t rawRead(uint8_t a) { return read(a); }   // expose read() (protected)
};
static TMCProbe sg2_tmcY   (Y_RX,   Y_TX,   R_SENSE);
static TMCProbe sg2_tmcRot (ROT_RX, ROT_TX, R_SENSE);
static AccelStepper   sg2_stpY   (1, Y_PIN_STP,   Y_PIN_DIR);
static AccelStepper   sg2_stpRot (1, ROT_PIN_STP, ROT_PIN_DIR);

static const AxisCfg* g_cfg = nullptr;

#define SG2_ACCEL      300

// ===== Positions des bits dans le payload 32 bits de DRV_STATUS =====
// D'apres la structure officielle DRV_STATUS_t de la lib TMCStepper
// (TMC2208_bitfields.h), affectation LSB first :
//   bit 0: otpw | 1: ot | 2: s2ga | 3: s2gb | 4: s2vsa | 5: s2vsb
//   bit 6: ola  | 7: olb| 8: t120 | 9: t143 | 10: t150 | 11: t157
//   bits 16-20: cs_actual (5b) | bit 30: stealth | bit 31: stst
// (ATTENTION : la 1ere version de ce test lisait les bits 26-29 = zone
//  reservee, d'ou les "toujours 0". C'est corrige ici.)
#define BIT_OTPW    (1UL << 0)
#define BIT_OT      (1UL << 1)
#define BIT_S2GA    (1UL << 2)
#define BIT_S2GB    (1UL << 3)
#define BIT_S2VSA   (1UL << 4)
#define BIT_S2VSB   (1UL << 5)
#define BIT_OLA     (1UL << 6)
#define BIT_OLB     (1UL << 7)
#define BIT_STEALTH (1UL << 30)
#define BIT_STST    (1UL << 31)
#define GET_CS(sr)  (uint16_t)(((sr) >> 16) & 0x1F)

#define SG2_SAMPLE_MS  20   // echantillonnage SG2 (bits read-clear) — assez serré

static const char* phase = "";
static unsigned long tPhase = 0, tStart = 0, lastPrint = 0, lastSample = 0;
// Les bits SG2 sont READ-CLEAR : on compte chaque echantillon ou le bit est armé.
static unsigned long evS2GA = 0, evS2GB = 0, evS2VSA = 0, evS2VSB = 0;
static unsigned long evOLA = 0, evOLB = 0, evRerr = 0;
static uint32_t lastSR = 0;   // derniere valeur brute (pour le print 10 Hz)
static uint16_t sgres = 0;    // derniere valeur 0x41 (SG_RESULT, style 2209)
static bool     sgresOK = false;
// Stats SG_RESULT par phase (min/max/moyenne) — la charge se voit sur la
// MOYENNE, pas sur un echantillon isole (signal bruité, cf. datasheet §11.5).
// Stats SG_RESULT decoupees en 3 buckets PAR POSITION (pas par phase) :
//   b0 LIBRE  : descente libre a vitesse de service (mouvement seulement)
//   b1 REMONT : remontee libre jusqu'a la butee (0 -> +1400 pas)
//   b2 GRIND  : poussee CONTRE la butee (+1400 -> fin du push, ~3 s)
// Les maintiens (SG fige) sont exclus : ils polluaient les moyennes.
struct SgBuck { int mn, mx; unsigned long sum, n; };
static SgBuck sgB[3];
static long g_bloqStart = 0;
static void sgStatReset() {
    for (int i = 0; i < 3; i++) { sgB[i].mn = 1023; sgB[i].mx = 0; sgB[i].sum = 0; sgB[i].n = 0; }
}
static void sgBuckPrint(int i, const char* name) {
    Serial.print("SGSTAT ");
    Serial.print(name);
    Serial.print(F(" min=")); Serial.print(sgB[i].n ? sgB[i].mn : -1);
    Serial.print(F(" max=")); Serial.print(sgB[i].mx);
    Serial.print(F(" avg=")); Serial.print(sgB[i].n ? sgB[i].sum / sgB[i].n : 0);
    Serial.print(F(" n=")); Serial.println(sgB[i].n);
}

static AccelStepper&  actStp() { return g_cfg->isY ? sg2_stpY : sg2_stpRot; }
static TMC2208Stepper& actTmc() { return g_cfg->isY ? sg2_tmcY : sg2_tmcRot; }
static TMCProbe&        actPrb() { return g_cfg->isY ? sg2_tmcY : sg2_tmcRot; }

// --- Home : même logique que dev.cpp (initY / initRot) ---
static bool sg2_home() {
    AccelStepper& stp = actStp();
    stp.setMaxSpeed(g_cfg->homeSpeed);
    stp.setAcceleration(g_cfg->homeAccel);
    stp.setCurrentPosition(0);
    stp.moveTo(g_cfg->homeSteps);

    unsigned long t0 = millis();
    if (g_cfg->homeEndstop) {
        // Y : butée active BAS (la boucle sort quand la pin passe à LOW)
        while (digitalRead(g_cfg->pinHome)) {
            if (millis() - t0 > 10000) {
                Serial.println(F("ERREUR HOME Y : butée non trouvée, STOP."));
                finishTest();
                return false;
            }
            stp.run();
        }
    } else {
        // ROT : opto actif HAUT (la boucle sort quand la marque est détectée)
        while (!digitalRead(g_cfg->pinHome) && stp.distanceToGo() != 0) {
            if (millis() - t0 > 10000) {
                Serial.println(F("ERREUR HOME ROT : opto non trouvée, STOP."));
                finishTest();
                return false;
            }
            stp.run();
        }
    }
    stp.stop();
    stp.run();                // comme dev.cpp : finir le pas en cours
    return true;
}

static void sg2_start(const AxisCfg& cfg) {
    g_cfg = &cfg;
    AccelStepper& stp = actStp();
    TMC2208Stepper& tmc = actTmc();

    pinMode(cfg.isY ? Y_PIN_STP : ROT_PIN_STP, OUTPUT);
    pinMode(cfg.isY ? Y_PIN_DIR : ROT_PIN_DIR, OUTPUT);
    pinMode(cfg.isY ? Y_PIN_ENABLE : ROT_PIN_ENABLE, OUTPUT);
    digitalWrite(cfg.isY ? Y_PIN_ENABLE : ROT_PIN_ENABLE, HIGH);  // OFF

    pinMode(cfg.pinHome, INPUT);

    // Init TMC A L'IDENTIQUE du firmware principal (dev.cpp) :
    // le comportement SG2 dépend du courant/mode.
    tmc.begin();
    tmc.toff(5);
    tmc.rms_current(cfg.rmsCurrent);
    tmc.microsteps(cfg.microsteps);
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(cfg.spreadCycle);
    tmc.tbl(cfg.tbl);
    tmc.hend(cfg.hend);
    if (cfg.hstrt >= 0) tmc.hstrt(cfg.hstrt);
    delay(200);

    // ---- DIAGNOSTIC : lecture retour de la config APPLIQUEE au driver ----
    // La voie lecture est vivante (CRC OK) mais cs_actual lisait 0 alors que
    // le moteur est bien pilote : on relit ce qui est vraiment dans la puce.
    Serial.println(F("DIAG lecture retour config"));
    Serial.print(F("  irun="));    Serial.println((int)tmc.irun());
    Serial.print(F("  ihold="));   Serial.println((int)tmc.ihold());
    Serial.print(F("  mres="));    Serial.println((int)tmc.mres());
    Serial.print(F("  toff="));    Serial.println((int)tmc.toff());
    Serial.print(F("  gconf=0x")); Serial.println(tmc.GCONF(), HEX);
    Serial.print(F("  cs_actual=")); Serial.println((int)tmc.cs_actual());

    // ---- PROBE SG2 : la carte de registres est-elle style 2209 (SG regulable) ? ----
    // TMC2209 : 0x40 = SGTHRS (seuil SG2, 8b), 0x41 = SG_RESULT (resultat analogique).
    // TMC2208 : ces adresses n'existent pas -> lecture en erreur CRC.
    // Le 2226 est le petit frere du 2209 : a trancher empiriquement ici.
    {
        uint32_t a40 = actPrb().rawRead(0x40);
        Serial.print(F("SG 0x40(SGTHRS?)=0x")); Serial.print(a40, HEX);
        Serial.print(F(" err=")); Serial.println(actPrb().CRCerror ? 1 : 0);
        uint32_t a41 = actPrb().rawRead(0x41);
        Serial.print(F("SG 0x41(SG_RESULT?)=0x")); Serial.print(a41, HEX);
        Serial.print(F(" err=")); Serial.println(actPrb().CRCerror ? 1 : 0);
    }

    digitalWrite(cfg.isY ? Y_PIN_ENABLE : ROT_PIN_ENABLE, LOW);   // moteur ON
    if (cfg.isY) digitalWrite(Y_BRAKE_PIN, HIGH);                 // = dev.cpp

    // PROBE 1 : la lecture est-elle encore vivante juste avant le home ?
    {
        uint32_t v = tmc.DRV_STATUS();
        Serial.print(F("PROBE pre-HOME drv=0x")); Serial.println(v, HEX);
        Serial.print(F("PROBE pre-HOME err=")); Serial.println(tmc.CRCerror ? 1 : 0);
    }

    Serial.println(F("HOME..."));
    if (!sg2_home()) return;

    // PROBE 2 : la lecture est-elle encore vivante apres le home ?
    {
        uint32_t v = tmc.DRV_STATUS();
        Serial.print(F("PROBE post-HOME drv=0x")); Serial.println(v, HEX);
        Serial.print(F("PROBE post-HOME err=")); Serial.println(tmc.CRCerror ? 1 : 0);
    }

    // Position de départ déterministe, puis phase LIBRE.
    stp.setCurrentPosition(0);
    stp.setMaxSpeed(cfg.speed);
    stp.setAcceleration(cfg.phaseAccel);
    stp.moveTo(cfg.freeTravel);

    tStart = millis();
    tPhase = millis();
    lastPrint = 0;
    lastSample = 0;
    evS2GA = evS2GB = evS2VSA = evS2VSB = 0;
    evOLA = evOLB = evRerr = 0;
    lastSR = 0;
    sgStatReset();
    phase = "LIBRE";
    Serial.println(F("PHASE LIBRE - 3s - NE PAS BLOQUER"));
}

static void sg2_loop() {
    actStp().run();
    unsigned long now = millis();

    // ---- echantillonnage SG2 @ ~20 ms : bits READ-CLEAR, on compte ----
    // Lecture UNIQUE de DRV_STATUS ici (le print 10 Hz reutilise lastSR).
    if (now - lastSample >= SG2_SAMPLE_MS) {
        lastSample = now;
        TMC2208Stepper& tmc = actTmc();
        uint32_t sr = tmc.DRV_STATUS();
        lastSR = sr;
        if (tmc.CRCerror) evRerr++;        // 1 = lecture ECHEC (timeout/CRC)
        if (sr & BIT_S2GA)  evS2GA++;
        if (sr & BIT_S2GB)  evS2GB++;
        if (sr & BIT_S2VSA) evS2VSA++;
        if (sr & BIT_S2VSB) evS2VSB++;
        if (sr & BIT_OLA)   evOLA++;
        if (sr & BIT_OLB)   evOLB++;
        // Resultat analogique SG (registre 0x41 s'il existe) : signal de contre-EMF.
        sgres = actPrb().rawRead(0x41);
        sgresOK = !actPrb().CRCerror;
        if (sgresOK) {
            int b = -1;
            if (phase[0] == 'L') {
                if (actStp().distanceToGo() != 0) b = 0;          // descente en cours
            } else if (phase[0] == 'B') {
                if (g_cfg->topOffset >= 0) {
                    if (now - tPhase < 3000)                       // fenetre push (~3 s)
                        b = (actStp().currentPosition() < g_bloqStart + g_cfg->topOffset) ? 1 : 2;
                } else if (actStp().distanceToGo() != 0) {
                    b = 1;                                          // ROT : push = mouvement complet
                }
            }
            if (b >= 0) {
                int v = (int)(sgres & 0x3FF);
                SgBuck& B = sgB[b];
                if (B.n == 0 || v < B.mn) B.mn = v;
                if (v > B.mx) B.mx = v;
                B.sum += v;
                B.n++;
            }
        }
    }

    // ---- print @ 10 Hz : instantane (pas de 2e lecture UART ici) ----
    if (now - lastPrint >= 100) {
        lastPrint = now;
        uint32_t sr = lastSR;
        // (pas de print en chaine : sur AVR print() retourne size_t)
        Serial.print("[");
        Serial.print(now - tStart);
        Serial.print("ms] drv=0x");
        Serial.print(sr, HEX);
        Serial.print(g_cfg->name);
        Serial.print(" ");
        Serial.print(phase);
        Serial.print(" s2ga=");
        Serial.print((sr & BIT_S2GA) ? 1 : 0);
        Serial.print(" s2gb=");
        Serial.print((sr & BIT_S2GB) ? 1 : 0);
        Serial.print(" s2vsa=");
        Serial.print((sr & BIT_S2VSA) ? 1 : 0);
        Serial.print(" s2vsb=");
        Serial.print((sr & BIT_S2VSB) ? 1 : 0);
        Serial.print(" ola=");
        Serial.print((sr & BIT_OLA) ? 1 : 0);
        Serial.print(" olb=");
        Serial.print((sr & BIT_OLB) ? 1 : 0);
        Serial.print(" cs=");
        Serial.print(GET_CS(sr));
        Serial.print(" stst=");
        Serial.print((sr & BIT_STST) ? 1 : 0);
        Serial.print(" stlth=");
        Serial.print((sr & BIT_STEALTH) ? 1 : 0);
        Serial.print(" | EVT s2ga/s2gb/s2vsa/s2vsb=");
        Serial.print(evS2GA); Serial.print("/");
        Serial.print(evS2GB); Serial.print("/");
        Serial.print(evS2VSA); Serial.print("/");
        Serial.print(evS2VSB);
        Serial.print(" ola/olb=");
        Serial.print(evOLA); Serial.print("/");
        Serial.print(evOLB);
        Serial.print(" sgres=");
        if (sgresOK) Serial.print((int)(sgres & 0x3FF)); else Serial.print("e");
        Serial.println();
    }

    // ---- machine à phases ----
    unsigned long el = now - tPhase;
    if (phase[0] == 'L' && el > 3000) {
        g_bloqStart = actStp().currentPosition();
        phase = "BLOQUEZ";
        tPhase = now;
        // Push CONTINU sur toute la phase : l'axe ne s'arrete pas, il faut
        // poser l'arret rigide sur sa trajectoire (ou saisir ROT a la main).
        actStp().moveTo(actStp().currentPosition() + g_cfg->pushTravel);
        Serial.println(F(">>> BLOQUEZ L'AXE (poussee continue 8 s) <<<"));
    } else if (phase[0] == 'B' && el > 8000) {
        sgBuckPrint(0, "LIBRE desce");
        sgBuckPrint(1, "REMONT libre");
        sgBuckPrint(2, "GRIND");
        phase = "RELACHEZ";
        tPhase = now;
        actStp().stop();
        Serial.println(F(">>> RELACHEZ L'AXE MAINTENANT <<<"));
    } else if (phase[0] == 'R' && el > 6000) {
        Serial.print(F("TEST SG2 FINI - EVT s2ga/s2gb/s2vsa/s2vsb="));
        Serial.print(evS2GA); Serial.print("/");
        Serial.print(evS2GB); Serial.print("/");
        Serial.print(evS2VSA); Serial.print("/");
        Serial.print(evS2VSB);
        Serial.print(F("  ola/olb="));
        Serial.print(evOLA); Serial.print("/"); Serial.print(evOLB);
        Serial.print(F("  rerr=")); Serial.println(evRerr);
        finishTest();
    }
}

// ===== Enveloppe publique (une entrée du registre par axe) =====
void test_sg2_y_setup()    { sg2_start(CFG_Y); }
void test_sg2_y_loop()     { sg2_loop(); }
void test_sg2_y_teardown() {
    sg2_stpY.stop();
    digitalWrite(Y_PIN_ENABLE, HIGH);   // moteur Y coupé
    Serial.println(F("MOTEUR Y COUPE"));
}

void test_sg2_rot_setup()   { sg2_start(CFG_ROT); }
void test_sg2_rot_loop()    { sg2_loop(); }
void test_sg2_rot_teardown(){
    sg2_stpRot.stop();
    digitalWrite(ROT_PIN_ENABLE, HIGH); // moteur ROT coupé
    Serial.println(F("MOTEUR ROT COUPE"));
}

// ===== Home Y seul (touche 'y') =====
// Amène Y au switch (haut) et MAINTIENT le moteur alimenté : le test ROT
// exige Y en haut. Le teardown ne coupe donc PAS l'alimentation de Y.
void test_home_y_setup() {
    g_cfg = &CFG_Y;
    pinMode(Y_PIN_STP, OUTPUT);
    pinMode(Y_PIN_DIR, OUTPUT);
    pinMode(Y_PIN_ENABLE, OUTPUT);
    digitalWrite(Y_PIN_ENABLE, HIGH);   // OFF d'abord
    pinMode(Y_ENDSTOP_PIN, INPUT);

    TMC2208Stepper& tmc = sg2_tmcY;
    tmc.begin();
    tmc.toff(5);
    tmc.rms_current(CFG_Y.rmsCurrent);
    tmc.microsteps(CFG_Y.microsteps);
    tmc.pwm_autoscale(true);
    tmc.en_spreadCycle(CFG_Y.spreadCycle);
    tmc.tbl(CFG_Y.tbl);
    tmc.hend(CFG_Y.hend);
    tmc.hstrt(CFG_Y.hstrt);
    delay(200);
    digitalWrite(Y_PIN_ENABLE, LOW);    // moteur ON
    digitalWrite(Y_BRAKE_PIN, HIGH);    // = dev.cpp

    Serial.println(F("HOME Y en cours..."));
    if (sg2_home()) {
        Serial.println(F("Y EN HAUT (switch) - moteur MAINTENU. Taper 2 = test ROT."));
        finishTest();   // teardown = maintien (ne coupe pas Y)
    }
    // erreur home : sg2_home() a déjà appelé finishTest()
}
void test_home_y_loop() { /* home bloquant dans setup (comme les tests Y/ROT) */ }
void test_home_y_teardown() {
    sg2_stpY.stop();
    // IMPORTANT : Y_PIN_ENABLE reste à LOW (moteur alimenté) — Y doit
    // rester maintenu en haut pendant le test ROT qui suit.
    Serial.println(F("Y maintenu en haut (moteur actif)."));
}
