/**
 * @file constants.example.h
 * @brief Constants and settings specific to individual power meters.
 * 
 * Copy this file and rename it to `constants.h`. Tweak the values to calibrate the power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-28
 */
#pragma once

#define DEVICE_NAME "Power meter prototype" // Name of the power meter for easier identification later.

/**
 * Hardware version (v{MAJOR}.{MINOR}.{PATCH})
 */
#define HW_VERSION_V1_0_4 1000004
#define HW_VERSION_V1_0_5 1000005
#define HW_VERSION_V1_1_1 1001001
// #define HW_VERSION HW_VERSION_V1_0_5
#define HW_VERSION HW_VERSION_V1_1_1
/**
 * @brief Constants for the Kalman filter used to estimate the current rotation angle and angular velocity.
 * 
 */
#define DEFAULT_KALMAN_Q {0.002, 0, 0, 0.1} // Environmental covariance matrix.
#define DEFAULT_KALMAN_R {100, 0, 0, 0.01} // Measurement covariance matrix.
#define DEFAULT_STRAIN_OFFSET 0
#define DEFAULT_STRAIN_COEFFICIENT ((1/2873.3978550876277)*9.81*0.13) // Converted to Nm.
#define DEFAULT_STRAIN_TEMP_CO 0
#define DEFAULT_STRAIN_TEST_TEMP 24.25

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