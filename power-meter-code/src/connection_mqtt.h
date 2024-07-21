/**
 * @file connection_mqtt.h
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-21
 */
#pragma once
#include "../defines.h"
#include "connections.h"
#include "states.h"
#include <WiFi.h>
#include <PubSubClient.h>

/**
 * @brief MQTT Topics
 * 
 */
#define MQTT_TOPIC_PREFIX "/power/"
#define MQTT_TOPIC_HOUSEKEEPING MQTT_TOPIC_PREFIX "housekeeping"
#define MQTT_TOPIC_LOW_SPEED MQTT_TOPIC_PREFIX "power"
#define MQTT_TOPIC_HIGH_SPEED MQTT_TOPIC_PREFIX "fast/"
#define MQTT_TOPIC_IMU MQTT_TOPIC_PREFIX "imu"
#define MQTT_TOPIC_LEFT "left"
#define MQTT_TOPIC_RIGHT "right"

#define MQTT_FAST_BUFFERING 50 // How many datapoints to bundle and send together at a time.
class MQTTConnection : public HighSpeedConnection, public IMUConnection, public Connection
{
public:
    /**
     * @brief Construct a new MQTTConnection object.
     *
     */
    MQTTConnection()
        : Connection(m_stateWiFiConnect),
          m_stateWiFiConnect(m_stateMQTTConnect, m_stateShutdown),
          m_stateMQTTConnect(m_stateWiFiConnect, m_stateActive, m_stateShutdown),
          m_stateActive(*this, m_stateWiFiConnect, m_stateMQTTConnect, m_stateShutdown),
          m_stateShutdown(m_stateDisabled) {}

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
         * @param mqttState the state to transition to if successful.
         * @param shutdownState the state to transition to if failed.
         */
        StateWiFiConnect(State &mqttState, State &shutdownState)
            : State("WiFi"), m_mqttState(mqttState), m_shutdownState(shutdownState) {}

        /**
         * @brief Connects to WiFi.
         *
         * @return State* Once connected, returns `m_stateMQTTConnect`.
         * @return State* If interrupted, returns `m_stateDisconnect`.
         */
        virtual State *enter();

    private:
        State &m_mqttState, &m_shutdownState;

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
         * @param wifiState is the state to transition to if the WiFi connection drops out.
         * @param activeState the state to transition to if successful.
         * @param shutdownState the state to transition to if failed.
         */
        StateMQTTConnect(State &wifiState, State &activeState, State &shutdownState)
            : State("MQTT"),
              m_wifiState(wifiState),
              m_activeState(activeState),
              m_shutdownState(shutdownState) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* Once connected, returns `m_stateActive`.
         * @return State* If interrupted, returns `m_stateDisconnect`.
         */
        virtual State *enter();

    private:
        State &m_wifiState, &m_activeState, &m_shutdownState;

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
         * @param connection the MQTTConnection to look at the queues of. Feels rather evil.
         * @param wifiState the state to transition to when WiFi fails.
         * @param mqttState the state to transition to when MQTT fails.
         * @param shutdownState the state to transition to when interrupted.
         */
        StateActive(MQTTConnection connection, State &wifiState, State &mqttState, State &shutdownState)
            : State("Active"),
              m_connection(connection),
              m_wifiState(wifiState),
              m_mqttState(mqttState),
              m_shutdownState(shutdownState) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        MQTTConnection &m_connection;
        State &m_wifiState, &m_mqttState, &m_shutdownState;

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
         * @param disabledState the low-power state to transition to.
         */
        StateShutdown(State &disabledState)
            : State("Shutdown"), m_disabledState(disabledState) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        State &m_disabledState;

    } m_stateShutdown;
};