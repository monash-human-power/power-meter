/**
 * @file config.cpp
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-11
 */
#include "config.h"
extern SemaphoreHandle_t serialMutex;
extern Preferences prefs;
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
            prefs.remove(CONF_KEY);
        }

        // Defaults should be set on initialise
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
    SERIAL_TAKE();
    log_printf("Current config:\n");

    // Print the connection type in easy to read text.
    log_printf("  - Connection:");
    switch (connectionMethod)
    {
    case CONNECTION_MQTT:
        log_printf("MQTT\n");
        break;
    case CONNECTION_BLE:
        log_printf("BLE\n");
        break;
    default:
        log_printf("Unknown (%d)\n", connectionMethod);
    }

    log_printf("  - Q matrix:\n    [[%8.4g, %8.4g]\n     [%8.4g, %8.4g]]\n", qEnvCovariance(0, 0), qEnvCovariance(0, 1), qEnvCovariance(1, 0), qEnvCovariance(1, 1));
    log_printf("  - R matrix:\n    [[%8.4g, %8.4g]\n     [%8.4g, %8.4g]]\n", rMeasCovariance(0, 0), rMeasCovariance(0, 1), rMeasCovariance(1, 0), rMeasCovariance(1, 1));
    SERIAL_GIVE();
}

bool Config::readJSON(char *text, uint32_t length)
{
    // Intellisense seems to be broken for ArduinoJSON at the moment. Ignore the red squiggles for now
    StaticJsonDocument<100> json;
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
    return true;
}

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
 \"r(1,1)\": %.10g\
}"

#define CONF_JSON_TEXT_LENGTH (sizeof(CONF_JSON_TEXT) + (- 2 + 3) + 8*(-5 + 12))

void Config::writeJSON(char *text, uint32_t length)
{
    if (length < CONF_JSON_TEXT_LENGTH)
    {
        LOGE("Config", "Buffer to print to JSON needs to be at least %d long.", CONF_JSON_TEXT_LENGTH);
        return;
    }
    sprintf(
        text, CONF_JSON_TEXT,
        connectionMethod,
        qEnvCovariance(0, 0), qEnvCovariance(0, 1), qEnvCovariance(1, 0), qEnvCovariance(1, 1),
        rMeasCovariance(0, 0), rMeasCovariance(0, 1), rMeasCovariance(1, 0), rMeasCovariance(1, 1));
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
