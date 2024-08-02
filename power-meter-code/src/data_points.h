/**
 * @file data_points.h
 * @brief Classes that represent data from a single point.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-03
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
    float temperatures[2];

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
     * @brief The average power in W.
     *
     */
    float power;

    /**
     * @brief The average cadance in RPM.
     *
     */
    float cadence;

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
     * @param buffer is the buffer to put the data in. Needs to be at least 12 bytes long.
     */
    virtual void toBytes(uint8_t *buffer);
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
    float zGyro;

    /**
     * @brief Adds the current data point to a buffer for transmission.
     *
     * @param buffer is the buffer to put the data in. This needs to be at least 24 bytes long.
     */
    virtual void toBytes(uint8_t *buffer);
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
     */
    float torque();

    /**
     * @brief Calculates the power in W.
     *
     * @return float the power in W.
     */
    float power();

    /**
     * @brief Adds the current data point to a buffer for transmission.
     *
     * @param buffer is the buffer to put the data in. This needs to be at least 24 bytes long.
     */
    virtual void toBytes(uint8_t *buffer);
};