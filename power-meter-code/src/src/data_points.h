/**
 * @file data_points.h
 * @brief Classes that represent data from a single point.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once
#include "../defines.h"

#define VELOCITY_TO_CADENCE(vel) (vel * 60 / (2 * M_PI));

/**
 * @brief Adds an object to a buffer.
 *
 * This is the same endianness as the system (little endian for the ESP32).
 */
#define ADD_TO_BYTES(object, buffer, offset) \
    memcpy(buffer + offset, &object, sizeof(object))

/**
 * @brief Data to be passed on the housekeeping queue
 *
 */
class HousekeepingData
{
public:
    /**
     * @brief Temperature of each side in degrees C.
     *
     */
    float temperatures[3];

    /**
     * @brief Battery voltage in V.
     *
     */
    float battery;

    /**
     * @brief Calculates the average temperature of all sides.
     *
     * @return float The average temperature in degrees C.
     */
    float averageTemp();
};

/**
 * @brief Low speed data (what would be reported by a basic power meter).
 *
 */
class LowSpeedData
{
public:
    /**
     * @brief The number of rotations counted.
     * 
     */
    uint32_t rotationCount;

    /**
     * @brief The duration it took to complete the last rotation (us).
     * 
     */
    uint32_t lastRotationDuration;

    /**
     * @brief The time at which the last rotation occured (us).
     * 
     */
    uint32_t timestamp;

    /**
     * @brief Calculates the instantanerous cadence (angular velocity) in revolutions per minute.
     *
     * For some reason cyclists don't work exclusively in radians per second.
     *
     * @return float the cadence.
     */
    float cadence();

    /**
     * @brief The average power in W.
     *
     */
    float power;

    /**
     * @brief The average cadance in RPM.
     *
     */
    // float cadence;

    /**
     * @brief The pedal balance.
     *
     * 0 is completely left only, 0.5 is balanced, 1 is completely right only.
     *
     */
    float balance;
};

class BaseData
{
public:
    /**
     * @brief Time in ms since the system start.
     *
     */
    uint32_t timestamp;

    /**
     * @brief Instantaneous angular velocity in rad/s.
     *
     */
    float velocity;

    /**
     * @brief Instantaneous position in radians.
     *
     */
    float position;

    /**
     * @brief Calculates the instantanerous cadence (angular velocity) in revolutions per minute.
     *
     * For some reason cyclists don't work exclusively in radians per second.
     *
     * @return float the cadence.
     */
    float cadence();

    /**
     * @brief Adds the current data point to a buffer for transmission.
     *
     * @param buffer is the buffer to put the data in. Needs to be at least BASE_BYTE_SIZE bytes long.
     */
    void baseBytes(uint8_t *buffer);

    static const int BASE_BYTES_SIZE = 12;
};

/**
 * @brief High-speed IMU data.
 *
 */
class IMUData : public BaseData
{
public:
    float xAccel;
    float yAccel;
    float zAccel;
    float xGyro;
    float yGyro;
    float zGyro;

    /**
     * @brief Adds the current data point to a buffer for transmission.
     *
     * @param buffer is the buffer to put the data in. This needs to be at least IMU_BYTES_SIZE bytes long.
     */
    void toBytes(uint8_t *buffer);

    static const int IMU_BYTES_SIZE = 6*4 + BASE_BYTES_SIZE;
};

/**
 * @brief High speed data for one side.
 *
 */
class HighSpeedData : public BaseData
{
public:
    /**
     * @brief The raw reading from the strain gauge.
     * This is a 24 bit number.
     *
     */
    uint32_t raw;

    /**
     * @brief Calculates the torque in Nm.
     * 
     * This is calculated as part of the side cloass to aid in getting the correct configs and calibration for each
     * side.
     *
     */
    float torque;

    /**
     * @brief Calculates the power in W.
     *
     * @return float the power in W.
     */
    float power();

    /**
     * @brief Adds the current data point to a buffer for transmission.
     *
     * @param buffer is the buffer to put the data in. This needs to be at least FAST_BYTES_SIZE bytes long.
     */
    void toBytes(uint8_t *buffer);

    static const int FAST_BYTES_SIZE = 4 + 4 + 4 + BASE_BYTES_SIZE;
};