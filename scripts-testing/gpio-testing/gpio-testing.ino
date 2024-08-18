/**
 * @file gpio-testing.ino
 * @brief Tests GPIO pins for the ADCs before they are soldered.
 * 
 * Usage:
 * - Red light indicates waiting for a pin number to be typed in the serial console.
 * - Flashing green indicates that a pin is being tested (on is high, off is low).
 * - Press 'q' to got back and select another pin.
 * - All pins other than LEDs are left in high impedance states after testing that pin concludes.
 * 
 * @author Jotham Gates (koyugaDev@gmail.com)
 * @version 0.0.1
 * @date 2024-08-19
 */

// Strain gauge amplifiers
#define PIN_AMP1_DOUT 1
#define PIN_AMP1_SCLK 4
#define PIN_AMP2_DOUT 2
#define PIN_AMP2_SCLK 5
#define PIN_AMP_PWDN 6
#define PIN_POWER_SAVE 7

// LEDs and buttons
#define PIN_LED1 8
#define PIN_LED2 9
#define PIN_BOOT 0

#define TAG "GPIO Test"

void setup()
{
    ESP_LOGI(TAG, "GPIO testing started.");
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    digitalWrite(PIN_LED1, HIGH);
    Serial.setTimeout(65535);
}

void loop()
{
    ESP_LOGI(TAG, "Type a number to test the pin:");
    digitalWrite(PIN_LED1, HIGH);
    int pin = Serial.parseInt();
    digitalWrite(PIN_LED1, LOW);
    Serial.println(pin);
    if (pin != 0)
    {
        testPin(pin);
    }
    else
    {
        ESP_LOGE(TAG, "Pin 0 is reserved for the boot button. Will not test");
    }
}

void testPin(uint8_t pin)
{
    ESP_LOGI(TAG, "Testing pin %d. Press any 'q' to exit.", pin);
    setOut(pin);
    while(Serial.read() != 'q')
    {
        setHigh(pin);
        delay(1000);
        setLow(pin);
        delay(1000);
    }
    setIn(pin);
    ESP_LOGI(TAG, "Finished testing pin %d.", pin);
}

void setOut(uint8_t pin)
{
    ESP_LOGD(TAG, "Setting %d as an output", pin);
    pinMode(pin, OUTPUT);
}

void setIn(uint8_t pin)
{
    ESP_LOGD(TAG, "Setting %d as an input", pin);
    pinMode(pin, INPUT);
}

void setHigh(uint8_t pin)
{
    ESP_LOGD(TAG, "Setting %d high", pin);
    digitalWrite(pin, HIGH);
    digitalWrite(PIN_LED2, HIGH);
}

void setLow(uint8_t pin)
{
    ESP_LOGD(TAG, "Setting %d low", pin);
    digitalWrite(pin, LOW);
    digitalWrite(PIN_LED2, LOW);
}