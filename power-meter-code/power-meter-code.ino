/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-11
 */

#include "defines.h"
#include "src/power_meter.h"
#include "src/states.h"
#include "src/connections.h"
#include "src/connection_mqtt.h"
#include "src/connection_ble.h"
#include "src/config.h"

SemaphoreHandle_t serialMutex;
TaskHandle_t imuTaskHandle;
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

void setup()
{
    // Initialise mutexes
    serialMutex = xSemaphoreCreateMutex(); // Needs to be created before logging anything.
    // Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setDebugOutput(true);
    LOGI("Setup", "MHP Power meter v" VERSION ". Compiled " __DATE__ ", " __TIME__);

    // Load config
    config.load();
    config.print();

    // Start the hardware.
    powerMeter.begin();

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
        NULL,
        1);
    // TODO: Avoid race condition where the task handle is not initialised to enable or disable the connection.
    delay(100); // Very dodgy way to make sure the condition is avoided.
    // Start tasks
    // TODO: Determine RAM allocations
    xTaskCreatePinnedToCore(
        taskIMU,
        "IMU",
        4096,
        NULL,
        2, // Make this a higher priority than other tasks.
        &imuTaskHandle,
        1);
    delay(100);
}

void loop()
{
    // Run the state machine
    runStateMachine("Main States", &activeState);
}