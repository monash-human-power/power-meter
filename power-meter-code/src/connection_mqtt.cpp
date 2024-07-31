/**
 * @file connection_mqtt.cpp
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-31
 */
#include "connection_mqtt.h"
extern SemaphoreHandle_t serialMutex;

// MQTT client
WiFiClient wifi;
PubSubClient mqtt(wifi);

void MQTTConnection::begin()
{
    // Initialise the queues
    const int housekeeping = 1;
    const int lowSpeed = 1;
    const int highSpeed = MQTT_FAST_BUFFERING + 5;
    const int imu = MQTT_FAST_BUFFERING + 5;
    HighSpeedConnection::begin(highSpeed);
    IMUConnection::begin(imu);
    Connection::begin(housekeeping, lowSpeed);

    // Set the buffer to be big enough for the high speed data.
    if (!mqtt.setBufferSize(1300))
    {
        LOGE("MQTT", "Couldn't resize the MQTT buffer. Long messages mightn't send");
    }
}

#define MQTT_LOG_PUBLISH(topic, payload) \
    LOGV(topic, "%s", payload);          \
    mqtt.publish(topic, payload)

#define MQTT_LOG_PUBLISH_BUF(topic, payload, length) \
    LOGV(topic, "Binary data of length %d", length); \
    mqtt.publish(topic, payload, length)

void MQTTConnection::runActive()
{
    // Check the housekeeping queue
    HousekeepingData housekeeping;
    if (xQueueReceive(m_housekeepingQueue, &housekeeping, 0))
    {
        // Housekeeping data can be sent. Generate a json string.
        char payload[60];
        sprintf(
            payload,
            "{\"temps\":{\"left\":%.2f,\"right\":%.2f},\"battery\":%.2f}",
            housekeeping.temperatures[SIDE_LEFT],
            housekeeping.temperatures[SIDE_RIGHT],
            housekeeping.battery);

        // Publish
        MQTT_LOG_PUBLISH(MQTT_TOPIC_HOUSEKEEPING, payload);
    }

    // Check the low-speed queue
    LowSpeedData lowSpeed;
    if (xQueueReceive(m_lowSpeedQueue, &lowSpeed, 0))
    {
        // Low-speed data can be sent. Generate a json string.
        char payload[60];
        sprintf(
            payload,
            "{\"power\": %.1f, \"cadence\": %.1f, \"balance\": %.2f}",
            lowSpeed.power,
            lowSpeed.cadence,
            lowSpeed.balance);

        // Publish
        MQTT_LOG_PUBLISH(MQTT_TOPIC_LOW_SPEED, payload);
    }

    // High speed data
    m_handleSideQueue(SIDE_LEFT);
    m_handleSideQueue(SIDE_RIGHT);

    // IMU data
    m_handleIMUQueue();
}

void MQTTConnection::m_handleSideQueue(EnumSide side)
{
    if (uxQueueMessagesWaiting(m_sideQueues[side]) >= MQTT_FAST_BUFFERING)
    {
        // We have enough data to add.
        const unsigned int POINT_SIZE = 24;
        const unsigned int PAYLOAD_SIZE = POINT_SIZE * MQTT_FAST_BUFFERING;
        uint8_t buffer[PAYLOAD_SIZE];
        for (int i = 0; i < MQTT_FAST_BUFFERING; i++)
        {
            // For each point in the queue, get it and add it to the packet buffer.
            HighSpeedData data;
            xQueueReceive(m_sideQueues[side], &data, portMAX_DELAY);
            data.toBytes(buffer + POINT_SIZE * i);
        }

        // Send the buffer on the correct topic
        switch (side)
        {
        case SIDE_LEFT:
            MQTT_LOG_PUBLISH_BUF(MQTT_TOPIC_HIGH_SPEED MQTT_TOPIC_LEFT, buffer, PAYLOAD_SIZE);
            break;
        case SIDE_RIGHT:
            MQTT_LOG_PUBLISH_BUF(MQTT_TOPIC_HIGH_SPEED MQTT_TOPIC_RIGHT, buffer, PAYLOAD_SIZE);
            break;
        }
    }
}

void MQTTConnection::m_handleIMUQueue()
{
    if (uxQueueMessagesWaiting(m_imuQueue) >= MQTT_FAST_BUFFERING)
    {
        // We have enough data to add.
        const unsigned int POINT_SIZE = 12;
        const unsigned int PAYLOAD_SIZE = POINT_SIZE * MQTT_FAST_BUFFERING;
        uint8_t buffer[PAYLOAD_SIZE];
        for (int i = 0; i < MQTT_FAST_BUFFERING; i++)
        {
            // For each point in the queue, get it and add it to the packet buffer.
            IMUData data;
            xQueueReceive(m_imuQueue, &data, portMAX_DELAY);
            data.toBytes(buffer + POINT_SIZE * i);
        }

        // Send the buffer on the correct topic
        MQTT_LOG_PUBLISH_BUF(MQTT_TOPIC_IMU, buffer, PAYLOAD_SIZE);
    }
}

#define DELAY_WITH_DISABLE(ticks)             \
    if (m_connection.isDisableWaiting(ticks)) \
    return &m_connection.m_stateShutdown

State *MQTTConnection::StateWiFiConnect::enter()
{
    // Wait until connected to WiFi.
    do
    {
        // Reset and request connection.
        WiFi.disconnect(); // Just to be safe.
        LOGV("Networking", "Connecting to '" WIFI_SSID "'.");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait until WiFi is connected.
        for (uint32_t iterCount = 0; WiFi.status() != WL_CONNECTED && iterCount < WIFI_RECONNECT_ATTEMPT_TIME; iterCount++)
        {
            // Wait for a while.
            DELAY_WITH_DISABLE(2);
        }
    } while (WiFi.status() != WL_CONNECTED);

    // Successfully connected.
    LOGI("Networking", "Connected with IP address '%s'.", WiFi.localIP().toString().c_str());
    return &m_connection.m_stateMQTTConnect;
}

State *MQTTConnection::StateMQTTConnect::enter()
{
    // Initial setup
    LOGV("Networking", "Connecting to MQTT broker '" MQTT_BROKER "' on port " xstringify(MQTT_PORT) ".");
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    int iterations = 0;

    // Wait until connected
    while (!mqtt.connected())
    {
        mqtt.connect(MQTT_ID); // Modify here to set a password if needed.

        // Wait for a while
        DELAY_WITH_DISABLE(100);

        // Check if WiFi is connected and reconnect if needed.
        if (WiFi.status() != WL_CONNECTED)
        {
            return &m_connection.m_stateWiFiConnect;
        }

        // Attempt to restart the connection every so often.
        iterations++;
        if (iterations == MQTT_RETRY_ITERATIONS)
        {
            LOGD("Networking", "Having another go at connecting MQTT.");
            mqtt.connect(MQTT_ID); // Modify here to set a password if needed.
            iterations = 0;
        }
    }

    // Successfully connected to MQTT
    LOGI("Networking", "Connected to MQTT broker.");
    sendAboutMQTTMessage();
    return &m_connection.m_stateActive;
}

#define ABOUT_STR "\
{\
 \"name\": \"" DEVICE_NAME "\",\
 \"compiled\": \"" __DATE__ ", " __TIME__ "\",\
 \"version\": \"" VERSION "\",\
 \"connect-time\": %lu,\
 \"calibration\": \"{}\"\
}"                                                         // TODO: Calibration data
#define ABOUT_STR_BUFFER_SIZE (sizeof(ABOUT_STR) - 3 + 10) // Remove placeholder, add enough for 1 ul.
void MQTTConnection::StateMQTTConnect::sendAboutMQTTMessage()
{
    // Housekeeping data can be sent. Generate a json string.
    char payload[ABOUT_STR_BUFFER_SIZE];
    sprintf(
        payload,
        ABOUT_STR,
        millis());

    // Publish
    MQTT_LOG_PUBLISH(MQTT_TOPIC_ABOUT, payload);
}

State *MQTTConnection::StateActive::enter()
{
    // Check for data on the queues regularly and publish if so.
    while (!m_connection.isDisableWaiting(1))
    {
        // Check if WiFi is connected and reconnect if needed.
        if (WiFi.status() != WL_CONNECTED)
        {
            return &m_connection.m_stateWiFiConnect;
        }

        // Run the MQTT loop. Check if MQTT is connected and reconnect if needed.
        if (!mqtt.loop())
        {
            return &m_connection.m_stateMQTTConnect;
        }

        // Check each queue and send data if present. Queue sets could be useful here.
        // Call a function in the connection so that we have more convenient access to the queues.
        m_connection.runActive();

        // // Wait for a bit (now part of the notify take).
        // taskYIELD();
    }
    return &m_connection.m_stateShutdown;
}

State *MQTTConnection::StateShutdown::enter()
{
    mqtt.disconnect();
    WiFi.disconnect(true, false); // Turn the radio hardware off, keep saved data.
    return &m_connection.m_stateDisabled;
}