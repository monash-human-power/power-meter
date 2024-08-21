/**
 * @file temp-testing.ino
 * @brief Tests the temperature sensors.
 * Use the Arduino I2C (Wire) scan example to list all I2C devices on the bus.
 * 
 * @author Jotham Gates (koyugaDev@gmail.com)
 * @version 0.0.1
 * @date 2024-08-21
 */

#include "defines.h"
#include "temperature.h"
#define TAG "Temp test"
#define D 250
TempSensor temp1(TEMP1_I2C);
TempSensor temp2(TEMP2_I2C);

void setup()
{
    LOGI(TAG, "Temperature testing started.");
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    digitalWrite(PIN_LED1, HIGH);

    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    LOGI(TAG, "Starting sensor 1");
    temp1.begin();

    LOGI(TAG, "Starting sensor 2");
    temp2.begin();
}

void loop()
{

    float temp1Reading = temp1.readTemp();
    float temp2Reading = temp2.readTemp();
    LOGI(TAG, "Temperature 1 is %.2fC\tTemperature 2 is %.2fC.", temp1Reading, temp2Reading);
    digitalWrite(PIN_LED2, LOW);
    temp2.setLED(true);
    delay(D);
    temp1.setLED(true);
    delay(D);
    digitalWrite(PIN_LED1, HIGH);
    delay(D);
    digitalWrite(PIN_LED2, HIGH);
    delay(D);
    temp2.setLED(false);
    delay(D);
    temp1.setLED(false);
    delay(D);
    digitalWrite(PIN_LED1, LOW);
    delay(D-30);
}