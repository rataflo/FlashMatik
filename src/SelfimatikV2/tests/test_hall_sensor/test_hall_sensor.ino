

unsigned int maxHall = 0;
void setup() {
  Serial.begin(9600);
  pinMode(A4, INPUT);
  // put your setup code here, to run once:
}

void loop() {
  
  unsigned int value = fastAnalogReadMega(A4);
  if(value > maxHall){
    maxHall = value;
  }
  if (value < maxHall-10) {
      Serial.println("STOP");
      while(1==1){

      }
  }
}


int fastAnalogReadMega(uint8_t pin) {
    if (pin < A0 || pin > A15) return 0;  // Vérification de la broche

    uint8_t analog_pin = pin - A0;       // Conversion en numéro interne (0-15)

    // Réinitialisation des registres ADC pour éviter des conflits
    ADMUX  = (1 << REFS0);               // Référence AVcc (5V)
    ADCSRB = (analog_pin & 0x08) ? (1 << MUX5) : 0;  // Gestion de MUX5 pour A8-A15
    ADMUX  |= (analog_pin & 0x07);       // Configuration MUX3:0

    // Configuration du prescaler (division par 128 pour une conversion stable)
    ADCSRA = (1 << ADEN)  |  // Activation ADC
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler 128

    ADCSRA |= (1 << ADSC);    // Lancement conversion
    while (ADCSRA & (1 << ADSC));  // Attente fin conversion

    return ADC;  // Retourne la valeur (équivalent à ADCL | (ADCH << 8))
}

