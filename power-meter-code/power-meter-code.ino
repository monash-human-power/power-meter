/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-04
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
}

void loop()
{
    // Run the state machine
    State *current = &activeState;
    while (current)
    {
        LOGI("StateChange", current->name);
        current = current->enter();
    }
}

/**
 * @brief Interrupt called when the IMU has new data during tracking.
 * 
 */
void irqIMUActive()
{
    // Notify the IMU task. If the IMU task has a higher priority than the one currently running, force a context
    // switch.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(imuTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Interrupt called when the IMU detets movement to wake the unit up.
 * 
 */
void irqIMUWake()
{
    // TODO
    LOGI("Wake", "Wakeup interrupt received");
}

/**
 * @brief Function to conveniently handle data from the IMU.
 * 
 */
void callbackProcessIMU(inv_imu_sensor_event_t *evt)
{
    powerMeter.imuManager.processIMUEvent(evt);
}