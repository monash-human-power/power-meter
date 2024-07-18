/**
 * @file connections.h
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-19
 */
#pragma once
#include "Arduino.h"
#include "../defines.h"

#define VELOCITY_TO_CADENCE(vel) (vel * 60 / (2 * M_PI));

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

class IMUData
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
};

/**
 * @brief High speed data for one side.
 *
 */
class HighSpeedData : public IMUData
{
public:
    /**
     * @brief Torque in Nm.
     *
     */
    float torque;

    /**
     * @brief Calculates the power in W.
     *
     * @return float the power in W.
     */
    float power();
};

/**
 * @brief Abstract base class for a connection that handles incoming data and publishes it elsewhere.
 *
 * Other abstract classes inherit this one to implement additional data handling queues.
 *
 */
class Connection
{
public:
    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the low-speed and housekeeping queues if they don't already exist.
     *
     * @param housekeepingLength maximum length of the housekeeping queue.
     * @param lowSpeedLength maximum length of the low-speed queue.
     */
    virtual void begin(const int housekeepingLength, const int lowSpeedLength);

    /**
     * @brief Enables the connection after sleep or on startup.
     *
     */
    virtual void enable() {}

    /**
     * @brief Stops the connection and enters a low power mode.
     *
     */
    virtual void disable() {}

    /**
     * @brief Checks if the connection is currently active (can the device send data to the outside world)?
     *
     * @return true connected.
     * @return false not connected.
     */
    virtual bool isConnected() {}

    /**
     * @brief Adds housekeeping data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data The housekeeping data to send.
     */
    virtual void addHousekeeping(HousekeepingData &data);

    /**
     * @brief Adds low speed data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the low speed data.
     */
    virtual void addLowSpeed(LowSpeedData &data);

    /**
     * @brief Function that may be implemented to handle high speed data.
     *
     * @param data the data object to accept.
     * @param side the side that this data pertains to.
     */
    virtual void addHighSpeed(HighSpeedData &data, EnumSide side) {}

    /**
     * @brief Function that may be implemented to handle data from the IMU.
     *
     * @param data the data object to accept.
     */
    virtual void addIMU(IMUData &data) {}

protected:
    /**
     * @brief Queues that all connection types are expected to accept.
     *
     */
    QueueHandle_t m_housekeepingQueue, m_lowSpeedQueue;

    /**
     * @brief Attempts to add data to a queue and has a tantrum if it's full.
     *
     * @param queue the queue to add data to.
     * @param data the data.
     */
    void m_addToQueue(QueueHandle_t queue, void *data);
};

/**
 * @brief Adds the queue to implement a high speed connection.
 *
 */
class HighSpeedConnection : public Connection
{
public:
    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the high-speed queues if they don't already exist and calls `Connection::begin()`.
     *
     * @param housekeepingLength maximum length of the housekeeping queue.
     * @param lowSpeedLength maximum length of the low-speed queue.
     * @param highSpeedLength maximum length of the high-speed queues.
     */
    virtual void begin(const int housekeepingLength, const int lowSpeedLength, const int highSpeedLength);

    /**
     * @brief Adds high-speed data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the high-speed data.
     */
    virtual void addHighSpeed(HighSpeedData &data, EnumSide side);

protected:
    /**
     * @brief Queues that all connection types are expected to accept.
     *
     * Using an array to allow for each side to be easily addressed.
     *
     */
    QueueHandle_t m_sideQueues[2];

private:
    /**
     * @brief Creates the queue for a side
     *
     * @param side the side.
     * @param length the length of the queue.
     */
    void m_createSideQueue(EnumSide side, int length);
};

/**
 * @brief Adds the queue to implement high-speed IMU data.
 *
 */
class IMUConnection : public Connection
{
public:
    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the IMU queue if they don't already exist and calls `Connection::begin()`.
     *
     * @param housekeepingLength maximum length of the housekeeping queue.
     * @param lowSpeedLength maximum length of the low-speed queue.
     * @param imuLength maximum length of the imu queue.
     */
    virtual void begin(const int housekeepingLength, const int lowSpeedLength, const int imuLength);

    /**
     * @brief Adds high-speed IMU data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the IMU data.
     */
    virtual void addIMU(IMUData &data);

protected:
    /**
     * @brief Queues that all connection types are expected to accept.
     *
     * Using an array to allow for each side to be easily addressed.
     *
     */
    QueueHandle_t m_imuQueue;
};