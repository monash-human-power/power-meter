/**
 * @file states.h
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
#pragma once
#include "../defines.h"
#include "power_meter.h"
/**
 * @brief Parent class for a state in a state machine.
 *
 */
class State
{
public:
    /**
     * @brief Runs the state.
     *
     * @return State* the next state to run. To break out of the state
     *         machine, return nullptr.
     */
    virtual State *enter();

    /**
     * @brief A string representing the name of the state. Useful for debugging purposes.
     *
     * This is initialised in the protected constructor.
     */
    const char *name;

protected:
    /**
     * @brief Initialises the State object with the given name.
     *
     * @param name the name of the state to use.
     */
    State(const char *name) : name(name) {}
};

/**
 * @brief The active state when the device is not sleeping.
 *
 */
class StateActive : public State
{
public:
    StateActive(State &sleepState, State &flatState) : State("Active"), m_sleepState(sleepState), m_flatState(flatState) {}
    virtual State *enter();

private:
    State &m_sleepState;
    State &m_flatState;

    uint32_t m_flatSuccessiveReadings = MINIMUM_BATTERY_SUCCESSIVE;

    /**
     * @brief Checks if the power meter is deemed to be inactive according to the IMU / rotation counter (no forwards rotations for x min).
     * 
     * @return true Power meter is active (don't enter sleep mode).
     * @return false Inactive, sleep mode recommended.
     */
    bool m_isActive();
};

/**
 * @brief The sleep state when the device is sleeping and minimising power usage.
 *
 */
class StateSleep : public State
{
public:
    StateSleep(State &activeState) : State("Sleep"), m_activeState(activeState) {}
    virtual State *enter();

private:
    State &m_activeState;
};

/**
 * @brief Shuts the unit down permenantly until it is reset by changing the battery (or pressing reset).
 * 
 */
class StateFlatBattery : public State
{
    public:
    StateFlatBattery() : State("Flat battery") {}
    virtual State *enter();
};

/**
 * @brief Shuts down the system and reboots.
 * 
 * @param dfu if true, will reboot into DFU mode. If false, will restart into application mode.
 */
void reboot(bool dfu = false);

/**
 * @brief Runs the state machine.
 *
 * @param name the name to use for debugging.
 * @param initial the initial state.
 */
void runStateMachine(const char *name, State *initial);

/**
 * @brief Prints some help text to use the serial command line.
 * 
 */
void printHelp();