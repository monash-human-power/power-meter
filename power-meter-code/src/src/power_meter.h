/**
 * @file power_meter.h
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once

#include "../defines.h"
#include "imu.h"
#include "temperature.h"
#include "data_points.h"

/**
 * @brief Class for interfacing with a single strain gauge and temperature sensor.
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
     * @param irqAmp the interrupt handler to use when more data is available from the ADC.
     * @param i2cAddress the I2C address of the temperature sensor on this side, set by physical jumpers / solder
     *                   bridges.
     */
    Side(const EnumSide side, const uint8_t pinDout, const uint8_t pinSclk, void (*irqAmp)(), const uint8_t i2cAddress)
        : m_side(side), m_pinDout(pinDout), m_pinSclk(pinSclk), m_irq(irqAmp), tempSensor(i2cAddress) {}

    /**
     * @brief Initialises the hardware specific to the side.
     *
     */
    void begin();

    /**
     * @brief Creates a freeRTOS task to handle data input from the ADC.
     *
     */
    void createDataTask(uint8_t id);

    /**
     * @brief Reads data and runs as the main task for the side.
     *
     * Waits for an interrupt when DOUT/DREADY goes low and receives a notification from it.
     *
     */
    void readDataTask();

    /**
     * @brief Handles a raw data point.
     * 
     * Calculates the torque, sends high speed data and sums energy to calculate average power.
     * 
     * @param timestamp the timestamp in microseconds.
     * @param position the angle and velocity.
     * @param raw the raw value from the ADC.
     */
    void processData(uint32_t timestamp, Matrix<2, 1, float> position, uint32_t raw);

    /**
     * @brief Tells the ADC to perform offset calibration the next time data is read.
     *
     * This state will be automaticalyl cleared afterwards.
     *
     */
    void enableADCOffsetCalibration();

    /**
     * @brief Averages the next OFFSET_COMPENSATION_SAMPLES to get a zero reading.
     * 
     * No force must be present on the cranks.
     * 
     */
    void enableStrainOffsetCalibration();

    /**
     * @brief Starts listening to strain gauge amplifier data.
     *
     */
    void startAmp();

    /**
     * @brief The temperature sensor used for temperature compensation on this side.
     *
     */
    TempSensor tempSensor;

    /**
     * @brief The handle of the task collecting data from the ADC.
     *
     */
    TaskHandle_t taskHandle;

    float averagePower; // The average power for the previous rotation.
private:
    /**
     * @brief Pins specific to this amplifier.
     *
     */
    const uint8_t m_pinDout, m_pinSclk;

    /**
     * @brief Reads the ADC.
     * Note that there is a short window to call this in between samples - only call this in the interrupt /
     * notification task.
     * @return uint32_t the raw value.
     */
    uint32_t m_readADC();

    /**
     * @brief Handles a new raw data point.
     *
     * @param raw the reading to convert to a torque.
     * @param temperature the temperature of the gauges.
     */
    float m_calculateTorque(uint32_t raw, float temperature);

    /**
     * @brief If this is the first reading in a new rotation, update the average power.
     * 
     * @param timestamp 
     */
    void m_updateAveragePower(uint32_t timestamp);

    bool m_adcOffsetCalibration = false;

    const EnumSide m_side;

    void (*m_irq)();

    // Variables for accumulating energy
    uint32_t m_lastTime, m_segStartTime; // Time that the last sample occurred and time that the current period started.
    float m_energy; // Accumulator for the energy.
    uint32_t m_lastRotation; // What the last rotation was on.

    /**
     * @brief Number of samples remaining to complete offset compensation.
     * 
     * If set to zero, operates in normal mode. Set to OFFSET_COMPENSATION_SAMPLES by calling  to begin.
     * 
     */
    uint8_t m_offsetSteps = 0;
};

/**
 * @brief Task for reading ADC data from a side.
 *
 * @param pvParameters is a pointer to the side to operate on.
 */
void taskAmp(void *pvParameters);

/**
 * @brief Interrupt for when an amplifier / ADC has data.
 *
 * Using a template so there will be a fixed address / passing function pointers works as expected.
 *
 */
template <EnumSide sideEnum, uint8_t pinDout>
void irqAmp();

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
    PowerMeter() : sides{Side(SIDE_LEFT, PIN_AMP2_DOUT, PIN_AMP2_SCLK, &irqAmp<SIDE_LEFT, PIN_AMP2_DOUT>, TEMP2_I2C),
                         Side(SIDE_RIGHT, PIN_AMP1_DOUT, PIN_AMP1_SCLK, &irqAmp<SIDE_RIGHT, PIN_AMP1_DOUT>, TEMP1_I2C)} {}

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
     * @brief Enables offset compensation for the strain gauge.
     * 
     */
    void offsetCompensate();

    /**
     * @brief Calculates the battery voltage.
     *
     * @return uint32_t the battery voltage in mV.
     */
    uint32_t batteryVoltage();

    /**
     * @brief Keeps a record of the current orientation and speed.
     *
     */
    IMUManager imuManager;

    /**
     * @brief Each side of the power meter. Using an array for easy manipulation later.
     *
     */
    Side sides[2];
};

/**
 * @brief Waits until a full revolution has been completed and both tasks have calculated their average power.
 * 
 * @param timeout the timeout in ticks. As two notifications are expected, the maximum timeout could theoretically be
 *                double this value.
 */
bool waitLowSpeedNofity(uint32_t timeout);

/**
 * @brief Task for processing low speed data.
 *
 * @param pvParameters is any data given (currently unused).
 */
void taskLowSpeed(void *pvParameters);

/**
 * @brief Prints memory usage info.
 */
void debugMemory();