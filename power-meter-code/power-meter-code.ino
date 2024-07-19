/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-20
 */

#include "defines.h"
#include "src/power_meter.h"
#include "src/states.h"
#include "src/connections.h"

SemaphoreHandle_t serialMutex;
TaskHandle_t imuTaskHandle;

StateActive activeState;
StateSleep sleepState;

PowerMeter powerMeter;

void setup()
{
    Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setDebugOutput(true);
    LOGI("Setup", "MHP Power meter v" VERSION ". Compiled " __DATE__ ", " __TIME__);

    // Start the hardware.
    powerMeter.begin();
    
    // Start tasks
    // TODO: Determine RAM allocations
    xTaskCreatePinnedToCore(
        taskIMU,
        "IMU",
        4096,
        NULL,
        1,
        &imuTaskHandle,
        1);
}

void loop()
{
    // Run the state machine
    runStateMachine("Main States", &activeState);
}