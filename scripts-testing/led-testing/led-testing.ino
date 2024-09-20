/**
 * @brief This script turns on and flashes the red and green LEDs (D1 and D2).
 * 
 */
void setup() {
  // put your setup code here, to run once:
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(12, OUTPUT);

  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);
  digitalWrite(12, HIGH);
  delay(1000);
}

void loop() {
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(12, HIGH);
  delay(500);
}
