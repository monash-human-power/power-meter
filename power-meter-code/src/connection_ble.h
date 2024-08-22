/**
 * @file connection_ble.h
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-23
 */
#pragma once
#include "../defines.h"
#include "connections.h"
#include <ArduinoBLE.h>

const uint16_t MEAS_CHARACTERISTIC_DEF = 0b0000000000100000;                 // cycle power config flags
const uint32_t FEAT_CHARACTERISTIC_DEF = 0b00000000000100000000000000001000; // 000000000000000000000b;
//                                                   ^^                ^
//                                                   ||                Crank Revolution Data Supported = true
//                                                   Distributed System Support: 00=Legacy Sensor (all power, we must *2), 01=Not for distributed system (all power, we must *2), 10=Can be used in distributed system (if only one sensor connected - Collector may double the value)

class BLEConnection : public Connection
{
public:
    /**
     * @brief Construct a new BLEConnection object.
     *
     */
    BLEConnection()
        : Connection(m_stateBLEConnect), m_stateBLEConnect(*this), m_stateActive(*this), m_stateShutdown(*this),
          m_cyclingPowerService("1818"), m_cpsLocation("2A5D", BLERead), m_cpsFeature("2A65", BLERead, 4),
          m_cpsMeasurement("2A63", BLERead | BLENotify, 8) {}

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
         * @param connection is the BLEConnection object to operate on.
         */
        StateActive(BLEConnection &connection)
            : State("Shutdown"),
              m_connection(connection) {}

        /**
         * @brief Sends data to a connected bike computer over BLE.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        BLEConnection &m_connection;

    } m_stateActive;

    /**
     * @brief State for actively publishing data.
     *
     */
    class StateShutdown : public State
    {
    public:
        /**
         * @brief Construct a new State Active object
         *
         * @param connection is the BLEConnection object to operate on.
         */
        StateShutdown(BLEConnection &connection)
            : State("Shutting down"),
              m_connection(connection) {}

        /**
         * @brief Sends data to a connected bike computer over BLE.
         *
         * @return State* the state to transition to when interrupted.
         */
        virtual State *enter();

    private:
        BLEConnection &m_connection;

    } m_stateShutdown;

    /**
     * @brief Cycling Power Service (CPS).
     *
     * This should be initialised with a UUID of `"1818"` according to the BLE GATT website.
     *
     */
    BLEService m_cyclingPowerService;

    /**
     * @brief Required characteristics for the CPS.
     *
     */
    BLEByteCharacteristic m_cpsLocation;
    BLECharacteristic m_cpsFeature;
    BLECharacteristic m_cpsMeasurement;

    /**
     * @brief Array that stores feature characteristic flags.
     * 
     */
    uint8_t m_featData[4] = {
        (uint8_t)(FEAT_CHARACTERISTIC_DEF & 0xff),
        (uint8_t)(FEAT_CHARACTERISTIC_DEF >> 8),
        (uint8_t)(FEAT_CHARACTERISTIC_DEF >> 16),
        (uint8_t)(FEAT_CHARACTERISTIC_DEF >> 24),
    };

    /**
     * @brief Array that holds the data to send over BLE as part of the measurement characteristic.
     *
     * Data (each field is 16 bit little endian):
     *   - Flags
     *   - Power
     *   - Cranks
     *   - Crant time
     *   - // TODO: Study BLE characteristic and add power balance and other stuff.
     *
     */
    uint8_t m_measData[8] = {
        (uint8_t)(MEAS_CHARACTERISTIC_DEF & 0xff),
        (uint8_t)(MEAS_CHARACTERISTIC_DEF >> 8),
        0,
        0,
        0,
        0,
        0,
        0
    };

    BLEDevice m_central;
};