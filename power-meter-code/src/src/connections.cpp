/**
 * @file connections.cpp
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once
#include "connections.h"
extern SemaphoreHandle_t serialMutex;

void Connection::begin(const int housekeepingLength, const int lowSpeedLength, const int highSpeedLength, const int imuLength)
{
    // Housekeeping queue
    // Check if the queue has already been created.
    if (!m_housekeepingQueue)
    {
        // Doesn't exist, create.
        m_housekeepingQueue = xQueueCreate(housekeepingLength, sizeof(HousekeepingData));

        // Check it was created successfully.
        if (!m_housekeepingQueue)
        {
            LOGE("Queues", "Couldn't create housekeeping queue");
        }
    }

    // Low-speed queue
    // Check if the queue has already been created.
    if (!m_lowSpeedQueue)
    {
        // Doesn't exist, create.
        m_lowSpeedQueue = xQueueCreate(lowSpeedLength, sizeof(LowSpeedData));

        // Check it was created successfully.
        if (!m_lowSpeedQueue)
        {
            LOGE("Queues", "Couldn't create low-speed queue");
        }
    }

    // High speed queue. Check if the queue has already been created and is needed.
    if (highSpeedLength)
    {
        m_createSideQueue(SIDE_LEFT, highSpeedLength);
        m_createSideQueue(SIDE_RIGHT, highSpeedLength);
    }

    // Check if the queue has already been created and the queue is needed.
    if (!m_imuQueue && imuLength)
    {
        // Doesn't exist, create.
        m_imuQueue = xQueueCreate(imuLength, sizeof(IMUData));

        // Check it was created successfully.
        if (!m_imuQueue)
        {
            LOGE("Queues", "Couldn't create IMU queue");
        }
    }
}

void Connection::enable()
{
    if (m_taskHandle)
    {
        // Can't use indexed notifications by default in Arduino, so use notification values instead (the notification
        // array is fixed at length 1).
        xTaskNotify(m_taskHandle, CONN_NOTIFY_ENABLE, eSetBits);
    }
    else
    {
        LOGE("Connection", "No task handle to enable.");
    }
}

void Connection::disable()
{
    if (m_taskHandle)
    {
        xTaskNotify(m_taskHandle, CONN_NOTIFY_DISABLE, eSetBits);
    }
    else
    {
        LOGE("Connection", "No task handle to disable.");
    }
}

void Connection::run(TaskHandle_t taskHandle)
{
    m_taskHandle = taskHandle;
    runStateMachine("Connections", &m_stateDisabled);
}

void Connection::addHousekeeping(HousekeepingData &data)
{
    m_addToQueue(m_housekeepingQueue, (void *)&data);
}

void Connection::addLowSpeed(LowSpeedData &data)
{
    m_addToQueue(m_lowSpeedQueue, (void *)&data);
}

void Connection::addHighSpeed(HighSpeedData &data, EnumSide side)
{
    if (m_sideQueues[side])
    {
        m_addToQueue(m_sideQueues[side], (void *)&data);
    }
}

void Connection::addIMU(IMUData &data)
{
    if (m_imuQueue)
    {
        digitalWrite(PIN_LED2, HIGH);
        m_addToQueue(m_imuQueue, &data);
        digitalWrite(PIN_LED2, LOW);
    }
}

bool Connection::isDisableWaiting(uint32_t yieldTicks)
{
    return isNotificationWaiting(yieldTicks, CONN_NOTIFY_DISABLE);
}

inline bool Connection::m_isConnected()
{
    // Accessing a boolean should be monatomic.
    bool connected = m_connected;
    return connected;
}

void Connection::setAllowData(bool state)
{
    // Accessing a boolean should be monatomic.
    m_connected = state;
}

inline bool Connection::isNotificationWaiting(uint32_t yieldTicks, uint32_t bits)
{
    uint32_t notificationValue = 0;
    bool result = xTaskNotifyWait(
        0x00,                // Don't clear anything on entry.
        bits, // Clear disable bit on exit.
        &notificationValue,  // Save the received
        yieldTicks);         // How long to wait
    return result && (notificationValue & bits);
}

void Connection::m_createSideQueue(EnumSide side, int length)
{
    // Check if the queue has already been created.
    if (!m_sideQueues[side])
    {
        // Doesn't exist, create.
        m_sideQueues[side] = xQueueCreate(length, sizeof(HighSpeedData));

        // Check it was created successfully.
        if (!m_sideQueues[side])
        {
            LOGE("Queues", "Couldn't create queue for side %d", side);
        }
    }
}

State *Connection::StateDisabled::enter()
{
    // Make sure we aren't accepting data.
    m_connection.setAllowData(false); // Make sure we aren't accepting data until we are ready.
    // Wait for a notification that also has the enable bits set.
    while (!isNotificationWaiting(portMAX_DELAY, CONN_NOTIFY_ENABLE)) {
        LOGD("DisabledState", "Notification received, but not to enable");
    }
    
    return &m_enableState;
}

void Connection::m_addToQueue(QueueHandle_t queue, void *data)
{
    // Check we can actually accept the data
    if (m_isConnected())
    {
        const int MAX_DELAY = 0;
        int error = xQueueSend(queue, data, MAX_DELAY);
        // if (error != pdTRUE)
        // {
        //     LOGE("Queues", "Couldn't add data to a queue (%d)", error);
        // }
    }
}

void taskConnection(void *pvParameters)
{
    Connection *connection = (Connection *)pvParameters;
    connection->run(xTaskGetCurrentTaskHandle());
}