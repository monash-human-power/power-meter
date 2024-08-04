/**
 * @file ota.h
 * @brief Handles over the air (OTA) updates.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-04
 */
#pragma once

#include "../defines.h"

#ifdef OTA_ENABLE
#include <ArduinoOTA.h>
namespace OTAManager
{

    /**
     * @brief Sets up OTA. Mainly based on the Arduino BasicOTA example.
     *
     */
    void setupOTA();

    /**
     * @brief Same as calling ArduinoOTA.handle()
     * 
     */
    inline void handleOTA() { ArduinoOTA.handle(); }

    /**
     * @brief Callback for when OTA is started. Logs this event.
     *
     */
    void otaStartCallback();

    /**
     * @brief Callback for when OTA is finished. Logs this event.
     *
     */
    void otaEndCallback();

    /**
     * @brief Callback for OTA progress. Logs this event.
     *
     * @param progress the current progress value.
     * @param total the total value to reach.
     */
    void otaProgressCallback(unsigned int progress, unsigned int total);

    /**
     * @brief Callback for an error for OTA. Logs this event.
     *
     * @param error the error value.
     */
    void otaErrorCallback(ota_error_t error);
};
#endif