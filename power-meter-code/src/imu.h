/**
 * @file imp.h
 * @brief Class and function to process the IMU data on the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-09
 */

#pragma once

#include "Arduino.h"
#include "../defines.h"
#include "kalman.h"
#include <ICM42670P.h>

#define SCALE_ACCEL(raw) (raw / (float)((1<<15)-1)) * IMU_ACCEL_RANGE
#define SCALE_GYRO(raw) (raw / (float)((1<<15)-1)) * IMU_GYRO_RANGE

class IMUManager
{
public:
    /**
     * @brief Constructs the manager and assigns pins.
     * 
     */
    IMUManager(): imu(SPI, PIN_SPI_AC_CS), m_kalman(KALMAN_Q, KALMAN_R, KALMAN_X0, KALMAN_P0) {}

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

    ICM42670 imu;

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

    Kalman<float> m_kalman;
    uint16_t m_lastTimestamp = 0;
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