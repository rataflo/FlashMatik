import serial
import time

try:
    # Ouvrir avec timeout
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.5, write_timeout=0.5)
    time.sleep(0.5)
    
    # Envoyer la commande reset
    ser.write(b'\x00')
    ser.flush()
    print("Reset command sent")
    
    # Lire la réponse (si elle existe)
    response = ser.read(100)
    if response:
        print(f"Response: {response}")
    else:
        print("No response (timeout)")
    
    ser.close()
    
except serial.SerialException as e:
    print(f"Erreur: {e}")
except serial.SerialTimeoutException:
    print("Timeout - le lecteur ne répond pas")