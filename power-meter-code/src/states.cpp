/**
 * @file states.cpp
 * @brief Framework for the state machine.
 *
 * Loosly based on code from the burgler alarm extension of this project:
 * https://github.com/jgOhYeah/BikeHorn
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-22
 */
#include "states.h"

// Don't include in the header file to stop circular issues.
#include "connection_mqtt.h"
extern MQTTConnection connection;

State *StateActive::enter()
{
    connection.enable();
    delay(30000);
    return &m_sleepState;
}

State *StateSleep::enter()
{
    connection.disable();
    LOGD("Sleep", "Simulating sleeping");
    delay(10000);
    LOGD("Sleep", "Waking up");
    return &m_activeState;
}

void runStateMachine(const char *name, State *initial)
{
    // Run the state machine
    while (initial)
    {
        // LOGD("state", "hi");
        LOGD("states", "Hi");
        LOGI(name, "Changing state to %s", initial->name);
        initial = initial->enter();
    }

    // Was given a null state, so fall through.
    LOGI(name, "State machine finished.");
}