/**
 * @file defines.h
 * @brief Enough definitions to talk to the temperature sensors.
 * 
 * @author Jotham Gates (koyugaDev@gmail.com)
 * @version 0.0.1
 * @date 2024-08-21
 */

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

// Logging (with mutexes)
// #define SERIAL_TAKE() xSemaphoreTake(serialMutex, portMAX_DELAY)
// #define SERIAL_GIVE() delay(200); xSemaphoreGive(serialMutex)
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