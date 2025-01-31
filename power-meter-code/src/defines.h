/**
 * @file defines.h
 * @brief Useful pin and constant definitions and preprocessor directives.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-28
 */
#pragma once
#define SW_VERSION "s0.1.0"
#include "constants.h"

/**
 * Hardware versions
 */
// Check the versions are valid.
#if !defined(HW_VERSION)
#error "No hardware version specified (set this in constants.h)."
#endif
#if (HW_VERSION != HW_VERSION_V1_0_4) && (HW_VERSION != HW_VERSION_V1_0_5) && (HW_VERSION != HW_VERSION_V1_1_1)
#error "Unsupported hardware version specified (set this in constants.h)"
#endif

// Define a nicely formatted version string according to the version.
#if HW_VERSION == HW_VERSION_V1_0_4
#define HW_VERSION_STR "v1.0.4"
#elif HW_VERSION == HW_VERSION_V1_0_5
#define HW_VERSION_STR "v1.0.5"
#elif HW_VERSION == HW_VERSION_V1_1_1
#define HW_VERSION_STR "v1.1.1"
#endif

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
#if (HW_VERSION == HW_VERSION_V1_0_4) || (HW_VERSION == HW_VERSION_V1_0_5)
#define PIN_LEDR 8
#define PIN_LEDG 9
// #define PIN_LEDB 12 // No Blue LED
#define PIN_CONNECTION_LED PIN_LEDG
#elif HW_VERSION == HW_VERSION_V1_1_1
#define PIN_LEDR 12
#define PIN_LEDG 9
#define PIN_LEDB 8
#define PIN_CONNECTION_LED PIN_LEDG
#define HAS_BLUE_LED
#endif

#define PIN_BOOT 0

// I2C for temperature sensor
#define PIN_I2C_SDA 10
#define PIN_I2C_SCL 11
#define I2C_BUS_FREQ 400000
#define TEMP1_I2C 0b1001001
#define TEMP2_I2C 0b1001000

// Accelerometer
#if (HW_VERSION == HW_VERSION_V1_0_4) || (HW_VERSION == HW_VERSION_V1_0_5)
#define PIN_ACCEL_INTERRUPT 38
#elif (HW_VERSION == HW_VERSION_V1_1_1)
#define ACCEL_RTC_CAPABLE
#define PIN_ACCEL_INTERRUPT 21
#endif
#define PIN_SPI_SDI 39
#define PIN_SPI_SDO 40
#define PIN_SPI_SCLK 41
#define PIN_SPI_AC_CS 42
#define IMU_SAMPLE_RATE 100 // Options are 12, 25, 50, 100, 200, 400, 800, 1600 Hz (any other value defaults to 100 Hz).
#define IMU_ACCEL_RANGE 4   // Options are 2, 4, 8, 16 G (any other value defaults to 16 G).
#define IMU_GYRO_RANGE 2000 // Options are 250, 500, 1000, 2000 dps (any other value defaults to 2000 dps).

// Power management
#if HW_VERSION == HW_VERSION_V1_0_4
#define PIN_BATTERY_VOLTAGE 12 // As per design.
#elif HW_VERSION == HW_VERSION_V1_0_5
#define PIN_BATTERY_VOLTAGE 15 // Bodge wire as original is shorted to ground.
#elif HW_VERSION == HW_VERSION_V1_1_1
#define PIN_BATTERY_VOLTAGE 13 // Bodge wire as original is shorted to ground.
#endif

// Extra GPIO pins
#if (HW_VERSION == HW_VERSION_V1_0_4) || (HW_VERSION == HW_VERSION_V1_0_5)
// Spare GPIO pins are // TODO:
#define SPARE_GPIO_6 15
#elif (HW_VERSION == HW_VERSION_V1_1_1)
#define SPARE_GPIO_1 48
#define SPARE_GPIO_2 33
#define SPARE_GPIO_3 18
#define SPARE_GPIO_4 17
#define SPARE_GPIO_5 16
#define SPARE_GPIO_6 15
#endif

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
#define SERIAL_GIVE() xSemaphoreGive(serialMutex)
#define LOGV(tag, format, ...)                 \
    SERIAL_TAKE();                             \
    log_v("[%s] " format, tag, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGD(tag, format, ...)                 \
    SERIAL_TAKE();                             \
    log_d("[%s] " format, tag, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGI(tag, format, ...)                 \
    SERIAL_TAKE();                             \
    log_i("[%s] " format, tag, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGW(tag, format, ...)                 \
    SERIAL_TAKE();                             \
    log_w("[%s] " format, tag, ##__VA_ARGS__); \
    SERIAL_GIVE()
#define LOGE(tag, format, ...)                 \
    SERIAL_TAKE();                             \
    log_e("[%s] " format, tag, ##__VA_ARGS__); \
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
 * @brief Enumerator for connection state
 *
 */
enum EnumConnState
{
    CONN_STATE_DISABLED,
    CONN_STATE_CONNECTING_1,
    CONN_STATE_CONNECTING_2,
    CONN_STATE_ACTIVE,
    CONN_STATE_SHUTTING_DOWN,
    CONN_STATE_SENDING,
    CONN_STATE_RECEIVING
};

/**
 * WiFi settings.
 */
#define MQTT_TOPIC_SEPARATOR '/'
#define RECONNECT_DELAY 1000
#define MQTT_RETRY_ITERATIONS 20
#define WIFI_RECONNECT_ATTEMPT_TIME 60000 // If not connected in 1 minute, disconnect and attempt again.

// Constant constants.
#define GRAVITY 9.81                   // Accelerometer operates in g, calculations are done in SI units.
#define KALMAN_X0 {0, 0}               // Initial state.
#define KALMAN_P0 {1e6, 1e6, 1e6, 1e6} // Initial covariance. High numbers mean we don't know to start with.

#define SUPPLY_VOLTAGE 3300             // Power supply in mV, used to calculate the battery voltage.
#define OFFSET_COMPENSATION_SAMPLES 200 // How many samples to average to calculate the offset.
#define INVALID_TEMPERATURE -1000       // Invalid temperature so that I don't have to work out formatting NaN in a JSON compatible way.

// Settings for ArduinoJSON
#define ARDUINOJSON_NEGATIVE_EXPONENTIATION_THRESHOLD 1e-2
#define ARDUINOJSON_POSITIVE_EXPONENTIATION_THRESHOLD 1e4
#include "Arduino.h"