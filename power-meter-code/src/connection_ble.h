/**
 * @file connection_ble.h
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-07
 */
#pragma once
#include "../defines.h"
#include "connections.h"
class BLEConnection : public Connection
{
public:
    /**
     * @brief Construct a new BLEConnection object.
     *
     */
    BLEConnection()
        : Connection(m_stateBLEConnect), m_stateBLEConnect(*this) {}

    /**
     * @brief Initialises the connection
     *
     */
    virtual void begin();

protected:
    /**
     * @brief State for connecting to a central BLE device (bike computer).
     *
     */
    class StateBLEConnect : public State
    {
    public:
        /**
         * @brief Construct a new State WiFi Connect object
         *
         * @param connection is the BLEConnection object to operate on.
         */
        StateBLEConnect(BLEConnection &connection)
            : State("BLEConnect"), m_connection(connection) {}

        /**
         * @brief Connects to a central BLE device.
         *
         * // TODO:
         * @return State* Once connected, returns `m_stateActive`.
         * @return State* If interrupted, returns `m_stateDisconnect`.
         */
        virtual State *enter();

    private:
        BLEConnection &m_connection;

    } m_stateBLEConnect;
};