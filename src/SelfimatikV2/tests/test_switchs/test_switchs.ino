
#define PIN_SWITCH 14

void setup() {
  Serial.begin(9600);
  pinMode(PIN_SWITCH, INPUT_PULLUP);
  // put your setup code here, to run once:
}

void loop() {
  
  Serial.println(digitalRead(PIN_SWITCH));
}
