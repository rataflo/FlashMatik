#define FLASH_PIN 45

void setup() {
  // put your setup code here, to run once:
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  digitalWrite(FLASH_PIN, HIGH);
  delay(50);
  digitalWrite(FLASH_PIN, LOW);
}
