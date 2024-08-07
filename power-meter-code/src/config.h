/**
 * @file config.h
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-07
 */
#pragma once
#include "../defines.h"
#include <BasicLinearAlgebra.h>
#include <Preferences.h>

using namespace BLA;
extern Preferences prefs;

#define CONF_KEY "power-conf"
#define MAX_INPUT_LENGTH 40

/**
 * @brief Class that contains configs.
 *
 */
class Config
{
public:
    /**
     * @brief Loads data from flash memory.
     * 
     */
    void load();
    
    /**
     * @brief Saves the preferences to flash memory.
     * 
     */
    void save();

    /**
     * @brief Prints the current settings loaded into RAM.
     * 
     */
    void print();

    /**
     * @brief Command line interface for setting configs.
     * 
     */
    void commandLine();

    EnumConnection connectionMethod = CONNECTION_MQTT;
    Matrix<2, 2, float> qEnvCovariance = DEFAULT_KALMAN_Q;
    Matrix<2, 2, float> rMeasCovariance = DEFAULT_KALMAN_R;
};