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
 * @date 2024-08-08
 */
#include "states.h"
extern SemaphoreHandle_t serialMutex;

// Don't include in the header file to stop circular issues.
#include "connections.h"
extern Connection *connectionBasePtr;
extern PowerMeter powerMeter;

State *StateActive::enter()
{
    connectionBasePtr->enable();
    powerMeter.powerUp();

    // Wait in this state forever temporarily.
    bool state = false;
    while (1)
    {
        state = !state;
        LOGD("Temp", "Turning LED %d", state);
        powerMeter.sides[SIDE_RIGHT].temperature.setLED(state);
        delay(2000);
        for (int i = 0; i < 5; i++)
        {
            float temp = powerMeter.sides[SIDE_RIGHT].temperature.readTemp();
            LOGD("Temp", "Temperature is %f C", temp);
            delay(1000);
        }
    }
    // delay(30000);
    return &m_sleepState;
}

State *StateSleep::enter()
{
    connectionBasePtr->disable();
    powerMeter.powerDown();
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
        LOGI(name, "Changing state to %s", initial->name);
        initial = initial->enter();
    }

    // Was given a null state, so fall through.
    LOGI(name, "State machine finished.");
}