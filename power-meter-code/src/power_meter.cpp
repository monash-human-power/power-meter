/**
 * @file power_meter.cpp
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-31
 */

#include "power_meter.h"
extern SemaphoreHandle_t serialMutex;

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
    // Set the configuration register to as initialised + one shot.
    Wire.write(bit(CONF_BIT_R0) | bit(CONF_BIT_F0) | bit(CONF_BIT_SD) | bit(CONF_BIT_OS));
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

void Side::begin()
{
    LOGD("Side", "Starting hardware for a side");
    temperature.begin();
}

void PowerMeter::begin()
{
    LOGD("Power", "Starting hardware");
    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    // Initialise the strain gauges
    m_sides[SIDE_LEFT].begin();
    m_sides[SIDE_RIGHT].begin();

    // Initialise the IMU
    imuManager.begin();
}

void PowerMeter::powerDown()
{
    LOGI("Power", "Power down");
    digitalWrite(PIN_AMP_PWDN, LOW);
    digitalWrite(PIN_POWER_SAVE, LOW);
}

void PowerMeter::powerUp()
{
    LOGI("Power", "Power up");
    // Set pin modes. // TODO: OOP
    pinMode(PIN_POWER_SAVE, OUTPUT);
    pinMode(PIN_AMP1_SCLK, OUTPUT);
    pinMode(PIN_AMP2_SCLK, OUTPUT);
    pinMode(PIN_AMP_PWDN, OUTPUT);
    pinMode(PIN_AMP1_DOUT, INPUT);
    pinMode(PIN_AMP2_DOUT, INPUT);
    pinMode(PIN_ACCEL_INTERRUPT, INPUT);
    // Reset the strain gauge ADCs as per the manual (only needed first time, but should be ok later?).
    digitalWrite(PIN_POWER_SAVE, HIGH); // Turn on the strain gauges.
    delay(5);                           // Way longer than required, but should let the reference and strain gauge voltages settle.

    // Reset sequence specified in the ADS1232 datasheet.
    digitalWrite(PIN_AMP_PWDN, HIGH);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, LOW);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, HIGH);

    // Start the IMU
    imuManager.startEstimating();
}