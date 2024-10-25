/**
 * @file states.cpp
 * @brief Framework for the state machine.
 *
 * Loosly based on code from the burgler alarm extension of this project:
 * https://github.com/jgOhYeah/BikeHorn
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-10-01
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
    while (m_isActive())
    {
        // Populate and send the housekeeping data. Update the temperatures at the same time.
        HousekeepingData housekeeping;
        housekeeping.temperatures[SIDE_LEFT] = powerMeter.sides[SIDE_LEFT].tempSensor.readTemp();
        housekeeping.temperatures[SIDE_RIGHT] = powerMeter.sides[SIDE_RIGHT].tempSensor.readTemp();
        housekeeping.temperatures[SIDE_IMU_TEMP] = powerMeter.imuManager.getLastTemperature();
        housekeeping.battery = powerMeter.batteryVoltage();
        housekeeping.offsets[SIDE_LEFT] = config.strain[SIDE_LEFT].offset;
        housekeeping.offsets[SIDE_RIGHT] = config.strain[SIDE_RIGHT].offset;
        connectionBasePtr->addHousekeeping(housekeeping);

        // Shut down if the battery is flat a certain number of times in a row.
        if (housekeeping.battery < MINIMUM_BATTERY)
        {
            // Flat this time.
            m_flatSuccessiveReadings--;
            if (!m_flatSuccessiveReadings)
            {
                // Flat several times in a row. Time to shut down.
                return &m_flatState;
            }
        }
        else
        {
            // Reset counter.
            m_flatSuccessiveReadings = MINIMUM_BATTERY_SUCCESSIVE;
        }

        debugMemory();

        // Delay for ~10s whilst checking UI regularly.
        for (int i = 0; i < 100; i++)
        { 
            // If the boot button is pressed, toggle the connection method on next boot.
            if (digitalRead(PIN_BOOT) == LOW)
            {
                // Boot button is pressed.
                LOGI("Housekeeping", "Boot button pressed. Will toggle connection mode.");
                config.toggleConnection();
                config.save();
                reboot(false);
            }

            // Check if we received anything on the serial port and if so, act on it.
            if (Serial && Serial.available())
            {
                char value = Serial.read();
                switch (value)
                {
                case 'r':
                case 'R':
                    // Reboot normally.
                    reboot(false);
                    break;

                case 'p':
                case 'P':
                    // Reboot into download mode.
                    reboot(true);
                    break;

                case 'g':
                case 'G':
                    // Gets the config and prints them.
                    config.print();
                    break;

                case 's':
                case 'S':
                    // Read configs from the serial port and update them.
                    config.serialRead();
                    break;

                case 'f':
                case 'F':
                    // Reset config to defaults.
                    config.removeKey();
                    LOGI("Housekeeping", "Config will be reset on next boot.");
                    break;
                
                case 'c':
                case 'C':
                    // Perform offset compensation.
                    powerMeter.offsetCompensate();
                    break;

                default:
                    // Unrecognised, print an unrecognised message and follow on to the help text.
                    LOGW("Housekeeping", "Unrecognised instruction '%c'.", value);
                case 'h':
                case 'H':
                    printHelp();
                }

                // Clear the buffer
                while (Serial.available())
                {
                    Serial.read();
                }
            }

            delay(100);
        }
    }
    return &m_sleepState;
}

bool StateActive::m_isActive()
{
    // Calculate how long it has been since the last rotation.
    LowSpeedData data;
    powerMeter.imuManager.setLowSpeedData(data);
    uint32_t duration = (micros() - data.timestamp) / 1000000L;

    // Work out if it has been too long.
    return config.sleepTime == 0 || duration < config.sleepTime;
}

State *StateSleep::enter()
{
    LOGD("Sleep", "Shutting down the hardware");
    connectionBasePtr->disable();
    powerMeter.leds.setImpendingSleep();
    powerMeter.imuManager.enableMotion();
    delay(2000);
    powerMeter.powerDown();
    LOGD("Sleep", "Going to sleep");
#ifdef ACCEL_RTC_CAPABLE
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_ACCEL_INTERRUPT, HIGH);
#else
    LOGE("Sleep", "This hardware version can't accept accelerometer interrupts in sleep mode!");
    esp_sleep_enable_timer_wakeup(20000000); // 20s to simulate
#endif

    // Start deep sleep. The MCU will restart on wake.
    esp_deep_sleep_start();
}

State *StateFlatBattery::enter()
{
    LOGD("Flat", "Shutting down the hardware");
    connectionBasePtr->disable();
    powerMeter.leds.setImpendingSleep();
    powerMeter.imuManager.enableMotion(); // TODO: Shutdown entirely.
    delay(2000);
    powerMeter.powerDown();
    LOGD("Flat", "Going to sleep permenantly");
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_deep_sleep_start();
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
        powerMeter.sides[SIDE_LEFT].tempSensor.setLED(true);
        powerMeter.sides[SIDE_LEFT].tempSensor.setLED(true);
        digitalWrite(PIN_LEDR, HIGH);
        digitalWrite(PIN_LEDG, HIGH);
        delay(100);
        powerMeter.sides[SIDE_LEFT].tempSensor.setLED(false);
        powerMeter.sides[SIDE_LEFT].tempSensor.setLED(false);
        digitalWrite(PIN_LEDR, LOW);
        digitalWrite(PIN_LEDG, LOW);
        delay(100);
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

inline void printHelp()
{
    SERIAL_TAKE();
    log_printf("Usage:\n  - 'r' reboots.\n  - 'p' reboots into DFU mode.\n  - 'g' gets the current config.\n  - 's' sets the latest config.\n  - 'f' removes saved presences so they will be set to defaults on next boot.\n  - 'c' performs offset compensation.\n  - 'h' prints this help message.\n");
    SERIAL_GIVE();
}

State *State::enter()
{
    return nullptr;
}