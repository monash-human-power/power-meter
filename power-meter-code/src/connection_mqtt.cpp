/**
 * @file connection_mqtt.cpp
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-20
 */
#include "connection_mqtt.h"

// MQTT client
WiFiClient wifi;
PubSubClient mqtt(wifi);

void MQTTConnection::begin()
{
    // Initialise the queues
    const int housekeeping = 1;
    const int lowSpeed = 1;
    const int highSpeed = 55;
    const int imu = 55;
    HighSpeedConnection::begin(highSpeed);
    IMUConnection::begin(imu);
    Connection::begin(housekeeping, lowSpeed);
}

#define DELAY_WITH_DISABLE(time)                     \
    if (isDisableWaiting(time / portTICK_PERIOD_MS)) \
    return &m_shutdownState

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
    return &m_mqttState;
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
        DELAY_WITH_DISABLE(500);

        // Check if WiFi is connected and reconnect if needed.
        if (WiFi.status() != WL_CONNECTED)
        {
            return &m_wifiState;
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
    return &m_activeState;
}

State *MQTTConnection::StateActive::enter()
{
    // TODO: Active state code
    return &m_shutdownState;
}

State *MQTTConnection::StateShutdown::enter()
{
    // TODO: Shut down
    return &m_disabledState;
}