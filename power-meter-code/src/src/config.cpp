/**
 * @file config.cpp
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-10
 */
#include "config.h"
extern SemaphoreHandle_t serialMutex;
extern Preferences prefs;

// Uses this to limit the maximum number of packets to queue.
#include "connection_mqtt.h"

void StrainConf::writeJSON(JsonObject json)
{
    json["offset"] = offset;
    json["coef"] = coefficient;
    json["temp-test"] = tempTest;
    json["temp-coef"] = tempCoefficient;
}

void StrainConf::readJSON(JsonObject doc)
{
    offset = doc["offset"];
    coefficient = doc["coef"];
    tempTest = doc["temp-test"];
    tempCoefficient = doc["temp-coef"];
}

void Config::load()
{
    LOGI(CONF_KEY, "Loading preferences");
    prefs.begin(CONF_KEY);

    // Check that the data stored is the correct type and length.
    bool keyExists = prefs.isKey(CONF_KEY);
    if (keyExists && prefs.getType(CONF_KEY) == PT_BLOB && prefs.getBytesLength(CONF_KEY) == sizeof(*this))
    {
        // Everything matches up. Load the preferences.
        prefs.getBytes(CONF_KEY, this, sizeof(*this));
    }
    else
    {
        LOGW(CONF_KEY, "Cannot load preferences. Will reset them");
        // Key doesn't exist, is the wron type or wrong length.
        if (keyExists)
        {
            // Key exists, clear it.
            removeKey();
        }

        // Defaults should have been set on initialise
        save();
    }
}

void Config::save()
{
    LOGI(CONF_KEY, "Saving preferences");
    prefs.begin(CONF_KEY);
    prefs.putBytes(CONF_KEY, this, sizeof(*this));
    LOGV(CONF_KEY, "Finished saving");
}

void Config::print()
{
    char text[CONF_JSON_TEXT_LENGTH];
    writeJSON(text, CONF_JSON_TEXT_LENGTH, true);
    LOGI(CONF_KEY, "Current config: %s", text);
}

void Config::serialRead()
{
    print();
    // Stop all other distracting logging for now:
    LOGI(CONF_KEY, "Paste the new config here:\n");
    char text[CONF_JSON_TEXT_LENGTH];
    Serial.readBytesUntil('\n', text, CONF_JSON_TEXT_LENGTH);
    LOGI(CONF_KEY, "Given '%s'", text);
    if (readJSON(text, CONF_JSON_TEXT_LENGTH))
    {
        LOGI(CONF_KEY, "Successfully set.");
        save();
        print();
    };
}

bool Config::readJSON(char *text, uint32_t length)
{
    // Intellisense seems to be broken for ArduinoJSON at the moment. Ignore the red squiggles for now
    JsonDocument json;
    DeserializationError error = deserializeJson(json, text, length);
    if (error)
    {
        LOGW("Config", "Could not deserialise the json document. Discarding");
        return false;
    }

    // Handle each known connection method
    uint8_t connection = json["connection"];
    switch (connection)
    {
    case CONNECTION_MQTT:
    case CONNECTION_BLE:
        connectionMethod = (EnumConnection)connection;
        break;
    default:
        LOGW("Config", "Unrecognised connection type. Ignoring.");
    }

    // Kalman filters
    JsonObject kalman = json["kalman"];
    qEnvCovariance = m_readMatrix(kalman["Q"]);
    rMeasCovariance = m_readMatrix(kalman["R"]);

    // How often to send IMU data
    imuHowOften = json["imuHowOften"];

    // Read the strain gauge input data.
    strain[SIDE_LEFT].readJSON(json["left-strain"]);
    strain[SIDE_RIGHT].readJSON(json["right-strain"]);

    // MQTT
    JsonVariant mqttDoc = json["mqtt"];

    // Get the length, checking that we have sufficient buffer to accomodate it.
    uint16_t proposedSize = mqttDoc["length"];
    if (proposedSize <= MQTT_FAST_BUFFER)
    {
        mqttPacketSize = mqttDoc["length"];
    }
    else
    {
        LOGW(CONF_KEY, "MQTT size of %u is greater than buffer of " stringify(MQTT_FAST_BUFFER) ". Ignoring this field.", proposedSize);
    }
    // Get the broker.
    m_safeReadString(mqttBroker, mqttDoc["broker"], CONF_MQTT_BROKER_MAX_LENGTH);

    // Get the WiFi SSID and password
    JsonVariant wifiDoc = json["wifi"];
    // Guard against someone sending redacted credentials back to this device.
    if (wifiDoc["redacted"] == false)
    {
        m_safeReadString(wifiSSID, wifiDoc["ssid"], CONF_WIFI_SSID_MAX_LENGTH);
        m_safeReadString(wifiPSK, wifiDoc["psk"], CONF_WIFI_PSK_MAX_LENGTH);
    }
    else
    {
        LOGW(CONF_KEY, "WiFi settings were redacted, will not update.");
    }
    return true;
}

void Config::writeJSON(char *text, uint32_t length, bool showWiFi)
{
    JsonDocument doc;
    writeJSON(doc, showWiFi);
    serializeJson(doc, text, length);
}

void Config::writeJSON(JsonVariant doc, bool showWiFi)
{
    // Useful: https://arduinojson.org/v7/assistant
    doc["connection"] = connectionMethod;

    // Kalman filter
    JsonObject kalman = doc["kalman"].to<JsonObject>();
    JsonArray kalmanQ = kalman["Q"].to<JsonArray>();
    m_writeMatrix(kalmanQ, qEnvCovariance);

    JsonArray kalmanR = kalman["R"].to<JsonArray>();
    m_writeMatrix(kalmanR, rMeasCovariance);

    // How often to record IMU data.
    doc["imuHowOften"] = 1;

    // Read configs for each side.
    JsonObject leftStrain = doc["left-strain"].to<JsonObject>();
    strain[SIDE_LEFT].writeJSON(leftStrain);

    JsonObject rightStrain = doc["right-strain"].to<JsonObject>();
    strain[SIDE_RIGHT].writeJSON(rightStrain);

    // Read MQTT conf
    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    mqtt["length"] = mqttPacketSize;
    mqtt["broker"] = mqttBroker;

    // WiFi conf (if allowed to divulge such secrets).
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    if (showWiFi)
    {
        // Send the details.
        wifi["ssid"] = wifiSSID;
        wifi["psk"] = wifiPSK;
        wifi["redacted"] = false;
    }
    else
    {
        // Don't send WiFi details.
        wifi["ssid"] = "";
        wifi["psk"] = "";
        wifi["redacted"] = true;
    }
}

void Config::toggleConnection()
{
    if (connectionMethod == CONNECTION_MQTT)
    {
        connectionMethod = CONNECTION_BLE;
        LOGI("Config", "Setting connection method to BLE.");
    }
    else
    {
        connectionMethod = CONNECTION_MQTT;
        LOGI("Config", "Setting connection method to MQTT");
    }
}

void Config::removeKey()
{
    LOGI(CONF_KEY, "Removing key from storage.");
    prefs.remove(CONF_KEY);
}

inline void Config::m_safeReadString(char *dest, const char *source, size_t maxLength)
{
    strncpy(dest, source, maxLength);
    dest[maxLength - 1] = '\0';
}

void Config::m_writeMatrix(JsonArray jsonArray, Matrix<2, 2, float> matrix)
{
    // Top row
    JsonArray topRow = jsonArray.add<JsonArray>();
    topRow.add(matrix(0, 0));
    topRow.add(matrix(0, 1));

    JsonArray bottomRow = jsonArray.add<JsonArray>();
    bottomRow.add(matrix(1, 0));
    bottomRow.add(matrix(1, 1));
}

Matrix<2, 2, float> Config::m_readMatrix(JsonArray jsonArray)
{
    Matrix<2, 2, float> matrix;
    JsonArray topRow = jsonArray[0];
    matrix(0, 0) = topRow[0];
    matrix(0, 1) = topRow[1];

    JsonArray bottomRow = jsonArray[1];
    matrix(1, 0) = bottomRow[0];
    matrix(1, 1) = bottomRow[1];

    return matrix;
}
