/**
 * @file imp.h
 * @brief Class and function to process the IMU data on the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */

#pragma once

#include "Arduino.h"
#include "../defines.h"
#include "kalman.h"
#include "data_points.h"
#include "config.h"
#include <ICM42670P.h>

extern Config config;

/**
 * @brief Calculates the acceleration in m/s
 *
 */
#define SCALE_ACCEL(raw) ((raw / (float)((1 << 15) - 1)) * IMU_ACCEL_RANGE * GRAVITY)

/**
 * @brief Calculates the rotation rate in rad/s
 *
 */
#define SCALE_GYRO(raw) ((raw / (float)((1 << 15) - 1)) * IMU_GYRO_RANGE * M_PI / 180)

/**
 * @brief Class that communicates with the IMU and calculates the current position and velocity using a Kalman filter.
 * 
 */
class IMUManager
{
public:
    /**
     * @brief Constructs the manager and assigns pins.
     *
     */
    IMUManager() : imu(SPI, PIN_SPI_AC_CS), kalman(config.qEnvCovariance, config.rMeasCovariance, KALMAN_X0, KALMAN_P0) {}
    // IMUManager() : imu(SPI, PIN_SPI_AC_CS), kalman(DEFAULT_KALMAN_Q, DEFAULT_KALMAN_R, KALMAN_X0, KALMAN_P0) {}

    /**
     * @brief Initialises the power meter hardware.
     *
     */
    void begin();

    /**
     * @brief Starts the IMU.
     */
    void startEstimating();

    /**
     * @brief Enables tilt / movement detection to wake the system up again.
     *
     */
    void enableMotion();

    /**
     * @brief Processes the IMU event to work out where we are.
     *
     * @param evt data from the IMU.
     */
    void processIMUEvent(inv_imu_sensor_event_t *evt);

    /**
     * @brief Set the rotation count and last rotation duration in a low speed data object.
     * 
     * @param data the data to update.
     */
    void setLowSpeedData(LowSpeedData &data);

    /**
     * @brief Calculates and returns the last reported temperature in a thread-safe manner.
     * 
     * @return float the temperature in Celcius.
     */
    float getLastTemperature();

    ICM42670 imu;
    Kalman<float> kalman;
    uint32_t rotations = 0;

private:
    /**
     * @brief Corrects a given reading for an axis for centripedal acceleration due to the offset of the IMU relative
     * to the centre of rotation.
     *
     * @param reading the given acceleration in ms^-2
     * @param radius the radius of the IMU for that axis (x or y offset most likely).
     * @param velocity the angular velocity in radians per second.
     * @return float const the corrected acceleration value.
     */
    float const m_correctCentripedal(float reading, float radius, float velocity);

    /**
     * @brief Calculates the angle based on two acceleration readings.
     *
     * @param x the x axis acceleration.
     * @param y the y axis acceleration.
     * @return float const the calculated angle (radians).
     */
    float const m_calculateAngle(float x, float y);

    /**
     * @brief Assigns an angle to one of 3 sectors.
     * 
     * @param angle the input angle.
     * @return int8_t the sector (0, 1 or 2)
     */
    int8_t m_angleToSector(float angle);

    /**
     * @brief Stores the last sector of rotation so that complete turns can be detected.
     * 
     */
    int8_t m_lastRotationSector = 0;
    bool m_armRotationCounter = false;
    uint32_t m_lastRotationDuration = 0;
    uint32_t m_lastRotationTime = 0;
    uint8_t m_sendCount = 0; // Only send once every so often, defined in the config.
    uint16_t m_lastTemperature;
};

/**
 * @brief Task for operating the IMU.
 *
 * @param pvParameters
 */
void taskIMU(void *pvParameters);

/**
 * @brief Interrupt called when the IMU has new data during tracking.
 *
 */
void irqIMUActive();

/**
 * @brief Interrupt called when the IMU detets movement to wake the unit up.
 *
 */
void irqIMUWake();