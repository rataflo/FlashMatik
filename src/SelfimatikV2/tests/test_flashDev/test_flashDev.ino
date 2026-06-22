#define FLASHDEV_PIN 8

void setup() {
  // put your setup code here, to run once:
  pinMode(FLASHDEV_PIN, OUTPUT);
  digitalWrite(FLASHDEV_PIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  digitalWrite(FLASHDEV_PIN, HIGH);
  delay(1000);
  digitalWrite(FLASHDEV_PIN, LOW);
}
