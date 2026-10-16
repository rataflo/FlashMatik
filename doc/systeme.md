# FlashMatik — Description du système

> Document de référence, maintenu à la main (Markdown, modifiable).
> Source de vérité : le code (`src/`) et les mesures au banc. En cas de divergence, le code gagne — mettre à jour ce document.

---

## 1. Description du code

### 1.1 Arborescence du dépôt

```
FlashMatik/
├── src/
│   ├── SelfimatikV2/          FIRMWARE PRINCIPAL (Arduino Mega 2560)
│   │   ├── platformio.ini     cible megaatmega2560, libs TMCStepper + AccelStepper
│   │   ├── src/               le firmware (≈ 2800 lignes)
│   │   └── tests/             firmwares de banc (bench_runner, scissor_diag,
│   │                          scissor_bench, test_rotation) — README local
│   ├── raspi/                 CÔTÉ RASPBERRY PI (Python)
│   │   ├── main.py            orchestration machine (QUERY/START, paiement, GPIO)
│   │   ├── camera.py          prises de vue / effets
│   │   ├── cashlessMDB.py     bus MDB cashless
│   │   ├── config.json        prix, paiement (mdb on / pièces off)
│   │   ├── boothlauncher.sh   lancement en tâche de fond (nohup)
│   │   ├── resetCB.py, stresstest.py, testgpio.py, activatevenv.sh
│   │   └── old/               anciens main.py
│   └── DeliveryUnit/          firmware de l'unité de SORTIE PAPIER (Arduino UNO
│                              "exit board" : 1 stepper + LED, pins 8/9/7)
├── 3D/                        pièces 3D (dxf, models, stl)
├── design/                    graphisme (stickers, découpes DXF, PDF/SVG)
├── doc/                       présent document + schémas, manuel, PDF MKS
├── .beads/                    suivi des travaux (bd)
└── AGENTS.md / CLAUDE.md      conventions agent (mode conservateur)
```

### 1.2 Firmware Mega — `src/SelfimatikV2/src/`

| Fichier | Lignes | Rôle |
|---|---|---|
| `main.cpp` | 561 | Séquence de démarrage (LCD → drivers TMC → home des axes), boucle principale, dialogue série avec le Pi |
| `dev.cpp` | 514 | Axes ROT/Y (+ frein, séchoir) : init TMC **identique pour tous les drivers** (`rms_current`, `microsteps`, `pwm_autoscale`, `en_spreadCycle`, `tbl/hend/hstrt`), home, déplacements |
| `lcd.cpp` | 390 | LCD 20×4 + encodeur rotatif, menus de calibration |
| `shutter.cpp` | 255 | Obturateur |
| `parameters.cpp` | 236 | **EEPROM** : structure `storage` (≈ 50 paramètres), migrations `checkCode` (v6→v10), `dumpParameters()` au boot (attend `TXC0`, pas de `delay`) |
| `paper.cpp` | 177 | Avance / coupe / sortie pellicule |
| `scissor.cpp` | 85 | Cisaille : **pas complets** (`microsteps(0)`), `rms_current(1900)` (plafonné 1,77 A), course 220 pas |
| `constants.h` | 278 | **Tous** les pins, vitesses, courses, structure `storage`, enums d'états |
| `tests.cpp` | 33 | Mode test interne |

Deux machines à états pilotent la machine (définies dans `constants.h`) :
- `PhotoState` : compteur → prise de vue → avance papier → coupe → sortie…
- `DevState` : rotation cuve → descente → agitation → sortie → séchage…

### 1.3 Côté Pi — `src/raspi/`

- **`main.py`** : point d'entrée. Ouvre `/dev/flashmatik` à 115200 (symlink udev,
  doc `doc/Raspi_udev_arduino.odt`), dialogue `QUERY` → `START`/`NO_START`,
  gère cashless MDB (`cashlessMDB.py`) et/ou pièces (impulsions GPIO 14),
  log dans `log.txt`.
- **`camera.py`** : acquisition photo.
- **`config.json`** : `price: 1000` (10 €), `cashless_mdb: true`, `coin_acceptor: false`.
- **Déploiement sur la machine** (cf. `doc/Maj_firmware_arduino.txt`) :
  le Pi vit dans `/home/minutepapillons/photomaiton/` ; le firmware est flashé
  **depuis le Pi** avec avrdude (`-c wiring -P /dev/flashmatik`), puis
  `boothlauncher.sh` relancé en `nohup`.

### 1.4 Firmwares de test — `src/SelfimatikV2/tests/`

Projet PlatformIO séparé par test, qui **partagent `constants.h` du firmware**
(`-I../../src`) pour reproduire l'init TMC à l'identique :
- **`bench_runner/`** : dispatch par série — `1` = SG2 axe Y, `2` = SG2 axe ROT, `s` = stop.
- **`scissor_diag/`**, **`scissor_bench/`** : diagnostic et sweep cisaille.
- **`src/test_rotation.cpp`** : mesure du carrousel ROT (T = 4940 pas/tour).

---

## 2. Vue globale

FlashMatik est une **cabine photo automatique** (self-service) : le client se place,
la machine expose, tire la pellicule photo, la coupe, la sèche et la restitue.

```
┌────────────────────────────┐   USB 115200            ┌─────────────────────────────┐
│   RASPBERRY PI (supervision)│ ◄──────────────────────► │  CARTE PRINCIPALE MKS GEN_L V2.1 │
│                            │  /dev/flashmatik (CH340)  │  (Arduino MEGA 2560 intégré)     │
│  main.py   : orchestration │   (connecteur optique     │   firmware SelfimatikV2         │
│  camera.py : prises de vue │    d'isolement entre les  │                             │
│  cashless  : paiement MDB  │    deux cartes)           │  axes : ROT, Y, PAPER,        │
│  pièces    : GPIO 14       │                           │  SCISSOR, SHUTTER + servo,     │
│  caméra nuit (dev, HDMI)   │                           │  flash, pré-éclairage, LCD     │
└────────────────────────────┘                           └───────┬─────────────┬─────────┘
                                             UART 115200 (SW)   │             │ 24V DC
                                                                ▼             ▼
                                                   TMC2226 x5 (X/Y/Z/E0/E1)  alimentations :
                                                                ▼             24V / 12V / 5V DC
                                                        moteurs + capteurs

+ carte secondaire "exit board" (ARDUINO UNO, firmware DeliveryUnit) :
  sortie papier fin de développement (stepper UNO D4), alimentée depuis la
  carte principale (24V + 5V). Séparée électriquement du reste.
```

- **Pi** : caméra, paiement, séquence photo, dialogue client.
- **Mega** : toute la mécanique en temps réel.
- **Drivers** : 5 × TMC2226 en UART ; le firmware n'en **écrit** qu'au boot
  (fire-and-forget) et **ne les lit pas** en production — la voie lecture
  n'est exploitée qu'au banc (cf. `FlashMatik-ahq.1`).

## 3. Mécanique — les axes

| Axe | Rôle | Moteur | Recadrage (home) | Valeurs clés |
|---|---|---|---|---|
| **ROT** | Carrousel de cuves (**7 bras / 14 cuves** machine actuelle ; manuel V1.0 = 6 bras / 12 bacs) | NEMA17 | Opto (pin 6) à chaque tour impair | 4940 pas/tour ; 353 (pair) / 360 (impair, valeur machine) pas/position |
| **Y** | Carrier : descente sous bac, agitation, remontage | NEMA17 | Butée (pin 3) + frein électromécanique (pin 7) | 1555 pas pleine course ; 175 pas d'agitation ; 102 pas en sortie |
| **PAPER** | Avance pellicule (boîte papier), train 20T/60T (×3/2), synchronisée par courroie avec la caméra | NEMA17 | Opto papier (pin 18) | 1797 / 3651 / 5361 pas (bande 20,0 cm, série 20/20,3 — EEPROM) |
| **SCISSOR** | **Lame de cutter** actionnée par stepper (feed down) | **NEMA34 2,1 A / 65 Ncm**, courroie 40T→20T (×2) | Butée (pin 15) | **220 pas complets** = course machine ; MRES=8 forcé |
| **SHUTTER** | Obturateur (temps d'exposition) | NEMA17 | Microswitch (pin 14) | 215 pas/tour |
| **EXIT** | Sortie papier fin de développement — **Arduino UNO** (DeliveryUnit) | NEMA17 (UNO D4) | — | cf. `src/DeliveryUnit/` |
| **SERVO** | **Ouverture / fermeture des bras** (pin 11) | Servo | Positions EEPROM | idle/open/close (10…140) |

Autres éléments :
- **Bacs de chimie** : 3 bacs chauffés à 35 °C (résistances aquarium, pin 12, 10 s),
  séjour en cuve pair/impair (25/20 s), goutte à goutte (5 s) — EEPROM.
- **Capteurs** : butée Y (3), butée shutter (14), butée cisaille (15), opto ROT (6),
  opto papier (18), capteur sortie (2), bouton start (19).
- **Éclairage photo** : **3 flashs** (2 haut + 1 bas) — flash principal (pin 45, via
  petit circuit transistor) + pré-éclairage RGB (100/25/12 ms) + matrice 6 pixels (SPI).

## 4. Matériel — modèles

| Composant | Modèle / référence |
|---|---|
| Carte principale | **MKS GEN_L V2.1** (schémas PDF dans `doc/`) : intègre le Mega 2560 (USB CH340, COM14), les 5 drivers et les sorties steppers X/Y/Z/E0/E1 — alimentée en **24 V DC** |
| Unité sortie papier | **Arduino UNO** ("exit board", firmware DeliveryUnit) |
| Supervision | **Raspberry Pi** (modèle : à préciser), câblé en USB à la carte principale via connecteur optique d'isolement |
| Drivers stepper | **TMC2226** DIP-8, R_SENSE 110 mΩ, UART (cœur de registres TMC2208) |
| Courant max / driver | IRUN=31 → **1,77 A RMS / 2,5 A peak** (R_SENSE 0,11) |
| Moteurs axes | **NEMA17** (`doc/nema17.jpg`) sauf cisaille |
| Moteur cisaille (cutter) | **NEMA34, 2,1 A / 65 Ncm** |
| Affichage local | LCD **20×4** + encodeur rotatif |
| Paiement | Cashless **MDB** (Pi) ; accepte-pièces impulsions (GPIO 14, option) |
| Caméras | Prise de vue (`camera.py`) + **caméra de nuit** sur le développement (affichable en HDMI à l'arrière) — modèles : à préciser |
| Alimentation | Convertisseur 110/220 V ; 110 V AC → **24 V DC** ; 110 V AC → **12 V + 5 V DC** |
| Câblage / schémas | `doc/WiringDetails.ods` (câbles 0-20), `doc/Flashmatik_schematic.odg`, manuel `doc/FlashMatik_Manuel.odt` V1.0 |

**UART TMC2226 (Mega ↔ driver)**

| Axe | Mega TX→DI | Mega RX←DO | STEP | DIR | ENABLE |
|---|---|---|---|---|---|
| ROT | A5 | A10 | A6 | A7 | A2 |
| Y | 40 | A9 | A0 | A1 | 38 (+ frein 7) |
| PAPER | 20 | 12 | 36 | 34 | 30 |
| SCISSOR | 42 | A11 | 46 | 48 | A8 |
| SHUTTER | 44 | A12 | 26 | 28 | 24 |

## 5. Calibration (EEPROM)

- `checkCode` **10** ; dump complet au boot (`dumpParameters()`).
- Valeurs machine actuelles : bande `1797/3651/5361` (série 20/20,3 cm),
  `scissorStepOpened=220`, `rotStepPair=353`, `rotStepImpair=360` (volontaire),
  cisaille 8000/9000 (coupe ≈ 311 ms au banc).
- Menu LCD pour saisir/calibrer ; les `constants.h` ne sont que les **défauts**.

## 6. Autres

- **Tests banc** : `tests/README.md` (protocoles). État en cours : identification SG2
  des TMC2226 (beads `FlashMatik-ahq` + bug `FlashMatik-ahq.1` : lectures DRV_STATUS
  mortes au mouvement — sondes instrumentées prêtes à flasher).
- **3D / graphisme** : `3D/` (pièces imprimées), `design/` (stickers, DXF, PDF).
- **Docs de référence** : `doc/` — manuel `FlashMatik_Manuel.odt` **V1.0** (description,
  mise en service ; ⚠️ daté : décrit 6 bras / 12 bacs, la machine actuelle en a 7/14),
  schéma ODG, PDF MKS, `WiringDetails.ods` (câbles 0-20),
  `Maj_firmware_arduino.txt` (maj firmware depuis le Pi : avrdude `-c wiring`),
  `Raspi_udev_arduino.odt` (symlink `/dev/flashmatik`).
- **Conventions** : `AGENTS.md` / `CLAUDE.md`.
- **À préciser** : modèle de Pi, modèle(s) caméra, modèle du mécanisme de paiement MDB.
