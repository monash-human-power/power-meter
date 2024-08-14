/**
 * @file power_meter.cpp
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-14
 */

#include "power_meter.h"

extern SemaphoreHandle_t serialMutex;
extern PowerMeter powerMeter;

#include "connections.h"
extern Connection *connectionBasePtr;

void Side::begin()
{
    LOGD("Side", "Starting hardware for a side");
    temperature.begin();
}

void Side::readDataTask()
{
    while (true)
    {
        // Wait for the interrupt to occur and we get a notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Interrupt occured, data is ready to read.
        // Get the current time and position now in case we get interrupter later. We have many ms for the data to sit
        // in the ADC's buffer, so not too much of a rush.
        HighSpeedData data;
        data.timestamp = micros();
        // Using predict so we can have a lower sample rate on the IMU hopefully.
        Matrix<2, 1, float> state;
        powerMeter.imuManager.kalman.predict(data.timestamp, state);
        data.position = state(0, 0);
        data.velocity = state(1, 0);
        data.raw = 0;

        // Start reading
        uint8_t clockBits = 24;
        if (m_offsetCalibration)
        {
            // Add 2 additional clock pulses to enter offset calibrarion mode.
            clockBits = 26;
        }
        for (uint8_t i = 0; i < clockBits; i++)
        {
            // Read 1 bit
            digitalWrite(m_pinSclk, HIGH);
            delayMicroseconds(1);
            data.raw = (data.raw << 1) | digitalRead(m_pinDout);
            digitalWrite(m_pinSclk, LOW);
            delayMicroseconds(1);
        }

        // Send the contents of raw somewhere as needed.
        if (m_offsetCalibration)
        {
            // Remove the extra 2 bits
            data.raw >>= 2;
            m_offsetCalibration = false; // Disable automatically.
        }

        // Finish calculating and send raw to where it needs to go.
        data.torque = m_calculateTorque(data.raw);
        connectionBasePtr->addHighSpeed(data, m_side);
    }
}

inline void Side::enableOffsetCalibration()
{
    m_offsetCalibration = true;
}

float Side::m_calculateTorque(uint32_t raw)
{
    return raw / 1e3f; // TODO: Scary calibration part.
}

void PowerMeter::begin()
{
    LOGD("Power", "Starting hardware");
    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    // Initialise the strain gauges
    sides[SIDE_LEFT].begin();
    sides[SIDE_RIGHT].begin();

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

    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);

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