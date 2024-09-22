/**
 * @file temperature.h
 * @brief Reads data from a P3T1755 temperature sensor.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */

#include "temperature.h"

extern SemaphoreHandle_t serialMutex;
extern portMUX_TYPE spinlock;

void TempSensor::begin()
{
    Wire.beginTransmission(m_i2cAddress);
    // Set the configuration to default conversion time, default fault queue and enable shutdown.
    Wire.write(PTR_CONF);
    Wire.write(bit(CONF_BIT_R0) | bit(CONF_BIT_F0) | bit(CONF_BIT_SD)); // Default R0 and F0 are 1.
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
    Wire.write(PTR_CONF);
    // Set the configuration register to 55ms conversion time, fault queue length 0, one shot.
    // Polarity is set as per the desired LED state.
    Wire.write(bit(CONF_BIT_R0) | bit(CONF_BIT_F0) | bit(CONF_BIT_SD) | bit(CONF_BIT_OS) | m_polarity);
    Wire.endTransmission(true);
}

float TempSensor::readTempRegister()
{
    // Set the pointer to the temperature register.
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(PTR_TEMP);
    Wire.endTransmission(true);

    // Attempt to get the data
    uint8_t receivedSize = Wire.requestFrom(m_i2cAddress, 2);
    float temperature;
    if (receivedSize == 2)
    {
        // We received the expected number of bytes.
        int16_t rawTemp = Wire.read() << 8;
        rawTemp |= Wire.read();
        temperature = rawTemp / 256.0;
    }
    else
    {
        // Incorrect number of bytes received.
        LOGE("Temp", "Error reading temperature from %d", m_i2cAddress);
        temperature = INVALID_TEMPERATURE;
    }

    // Store the temperature as the most recent.
    taskENTER_CRITICAL(&spinlock);
    m_lastTemp = temperature;
    taskEXIT_CRITICAL(&spinlock);

    // Return the temperature as well.
    return temperature;
}

void TempSensor::setLED(bool state)
{
    if (state)
    {
        m_polarity = bit(CONF_BIT_POL);
    }
    else
    {
        m_polarity = 0;
    }

    // Set the LED state now.
    Wire.beginTransmission(m_i2cAddress);
    Wire.write(PTR_CONF);
    // Set the configuration register to 55ms conversion time, fault queue length 0.
    // Polarity is set as per the desired LED state. Don't start a conversion.
    Wire.write(bit(CONF_BIT_R0) | bit(CONF_BIT_F0) | bit(CONF_BIT_SD) | m_polarity);
    Wire.endTransmission(true);
}

float TempSensor::getLastTemp()
{
    taskENTER_CRITICAL(&spinlock);
    float temp = m_lastTemp;
    taskEXIT_CRITICAL(&spinlock);
    return temp;
}