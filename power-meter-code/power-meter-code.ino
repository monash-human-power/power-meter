/**
 * @file power-meter-code.ino
 * @brief Main file for a cycling power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-06-26
 */

#include "defines.h"
#include "states.h"

SemaphoreHandle_t serialMutex;

StateActive active;
StateSleep sleep;

void setup()
{
    Serial.begin(SERIAL_BAUD); // Already running from the bootloader.
    Serial.setDebugOutput(true);
    LOGI("Setup", "MHP Power meter v" VERSION ". Compiled " __DATE__ ", " __TIME__);
}

void loop()
{
    // Run the state machine
    State *current = &active;
    while (current)
    {
        LOGI("StateChange", current->name);
        current = current->enter();
    }
}