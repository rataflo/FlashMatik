import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)
print(GPIO.input(22))  # doit retourner 1 si rien n'est branché
GPIO.remove_event_detect(22)
GPIO.add_event_detect(22, GPIO.FALLING, callback=lambda x: print("ok"), bouncetime=200)