/**
 * @file power_meter.cpp
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-06-27
 */

#include "Arduino.h"
#include "power_meter.h"

void TempSensor::begin()
{
    Wire.beginTransmission(m_i2cAddress);
    // Set the configuration to default conversion time, default fault queue and enable shutdown.
    Wire.write(ptrConf);
    Wire.write(bit(confBitR0) | bit(confBitF0) | bit(confBitSD)); // Default R0 and F0 are 1.
    Wire.endTransmission(true);
}

float TempSensor::readTemp()
{
    startCapture();
    delay(12);
    return readTempRegister();
}

void TempSensor::startCapture()
{
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(ptrConf);
    // Set the configuration register to as initialised + one shot.
    Wire.write(bit(confBitR0) | bit(confBitF0) | bit(confBitSD) | bit(confBitOS));
    Wire.endTransmission(true);
}

float TempSensor::readTempRegister()
{
    // Set the pointer to the temperature register.
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(ptrTemp);
    Wire.endTransmission(true);

    // Attempt to get the data
    uint8_t receivedSize = Wire.requestFrom(m_i2cAddress, 2);
    if (receivedSize == 2)
    {
        // We received the expected number of bytes.
        int16_t rawTemp = (Wire.read() << 8) | (Wire.read() >> 4);
        return rawTemp * 0.0625;
    }
    else
    {
        // Incorrect number of bytes received.
        LOGE("Temp", "Error reading temperature from %d", m_i2cAddress);
        return NAN;
    }
}

void StrainGauge::begin()
{
    temperature.begin();
}

void AllStrainGauges::begin()
{
    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    // Initialise the strain gauges
    left.begin();
    right.begin();
}

void AllStrainGauges::powerDown()
{
    digitalWrite(PIN_AMP_PWDN, LOW);
    digitalWrite(PIN_POWER_SAVE, LOW);
}

void AllStrainGauges::powerUp()
{
    digitalWrite(PIN_POWER_SAVE, HIGH); // Turn on the strain gauges.
    delay(5); // Way longer than required, but should let the reference and strain gauge voltages settle.
    // Reset sequence specified in the ADS1232 datasheet.
    digitalWrite(PIN_AMP_PWDN, HIGH);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, LOW);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, HIGH);
}