/**
 * @file constants.example.h
 * @brief Constants and settings specific to individual power meters.
 * 
 * Copy this file and rename it to `constants.h`. Tweak the values to calibrate the power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#pragma once

#define DEVICE_NAME "Power meter prototype" // Name of the power meter for easier identification later.

/**
 * @brief Constants for the Kalman filter used to estimate the current rotation angle and angular velocity.
 * 
 */
#define DEFAULT_KALMAN_Q {0.002, 0, 0, 0.1} // Environmental covariance matrix.
#define DEFAULT_KALMAN_R {100, 0, 0, 0.01} // Measurement covariance matrix.
#define KALMAN_X0 {0, 0} // Initial state.
#define KALMAN_P0 {1e6, 1e6, 1e6, 1e6} // Initial covariance. High numbers mean we don't know to start with.

/**
 * @brief Relative physical offset of the IMU from the axle centre (in m).
 * 
 */
#define IMU_OFFSET_X 0 // TODO: Measure
#define IMU_OFFSET_Y 0 // TODO: Measure

/**
 * @brief WiFi settings
 * 
 */
#define WIFI_SSID "MyWiFiNetwork"
#define WIFI_PASSWORD "MyWiFiPassword"

/**
 * @brief MQTT settings
 * 
 */
#define MQTT_BROKER "computername.local"
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