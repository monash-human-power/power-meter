/**
 * @file connection_ble.cpp
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-23
 */
#include "connection_ble.h"
extern SemaphoreHandle_t serialMutex;

void BLEConnection::begin()
{
    // Initialise the queues. Don't use high speed data.
    const int housekeeping = 1;
    const int lowSpeed = 1;
    const int highSpeed = 0;
    const int imu = 0;
    Connection::begin(housekeeping, lowSpeed, highSpeed, imu);

    if (!BLE.begin())
    {
        LOGE("BLE", "Starting BLE failed");
    }

    LOGD("BLE", "Measurement characteristic is '0x%x'", BLEMeasCharacteristic::CHARACTERISTIC);
    LOGD("BLE", "Feature characteristic is '0x%lx'", BLEFeatureCharacteristic::CHARACTERISTIC);

    BLE.setLocalName(DEVICE_NAME);
    BLE.setAdvertisedService(m_cyclingPowerService);
    m_cyclingPowerService.addCharacteristic(m_cpsMeasurement);
    m_cyclingPowerService.addCharacteristic(m_cpsFeature);
    m_cyclingPowerService.addCharacteristic(m_cpsLocation);
    BLE.addService(m_cyclingPowerService); // TODO: Add battery service
    // TODO: Finish up
}

State *BLEConnection::StateBLEConnect::enter()
{
    // Start connection process
    BLE.advertise();
    LOGI("BLE", "Waiting for central to connect");

    // Wait for a central device to connect
    do {
        m_connection.m_central = BLE.central();
        DELAY_WITH_DISABLE(2);
    } while(!m_connection.m_central);

    // Connected successfully, 
    String address = m_connection.m_central.address();
    LOGI("BLE", "BLE Connected to '%s'", address.c_str());
    return &m_connection.m_stateActive;
}

State *BLEConnection::StateActive::enter()
{
    while (m_connection.m_central.connected())
    {
        // TODO
    }
    LOGI("BLE", "Lost connection");
    return &m_connection.m_stateBLEConnect;
}

State *BLEConnection::StateShutdown::enter()
{

    BLE.stopAdvertise();
    // TODO
}