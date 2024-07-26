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
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 50; i++)
  {
    Serial.println(i);
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
    delay(20);
    digitalWrite(i, LOW);
  }
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
  delay(500);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  delay(500);
}
