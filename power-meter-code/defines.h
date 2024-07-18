/**
 * @file defines.h
 * @brief Useful pin and constant definitions and preprocessor directives.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-19
 */
#pragma once
#define VERSION "0.0.0"

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
#define PIN_I2C_SCL 10
#define PIN_I2C_SDA 11
#define I2C_BUS_FREQ 400000
#define TEMP1_I2C 0b1001000
#define TEMP2_I2C 0b1001001

// Accelerometer
#define PIN_ACCEL_INTERRUPT 38
#define PIN_SPI_SDO 39
#define PIN_SPI_SDI 40
#define PIN_SPI_SCLK 41
#define PIN_SPI_AC_CS 42
#define IMU_SAMPLE_RATE 800 // Options are 12, 25, 50, 100, 200, 400, 800, 1600 Hz (any other value defaults to 100 Hz).
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
// #define SERIAL_TAKE() xSemaphoreTake(serialMutex, portMAX_DELAY)
// #define SERIAL_GIVE() xSemaphoreGive(serialMutex)
#define SERIAL_TAKE()
#define SERIAL_GIVE()
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

enum EnumSide
{
    SIDE_LEFT = 0,
    SIDE_RIGHT = 1
};

#define PROTECT_KALMAN_STATES

#include "constants.h"