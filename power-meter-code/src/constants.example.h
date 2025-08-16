/**
 * @file constants.example.h
 * @brief Constants and settings specific to individual power meters.
 * 
 * Copy this file and rename it to `constants.h`. Tweak the values to calibrate the power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2025-08-16
 */
#pragma once

#define DEVICE_NAME "Power meter prototype" // Name of the power meter for easier identification later.

/**
 * Hardware version is defined in `platform.ini`.
 */

/**
 * @brief Constants for the Kalman filter used to estimate the current rotation angle and angular velocity.
 * 
 */
#define DEFAULT_KALMAN_Q {0.002, 0, 0, 0.1} // Environmental covariance matrix.
#define DEFAULT_KALMAN_R {100, 0, 0, 0.01} // Measurement covariance matrix.

/**
 * @brief Constants for the strain gauge calibration.
 * 
 */
#define DEFAULT_STRAIN_OFFSET 0
#define DEFAULT_STRAIN_COEFFICIENT ((1/2873.3978550876277)*9.81*0.13) // Converted to Nm.
#define DEFAULT_STRAIN_TEMP_CO 0
#define DEFAULT_STRAIN_TEST_TEMP 24.25

/**
 * @brief How long between rotations to wait before entering sleep mode.
 * 
 */
#define DEFAULT_SLEEP_TIME 600 // 10 minutes of no rotations to sleep.

#define MINIMUM_BATTERY 3060 // Battery voltage to shut down at.
#define MINIMUM_BATTERY_SUCCESSIVE 3 // Needs to be this many successive readings before shutting down.

/**
 * @brief Relative physical offset of the IMU from the axle centre (in m).
 * 
 */
#define IMU_OFFSET_X -59.612e-3 // Length along the crank (should be negative with the current design).
#define IMU_OFFSET_Y -0.874e-3 // Length across crank (should also be negative with the current design).

/**
 * @brief WiFi settings
 * 
 */
#define DEFAULT_WIFI_SSID "MyWiFiNetwork"
#define DEFAULT_WIFI_PASSWORD "MyWiFiPassword"

/**
 * @brief MQTT settings
 * 
 */
#define DEFAULT_MQTT_BROKER "computername.local"
#define MQTT_PORT 1883
#define MQTT_ID "power-meter"

/**
 * @brief OTA Settings
 * 
 */
#define OTA_ENABLE
#ifdef OTA_ENABLE
#define OTA_PORT 3232
#define OTA_HOSTNAME "power"
#define OTA_PASSWORD "MyOTAPassword"
#endif