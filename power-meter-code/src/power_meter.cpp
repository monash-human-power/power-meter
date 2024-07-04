/**
 * @file power_meter.cpp
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-04
 */

#include "Arduino.h"
#include "power_meter.h"

extern void irqIMUActive();
extern void irqIMUWake();
extern void callbackProcessIMU(inv_imu_sensor_event_t *evt);

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

void Side::begin()
{
    temperature.begin();
}

void IMUManager::begin()
{
    // Initialise the IMU
    // Manually initialise the SPI bus beforehand so that the pins can be specified. The call to `SPI.begin()` in the
    // IMU library should have no effect / returned early.
    SPI.begin(PIN_SPI_SCLK, PIN_SPI_SDI, PIN_SPI_SDO, PIN_SPI_AC_CS);
    int result = m_imu.begin();
    if(!result)
    {
        LOGE("IMU", "Cannot connect to IMU, error %d.", result);
    }
}

void IMUManager::startEstimating()
{
    // Setup the IMU
    m_imu.enableFifoInterrupt(PIN_ACCEL_INTERRUPT, irqIMUActive, 10);
    m_imu.startAccel(IMU_SAMPLE_RATE, IMU_ACCEL_RANGE);
    m_imu.startGyro(IMU_GYRO_RANGE, IMU_GYRO_RANGE);
}

void IMUManager::enableMotion()
{
    // TODO: Interrupt handler.
    m_imu.startWakeOnMotion(PIN_ACCEL_INTERRUPT, irqIMUWake);
}

void IMUManager::taskIMU(void *pvParameters)
{
    while (true)
    {
        // Wait for the interrupt to occur and we get a notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Get data from the accelerometer
        m_imu.getDataFromFifo(callbackProcessIMU);
    }
}

void IMUManager::processIMUEvent(inv_imu_sensor_event_t *evt)
{
    if (m_imu.isAccelDataValid(evt) && m_imu.isGyroDataValid(evt))
    {
        // X accel = evt->accel[0]
        // TODO
    }
}

inline float const IMUManager::m_correctCentripedal(float reading, float radius, float velocity)
{
    // TODO
}

void PowerMeter::begin()
{
    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    // Initialise the strain gauges
    left.begin();
    right.begin();

    // Initialise the IMU
    imuManager.begin();
}

void PowerMeter::powerDown()
{
    digitalWrite(PIN_AMP_PWDN, LOW);
    digitalWrite(PIN_POWER_SAVE, LOW);
}

void PowerMeter::powerUp()
{
    // Reset the strain gauge ADCs as per the manual (only needed first time, but should be ok later?).
    digitalWrite(PIN_POWER_SAVE, HIGH); // Turn on the strain gauges.
    delay(5); // Way longer than required, but should let the reference and strain gauge voltages settle.
    // Reset sequence specified in the ADS1232 datasheet.
    digitalWrite(PIN_AMP_PWDN, HIGH);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, LOW);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, HIGH);

    // Start the IMU
    imuManager.startEstimating();
}