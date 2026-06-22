#!/usr/bin/env python3
# flashmatik_auto.py – Déclenchement aléatoire toutes les 10 minutes max

import serial
import time
import random
import logging
import sys

# Configuration
ARDUINO_PORT = "/dev/flashmatik"    # ou "/dev/ttyUSB0"
BAUDRATE = 115200

# Protocole
CMD_QUERY = "QUERY"
REPLY_START = "START"
REPLY_NO_START = "NO_START"

# Logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)

class RandomPhotoTrigger:
    def __init__(self):
        self.ser = None
        self.photo_requested = False   # True quand il faut déclencher
        self.photo_in_progress = False # True après envoi de START, jusqu'à DONE
        self.last_done_time = None
        self.next_delay = 0

    def connect(self):
        """Connexion série à l'Arduino avec reconnexion automatique"""
        while True:
            try:
                self.ser = serial.Serial(
                    port=ARDUINO_PORT,
                    baudrate=BAUDRATE,
                    timeout=0.1,
                    dsrdtr=False
                )
                time.sleep(1)               # laisse l'Arduino démarrer
                self.ser.reset_input_buffer()
                logging.info(f"Connecté à {ARDUINO_PORT}")
                return True
            except serial.SerialException as e:
                logging.error(f"Impossible de se connecter: {e}")
                logging.info("Nouvel essai dans 2 secondes...")
                time.sleep(2)

    def schedule_next(self):
        """Programme un délai aléatoire entre 0 et 600 secondes (10 min)"""
        self.next_delay = random.uniform(0, 600)
        self.last_done_time = time.time()
        logging.info(f"Prochaine photo dans {self.next_delay:.1f} secondes")

    def check_trigger(self):
        """Vérifie si l'heure de déclencher est arrivée"""
        if self.photo_requested or self.photo_in_progress:
            return
        if self.last_done_time is None:
            self.schedule_next()
            return
        elapsed = time.time() - self.last_done_time
        if elapsed >= self.next_delay:
            self.photo_requested = True
            logging.info("Demande de déclenchement – en attente du prochain QUERY")

    def send_start(self):
        """Envoie START à l'Arduino et bascule en mode 'en cours'"""
        self.ser.write((REPLY_START + "\n").encode())
        self.photo_requested = False
        self.photo_in_progress = True
        logging.info("START envoyé, séquence photo en cours")

    def send_no_start(self):
        self.ser.write((REPLY_NO_START + "\n").encode())

    def handle_serial(self):
        """Lit les messages de l'Arduino et agit"""
        if not self.ser or not self.ser.in_waiting:
            return
        try:
            line = self.ser.readline().decode().strip()
        except UnicodeDecodeError:
            return

        if line == CMD_QUERY:
            if self.photo_requested:
                self.send_start()
            else:
                self.send_no_start()

        elif line == "DONE":
            logging.info("Arduino a terminé la séquence")
            self.photo_in_progress = False
            self.schedule_next()

        elif line == "ERROR":
            logging.error("Arduino a signalé une erreur")
            self.photo_in_progress = False
            self.schedule_next()  # on réessaie plus tard

    def run(self):
        """Boucle principale"""
        self.connect()
        self.schedule_next()
        logging.info("Démarrage du déclencheur aléatoire (max 10 min)")

        while True:
            self.check_trigger()
            self.handle_serial()
            time.sleep(0.05)   # économie CPU

def main():
    trigger = RandomPhotoTrigger()
    try:
        trigger.run()
    except KeyboardInterrupt:
        logging.info("Arrêt demandé par l'utilisateur")
        sys.exit(0)
    except Exception as e:
        logging.exception("Erreur fatale")
        sys.exit(1)

if __name__ == "__main__":
    main()