/**
 * @file config.h
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once
#include "../defines.h"
#include <BasicLinearAlgebra.h>
#include <Preferences.h>
#include <ArduinoJson.h>
using namespace BLA;

#define CONF_KEY "power-conf"

// Text to use when printing to JSON.
#define CONF_SG_JSON_TEXT "\
{\
 \"offset\": %10lu,\
 \"coef\": %.10g,\
 \"temp-test\": %.10g,\
 \"temp-coef\": %.10g\
}"

#define CONF_JSON_TEXT "\
{\
 \"connection\": %d,\
 \"q(0,0)\": %.10g,\
 \"q(0,1)\": %.10g,\
 \"q(1,0)\": %.10g,\
 \"q(1,1)\": %.10g,\
 \"r(0,0)\": %.10g,\
 \"r(0,1)\": %.10g,\
 \"r(1,0)\": %.10g,\
 \"r(1,1)\": %.10g,\
 \"imuHowOften\": %d,\
 \"left-strain\": %s,\
 \"right-strain\": %s\
}"

// Maximum length of the text to print to JSON.
#define CONF_SG_JSON_TEXT_LENGTH (sizeof(CONF_SG_JSON_TEXT) + 4*(-5 + 12))
#define CONF_JSON_TEXT_LENGTH (sizeof(CONF_JSON_TEXT) + (-2 + 3) + 8 * (-5 + 12)) + (-2 + 4) + 2*(-2 + CONF_SG_JSON_TEXT_LENGTH)

/**
 * @brief Class that bundles all the configs / calibration settings for a strain gauge.
 *
 */
class StrainConf
{
public:
    /**
     * @brief Writes the strain gauge config to a string in json format.
     *
     * @param text the buffer to write to.
     * @param length the length available.
     */
    void writeJSON(char *text, uint32_t length);

    /**
     * @brief Reads a JSON document into this object.
     * 
     * @param doc The document to read data from.
     */
    void readJSON(JsonDocument doc);

    uint32_t offset = DEFAULT_STRAIN_OFFSET;
    float coefficient = DEFAULT_STRAIN_COEFFICIENT;
    float tempTest = DEFAULT_STRAIN_TEST_TEMP;
    float tempCoefficient = DEFAULT_STRAIN_TEMP_CO;
};

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
     * @brief Reads new config in from the serial terminal.
     * 
     */
    void serialRead();

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
     * @param length the maximum length. Should be at least `CONF_JSON_TEXT_LENGTH`.
     */
    void writeJSON(char *text, uint32_t length);

    /**
     * @brief Toggles the connection between WiFi / MQTT and BLE.
     *
     */
    void toggleConnection();

    /**
     * @brief Removes the config key from storage.
     * 
     */
    void removeKey();

    EnumConnection connectionMethod = CONNECTION_MQTT;
    Matrix<2, 2, float> qEnvCovariance = DEFAULT_KALMAN_Q;
    Matrix<2, 2, float> rMeasCovariance = DEFAULT_KALMAN_R;
    int8_t imuHowOften = 1; // Set to -1 to disable sending IMU data.
    StrainConf strain[2];

    // TODO: MQTT and WiFi settings, device name, MQTT prefix.
};