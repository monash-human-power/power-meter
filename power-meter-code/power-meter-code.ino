/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-22
 */

#include "defines.h"
#include "src/power_meter.h"
#include "src/states.h"
#include "src/connections.h"
#include "src/connection_mqtt.h"

SemaphoreHandle_t serialMutex;
TaskHandle_t imuTaskHandle;

extern StateSleep sleepState;
StateActive activeState(sleepState);
StateSleep sleepState(activeState);

PowerMeter powerMeter;
MQTTConnection connection;

void setup()
{
    // Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setDebugOutput(true);
    LOGI("Setup", "MHP Power meter v" VERSION ". Compiled " __DATE__ ", " __TIME__);

    // Start the hardware.
    // powerMeter.begin();

    // Initialise the connection.
    connection.begin();
    xTaskCreatePinnedToCore(
        taskConnection,
        "Connection",
        8000,
        (void *)&connection,
        1,
        NULL,
        1);

    // Start tasks
    // TODO: Determine RAM allocations
    // xTaskCreatePinnedToCore(
    //     taskIMU,
    //     "IMU",
    //     4096,
    //     NULL,
    //     1,
    //     &imuTaskHandle,
    //     1);
}

void loop()
{
    // Run the state machine
    // runStateMachine("Main States", &activeState);
}