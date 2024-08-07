/**
 * @file config.cpp
 * @brief Classes that handles configuration.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-07
 */
#include "config.h"
extern SemaphoreHandle_t serialMutex;

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
}

void Config::print()
{
    SERIAL_TAKE();
    Serial1.println("Current config:");
    Serial1.printf("  - Connection: %d\n", connectionMethod);
    Serial1.println("  - Q matrix:");
    Serial1.println(qEnvCovariance);
    Serial1.println("  - R matrix:");
    Serial1.println(rMeasCovariance);
    SERIAL_GIVE();
}

void Config::commandLine()
{
    SERIAL_TAKE();
    Serial.setTimeout(30000); // 30s timeout.
    Serial.println("Set configuration values. Type `set FIELD ");
    SERIAL_GIVE();

    // Read the response.
    char buffer[MAX_INPUT_LENGTH];
    size_t length = Serial.readBytesUntil('\n', buffer, MAX_INPUT_LENGTH);
    char name[MAX_INPUT_LENGTH];
    float value;
    int row, column;
    int len = sscanf("set %s(%d,%d) %f", name, &row, &column, &value);

    if (len > 0)
    {
        if (strcmp(name, "R"))
        {
            rMeasCovariance(row, column) = value;
        }
        else if (strcmp(name, "Q"))
        {
            qEnvCovariance(row, column) = value;
        }
        else if (strcmp(name, "conn"))
        {
            // TODO: Round and set connection method.
        }
    }
}
