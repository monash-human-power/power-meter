/**
 * @file connection_ble.cpp
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-10
 */
#include "connection_ble.h"
extern SemaphoreHandle_t serialMutex;
#include "power_meter.h"
extern PowerMeter powerMeter;

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

    LOGD("BLE", "Measurement characteristic is '0x%4x'", BLEMeasCharacteristic::CHARACTERISTIC);
    LOGD("BLE", "Feature characteristic is '0x%8lx'", BLEFeatureCharacteristic::CHARACTERISTIC);

    BLE.setLocalName(DEVICE_NAME);
    BLE.setAdvertisedService(m_cyclingPowerService);
    m_cyclingPowerService.addCharacteristic(m_cpsMeasurement);
    m_cyclingPowerService.addCharacteristic(m_cpsFeature);
    m_cyclingPowerService.addCharacteristic(m_cpsLocation);
    BLE.addService(m_cyclingPowerService); // TODO: Add battery service
    // TODO: Finish up
}

void BLEConnection::runActive()
{
    // Check the housekeeping queue
    HousekeepingData housekeeping;
    if (xQueueReceive(m_housekeepingQueue, &housekeeping, 0))
    {
        // Housekeeping data can be sent.
        // TODO: Implement
        LOGW("BLE", "Housekeeping not implemented yet");
        
    }
    
    // Check the low-speed queue
    LowSpeedData lowSpeed;
    if (xQueueReceive(m_lowSpeedQueue, &lowSpeed, 0))
    {
        powerMeter.leds.setConnState(CONN_STATE_SENDING);
        // We have data to send.
        // Update the data array.
        // Power
        int16_t power = lowSpeed.power;
        m_measData[2] = (uint8_t)(power);
        m_measData[3] = (uint8_t)(power > 8);

        // Power balance
        m_measData[4] = 2 * lowSpeed.balance; // In 0.5% steps.

        // Crank revolution data
        m_measData[5] = (uint8_t)lowSpeed.rotationCount;
        m_measData[6] = (uint8_t)(lowSpeed.rotationCount >> 8);
        uint16_t time = m_scaleTime1024(lowSpeed.timestamp);
        m_measData[7] = (uint8_t)time;
        m_measData[8] = (uint8_t)(time >> 8);

        // Send
        m_cpsMeasurement.writeValue(m_measData, sizeof(m_measData));

        // TODO: Send these less often?
        m_cpsFeature.writeValue(m_featData, sizeof(m_featData));
        // Set the location of the sensor. Left crank is as good as anywhere. We ideally want a location for cranks in
        // general, but have to choose between left (5) and right (6). A spider-based power meter (15) or pedal-based
        // (7 & 8) may be fitted, so don't wish to cause confusion by using these locations. See section 3.196.1 of the
        // GATT specification supplement for more options.
        m_cpsLocation.writeValue(5);
        powerMeter.leds.setConnState(CONN_STATE_ACTIVE);
    }
}

uint16_t BLEConnection::m_scaleTime1024(uint32_t us)
{
    // Aiming to multiply by 1024/1e6 without running out of bits.
    // Relies on the fact that the output overflows on 2**16, so can take the modulus of anything bigger.
    us %= (1<<12) * 15625;
    us <<= 4;
    us /= 15625;
    return us;
}

State *BLEConnection::StateBLEConnect::enter()
{
    m_connection.setAllowData(false);
    powerMeter.leds.setConnState(CONN_STATE_CONNECTING_1);
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
    m_connection.setAllowData(true);
    powerMeter.leds.setConnState(CONN_STATE_ACTIVE);
    while (m_connection.m_central.connected() && !m_connection.isDisableWaiting(1))
    {
        // Check each queue and send data if present. Queue sets could be useful here.
        // Call a function in the connection so that we have more convenient access to the queues.
        m_connection.runActive();
    }
    LOGI("BLE", "Lost connection");
    return &m_connection.m_stateBLEConnect;
}

State *BLEConnection::StateShutdown::enter()
{
    powerMeter.leds.setConnState(CONN_STATE_SHUTTING_DOWN);
    m_connection.setAllowData(false);
    m_connection.m_central.disconnect();
    LOGI("BLE", "Shutting BLE down.");
    BLE.stopAdvertise(); // TODO: Any other power saving required?
    return &m_connection.m_stateDisabled;
}