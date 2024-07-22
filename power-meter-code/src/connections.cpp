/**
 * @file connections.cpp
 * @brief Classes to handle sending data to the outside world.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-22
 */
#pragma once
#include "Arduino.h"
#include "connections.h"

void Connection::begin(const int housekeepingLength, const int lowSpeedLength)
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
}

void Connection::enable()
{
    xTaskNotifyGiveIndexed(m_taskHandle, CONN_NOTIFY_ENABLE);
}

void Connection::disable()
{
    xTaskNotifyGiveIndexed(m_taskHandle, CONN_NOTIFY_DISABLE);
}

void Connection::run(TaskHandle_t taskHandle)
{
    m_taskHandle = taskHandle;
    runStateMachine("Connections", &m_stateDisabled);
}

void Connection::addHousekeeping(HousekeepingData &data)
{
    addToQueue(m_housekeepingQueue, (void *)&data);
}

void Connection::addLowSpeed(LowSpeedData &data)
{
    addToQueue(m_lowSpeedQueue, (void *)&data);
}

bool Connection::isDisableWaiting(BaseType_t yieldTicks)
{
    return ulTaskNotifyTakeIndexed(CONN_NOTIFY_DISABLE, pdTRUE, yieldTicks);
}

State *Connection::StateDisabled::enter()
{
    // Wait for a connection notification.
    ulTaskNotifyTakeIndexed(CONN_NOTIFY_ENABLE, pdTRUE, portMAX_DELAY);
    return &m_enableState;
}

void HighSpeedConnection::begin(const int highSpeedLength)
{
    // Connection::begin(housekeepingLength, lowSpeedLength);
    m_createSideQueue(SIDE_LEFT, highSpeedLength);
    m_createSideQueue(SIDE_RIGHT, highSpeedLength);
}

void HighSpeedConnection::m_createSideQueue(EnumSide side, int length)
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

inline void HighSpeedConnection::addHighSpeed(HighSpeedData &data, EnumSide side)
{
    addToQueue(m_sideQueues[side], (void *)&data);
}

void IMUConnection::begin(const int imuLength)
{
    // Connection::begin(housekeepingLength, lowSpeedLength);
    // Check if the queue has already been created.
    if (!m_imuQueue)
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

void IMUConnection::addIMU(IMUData &data)
{
    addToQueue(m_imuQueue, (void *)&data);
}

void addToQueue(QueueHandle_t queue, void *data)
{
    const int MAX_DELAY = 5;
    int error = xQueueSend(queue, data, MAX_DELAY);
    if (error != pdTRUE)
    {
        LOGE("Queues", "Couldn't add data to a queue (%d)", error);
    }
}

void taskConnection(void *pvParameters)
{
    Connection *connection = (Connection *)pvParameters;
    connection->run(xTaskGetCurrentTaskHandle());
}
