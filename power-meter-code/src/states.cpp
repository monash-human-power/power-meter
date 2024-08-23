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
 * @date 2024-08-24
 */
#include "states.h"
extern SemaphoreHandle_t serialMutex;

// Don't include in the header file to stop circular issues.
#include "connections.h"
extern Connection *connectionBasePtr;
extern PowerMeter powerMeter;
extern Config config;

#include "soc/rtc_cntl_reg.h"

State *StateActive::enter()
{
    connectionBasePtr->enable();
    powerMeter.powerUp();

    // Main housekeeping loop
    while (true)
    {
        // Populate and send the housekeeping data
        HousekeepingData housekeeping;
        // TODO: Store these so they can be accessed in the strain gauge calibration.
        housekeeping.temperatures[SIDE_LEFT] = powerMeter.sides[SIDE_LEFT].temperature.readTemp();
        housekeeping.temperatures[SIDE_RIGHT] = powerMeter.sides[SIDE_RIGHT].temperature.readTemp();
        housekeeping.temperatures[SIDE_IMU_TEMP] = powerMeter.imuManager.getLastTemperature();
        housekeeping.battery = powerMeter.batteryVoltage();
        connectionBasePtr->addHousekeeping(housekeeping);

        // Check if we want to reboot into download mode.
        if (Serial.available() && Serial.read() == 'P')
        {
            LOGI("Housekeeping", "'P' was sent on the serial port. About to reboot into DFU mode.");
            reboot(true);
        }

        // If the boot button is pressed, toggle the connection method on next boot.
        if (digitalRead(PIN_BOOT) == LOW)
        {
            // Boot button is pressed.
            LOGI("Housekeeping", "Boot button pressed. Will toggle connection mode.");
            config.toggleConnection();
            config.save();
            reboot(false);
        }

        // Housekeeping data isn't really frequent.
        delay(10000);
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

void reboot(bool dfu)
{
    LOGW("Reboot", "About to reboot.");
    connectionBasePtr->disable();
    powerMeter.powerDown();
    if (dfu)
    {
        LOGW("Reboot", "Will reboot into DFU mode, reset to exit afterwards.");
        REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    }

    // Allow plenty of time to shut everything down nicely. Flash the LEDs so it is a bit more obvious.
    for (uint8_t i = 0; i < 25; i++)
    {
        powerMeter.sides[SIDE_LEFT].temperature.setLED(true);
        powerMeter.sides[SIDE_LEFT].temperature.setLED(true);
        digitalWrite(PIN_LED1, HIGH);
        digitalWrite(PIN_LED2, HIGH);
        delay (100);
        powerMeter.sides[SIDE_LEFT].temperature.setLED(false);
        powerMeter.sides[SIDE_LEFT].temperature.setLED(false);
        digitalWrite(PIN_LED1, LOW);
        digitalWrite(PIN_LED2, LOW);
        delay (100);
    }

    // Enough warning, should be ok to reboot.
    esp_restart();
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