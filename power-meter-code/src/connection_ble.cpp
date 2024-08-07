/**
 * @file connection_ble.cpp
 * @brief Class to handle sending data to the outside world over Bluetooth Low Energy (BLE).
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-07
 */
#include "connection_ble.h"
extern SemaphoreHandle_t serialMutex;

void BLEConnection::begin()
{
    LOGE("BLE", "BLE CONNECTION IS NOT IMPLEMENTED YET!!!");
}

State *BLEConnection::StateBLEConnect::enter()
{
    LOGE("BLE", "BLE Connect state not implemented!!!");
    return nullptr;
}
