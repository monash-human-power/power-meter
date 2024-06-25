/**
 * @file states.h
 * @brief Framework for the state machine.
 *
 * Loosly based on code from the burgler alarm extension of this project:
 * https://github.com/jgOhYeah/BikeHorn
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-06-26
 */
#pragma once

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
    virtual State *enter() {}

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
    StateActive() : State("Active") {}
    virtual State *enter();
};

/**
 * @brief The sleep state when the device is sleeping and minimising power usage.
 *
 */
class StateSleep : public State
{
public:
    StateSleep() : State("Sleep") {}
    virtual State *enter();
};

/**
 * @brief A struct containing all states.
 *
 */
struct StatesList
{
    StateActive *active;
    StateSleep *sleep;
};