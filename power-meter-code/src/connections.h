/**
 * @file connections.h
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-11
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
    Connection(State &enableState) : m_stateDisabled(*this, enableState) {}

    /**
     * @brief Sets up the connection.
     *
     * As a base class, this sets up the low-speed and housekeeping queues if they don't already exist.
     *
     * @param housekeepingLength maximum length of the housekeeping queue.
     * @param lowSpeedLength maximum length of the low-speed queue.
     * @param highSpeedLength maximum length of the high-speed queues. Set length to 0 to disable high speed data
     *                        recording.
     * @param imuLength maximum length of the imu queue. Set length to 0 to disable IMU recording.
     */
    virtual void begin(const int housekeepingLength, const int lowSpeedLength, const int highSpeedLength = 0, const int imuLength = 0);

    /**
     * @brief Enables the connection after sleep or on startup.
     *
     * Sends a notification to the connection task. Remember to call `setAllowData(true)` when everything is set up to
     * start accepting data.
     *
     */
    void enable();

    /**
     * @brief Stops the connection and enters a low power mode.
     *
     * Sends a notification to the connection task. Remember to call `setAllowData(false)` to stop new data being
     * added to the queue.
     */
    void disable();

    /**
     * @brief Runs the task that manages the connection.
     *
     * This starts the state machine in the disabled state. When `enable()` is called, the next state will be called
     * automatically.
     *
     */
    void run(TaskHandle_t taskHandle);

    /**
     * @brief Adds housekeeping data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data The housekeeping data to send.
     */
    void addHousekeeping(HousekeepingData &data);

    /**
     * @brief Adds low speed data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the low speed data.
     */
    void addLowSpeed(LowSpeedData &data);

    /**
     * @brief Adds high-speed data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the high-speed data.
     * @param side is the side.
     */
    void addHighSpeed(HighSpeedData &data, EnumSide side);

    /**
     * @brief Adds high-speed IMU data that can be transmitted.
     *
     * Data is added to a queue to ensure this is thread-safe.
     *
     * @param data is the IMU data.
     */
    void addIMU(IMUData &data);

    /**
     * @brief Sets whether the connection should accept data to transmit.
     * 
     * @param state 
     */
    void setAllowData(bool state);

protected:
    /**
     * @brief Queues that all connection types are expected to accept.
     *
     */
    QueueHandle_t m_housekeepingQueue = 0;
    QueueHandle_t m_lowSpeedQueue = 0;

    /**
     * @brief Queues that all connection types are expected to accept.
     *
     * Using an array to allow for each side to be easily addressed.
     *
     */
    QueueHandle_t m_sideQueues[2] = {0, 0};

    /**
     * @brief Queues that all connection types are expected to accept.
     *
     * Using an array to allow for each side to be easily addressed.
     *
     */
    QueueHandle_t m_imuQueue = 0;

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
        StateDisabled(Connection &connection, State &enableState) : State("Disabled"), m_connection(connection), m_enableState(enableState) {}
        virtual State *enter();

    private:
        State &m_enableState;
        Connection &m_connection;
    } m_stateDisabled;

    /**
     * @brief Checks if the connection is currently active (can the device send data to the outside world)?
     * 
     * This check is thread safe. It is controlled by the `setAllowData` method.
     *
     * @return true connected.
     * @return false not connected.
     */
    bool m_isConnected();

private:
    /**
     * @brief Attempts to add data to a queue and has a tantrum if it's full.
     *
     * @param queue the queue to add data to.
     * @param data the data.
     */
    void m_addToQueue(QueueHandle_t queue, void *data);

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

    /**
     * @brief Creates the queue for a side
     *
     * @param side the side.
     * @param length the length of the queue.
     */
    void m_createSideQueue(EnumSide side, int length);

    bool m_connected = false;

};


/**
 * @brief Task that runs the connection.
 *
 * @param pvParameters contains a pointer to the connection to run.
 */
void taskConnection(void *pvParameters);