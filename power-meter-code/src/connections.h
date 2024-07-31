/**
 * @file connections.h
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-31
 */
#pragma once
#include "../defines.h"
#include "states.h"
#include "data_points.h"

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
     * @brief Construct a new Connection object.
     *
     * @param enableState is the state that should be called to enable the connection.
     */
    Connection(State &enableState) : m_stateDisabled(enableState) {}

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
     * Sends a notification by default.
     *
     */
    virtual void enable();

    /**
     * @brief Stops the connection and enters a low power mode.
     *
     * Sends a notification by default.
     */
    virtual void disable();

    /**
     * @brief Runs the task that manages the connection.
     *
     * This starts the state machine in the disabled state. When `enable()` is called, the next state will be called
     * automatically.
     *
     */
    void run(TaskHandle_t taskHandle);

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
     * @brief Enumberator to represent channels to notify over.
     *
     */
    enum ConnectionNotifyChannel
    {
        CONN_NOTIFY_ENABLE = 0b01,
        CONN_NOTIFY_DISABLE = 0b10
    };

    /**
     * @brief Handle of the task being run.
     *
     */
    TaskHandle_t m_taskHandle = NULL;

    /**
     * @brief Checks if the connection needs to be disabled and put to sleep.
     *
     * @param yieldTicks is the maximum ticks to wait (i.e. can be used as a delay that might exit early).
     *
     * @return true `disable()` has been called since the last check.
     * @return false `disable()` has not been called since the last check.
     */
    bool isDisableWaiting(uint32_t yieldTicks);

    /**
     * @brief State for when the connection is disabled and in low power.
     *
     * This waits for a notification to re-enable.
     *
     */
    class StateDisabled : public State
    {
    public:
        StateDisabled(State &enableState) : State("Disabled"), m_enableState(enableState) {}
        virtual State *enter();

    private:
        State &m_enableState;
    } m_stateDisabled;

    private:
    /**
     * @brief Checks / waits for a notification and checks if a particular bit is set.
     * 
     * Only the given bit is cleared if a notification is received.
     * 
     * @param yieldTicks the maximum time delay.
     * @param bits the bits to listen for. Any notification will be received, but only these bits will be cleared and
     *             checked for.
     * @return true a notification was received within the time limit and the given bits were set.
     * @return false a notification was not received or the given bits were not set.
     */
    static bool isNotificationWaiting(uint32_t yieldTicks, uint32_t bits);
};

/**
 * @brief Adds the queue to implement a high speed connection.
 *
 */
class HighSpeedConnection
{
public:
    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the high-speed queues if they don't already exist and calls `Connection::begin()`.
     *
     * @param highSpeedLength maximum length of the high-speed queues.
     */
    virtual void begin(const int highSpeedLength);

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
class IMUConnection
{
public:
    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the IMU queue if they don't already exist and calls `Connection::begin()`.
     *
     * @param imuLength maximum length of the imu queue.
     */
    virtual void begin(const int imuLength);

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

/**
 * @brief Attempts to add data to a queue and has a tantrum if it's full.
 *
 * @param queue the queue to add data to.
 * @param data the data.
 */
void addToQueue(QueueHandle_t queue, void *data);

/**
 * @brief Task that runs the connection.
 * 
 * @param pvParameters contains a pointer to the connection to run.
 */
void taskConnection(void *pvParameters);