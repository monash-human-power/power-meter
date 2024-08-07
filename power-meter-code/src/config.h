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
#include <ArduinoJson.h>
using namespace BLA;

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
     * @brief Reads a string containing JSON data and extracts preferences from this.
     * 
     * @param text the text containing the JSON data.
     * @param length the maximum length of the text.
     * @return true successfully read.
     * @return false failed to read.
     */
    bool readJSON(char *text, uint32_t length);

    /**
     * @brief Writes the config to JSON.
     * 
     * @param text the text buffer to put the result in.
     * @param length the maximum length.
     */
    void writeJSON(char *text, uint32_t length);

    EnumConnection connectionMethod = CONNECTION_MQTT;
    Matrix<2, 2, float> qEnvCovariance = DEFAULT_KALMAN_Q;
    Matrix<2, 2, float> rMeasCovariance = DEFAULT_KALMAN_R;
};