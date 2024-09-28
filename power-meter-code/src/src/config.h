/**
 * @file config.h
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-10
 */
#pragma once
#include "../defines.h"
#include <BasicLinearAlgebra.h>
#include <Preferences.h>
#include <ArduinoJson.h>
using namespace BLA;

#define CONF_KEY "power-conf"
#define CONF_JSON_TEXT_LENGTH 1000

/**
 * @brief Class that bundles all the configs / calibration settings for a strain gauge.
 *
 */
class StrainConf
{
public:
    /**
     * @brief Writes the strain gauge config to a JsonObject.
     *
     * @param json is the object to write to.
     */
    void writeJSON(JsonObject json);

    /**
     * @brief Reads a JSON document into this object.
     *
     * @param doc The document to read data from.
     */
    void readJSON(JsonObject doc);

    uint32_t offset = DEFAULT_STRAIN_OFFSET;
    float coefficient = DEFAULT_STRAIN_COEFFICIENT;
    float tempTest = DEFAULT_STRAIN_TEST_TEMP;
    float tempCoefficient = DEFAULT_STRAIN_TEMP_CO;
};

#define CONF_WIFI_SSID_MAX_LENGTH 40   // Includes terminating null.
#define CONF_WIFI_PSK_MAX_LENGTH 64    // Includes terminating null.
#define CONF_MQTT_BROKER_MAX_LENGTH 64 // Includes terminating null.

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
    void writeJSON(char *text, uint32_t length, bool showWiFi = false);

    /**
     * @brief Writes the config to a JSON object.
     *
     * @param json is the object to write to.
     */
    void writeJSON(JsonVariant doc, bool showWiFi = false);

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
    uint16_t mqttPacketSize = 50;
    char wifiSSID[CONF_WIFI_SSID_MAX_LENGTH] = DEFAULT_WIFI_SSID;
    char wifiPSK[CONF_WIFI_PSK_MAX_LENGTH] = DEFAULT_WIFI_PASSWORD;
    char mqttBroker[CONF_MQTT_BROKER_MAX_LENGTH] = DEFAULT_MQTT_BROKER;
    // TODO: MQTT and WiFi settings, device name, MQTT prefix.

private:
    /**
     * @brief Safely copies a string and makes sure that the last char is null.
     *
     * @param dest The destination to copy to.
     * @param source The source string.
     * @param maxLength The maximum length to copy.
     */
    void m_safeReadString(char *dest, const char *source, size_t maxLength);

    /**
     * @brief Writes a matrix to a json document.
     *
     * @param json JSON array to write to.
     * @param matrix Matrix to use values from.
     */
    void m_writeMatrix(JsonArray jsonArray, Matrix<2, 2, float> matrix);

    /**
     * @brief Reads a 2x2 matrix from a JSON array.
     *
     * @param jsonArray the matrix to read from.
     * @return Matrix<2, 2, float> the matrix.
     */
    Matrix<2, 2, float> m_readMatrix(JsonArray jsonArray);
};