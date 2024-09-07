/**
 * @file connection_mqtt.cpp
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#include "connection_mqtt.h"
#include "config.h"
extern SemaphoreHandle_t serialMutex;
extern Config config;

// MQTT client
WiFiClient wifi;
PubSubClient mqtt(wifi);

// Power meter
#include "power_meter.h"
extern PowerMeter powerMeter;

void MQTTConnection::begin()
{
    // Initialise the queues
    const int housekeeping = 1;
    const int lowSpeed = 1;
    const int highSpeed = MQTT_FAST_BUFFERING + MQTT_FAST_BUFFERING_BUFFER;
    const int imu = MQTT_FAST_BUFFERING + MQTT_FAST_BUFFERING_BUFFER;
    Connection::begin(housekeeping, lowSpeed, highSpeed, imu);

    // Set the buffer to be big enough for the high speed data.
    if (!mqtt.setBufferSize(MQTT_BUFFER_LENGTH))
    {
        LOGE("MQTT", "Couldn't resize the MQTT buffer. Long messages mightn't send");
    }
}

// Logging takes too long, so unless trying to debug stuff, do a simple version.
// #define MQTT_LOG_PUBLISH(topic, payload) \
//     LOGV(topic, "%s", payload);          \
//     uint32_t start = micros();           \
//     mqtt.publish(topic, payload);        \
//     uint32_t end = micros();             \
//     LOGI(topic, "%ldus to publish.", end - start)

#define MQTT_LOG_PUBLISH(topic, payload) mqtt.publish(topic, payload)

// #define MQTT_LOG_PUBLISH_BUF(topic, payload, length) \
//     LOGV(topic, "Binary data of length %d", length); \
//     uint32_t start = micros();                       \
//     mqtt.publish(topic, payload, length);            \
//     uint32_t end = micros();                         \
//     LOGI(topic, "%ldus to publish.", end - start)

#define MQTT_LOG_PUBLISH_BUF(topic, payload, length) mqtt.publish(topic, payload, length)

void MQTTConnection::runActive()
{
    // Check the housekeeping queue
    HousekeepingData housekeeping;
    if (xQueueReceive(m_housekeepingQueue, &housekeeping, 0))
    {
        // Housekeeping data can be sent. Generate a json string.
        char payload[80];
        sprintf(
            payload,
            "{\"temps\":{\"left\":%.2f,\"right\":%.2f, \"imu\":%.2f},\"battery\":%.2f}",
            housekeeping.temperatures[SIDE_LEFT],
            housekeeping.temperatures[SIDE_RIGHT],
            housekeeping.temperatures[SIDE_IMU_TEMP],
            housekeeping.battery);

        // Publish
        MQTT_LOG_PUBLISH(MQTT_TOPIC_HOUSEKEEPING, payload);
    }

    // Check the low-speed queue
    LowSpeedData lowSpeed;
    if (xQueueReceive(m_lowSpeedQueue, &lowSpeed, 0))
    {
        // Low-speed data can be sent. Generate a json string.
        char payload[100];
        sprintf(
            payload,
            "{\"timestamp\":%lu,\"cadence\":%.1f,\"rotations\":%lu,\"power\":%.1f,\"balance\":%.1f}",
            lowSpeed.timestamp,
            lowSpeed.cadence(),
            lowSpeed.rotationCount,
            lowSpeed.power,
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
        const unsigned int PAYLOAD_SIZE = HighSpeedData::FAST_BYTES_SIZE * MQTT_FAST_BUFFERING;
        uint8_t buffer[PAYLOAD_SIZE];
        for (int i = 0; i < MQTT_FAST_BUFFERING; i++)
        {
            // For each point in the queue, get it and add it to the packet buffer.
            HighSpeedData data;
            xQueueReceive(m_sideQueues[side], &data, portMAX_DELAY);
            data.toBytes(buffer + HighSpeedData::FAST_BYTES_SIZE * i);
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
        const unsigned int PAYLOAD_SIZE = IMUData::IMU_BYTES_SIZE * MQTT_FAST_BUFFERING;
        uint8_t buffer[PAYLOAD_SIZE];
        for (int i = 0; i < MQTT_FAST_BUFFERING; i++)
        {
            // For each point in the queue, get it and add it to the packet buffer.
            IMUData data;
            xQueueReceive(m_imuQueue, &data, portMAX_DELAY);
            data.toBytes(buffer + IMUData::IMU_BYTES_SIZE * i);
        }

        // Send the buffer on the correct topic
        MQTT_LOG_PUBLISH_BUF(MQTT_TOPIC_IMU, buffer, PAYLOAD_SIZE);
    }
}

State *MQTTConnection::StateWiFiConnect::enter()
{
    m_connection.setAllowData(false); // Make sure we aren't accepting data until we are ready.
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

#ifdef OTA_ENABLE
    // Setup OTA updates if needed.
    OTAManager::setupOTA();
#endif
    return &m_connection.m_stateMQTTConnect;
}

State *MQTTConnection::StateMQTTConnect::enter()
{
    // Initial setup
    m_connection.setAllowData(false); // Make sure we aren't accepting data until we are ready.
    LOGV("Networking", "Connecting to MQTT broker '" MQTT_BROKER "' on port " xstringify(MQTT_PORT) ".");
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
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

#ifdef OTA_ENABLE
        // Check OTA updates as needed
        OTAManager::handleOTA();
#endif
    }

    // Successfully connected to MQTT
    LOGI("Networking", "Connected to MQTT broker.");
    mqtt.subscribe(MQTT_TOPIC_CONFIG);
    mqtt.subscribe(MQTT_TOPIC_OFFSET_COMPENSATE);
    return &m_connection.m_stateActive;
}

State *MQTTConnection::StateActive::enter()
{
    // Setup to send data
    sendAboutMQTTMessage();
    m_connection.setAllowData(true); // We can start sending data.

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

#ifdef OTA_ENABLE
        // Check OTA updates as needed
        OTAManager::handleOTA();
#endif
    }
    return &m_connection.m_stateShutdown;
}

#define ABOUT_STR "\
{\
 \"name\": \"" DEVICE_NAME "\",\
 \"compiled\": \"" __DATE__ ", " __TIME__ "\",\
 \"version\": \"" VERSION "\",\
 \"connect-time\": %lu,\
 \"calibration\": %s,\
 \"mac\": \"%02x:%02x:%02x:%02x:%02x:%02x\"\
}"
#define ABOUT_STR_BUFFER_SIZE (sizeof(ABOUT_STR) + (-3 + 10) + 6 * (-4 + 2) + CONF_JSON_TEXT_LENGTH) // Remove placeholders, add enough for time and MAC.
void MQTTConnection::StateActive::sendAboutMQTTMessage()
{
    // About info can be sent. Generate a json string.
    uint8_t baseMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    char confJSON[CONF_JSON_TEXT_LENGTH];
    config.writeJSON(confJSON, CONF_JSON_TEXT_LENGTH);
    char payload[ABOUT_STR_BUFFER_SIZE];
    sprintf(
        payload,
        ABOUT_STR,
        millis(),
        confJSON,
        baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

    // Successfully connected.
    // Publish
    MQTT_LOG_PUBLISH(MQTT_TOPIC_ABOUT, payload);
}

State *MQTTConnection::StateShutdown::enter()
{
    m_connection.setAllowData(false); // Stop accepting new data.
    mqtt.disconnect();
    WiFi.disconnect(true, false); // Turn the radio hardware off, keep saved data.
    // TODO: Is disabling OTA updates needed?
    return &m_connection.m_stateDisabled;
}

inline void mqttUpdateConf(char *payload, unsigned int length)
{
    config.readJSON((char *)payload, length);
    config.print();
    config.save();
    LOGI("MQTT", "Finished updating config.");
}

void mqttCallback(const char *topic, byte *payload, unsigned int length)
{
    LOGI("MQTT", "Received a message with topic '%s'.", topic);
    if (STRINGS_MATCH(topic, MQTT_TOPIC_CONFIG))
    {
        mqttUpdateConf((char *)payload, length);
    }
    else if (STRINGS_MATCH(topic, MQTT_TOPIC_OFFSET_COMPENSATE))
    {
        powerMeter.offsetCompensate();
    }
}
