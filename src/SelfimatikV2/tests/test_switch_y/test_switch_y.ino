
#define Y_ENDSTOP_PIN 3

void setup() {
  Serial.begin(9600);
  pinMode(Y_ENDSTOP_PIN, INPUT_PULLUP);
  // put your setup code here, to run once:
}

void loop() {
  
  Serial.println(digitalRead(Y_ENDSTOP_PIN));
}
