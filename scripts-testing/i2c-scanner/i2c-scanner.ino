/**
* Copied from the WireScan example, correct I2C pins specified.
*/
#include "Wire.h"

void setup() {
  Serial.begin(115200);
  Wire.begin(10, 11, 400e3);
  // Wire.begin();
}

void loop() {
  byte error, address;
  int nDevices = 0;

  delay(5000);

  Serial.println("Scanning for I2C devices ...");
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      Serial.println(address, HEX);
      nDevices++;
    } else if (error != 2) {
      Serial.print("Error ");
      Serial.print(error);
      Serial.print("at address 0x");
      Serial.println(address);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  }
}
