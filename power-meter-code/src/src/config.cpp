/**
 * @file config.cpp
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#include "config.h"
extern SemaphoreHandle_t serialMutex;
extern Preferences prefs;

void StrainConf::writeJSON(char *text, uint32_t length)
{
    if (length < CONF_SG_JSON_TEXT_LENGTH)
    {
        LOGE("Config", "Buffer to print to JSON needs to be at least %d long.", CONF_SG_JSON_TEXT_LENGTH);
        return;
    }
    sprintf(text, CONF_SG_JSON_TEXT, offset, coefficient, tempTest, tempCoefficient);
}

void StrainConf::readJSON(JsonDocument doc)
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
    writeJSON(text, CONF_JSON_TEXT_LENGTH);
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
    StaticJsonDocument<150> json;
    DeserializationError error = deserializeJson(json, text, length);
    if (error)
    {
        LOGW("Config", "Could not deserialise the json document. Discarding");
        return false;
    }

    // Handle each known method
    LOGW("Config", "Deliberately not overwriting the connection type until a way to change back is implemented.");
    // Q matrix
    qEnvCovariance(0, 0) = json["q(0,0)"];
    qEnvCovariance(0, 1) = json["q(0,1)"];
    qEnvCovariance(1, 0) = json["q(1,0)"];
    qEnvCovariance(1, 1) = json["q(1,1)"];

    // R matrix
    rMeasCovariance(0, 0) = json["r(0,0)"];
    rMeasCovariance(0, 1) = json["r(0,1)"];
    rMeasCovariance(1, 0) = json["r(1,0)"];
    rMeasCovariance(1, 1) = json["r(1,1)"];

    // How often to send IMU data
    imuHowOften = json["imuHowOften"];

    // Read the strain gauge input data.
    strain[SIDE_LEFT].readJSON(json["left-strain"]);
    strain[SIDE_RIGHT].readJSON(json["right-strain"]);
    return true;
}

void Config::writeJSON(char *text, uint32_t length)
{
    // Check that text is long enough to work with.
    if (length < CONF_JSON_TEXT_LENGTH)
    {
        LOGE("Config", "Buffer to print to JSON needs to be at least %d long.", CONF_JSON_TEXT_LENGTH);
        return;
    }

    // Get the JSON text for each side.
    char leftText[CONF_SG_JSON_TEXT_LENGTH];
    char rightText[CONF_SG_JSON_TEXT_LENGTH];
    strain[SIDE_LEFT].writeJSON(leftText, CONF_SG_JSON_TEXT_LENGTH);
    strain[SIDE_RIGHT].writeJSON(rightText, CONF_SG_JSON_TEXT_LENGTH);

    // Write to the original text.
    sprintf(
        text, CONF_JSON_TEXT,
        connectionMethod,
        qEnvCovariance(0, 0), qEnvCovariance(0, 1), qEnvCovariance(1, 0), qEnvCovariance(1, 1),
        rMeasCovariance(0, 0), rMeasCovariance(0, 1), rMeasCovariance(1, 0), rMeasCovariance(1, 1),
        imuHowOften,
        leftText, rightText); // TODO
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

// void Config::commandLine()
// {
//     SERIAL_TAKE();
//     Serial.setTimeout(30000); // 30s timeout.
//     Serial.println("Set configuration values. Type `set FIELD ");
//     SERIAL_GIVE();

//     // Read the response.
//     char buffer[MAX_INPUT_LENGTH];
//     size_t length = Serial.readBytesUntil('\n', buffer, MAX_INPUT_LENGTH);
//     char name[MAX_INPUT_LENGTH];
//     float value;
//     int row, column;
//     int len = sscanf("set %s(%d,%d) %f", name, &row, &column, &value);

//     if (len > 0)
//     {
//         if (strcmp(name, "R"))
//         {
//             rMeasCovariance(row, column) = value;
//         }
//         else if (strcmp(name, "Q"))
//         {
//             qEnvCovariance(row, column) = value;
//         }
//         else if (strcmp(name, "conn"))
//         {
//             // TODO: Round and set connection method.
//         }
//     }
// }