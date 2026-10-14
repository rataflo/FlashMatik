// =====================================================================
// BENCH RUNNER — un seul firmware, plusieurs tests lancés au serial.
//
// Architecture :
//   - Un test = un module (tests/test_*.h/.cpp) qui implémente
//     test_XXX_setup() / test_XXX_loop() / test_XXX_teardown().
//   - Un seul test actif à la fois ; seul son axe est alimenté.
//   - 's' (stop) fonctionne TOUJOURS : teardown() est idempotent
//     et coupe le moteur.
//   - test_XXX_loop() doit rester non bloquant (pas de delay > 10 ms).
//
// Ajouter un test :
//   1) créer tests/test_XXXX.{h,cpp}
//   2) ajouter une ligne au registre tests[] ci-dessous
//   3) ajouter son caractère dans handleKey()
// =====================================================================
#include <Arduino.h>
#include "tests/test_sg2_ident.h"

// ===== Registre des tests =====
struct Test {
    const char* name;
    void (*setup)();
    void (*loop)();
    void (*teardown)();
};

static Test tests[] = {
    {"sg2_ident", test_sg2_ident_setup, test_sg2_ident_loop, test_sg2_ident_teardown},
    // Les autres axes (Y, ROT) = une ligne de plus ici quand on les ajoutera.
};
static int active = -1;

// À appeler par un test pour se terminer proprement (teardown + désactivation).
void finishTest() {
    if (active >= 0) {
        tests[active].teardown();
        active = -1;
    }
}

static void startTest(int i) {
    if (active >= 0) finishTest();
    active = i;
    Serial.print(F("=== START TEST : "));
    Serial.println(tests[active].name);
    tests[active].setup();
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("=== BENCH RUNNER ==="));
    Serial.println(F("1 = sg2_ident (SCISSOR) | s = stop | h = aide"));
}

void loop() {
    if (active >= 0) tests[active].loop();

    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 's':
            case 'S':
                finishTest();
                Serial.println(F("STOP - moteurs coupes."));
                break;
            case '1':
                startTest(0);
                break;
            case 'h':
                Serial.println(F("1 = sg2_ident (SCISSOR) | s = stop"));
                break;
        }
    }
}
