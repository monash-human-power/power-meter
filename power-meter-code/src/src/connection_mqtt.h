/**
 * @file connection_mqtt.h
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-10
 */
#pragma once
#include "../defines.h"
#include "connections.h"
#include "states.h"
#include "ota.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>

/**
 * @brief MQTT Topics
 *
 */
#define MQTT_TOPIC_PREFIX "/power/"
#define MQTT_TOPIC_ABOUT MQTT_TOPIC_PREFIX "about"
#define MQTT_TOPIC_HOUSEKEEPING MQTT_TOPIC_PREFIX "housekeeping"
#define MQTT_TOPIC_LOW_SPEED MQTT_TOPIC_PREFIX "power"
#define MQTT_TOPIC_HIGH_SPEED MQTT_TOPIC_PREFIX "fast/"
#define MQTT_TOPIC_IMU MQTT_TOPIC_PREFIX "imu"
#define MQTT_TOPIC_LEFT "left"
#define MQTT_TOPIC_RIGHT "right"
#define MQTT_TOPIC_CONFIG MQTT_TOPIC_PREFIX "conf"
#define MQTT_TOPIC_OFFSET_COMPENSATE MQTT_TOPIC_PREFIX "offset"

#define MQTT_FAST_BUFFER 160
#define MQTT_BUFFER_LENGTH (MQTT_FAST_BUFFER*IMUData::IMU_BYTES_SIZE + 100)

/**
 * @brief Connection that handles MQTT messages.
 * 
 */
class MQTTConnection : public Connection
{
public:
    /**
     * @brief Construct a new MQTTConnection object.
     *
     */
    MQTTConnection()
        : Connection(m_stateWiFiConnect),
          m_stateWiFiConnect(*this),
          m_stateMQTTConnect(*this),
          m_stateActive(*this),
          m_stateShutdown(*this) {}

    /**
     * @brief Initialises the connection
     *
     */
    virtual void begin();

protected:
    /**
     * @brief Checks the queues and acts on any new messages to publish.
     *
     */
    void runActive();

private:
    /**
     * @brief Checks if there is data for a side to send and does the sending if needed.
     *
     * @param side the side to check.
     */
    void m_handleSideQueue(EnumSide side);

    /**
     * @brief Checks if there is any data for the IMU queue and does the sending if needed.
     *
     */
    void m_handleIMUQueue();

    /**
     * @brief State for connecting to WiFi.
     *
     */
    class StateWiFiConnect : public State
    {
    public:
        /**
         * @brief Construct a new State WiFi Connect object
         *
         * @param connection is the MQTTConnection object to operate on.
         */
        StateWiFiConnect(MQTTConnection &connection)
            : State("WiFi"), m_connection(connection) {}

        /**
         * @brief Connects to WiFi.
         *
         * @return State* Once connected, returns `m_stateMQTTConnect`.
         * @return State* If interrupted, returns `m_stateDisconnect`.
         */
        virtual State *enter();

    private:
        MQTTConnection &m_connection;

    } m_stateWiFiConnect;

    /**
     * @brief State for connecting to MQTT.
     *
     */
    class StateMQTTConnect : public State
    {
    public:
        /**
         * @brief Construct a new State MQTT Connect object
         *
         * @param connection is the MQTTConnection object to operate on.
         */
        StateMQTTConnect(MQTTConnection &connection)
            : State("MQTT"),
              m_connection(connection) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* Once connected, returns `m_stateActive`.
         * @return State* If interrupted, returns `m_stateDisconnect`.
         */
        virtual State *enter();

    private:
        MQTTConnection &m_connection;

    } m_stateMQTTConnect;

    /**
     * @brief State for actively publishing data.
     *
     */
    class StateActive : public State
    {
    public:
        /**
         * @brief Construct a new State Active object
         *
         * @param connection is the MQTTConnection object to operate on.
         */
        StateActive(MQTTConnection &connection)
            : State("Active"),
              m_connection(connection) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

        /**
         * @brief Publishes an "about" message on MQTT on connection.
         *
         * Set this function as the on-connect callback.
         */
        void sendAboutMQTTMessage();

    private:
        MQTTConnection &m_connection;

    } m_stateActive;

    /**
     * @brief State for shutting down into a low-power mode.
     *
     */
    class StateShutdown : public State
    {
    public:
        /**
         * @brief Construct a new State Disable object
         *
         * @param connection is the MQTTConnection object to operate on.
         */
        StateShutdown(MQTTConnection &connection)
            : State("Shutdown"), m_connection(connection) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        MQTTConnection &m_connection;

    } m_stateShutdown;
};

#define STRINGS_MATCH(A, B) (strcmp(A, B) == 0)

/**
 * @brief Updates and saves the config.
 * 
 * @param payload the JSON formatted config.
 * @param length the length of the string.
 */
void mqttUpdateConf(char *payload, unsigned int length);

void mqttCallback(const char* topic, byte* payload, unsigned int length);