/**
 * @file temperature.h
 * @brief Reads data from a P3T1755 temperature sensor.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-14
 */

#pragma once

#include "../defines.h"
#include <Wire.h>

/**
 * @brief Class for communicating with a P3T1755 temperature sensor.
 * 
 */
class TempSensor
{
public:
    /**
     * @brief Construct a new Temp Sensor object.
     * 
     * @param i2cAddress the I2C address of this temperature sensor.
     */
    TempSensor(const uint8_t i2cAddress): m_i2cAddress(i2cAddress) {}

    /**
     * @brief Initialises the sensor into single shot mode for power saving.
     * 
     */
    void begin();

    /**
     * @brief Reads the current temperature in celcius.
     * 
     * This reading is synchronous and will take at least 12ms. For faster reading, consider using continuous
     * measurement instead of single shot or calling `startCapture()` in advance of calling `readTempRegister()`.
     * 
     * @return float the current temperature.
     */
    float readTemp();

    /**
     * @brief Starts a single shot temperature capture.
     * 
     * Temperature capture typically takes 7.8ms, but may take up to 12ms.
     */
    void startCapture();

    /**
     * @brief Reads the temperature register value and returns the result in celcius.
     * @return float the temperature value in the register (may not be current if `startCapture()` has not been called
     *         recently).
     */
    float readTempRegister();

    /**
     * @brief Turns the LED connected to the alarm pin on or off.
     * 
     * @param state if true, turns the LED on. If false, turns it off.
     */
    void setLED(bool state);

private:
    /**
     * @brief Enumerator to store the pointers for each register in the sensor.
     * 
     */
    enum TempPointers
    {
        PTR_TEMP = 0,
        PTR_CONF = 1,
        PTR_TEMP_LOW = 2,
        PTR_TEMP_HIGH = 3
    };

    /**
     * @brief Enumerator to label configuration bits in the configuration registor of the sensor.
     * 
     */
    enum ConfBits
    {
        CONF_BIT_SD = 0,
        CONF_BIT_TM = 1,
        CONF_BIT_POL = 2,
        CONF_BIT_F0 = 3,
        CONF_BIT_F1 = 4,
        CONF_BIT_R0 = 5,
        CONF_BIT_R1 = 6,
        CONF_BIT_OS = 7
    };
    const uint8_t m_i2cAddress;

    uint8_t m_polarity;
};