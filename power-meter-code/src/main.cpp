/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#include <Arduino.h>
#include "defines.h"
#include "src/power_meter.h"
#include "src/states.h"
#include "src/connections.h"
#include "src/connection_mqtt.h"
#include "src/connection_ble.h"
#include "src/config.h"

SemaphoreHandle_t serialMutex;
TaskHandle_t imuTaskHandle, lowSpeedTaskHandle, connectionTaskHandle;
portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
Preferences prefs;

// Main states for the state machine.
extern StateSleep sleepState;
StateActive activeState(sleepState);
StateSleep sleepState(activeState);

PowerMeter powerMeter;
Config config;

// Initialise the connection. We need a pointer to it's parent class that isn't on the stack to use as a task parameter.
MQTTConnection connectionMQTT;
BLEConnection connectionBLE;
Connection *connectionBasePtr;

// #define HW_MAJOR_VERSION_STR #HW_MAJOR_VERSION
// #define HW_MINOR_VERSION_STR #HW_MINOR_VERSION
// #define HW_PATCH_VERSION_STR #HW_PATCH_VERSION

void setup()
{
    // Initialise mutexes
    serialMutex = xSemaphoreCreateMutex(); // Needs to be created before logging anything.
    Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setTimeout(30000);
    LOGI("Setup", "MHP Power meter " SW_VERSION ", " HW_VERSION_STR ". Compiled " __DATE__ ", " __TIME__);

    // Load config
    config.load();
    config.print();

    // Start the hardware.
    powerMeter.begin();
    pinMode(PIN_BOOT, INPUT);

    // Initialise the selected connection.
    switch (config.connectionMethod)
    {
    case CONNECTION_MQTT:
        connectionMQTT.begin();
        connectionBasePtr = &connectionMQTT;
        break;
    case CONNECTION_BLE:
        connectionBLE.begin();
        connectionBasePtr = &connectionBLE;
        break;
    }

    xTaskCreatePinnedToCore(
        taskConnection,
        "Connection",
        9000,
        connectionBasePtr,
        1,
        &connectionTaskHandle,
        1);
    // TODO: Avoid race condition where the task handle is not initialised to enable or disable the connection.
    delay(100); // Very dodgy way to make sure the condition is avoided.

    // Start tasks
    // TODO: Determine RAM allocations

    // Communications need to have started before creating low speed. This also relies on queues created in power meter
    // Side::begin()
    xTaskCreatePinnedToCore(
        taskLowSpeed,
        "LowSpeed",
        4096,
        NULL,
        1, // Low priority.
        &lowSpeedTaskHandle,
        1);
    delay(100);

    // Low speed and communications need to have started before IMU.
    xTaskCreatePinnedToCore(
        taskIMU,
        "IMU",
        4096,
        NULL,
        3, // Make this a higher priority than other tasks.
        &imuTaskHandle,
        1);
    delay(100);

    // Create tasks to read data from ADCs
    powerMeter.sides[SIDE_LEFT].createDataTask(SIDE_LEFT);
    powerMeter.sides[SIDE_RIGHT].createDataTask(SIDE_RIGHT);
}

void loop()
{
    // Run the state machine
    runStateMachine("Main States", &activeState);
}