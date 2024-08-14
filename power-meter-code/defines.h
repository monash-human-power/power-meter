/**
 * @file defines.h
 * @brief Useful pin and constant definitions and preprocessor directives.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-14
 */
#pragma once
#define VERSION "0.0.1"

/**
 * Pins and hardware details
 */
// Strain gauge amplifiers
#define PIN_AMP1_DOUT 1
#define PIN_AMP1_SCLK 4
#define PIN_AMP2_DOUT 2
#define PIN_AMP2_SCLK 5
#define PIN_AMP_PWDN 6
#define PIN_POWER_SAVE 7
#define AMP_BIT_DEPTH 24

// Buttons and LEDs
#define PIN_LED1 8
#define PIN_LED2 9
#define PIN_BOOT 0

// I2C for temperature sensor
#define PIN_I2C_SDA 10
#define PIN_I2C_SCL 11
#define I2C_BUS_FREQ 400000
#define TEMP1_I2C 0b1001001
#define TEMP2_I2C 0b1001000

// Accelerometer
#define PIN_ACCEL_INTERRUPT 38
#define PIN_SPI_SDI 39
#define PIN_SPI_SDO 40
#define PIN_SPI_SCLK 41
#define PIN_SPI_AC_CS 42
#define IMU_SAMPLE_RATE 100 // Options are 12, 25, 50, 100, 200, 400, 800, 1600 Hz (any other value defaults to 100 Hz).
#define IMU_ACCEL_RANGE 4   // Options are 2, 4, 8, 16 G (any other value defaults to 16 G).
#define IMU_GYRO_RANGE 1000 // Options are 250, 500, 1000, 2000 dps (any other value defaults to 2000 dps).

// Power management
#define PIN_BATTERY_VOLTAGE 12

// Communications
#define PIN_UART_TX0
#define PIN_UART_RX0
#define SERIAL_BAUD 115200
#define PIN_USB_DN 19
#define PIN_USB_DP 20

/**
 * Logging
 * // TODO: Make work over USB.
 */
// Logging (with mutexes)
#define SERIAL_TAKE() xSemaphoreTake(serialMutex, portMAX_DELAY)
#define SERIAL_GIVE() delay(200); xSemaphoreGive(serialMutex)
// #define SERIAL_TAKE()
// #define SERIAL_GIVE()
#define LOGV(tag, format, ...)            \
    SERIAL_TAKE();                        \
    ESP_LOGV(tag, format, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGD(tag, format, ...)            \
    SERIAL_TAKE();                        \
    ESP_LOGD(tag, format, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGI(tag, format, ...)            \
    SERIAL_TAKE();                        \
    ESP_LOGI(tag, format, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGW(tag, format, ...)            \
    SERIAL_TAKE();                        \
    ESP_LOGW(tag, format, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGE(tag, format, ...)            \
    SERIAL_TAKE();                        \
    ESP_LOGE(tag, format, ##__VA_ARGS__); \
    SERIAL_GIVE()

// https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define xstringify(s) stringify(s)
#define stringify(s) #s

/**
 * @brief Enumerator to represent each side of the power meter.
 * 
 */
enum EnumSide
{
    SIDE_LEFT = 0,
    SIDE_RIGHT = 1,
    SIDE_IMU_TEMP = 2
};

/**
 * @brief Enumerator to represent the selected connection method.
 * 
 */
enum EnumConnection
{
    CONNECTION_MQTT,
    CONNECTION_BLE
};

/**
 * WiFi settings.
 */
#define MQTT_TOPIC_SEPARATOR '/'
#define RECONNECT_DELAY 1000
#define MQTT_RETRY_ITERATIONS 20
#define WIFI_RECONNECT_ATTEMPT_TIME 60000 // If not connected in 1 minute, disconnect and attempt again.

// Constant constants.
#define GRAVITY 9.81 // Accelerometer operates in g, calculations are done in SI units.

#include "constants.h"
#include "Arduino.h"