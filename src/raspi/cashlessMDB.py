'''
Python Script to interface MDB-USB as master with cashless reader as slave.
Adapté pour fonctionner en tant que module thread-safe avec callback.

Usage:
    manager = MDBManager(callback=ma_fonction_callback)
    manager.start()
    manager.start_payment(1.50)  # Montant en euros
'''
import serial
import time
import sys
import os
import threading
import queue
import logging

VEND_TIMEOUT = 10  # SECONDES

# Variables globales
ser = None
debug = True
args = None


def usb_replug(tty="ttyACM0"):
    """Débranche/rebranche le périphérique USB via sysfs (nécessite root).
    Pour un device bus-powered, c'est équivalent à un débranchement physique :
    reset complet du MCU de l'adaptateur.

    Garde-fou : ne JAMAIS retirer le device si le rescan n'est pas accessible,
    sinon le MCU reste orphelin jusqu'à une coupure d'alimentation."""
    try:
        dev = os.path.realpath(f"/sys/class/tty/{tty}")
        # Monter dans l'arbo jusqu'au dossier du device USB qui contient "remove"
        # (le nombre de niveaux intermédiaires varie selon le kernel, ex. dossier "tty" sur Pi 5)
        d = os.path.dirname(dev)
        for _ in range(5):
            if os.path.exists(f"{d}/remove"):
                break
            d = os.path.dirname(d)
        else:
            logging.error(f"usb_replug: dossier 'remove' introuvable depuis {dev}")
            return False

        remove_f = f"{d}/remove"
        rescan_f = "/sys/bus/usb/drivers/usb/rescan"

        # Test des droits AVANT de retirer (open en écriture sans écrire)
        test = open(rescan_f, "w")
        test.close()
        test = open(remove_f, "w")
        test.close()

        with open(remove_f, "w") as f:
            f.write("1")
        time.sleep(2)
        with open(rescan_f, "w") as f:
            f.write("1")
        time.sleep(3)
        logging.info(f"usb_replug({tty}): périphérique rebranché")
        return True
    except Exception as e:
        logging.error(f"usb_replug({tty}): {e}")
        return False


class MDBManager:
    """Manager thread-safe pour les transactions MDB"""

    def __init__(self, port='/dev/ttyACM0', debug_mode=False, callback=None):
        """
        Initialiser le manager MDB

        Args:
            port: Port série (défaut: /dev/ttyACM0)
            debug_mode: Mode debug
            callback: Fonction callback appelée avec (event, data)
                     event: 'PAYMENT_APPROVED', 'PAYMENT_FAILED'
                     data: Montant de la transaction
        """
        self.port = port
        self.debug = debug_mode
        self.callback = callback
        self.payment_queue = queue.Queue()
        self.running = False
        self.worker_thread = None
        self.starting = False
        self.current_transaction = None
        self.initialized = False
        # Set True only by stop(): _init_devices uses this to bail out of a long
        # wait loop when a stop arrives, NOT `starting` (which is False whenever a
        # payment re-initialises the device after boot — that used to abort every
        # payment with "Initialisation annulée", flashback 2026-09-21).
        self._abort_requested = False

        # Configuration du logging
        self.logger = logging.getLogger('MDBManager')
        if not self.logger.handlers:
            handler = logging.StreamHandler()
            formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
            handler.setFormatter(formatter)
            self.logger.addHandler(handler)
            self.logger.setLevel(logging.DEBUG if debug_mode else logging.WARNING)

    def _run_with_watchdog(self, func, timeout, label):
        """Exécuter func dans un thread démon ; lever une exception si ça dépasse timeout.
        Le thread bloqué est abandonné (démon : ne bloque pas la fin du process)."""
        box = {}

        def target():
            try:
                func()
            except Exception as e:
                box['error'] = e

        t = threading.Thread(target=target, daemon=True)
        t.start()
        t.join(timeout)
        if t.is_alive():
            self.logger.error(f"Timeout {timeout}s : {label} bloquée (thread abandonné)")
            raise Exception(f"Timeout {label}")
        if 'error' in box:
            raise box['error']

    def start(self):
        """Lancer l'initialisation en arrière-plan (non bloquant).
        Regarder self.initialized pour savoir quand c'est prêt,
        self.starting pour savoir si un essai est en cours."""
        if self.running:
            return True
        if self.starting:
            return False
        self.starting = True
        t = threading.Thread(target=self._start_bg, daemon=True, name="MDB-Init")
        t.start()
        return False

    def _start_bg(self):
        """Corps de l'initialisation, dans un thread démon."""
        global ser, debug
        debug = self.debug
        ser = None
        self._abort_requested = False
        try:
            ser = serial.Serial(port=self.port, baudrate=115200, timeout=1)
            ser.reset_input_buffer()

            try:
                # Watchdog : un os.write peut bloquer indéfiniment si le MCU
                # ne vide plus son RX USB (état résiduel après kill/reboot).
                self._run_with_watchdog(self._init_devices, 60, "init MDB")
            except Exception as e:
                self.logger.error(f"Init MDB échouée: {e} — tentative de replug USB")
                try:
                    ser.close()
                except Exception:
                    pass
                time.sleep(1)
                if not usb_replug(os.path.basename(self.port)):
                    raise Exception("Replug USB impossible")
                ser = serial.Serial(port=self.port, baudrate=115200, timeout=1)
                ser.reset_input_buffer()
                self._run_with_watchdog(self._init_devices, 60, "init MDB post-replug")

            if not self.starting:
                raise Exception("Initialisation annulée (stop pendant l'init)")

            self.running = True
            self.worker_thread = threading.Thread(target=self._worker, daemon=True)
            self.worker_thread.start()
            self.initialized = True
            self.logger.info("MDB Manager démarré avec succès")

        except Exception as e:
            self.logger.error(f"Erreur démarrage MDB Manager: {str(e)}")
            self.stop()
        finally:
            self.starting = False


    def stop(self):
        """Arrêter le manager et fermer la communication"""
        self.running = False
        self.starting = False  # annule aussi une init en cours
        self._abort_requested = True
        if self.worker_thread:
            self.payment_queue.put(('STOP', None))
            self.worker_thread.join(timeout=5)
            self.worker_thread = None

        if self.initialized:
            try:
                self._run_with_watchdog(self._end_communication, 10, "fin communication")
            except Exception as e:
                self.logger.error(f"Erreur fin communication: {e}")
            self.initialized = False

        global ser
        if ser and ser.is_open:
            ser.close()
        ser = None

        self.logger.info("MDB Manager arrêté")


    def start_payment(self, amount):
        """
        Démarrer une transaction de paiement

        Args:
            amount: Montant en euros (ex: 1.50)
        """
        if not self.initialized:
            self.logger.error("Manager non initialisé")
            if self.callback:
                self.callback('PAYMENT_FAILED', {'error': 'Manager non initialisé', 'amount': amount})
            return False

        if self.current_transaction:
            self.logger.warning("Transaction déjà en cours")
            return True

        self.logger.info(f"Démarrage transaction: {amount}€")
        self.current_transaction = {
            'amount': amount,
            'product': '1',
            'start_time': time.time(),
            'status': 'pending'
        }

        self.payment_queue.put(('START_PAYMENT', amount))
        return True

    def timeout_stalled_transaction(self, timeout_seconds=300):
        """Abandonner (et refonder si déjà approuvée) une transaction qui stagne.
        Appeler régulièrement depuis la boucle principale. Renvoie True si expirée."""
        t = self.current_transaction
        if not t or t.get('status') not in ('pending', 'approved'):
            return False
        if time.time() - t.get('start_time', time.time()) > timeout_seconds:
            if not t.get('_timeout_sent'):
                self.logger.warning(
                    f"Transaction {t.get('status')} en attente depuis >{timeout_seconds}s, annulation/refund"
                )
                t['_timeout_sent'] = True
                self.payment_queue.put(('CANCEL_SERVICE', None))
            return True
        return False

    def confirm_service(self):
        """Confirmer que le service a été délivré avec succès"""
        if not self.current_transaction:
            self.logger.warning("Aucune transaction en cours à confirmer")
            return False

        if self.current_transaction['status'] != 'approved':
            self.logger.warning(f"Transaction non approuvée (status: {self.current_transaction['status']})")
            return False

        self.logger.info("Confirmation du service")
        self.payment_queue.put(('CONFIRM_SERVICE', None))
        return True

    def cancel_service(self):
        """Annuler le service (en cas d'erreur)"""
        if not self.current_transaction:
            self.logger.warning("Aucune transaction en cours à annuler")
            return False

        self.logger.info("Annulation du service")
        self.payment_queue.put(('CANCEL_SERVICE', None))
        return True

    def _worker(self):
        """Thread worker pour gérer les transactions"""
        self.logger.info("Thread worker démarré")

        while self.running:
            try:
                cmd, data = self.payment_queue.get(timeout=0.5)

                if cmd == 'STOP':
                    break
                elif cmd == 'START_PAYMENT':
                    self._process_payment(data)
                elif cmd == 'CONFIRM_SERVICE':
                    self._confirm_transaction()
                elif cmd == 'CANCEL_SERVICE':
                    self._cancel_transaction()

            except queue.Empty:
                continue

            except Exception as e:
                self.logger.error(f"Erreur dans worker: {str(e)}")

        self.logger.info("Thread worker terminé")

    def _process_payment(self, amount):
        """Traiter un paiement"""
        self.logger.info(f"Traitement paiement: {amount}€")

        try:
            # Réinitialiser les appareils
            self._abort_requested = False
            self._init_devices()

            # Détecter si le terminal supporte le Direct Vend
            direct = self._detect_direct_vend(str(amount), "1")

            if not direct:
                # Mode normal (attente carte/pièces)
                self._normal_vend(str(amount), "1")
            else:
                # Mode Direct Vend
                self._direct_vend(str(amount), "1")

        except Exception as e:
            self.logger.error(f"Erreur traitement paiement: {str(e)}")
            if self.callback:
                self.callback('PAYMENT_FAILED', {
                    'error': str(e),
                    'amount': amount
                })
            self.current_transaction = None

    def _direct_vend(self, amount, product):
        """Traiter un paiement en mode Direct Vend"""
        self.logger.info("Mode Direct Vend détecté")

        while self.running and self.current_transaction:
            try:
                res = self._read_wait()
                if not res:
                    time.sleep(0.1)
                    continue

                self.logger.debug(f"Réponse MDB: {res}")

                # Vérifier le résultat de la transaction
                if "d,STATUS,RESULT," in res:
                    if "d,STATUS,RESULT,1" in res or "SUCCESS" in res:
                        # Paiement accepté !
                        self.logger.info(f"Paiement accepté: {amount}€")
                        self.current_transaction['status'] = 'approved'

                        if self.callback:
                            self.callback('PAYMENT_APPROVED', {
                                'amount': float(amount),
                                'timestamp': time.time(),
                                'response': res.strip()
                            })

                        # Attendre confirmation du service
                        # Le terminal CB attend maintenant notre réponse
                        break

                    else:
                        # Paiement refusé
                        self.logger.warning(f"Paiement refusé: {res}")
                        self.current_transaction['status'] = 'failed'

                        if self.callback:
                            self.callback('PAYMENT_FAILED', {
                                'amount': float(amount),
                                'reason': 'Transaction refusée par le terminal',
                                'response': res.strip()
                            })

                        # Réinitialiser pour prochaine transaction
                        self._reset_for_next_transaction()
                        break

            except Exception as e:
                self.logger.error(f"Erreur lecture MDB: {str(e)}")
                time.sleep(0.5)

    def _normal_vend(self, amount, product):
        """Mode normal (pour terminaux sans Direct Vend)"""
        self.logger.info("Mode normal - Attente insertion carte/pièces...")

        req_str = f"D,REQ,{amount},{product}\n"

        try:
            # Attendre crédit
            while self.running and self.current_transaction:
                res = self._read_wait()
                if not res:
                    time.sleep(0.1)
                    continue

                self.logger.debug(f"Réponse MDB (normal): {res}")

                if 'd,STATUS,CREDIT,' in res:
                    # Détecter le montant inséré
                    try:
                        credit_str = res.split('d,STATUS,CREDIT,')[-1].split(',')[0]
                        cash = float(credit_str)
                        self.logger.info(f"Crédit détecté: {cash}€")

                        if cash >= float(amount):
                            # Assez d'argent, démarrer la vente
                            res_vend = self._write_read(req_str.strip())

                            if 'd,STATUS,VEND' in res_vend:
                                # Transaction en attente de confirmation
                                self.logger.info("Vente démarrée, attente résultat...")
                                # Continuer à écouter le résultat
                                continue

                    except (ValueError, IndexError) as e:
                        self.logger.error(f"Erreur parsing crédit: {str(e)}")

                elif "d,STATUS,RESULT," in res:
                    # Résultat de la transaction
                    if "d,STATUS,RESULT,1" in res or "SUCCESS" in res:
                        self.logger.info(f"Paiement accepté (normal): {amount}€")
                        self.current_transaction['status'] = 'approved'

                        if self.callback:
                            self.callback('PAYMENT_APPROVED', {
                                'amount': float(amount),
                                'timestamp': time.time(),
                                'mode': 'normal',
                                'response': res.strip()
                            })
                        break
                    else:
                        self.logger.warning(f"Paiement refusé (normal): {res}")
                        self.current_transaction['status'] = 'failed'

                        if self.callback:
                            self.callback('PAYMENT_FAILED', {
                                'amount': float(amount),
                                'reason': 'Transaction refusée',
                                'mode': 'normal',
                                'response': res.strip()
                            })

                        self._reset_for_next_transaction()
                        break

        except Exception as e:
            self.logger.error(f"Erreur mode normal: {str(e)}")
            if self.callback:
                self.callback('PAYMENT_FAILED', {
                    'amount': float(amount),
                    'error': str(e),
                    'mode': 'normal'
                })
            self.current_transaction = None

    def _confirm_transaction(self):
        """Confirmer la transaction après service réussi"""
        if not self.current_transaction or self.current_transaction['status'] != 'approved':
            self.logger.warning("Tentative de confirmation sans transaction approuvée")
            return False

        try:
            self.logger.info("Finalisation transaction après service réussi")

            # Envoyer la commande de fin (confirmation)
            res = self._write_read("D,END")
            self.logger.debug(f"Confirmation transaction: {res}")

            transaction = self.current_transaction.copy()
            self._reset_for_next_transaction()

            if 'SUCCESS' in res or 'd,STATUS,IDLE' in res:
                self.logger.info(f"Transaction {transaction['amount']}€ confirmée avec succès")
                return True
            else:
                if not res:
                    self.logger.warning("Réponse vide du terminal CB - Transaction considérée comme confirmée")
                else:
                    self.logger.warning(f"Réponse inattendue: {res} - Transaction considérée comme confirmée")
                return True

        except Exception as e:
            self.logger.error(f"Erreur confirmation transaction: {str(e)}")
            return False

    def _cancel_transaction(self):
        """Annuler la transaction après échec de service"""
        if not self.current_transaction:
            self.logger.warning("Aucune transaction à annuler")
            return False

        try:
            self.logger.info("Annulation transaction")

            # Envoyer commande d'annulation
            res = self._write_read("D,END,-1")
            self.logger.debug(f"Annulation: {res}")

            transaction = self.current_transaction.copy()
            self._reset_for_next_transaction()

            self.logger.info(f"Transaction {transaction['amount']}€ annulée")
            return True

        except Exception as e:
            self.logger.error(f"Erreur annulation transaction: {str(e)}")
            return False

    def _reset_for_next_transaction(self):
        """Réinitialiser pour la prochaine transaction"""
        if self.current_transaction:
            self.logger.debug(f"Réinitialisation transaction {self.current_transaction['amount']}€")
        self.current_transaction = None

    # Méthodes de communication série

    def _read_wait(self):
        """Lire depuis le port série avec attente (timeout 1 s garanti par pyserial)"""
        global ser
        if ser is None:
            return ""

        for i in range(5):  # Attendre 500ms max
            try:
                line = ser.readline()
                if line:
                    buf = line.decode("ascii", "ignore")
                    if self.debug:
                        self.logger.debug(f"Read: {buf}")
                    return buf
                else:
                    time.sleep(0.1)
            except Exception as e:
                self.logger.error(f"Erreur lecture série: {str(e)}")
                return ""

        return ""

    def _write_serial(self, message):
        """Écrire sur le port série"""
        global ser

        if self.debug:
            self.logger.debug(f"Write: {message}")

        try:
            ser.write(message.encode("ascii") + b"\n")
        except Exception as e:
            self.logger.error(f"Erreur écriture série: {str(e)}")

    def _write_read(self, message):
        """Écrire et lire une réponse"""
        self._write_serial(message)
        return self._read_wait()

    def _init_devices(self):
        """Initialiser les appareils MDB (reset complet systématique)"""
        self.logger.info("Initialisation appareils MDB (reset complet)...")

        # Reset systématique : un kill précédent n'a jamais envoyé D,0,
        # la machine d'état du master peut être n'importe où.
        self._write_serial("D,0")
        time.sleep(1)

        # Démarrer le master en mode Direct Vend
        res = self._write_read("D,2")

        if 'D,ERR,"cashless master is on"' in res:
            self.logger.info("Redémarrage Cashless...")
            self._write_serial("D,0")
            time.sleep(1)
            res = self._write_read("D,2")

        # Attendre INIT du slave
        start_time = time.time()
        while 'd,STATUS,INIT' not in res:
            if self._abort_requested:
                raise Exception("Initialisation annulée")
            if time.time() - start_time > 30:  # Timeout 30s pour l'initialisation
                self.logger.error("Timeout initialisation MDB")
                raise Exception("Timeout initialisation MDB")

            self.logger.debug("Attente STATUS = INIT...")
            res = self._read_wait()
            time.sleep(1)

        # Activer le reader
        self._write_serial("D,READER,1")

        # Attendre IDLE
        start_time = time.time()
        while 'd,STATUS,IDLE' not in res:
            if self._abort_requested:
                raise Exception("Initialisation annulée")
            if time.time() - start_time > 30:  # Timeout 30s pour IDLE
                self.logger.error("Timeout attente IDLE")
                raise Exception("Timeout attente IDLE")

            self.logger.debug("Attente STATUS = IDLE...")
            res = self._read_wait()
            time.sleep(1)

        self.logger.info("Appareils MDB initialisés (IDLE)")

    def _detect_direct_vend(self, amount, product):
        """Détecter si le terminal supporte Direct Vend"""
        res = self._write_read(f"D,REQ,{amount},{product}")

        if 'd,ERR,"-1"' in res:
            self.logger.info("Terminal ne supporte pas Direct Vend")
            return False
        elif 'd,STATUS,VEND' in res:
            self.logger.info("Terminal supporte Direct Vend")
            return True
        else:
            self.logger.warning(f"Réponse inconnue pour détection Direct Vend: {res}")
            return False

    def _end_communication(self):
        """Terminer la communication MDB"""
        try:
            if self.current_transaction:
                self._cancel_transaction()

            self._write_read("D,READER,0")  # Désactiver le reader
            self._write_read("D,0")         # Désactiver le host
            self.logger.info("Communication MDB terminée")
        except Exception as e:
            self.logger.error(f"Erreur fin communication: {str(e)}")


# Fonctions de compatibilité pour l'ancien code
def initCB(port='/dev/ttyACM0', debug_mode=False):
    """Fonction de compatibilité - Initialiser MDB"""
    manager = MDBManager(port=port, debug_mode=debug_mode)
    return manager


# Mode standalone pour tests
if __name__ == "__main__":
    print("Test standalone MDB Manager")
    print("=" * 50)

    def test_callback(event, data):
        print(f"\n[Callback] Événement: {event}")
        print(f"[Callback] Données: {data}")
        print("-" * 30)

    # Créer et démarrer le manager
    manager = MDBManager(port='/dev/ttyACM0', debug_mode=True, callback=test_callback)

    if manager.start():
        print("Manager démarré avec succès")
        print("Appuyez sur:")
        print("  1. Démarrer transaction 0.16€")
        print("  2. Confirmer service (après succès)")
        print("  3. Annuler service")
        print("  q. Quitter")

        try:
            while True:
                choice = input("\nVotre choix (1-3, q): ").strip().lower()

                if choice == '1':
                    success = manager.start_payment(0.16)
                    if success:
                        print("Transaction démarrée")
                    else:
                        print("Echec démarrage transaction")

                elif choice == '2':
                    success = manager.confirm_service()
                    if success:
                        print("Service confirmé")
                    else:
                        print("Echec confirmation")

                elif choice == '3':
                    success = manager.cancel_service()
                    if success:
                        print("Service annulé")
                    else:
                        print("Echec annulation")

                elif choice == 'q':
                    print("Arrêt...")
                    break

                else:
                    print("Choix invalide")

        except KeyboardInterrupt:
            print("\nArrêt par Ctrl+C...")
        finally:
            manager.stop()
            print("Manager arrêté")
    else:
        print("Echec démarrage manager")