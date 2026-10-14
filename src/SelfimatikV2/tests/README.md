# Tests firmware SelfimatikV2

Programmes de test unitaires a flasher directement sur la Mega 2560
(port serie du spider). Chaque test est un projet PlatformIO autonome.

## test_rotation.cpp — mesure du pas entre deux bras (spider 7 bras / 14 tanks)

Mesure au banc le nombre de pas entre deux lamelles (bras) successifs,
puis calcule les valeurs fiables pour la rotation 14 positions :

- `T` = pas par revolution
- `S` = pas entre deux lamelles (T/7)
- `rotStepPair` (X_ROTATE_PAIR) = S/2 arrondi
- `rotStepImpair` (X_ROTATE_IMPAIR) = S - NB_STEP_CENTER_ARM - rotStepPair + marge

Seule l'axe ROT est pilotee (memorisation d'init TMC identique au
firmware). Le test attend 30 s avant de bouger, puis :
home sur la lamelle (opto), recentrage (+25 pas comme `initRot()`),
mesure des 7 intervalles lamelle->lamelle. A la fin le bras est centre
sur une lamelle (memorisation = fin de `initRot()`), pret a reflasher
le firmware principal.

### Utilisation

    pio run -d tests -t upload     # compile + flash (COM14)
    # le test se lance tout seul, sortie sur le moniteur serie 115200
    # taper 'r' dans le moniteur pour re-lancer sans reflasher

### Derniere mesure (2026-07-31)

| Grandeur | Valeur |
|---|---|
| T (pas/tour) | 4940 |
| S (pas/bras) | 705.7 (intervalles 694..715) |
| S/2 (pas/position) | 352.86 |
| X_ROTATE_PAIR | **353** |
| X_ROTATE_IMPAIR | **400** (min 368) |

Applique dans `src/constants.h` + migration EEPROM `checkCode v6`
(`src/parameters.cpp`).

## scissor_diag/ — diagnostic UART du driver scissor (TMC2226)

Verifie que le driver repond sur son UART (GCONF, GSTAT, DRV_STATUS)
avant d'envisager tout reglage de courant/micro-pas.

    cd tests/scissor_diag && pio run -t upload

## scissor_bench/ — reglages vitesse/accel de la cisaille

Balaye des couples (vitesse, accel) en pas complets et chronometre la
course d'ouverture (SCISSOR_STEP_OPENED). Permet de choisir le couple
plus rapide sans a-coup.

    cd tests/scissor_bench && pio run -t upload
    # 115200, taper '1'..'4' pour choisir le profil, 'h' = aide

## bench_runner/ — runner de tests generaliste (prefere pour les nouveaux tests)

Un seul firmware, plusieurs tests : chaque test est un module
`src/tests/test_*.h/.cpp` avec `setup()/loop()/teardown()`, lance au
serial (`'1'`, `'2'`, ...). `'s'` coupe les moteurs a tout moment.
Les pins/R_SENSE partagent le `constants.h` du firmware principal
(`-I../../src` dans `platformio.ini`).

- `test_sg2_ident` — identification des bits StallGuard2 du TMC2226 :
  home, course libre, puis prompts `BLOQUEZ` / `RELACHEZ` sur le
  chariot scissor. Printe toutes les 100 ms une lecture brute unique de
  DRV_STATUS (les bits SG2 sont read-clear) : s2vsa/s2vsb/s2ga/s2gb/ot/cs/stst.
  Livrables : quel bit s'arme au stall, sticky ou non, baseline ot.

    cd tests/bench_runner && pio run -t upload
    # 115200, '1' = sg2_ident, 's' = stop, 'h' = aide
