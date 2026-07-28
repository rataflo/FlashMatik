from ast import arg
from turtle import position
import os
import time
import datetime
from datetime import date
import sys
from tempfile import mktemp
import logging
from pathlib import Path
import uuid
import serial
from serial.tools import list_ports
import json
from io import BytesIO
import argparse
import traceback
import base64
#import usb.core
#import db
import random
import threading
import RPi.GPIO as GPIO
import tm1637


# Pins def
COIN_PIN = 14

NOM_BOOTH = os.uname().nodename

# Port série de l’Arduino (MKS Gen L)
ARDUINO_PORT = "/dev/flashmatik"
ARDUINO_BAUDRATE = 115200
arduino_ser = None

CMD_QUERY = "QUERY"
REPLY_START = "START"
REPLY_NO_START = "NO_START"

# constants
COINS_MULTI = 50
COINS = 0
WAITFORSTART = True
START = False

#Globals
CONFIG = type('Config', (), {})()
CONFIG.price = 0
CONFIG.cashless_mdb = False
NBPIECES = 0
SCROLL_TEXT = "    Please insert coin    "
SCROLL_POS = 0

global CURRENT_PATH

#cashless MDB
import cashlessMDB
import queue
global mdb_manager, payment_events, cb_transaction_requested, cb_transaction_active
mdb_manager = None
payment_events = queue.Queue()
cb_transaction_requested = False  # Flag pour demander une transaction
cb_transaction_active = False     # Flag pour transaction en cours

#COINS
PULSE_TIMEOUT = 200  # Délai en ms pour considérer la fin d'une série
last_pulse_time = 0  # Timestamp de la dernière impulsion
pulse_count = 0      # Compteur d'impulsions pour la série en cours

CURRENT_PATH = os.path.dirname(os.path.abspath(__file__))
LOG_FILENAME = Path(CURRENT_PATH, "log.txt")
logging.basicConfig(
    format="%(asctime)s %(levelname)s %(message)s",
    filename=LOG_FILENAME,
    level=logging.DEBUG,
    datefmt="%Y-%m-%d %H:%M:%S",
)
#output sur console
console = logging.StreamHandler()
console.setLevel(logging.DEBUG)
format=logging.Formatter("%(asctime)s %(levelname)s %(message)s")
console.setFormatter(format)
logging.getLogger("").addHandler(console)

def connect_arduino():
    global arduino_ser
    while True:
        try:
            ser = serial.Serial(
                port=ARDUINO_PORT,
                baudrate=ARDUINO_BAUDRATE,
                timeout=0.1,
                dsrdtr = False
            )
            # Assure que DTR reste à 0
            #ser.dtr = False
            #ser.rts = False
            time.sleep(0.5)   # laisse le temps à l'Arduino de démarrer sans reset
            ser.reset_input_buffer()
            logging.info(f"Arduino connecté sur {ARDUINO_PORT} (reset évité)")
            return ser
        except serial.SerialException as e:
            logging.error(f"Échec connexion: {e}, nouvel essai dans 2s")
            time.sleep(2)

def safe_arduino_write(data):
    global arduino_ser
    try:
        arduino_ser.write(data)
    except (serial.SerialException, OSError):
        logging.warning("Connexion Arduino perdue. Reconnexion...")
        arduino_ser = connect_arduino()
        arduino_ser.write(data)  # réessaie

def mdb_callback_handler(event, data):
    """
    Callback ultra-simplifié
    """
    payment_events.put({
        'event': event,
        'data': data,
        'timestamp': time.time()
    })

def process_mdb_events():
    """
    Traiter les événements MDB
    """
    global START, COINS, cb_transaction_active
    
    while not payment_events.empty():
        try:
            event_data = payment_events.get_nowait()
            event = event_data['event']
            
            if event == 'PAYMENT_APPROVED':
                # SEULEMENT CES 3 LIGNES
                START = True
                COINS = 0
                cb_transaction_active = True  # Transaction validée
                TM1637.show(str("busy"), colon=False)
                logging.info("[MDB] Paiement accepté - Prêt pour START")
                
            elif event == 'PAYMENT_FAILED':
                logging.warning("[MDB] Paiement échoué")
                cb_transaction_active = False  # Réinitialiser
            
            payment_events.task_done()
            
        except queue.Empty:
            break

def coin_interrupt(channel):
    global COINS, COINS_MULTI, START, TM1637, NBPIECES
    global pulse_count, last_pulse_time, PULSE_TIMEOUT
    
    # Sauvegarder le moment de cette impulsion
    current_pulse_time = time.monotonic() * 1000

    while GPIO.input(COIN_PIN):
        if (time.monotonic() * 1000) - current_pulse_time > PULSE_TIMEOUT:
            return
    endMillis = time.monotonic() * 1000
    if endMillis - current_pulse_time > 0.025 and  endMillis - current_pulse_time < 0.07 and endMillis - current_pulse_time < PULSE_TIMEOUT:
        logging.info(f"Impulsion détectée, durée: {endMillis - current_pulse_time} ms")

        if(current_pulse_time - last_pulse_time > PULSE_TIMEOUT):
            pulse_count = 1  # Nouvelle série d'impulsions
        else:
            pulse_count += 1  # Même série, incrémenter le compteur

        last_pulse_time = current_pulse_time

        #COINS = COINS - COINS_MULTI
        #NBPIECES = NBPIECES +1
        if not START and pulse_count >=4 :
            START = True
            COINS = 0
            TM1637.show(str("busy"), colon=False)
            logging.info("COINS")
    
 
def initCashlessMDB():
    global mdb_manager
    #cashless MDB
    logging.info("initCashlessMDB")
    try:
        mdb_manager = cashlessMDB.MDBManager(
            port='/dev/ttyACM0',
            debug_mode=False,
            callback=mdb_callback_handler
        )
        
        if mdb_manager.start():
            logging.info("Manager MDB démarré")
        else:
            logging.error("Échec démarrage MDB")
            mdb_manager = None
    except Exception as e:
        logging.error(f"Erreur MDB: {e}")
        mdb_manager = None       
    
def initPhotobooth():
    logging.info("initFlashmatik")
    global TM1637
    global CONFIG, arduino_ser
    
    logging.info("initGPIO")
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(COIN_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.add_event_detect(COIN_PIN, GPIO.FALLING, callback=coin_interrupt)
    
    #7 segment
    logging.info("initSegment")
    TM1637 = tm1637.TM1637(clk=15, dio=18)
    TM1637.brightness(1)
    TM1637.show(str("COIN"), colon=True)
    
    arduino_ser = connect_arduino()

    #init config
    initConfig()
    applyConfig()


def initConfig():
    logging.info("initConfig")
    global CONFIG, START, COINS
    global CURRENT_PATH

    #config json
    parser = argparse.ArgumentParser(description="Read config file path")
    parser.add_argument(
        "--config",
        type=str,
        default=Path(CURRENT_PATH, "config.json"),
        help="Path to the configuration file"
    )
    args = parser.parse_args()

    with open(args.config, "r") as f:
        JSON = json.load(f)

    #load config depuis json,
    CONFIG.price = JSON["price"]
    CONFIG.cashless_mdb = JSON.get("cashless_mdb", False)
    logging.info(f"Config loaded: price={CONFIG.price}, cashless_mdb={CONFIG.cashless_mdb}")

# Applique la configuration sur la machine       
def applyConfig():
    global CONFIG, COINS, TM1637, NBPIECES

    COINS = CONFIG.price

    #init cashless MDB
    if CONFIG.price == 0:
        CONFIG.cashless_mdb = False
    if CONFIG.cashless_mdb:
        logging.info("attente démarrage lecteur CB")
        #time.sleep(180) #Attente 3mn démarrage lecteur 4G du lecteur CB
        initCashlessMDB()

    if CONFIG.price == 0:
        TM1637.show("Free", colon=False)
    else:
        TM1637.show("COIN", colon=False)
    

def scroll_display():
    """
    Affiche les caractères du texte de défilement sur l'affichage 4-digits
    """
    global SCROLL_TEXT, SCROLL_POS, TM1637
    
    # Extraire 4 caractères du texte à partir de la position courante
    display_text = (SCROLL_TEXT + "    ")[SCROLL_POS:SCROLL_POS + 4]
    
    # Afficher sur le segment
    TM1637.show(display_text, colon=False)
    
    # Incrémenter la position et boucler
    SCROLL_POS = (SCROLL_POS + 1) % len(SCROLL_TEXT)


def main():

    global COINS, TM1637, NBPIECES, START
    global mdb_manager, cb_transaction_active

    try:
        currTime = time.time()
        lastRefreshConfig = currTime
        lastRefresh = currTime
        lastNbPieces = 0
        logging.info(f"Démarrage OK")

        # Boucle de la mort
        while True:
            currTime = time.time() 

            if lastNbPieces != NBPIECES:
                #logging.info(NBPIECES)
                lastNbPieces = NBPIECES
                
            #refresh Segment with scrolling text
            if CONFIG.price > 0 and currTime - lastRefresh >= 0.3:
                scroll_display()
                lastRefresh = currTime
                
            #check fichier de démarrage.
            if os.path.exists("start.txt"):
                os.remove("start.txt")
                START = True

            # Traiter les événements MDB
            if CONFIG.cashless_mdb:
                process_mdb_events()

                if not cb_transaction_active:
                    if mdb_manager and mdb_manager.initialized:
                        # Lancer la transaction
                        amount_eur = CONFIG.price / 100
                        logging.info(f"Lancement transaction CB: {amount_eur}€")
                        success = mdb_manager.start_payment(amount_eur)
                        if success:
                            cb_transaction_active = True  # Transaction lancée
                            logging.info("Transaction CB lancée")
                        else:
                            logging.error("Échec lancement transaction")
                            cb_transaction_active = False
                    else:
                        logging.error("Manager MDB non disponible")
                        initCashlessMDB()
                        cb_transaction_active = False

            if arduino_ser and arduino_ser.in_waiting:          
                line = arduino_ser.readline().decode().strip()
                if line == CMD_QUERY:
                    
                    if START:
                        # Un paiement est en attente
                        logging.info("Arduino demande un start - envoi START")
                        safe_arduino_write((REPLY_START + "\n").encode())

                elif line == "DONE":
                    # La séquence est terminée
                    logging.info("Arduino valide le start")
                    # Valider le paiement CB si nécessaire
                    if CONFIG.cashless_mdb and mdb_manager and cb_transaction_active:
                        logging.info("Validation transaction CB")
                        mdb_manager.confirm_service()
                        cb_transaction_active = False
                    # Réinitialiser START et le prix
                    START = False
                    COINS = CONFIG.price
                elif line == "ERROR":
                    logging.error("Arduino a signalé une erreur")
                    # Annuler la transaction CB si active
                    if CONFIG.cashless_mdb and mdb_manager and cb_transaction_active:
                        mdb_manager.cancel_service()
                        cb_transaction_active = False
                    START = False
                    COINS = CONFIG.price
                else:
                    logging.info("Arduino: %s", line)
                    
                
    except KeyboardInterrupt:
        logging.info("Received keyboard interrupt, shutting down")
    except Exception as e:
        logging.error(f"Fatal error in main: {str(e)}")
        raise
            
    return 0

if __name__ == "__main__":
    try:
        print("START")
        initPhotobooth()
        logging.info("Init OK")
        main()
    except Exception as e:
        logging.error(f"Unexpected error: {str(e)}")
        logging.error(traceback.format_exc())
    finally:
        GPIO.cleanup()
        if CONFIG.cashless_mdb and mdb_manager and cb_transaction_active:
            logging.info("Annulation transaction MDB (shutdown)")
            mdb_manager.cancel_service()
        if CONFIG.cashless_mdb and mdb_manager:
            mdb_manager.stop()
        logging.info("Stop")
