/**
 * @file power_meter.h
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-30
 */
#pragma once

#include "../defines.h"
#include "imu.h"
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
};

/**
 * @brief Class for interfacing with a single strain gauge.
 *
 */
class Side
{
public:
    /**
     * @brief Construct a new Strain Gauge object.
     *
     * @param pinDout the data out / data ready pin to use (input).
     * @param pinSclk the clock pin to use (output).
     * @param i2cAddress the I2C address of the temperature sensor on this side, set by physical jumpers / solder
     *                   bridges.
     */
    Side(const uint8_t pinDout, const uint8_t pinSclk, const uint8_t i2cAddress)
        : m_pinDout(pinDout), m_pinSclk(pinSclk), temperature(i2cAddress) {}

    /**
     * @brief Initialises the hardware specific to the side.
     *
     */
    void begin();

    /**
     * @brief The temperature sensor used for temperature compensation on this side.
     * 
     */
    TempSensor temperature;
private:
    /**
     * @brief Pins specific to this amplifier.
     *
     */
    const uint8_t m_pinDout, m_pinSclk;
};

/**
 * @brief Class for interfacing with all strain gauges.
 *
 */
class PowerMeter
{
public:
    /**
     * @brief Construct a new All Strain Gauges object. The left and right strain gauge objects are also initialised.
     *
     */
    PowerMeter() : m_sides{Side(PIN_AMP2_DOUT, PIN_AMP2_SCLK, TEMP2_I2C),
                        Side(PIN_AMP1_DOUT, PIN_AMP1_SCLK, TEMP1_I2C)} {}

    /**
     * @brief Initialises the power meter hardware.
     *
     */
    void begin();

    /**
     * @brief Puts the strain gauges, amps and other components into low power mode.
     *
     */
    void powerDown();

    /**
     * @brief Starts the strain gauges and amp.
     *
     * After calling this function, wait until data ready goes low before reading data.
     */
    void powerUp();

    /**
     * @brief Keeps a record of the current orientation and speed.
     * 
     */
    IMUManager imuManager;

private:
    /**
     * @brief Each side of the power meter. Using an array for easy manipulation later.
     * 
     */
    Side m_sides[2];
};