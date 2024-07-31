/**
 * @brief This script turns on and flashes the red and green LEDs (D1 and D2).
 * 
 */
void setup() {
  // put your setup code here, to run once:
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(10, OUTPUT);

  digitalWrite(7, HIGH);
  digitalWrite(10, HIGH);
}

void loop() {
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
  delay(500);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  delay(500);
}
