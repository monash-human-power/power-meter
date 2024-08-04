/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-05
 */

#include "defines.h"
#include "src/power_meter.h"
#include "src/states.h"
#include "src/connections.h"
#include "src/connection_mqtt.h"

SemaphoreHandle_t serialMutex;
TaskHandle_t imuTaskHandle;
portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

extern StateSleep sleepState;
StateActive activeState(sleepState);
StateSleep sleepState(activeState);

PowerMeter powerMeter;

// Initialise the connection. We need a pointer to it's parent class that isn't on the stack to use as a task parameter.
MQTTConnection connection;
Connection* connectionBasePtr = &connection;

void setup()
{
    // Initialise mutexes
    serialMutex = xSemaphoreCreateMutex(); // Needs to be created before logging anything.
    // Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setDebugOutput(true);
    LOGI("Setup", "MHP Power meter v" VERSION ". Compiled " __DATE__ ", " __TIME__);

    // Start the hardware.
    powerMeter.begin();

    // Initialise the connection.
    connection.begin();
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
        2048,
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