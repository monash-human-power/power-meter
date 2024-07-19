/**
 * @file connection_mqtt.h
 * @brief Class to handle sending data to the outside world over WiFi / MQTT.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-20
 */
#pragma once
#include "../defines.h"
#include "connections.h"
#include "states.h"
#include <WiFi.h>
#include <PubSubClient.h>

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
          m_stateActive(m_stateShutdown),
          m_stateShutdown(m_stateDisabled) {}

    /**
     * @brief Initialises the connection
     *
     */
    virtual void begin();

private:
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
         * @param shutdownState the state to transition to when interrupted.
         */
        StateActive(State &shutdownState)
            : State("Active"), m_shutdownState(shutdownState) {}

        /**
         * @brief Connects to MQTT.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        State &m_shutdownState;

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