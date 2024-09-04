/**
 * @file connection_ble.h
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once
#include "../defines.h"
#include "connections.h"
#include <ArduinoBLE.h>

/**
 * @brief Bits / config flags for the Cycling Power Measurement Characteristic.
 * See table 3.2 of the Cycling Power Service specification document for more details.
 * https://www.bluetooth.com/specifications/specs/cycling-power-service-1-1/
 *
 * For even more details, see the GATT specification supplement, section 3.65
 * https://btprodspecificationrefs.blob.core.windows.net/gatt-specification-supplement/GATT_Specification_Supplement.pdf
 */
namespace BLEMeasCharacteristic
{
    const uint8_t PEDAL_POWER_BALANCE = 1;       // Bit 0, Not present when 0, present when 1.
    const uint8_t PEDAL_POWER_BALANCE_REF = 1;   // Bit 1, Unkown when 0, Left when 1.
    const uint8_t ACCUMULATED_TORQUE = 0;        // Bit 2, Present when 1.
    const uint8_t ACCUMULATED_TORQUE_SOURCE = 0; // Bit 3, Wheel based when 0, crank based when 1.
    const uint8_t WHEEL_REVOLUTION_DATA = 0;     // Bit 4, Present when 1.
    const uint8_t CRANK_REVOLUTION_DATA = 1;     // Bit 5, Present when 1.
    const uint8_t EXTREME_FORCE_MAGNITUDES = 0;  // Bit 6, Present when 1.
    const uint8_t EXTREME_TORQUE_MAGNITUDES = 0; // Bit 7, Present when 1.
    const uint8_t EXTREME_ANGLES = 0;            // Bit 8, Present when 1.
    const uint8_t TOP_DEAD_SPOT_ANGLE = 0;       // Bit 9, Present when 1.
    const uint8_t BOTTOM_DEAD_SPOT_ANGLE = 0;    // Bit 10, Present when 1.
    const uint8_t ACCUMULATED_ENERGY = 0;        // Bit 11, Present when 1.

    // Bit 12, Offset compenstation action required when 1.
    // Offset compensation has no meaning when the corresponding feature bit of the feature characteristic is set to 0.
    // This should thus be set to 0 in this case.
    const uint8_t OFFSET_COMPENSATION_INDICATOR = 0;

    // The combined integer characteristic.
    const uint16_t CHARACTERISTIC = PEDAL_POWER_BALANCE | (PEDAL_POWER_BALANCE_REF << 1) | (ACCUMULATED_TORQUE << 2) |
                                    (ACCUMULATED_TORQUE_SOURCE << 3) | (WHEEL_REVOLUTION_DATA << 4) |
                                    (CRANK_REVOLUTION_DATA << 5) | (EXTREME_FORCE_MAGNITUDES << 6) |
                                    (EXTREME_TORQUE_MAGNITUDES << 7) | (EXTREME_ANGLES << 8) |
                                    (TOP_DEAD_SPOT_ANGLE << 9) | (BOTTOM_DEAD_SPOT_ANGLE << 10) |
                                    (ACCUMULATED_ENERGY << 11) | (OFFSET_COMPENSATION_INDICATOR << 12);
}

/**
 * @brief Bits / flags for the Cycling Power Feature Characteristic. This lists more details on what the power meter
 * is capable of.
 *
 * For details, see the GATT specification supplement, section 3.64
 * https://btprodspecificationrefs.blob.core.windows.net/gatt-specification-supplement/GATT_Specification_Supplement.pdf
 * (Mentioned in this page: https://stackoverflow.com/a/64916298)
 *
 */
namespace BLEFeatureCharacteristic
{
    const uint8_t PEDAL_POWER_BALANCE = 1; // Pedal power balance supported
    const uint8_t ACCUMULATED_TORQUE = 0;
    const uint8_t WHEEL_REVOLUTION_DATA = 0;
    const uint8_t CRANK_REVOLUTION_DATA = 1;
    const uint8_t EXTREME_MAGNITUDES = 0;
    const uint8_t EXTREME_ANGLES = 0;
    const uint8_t TOP_BOTTOM_DEAD_SPOTS = 0;
    const uint8_t ACCUMULATED_ENERGY = 0;
    const uint8_t OFFSET_COMPENSATION_INDICATOR = 0;
    const uint8_t OFFSET_COMPENSATION = 0;
    const uint8_t MEASUREMENT_CONTENT_MASKING = 0;
    const uint8_t MULTIPLE_SENSOR_LOCATIONS = 0;
    const uint8_t CRANK_LENGTH_ADJUSTMENT = 0;
    const uint8_t CHAIN_LENGTH_ADJUSTMENT = 0;
    const uint8_t CHAIN_WEIGHT_ADJUSTMENT = 0;
    const uint8_t SPAN_LENGTH_ADJUSTMENT = 0;
    const uint8_t SENSOR_MEAS_CONTEXT = 0; // 0 is force based, 1 is torque based.
    const uint8_t INSTANTANOUS_MEAS_DIRECTION = 0;
    const uint8_t FACTORY_CALIBRATION_DATE = 0;
    const uint8_t ENHANCED_OFFSET_COMPENSATION_PROCEDURE = 0;

    // Distributed system support.
    //   - `0b00` = Unspecified (Legacy Sensor)
    //   - `0b01` = Not for use in a distributed system
    //   - `0b10` = For use in a distributed system
    //   - `0b11` = Reserved for Future Use
    const uint8_t DISTRIBUTED_SYSTEM = 0b01;

    // Bits after this (22-31) are reserved for future use.

    // The combined integer characteristic.
    const uint32_t CHARACTERISTIC = PEDAL_POWER_BALANCE | (ACCUMULATED_TORQUE << 1) | (WHEEL_REVOLUTION_DATA << 2) |
                                    (CRANK_REVOLUTION_DATA << 3) | (EXTREME_MAGNITUDES << 4) | (EXTREME_ANGLES << 5) |
                                    (TOP_BOTTOM_DEAD_SPOTS << 6) | (ACCUMULATED_ENERGY << 7) |
                                    (OFFSET_COMPENSATION_INDICATOR << 8) | (OFFSET_COMPENSATION << 9) |
                                    (MEASUREMENT_CONTENT_MASKING << 10) | (MULTIPLE_SENSOR_LOCATIONS << 11) |
                                    (CRANK_LENGTH_ADJUSTMENT << 12) | (CHAIN_LENGTH_ADJUSTMENT << 13) |
                                    (CHAIN_WEIGHT_ADJUSTMENT << 14) | (SPAN_LENGTH_ADJUSTMENT << 15) |
                                    (SENSOR_MEAS_CONTEXT << 16) | (INSTANTANOUS_MEAS_DIRECTION << 17) |
                                    (FACTORY_CALIBRATION_DATE << 18) | (ENHANCED_OFFSET_COMPENSATION_PROCEDURE << 19) |
                                    (DISTRIBUTED_SYSTEM << 20);
}

class BLEConnection : public Connection
{
public:
    /**
     * @brief Construct a new BLEConnection object.
     *
     */
    BLEConnection()
        : Connection(m_stateBLEConnect), m_stateBLEConnect(*this), m_stateActive(*this), m_stateShutdown(*this),
          m_cyclingPowerService("1818"), m_cpsLocation("2A5D", BLERead), m_cpsFeature("2A65", BLERead, sizeof(m_featData)),
          m_cpsMeasurement("2A63", BLERead | BLENotify, sizeof(m_measData)) {}

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
            : State("Active"),
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
     * The sensor location characteristic is defined in section 3.196 of the GATT Specification Supplement.
     */
    BLEByteCharacteristic m_cpsLocation;
    BLECharacteristic m_cpsFeature;
    BLECharacteristic m_cpsMeasurement;

    /**
     * @brief Array that stores feature characteristic flags.
     *
     */
    uint8_t m_featData[4] = {
        (uint8_t)(BLEFeatureCharacteristic::CHARACTERISTIC & 0xff),
        (uint8_t)(BLEFeatureCharacteristic::CHARACTERISTIC >> 8),
        (uint8_t)(BLEFeatureCharacteristic::CHARACTERISTIC >> 16),
        (uint8_t)(BLEFeatureCharacteristic::CHARACTERISTIC >> 24),
    };

    /**
     * @brief Array that holds the data to send over BLE as part of the measurement characteristic.
     *
     * These are some comments and notes based on section 3.65 of the GATT specification supplement.
     *
     * Data (varies based on BLEMeasCharacterisitc):
     *   - Flags (`boolean[16]`) - See BLEMeasCharacteristic.
     *   - Power (`sint16`) - Instantaneous power in W.
     *   - Pedal power balance (`uint8`) - Power balance in 0.5% steps, only present if BLEMeasCharacteristic::PEDAL_POWER_BALANCE=1.
     *   - Accumulated torque (`uint16`) - Unit is 1/32 Nm, only present if BLEMeasCharacteristic::ACCUMULATED_TORQUE=1.
     *   - Wheel revolution data (`struct 6`) - Only present if BLEMeasCharacteristic::WHEEL_REVOLUTION_DATA=1.
     *   - Crank revolution data (`struct 4`) - Only present if BLEMeasCharacteristic::CRANK_REVOLUTION_DATA=1.
     *   - Extreme force magnitudes (`struct 4`) - Only present if BLEMeasCharacteristic::EXTREME_FORCE_MAGNITUDES=1.
     *   - Extreme torque magnitudes (`struct 4`) - Only present if BLEMeasCharacteristic::EXTREME_TORQUE_MAGNITUDES=1.
     *   - Extreme angles (`struct 3`) - Only present if BLEMeasCharacteristic::EXTREME_ANGLES=1.
     *   - Top dead spot angle (`uint16`) - Only present if BLEMeasCharacteristic::TOP_DEAD_SPOT_ANGLE=1.
     *   - Bottom dead spot angle (`uint16`) - Only present if BLEMeasCharacteristic::BOTTOM_DEAD_SPOT_ANGLE=1.
     *   - Accumulated energy (`uint16`) - Unit is kJ, only present if BLEMeasCharacteristic::ACCUMULATED_ENERGY=1.
     *
     * Crank revolution data:
     *   - Cumulative crank revolutions (`uint16`)
     *   - Last crank event time (`uint16`) - Unit is in 1/1024 seconds.
     *
     * Extreme forces and torques:
     *   - Maximum (`sint16`) - In N for force, Nm for torque.
     *   - Minimum (`sint16`) - In N for force, Nm for torque.
     *
     * // TODO: Document all the other stuff as this power meter will be capable of most of it.
     */
    uint8_t m_measData[9] = {
        (uint8_t)(BLEMeasCharacteristic::CHARACTERISTIC & 0xff),
        (uint8_t)(BLEMeasCharacteristic::CHARACTERISTIC >> 8),
        0,
        0,
        0,
        0,
        0,
        0};

    BLEDevice m_central;

private:
    /**
     * @brief Scales the given time in us to 1/1024 second units for BLE transmission.
     *
     * @param us the input time in us.
     * @return uint16_t the time in 1024 second units. This will overflow as expected / consitently.
     */
    uint16_t m_scaleTime1024(uint32_t us);
};