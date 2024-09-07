/**
 * @file power_meter.cpp
 * @brief Classes to handle the hardware side of things of the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */

#include "power_meter.h"

extern SemaphoreHandle_t serialMutex;
extern PowerMeter powerMeter;

#include "connections.h"
extern Connection *connectionBasePtr;

extern TaskHandle_t imuTaskHandle, lowSpeedTaskHandle, connectionTaskHandle;
extern portMUX_TYPE spinlock;

void Side::begin()
{
    LOGD("Side", "Starting hardware for a side");
    tempSensor.begin();
}

void Side::createDataTask(uint8_t id)
{
    char buff[6];
    sprintf(buff, "Amp%d", id);
    xTaskCreatePinnedToCore(
        taskAmp,
        buff,
        4096,
        this,
        2,
        &taskHandle,
        1);
}

void Side::readDataTask()
{
    LOGI("AMP", "Starting to read data");
    while (true)
    {
        // Wait for the interrupt to occur and we get a notification. The time of the interrupt is passed using the
        // notification.
        uint32_t timestamp;
        bool success = xTaskNotifyWait(0, 0xffffffff, &timestamp, pdMS_TO_TICKS(100));

        uint32_t raw;
        if (success)
        {
            // Interrupt occured, data is ready to read. We have many ms for the data to sit in the ADC's buffer, so not
            // too much of a rush now we have the time that the reading was made available by the ADC.
            // Using predict so we can have a lower sample rate on the IMU hopefully.
            Matrix<2, 1, float> state;
            powerMeter.imuManager.kalman.predict(timestamp, state);
            // Valid data was received.
            raw = m_readADC();

            // Enable interrupts again
            attachInterrupt(digitalPinToInterrupt(m_pinDout), m_irq, FALLING);

            // Handle the data. Either perform offset calibration or use the data.
            if (m_offsetSteps == 0)
            {
                // No offset compensation. Proceed as normal.
                processData(timestamp, state, raw);
            }
            else
            {
                // Offset compensation mode.
                taskENTER_CRITICAL(&spinlock);
                config.strain[m_side].offset += raw / OFFSET_COMPENSATION_SAMPLES;
                m_offsetSteps--;
                taskEXIT_CRITICAL(&spinlock);

                m_updateAveragePower(timestamp);
            }
        }
        else
        {
            // No data. Don't calculate the energy, check and send a rotation notification if needed to allow low speed
            // data to be sent in the event that one side dies.

            // Enable interrupts again just in case something recovers.
            attachInterrupt(digitalPinToInterrupt(m_pinDout), m_irq, FALLING);

            m_updateAveragePower(micros()); // We don't have the time the interrupt occured to use, so just do now as
            // close enough.
        }
    }
}

inline void Side::processData(uint32_t timestamp, Matrix<2, 1, float> state, uint32_t raw)
{
    // Create the object and copy main parameters
    HighSpeedData data;
    data.timestamp = timestamp;
    data.position = state(0, 0);
    data.velocity = state(1, 0);
    data.raw = raw;

    // Finish calculating and send raw to where it needs to go.
    data.torque = m_calculateTorque(raw, tempSensor.getLastTemp());
    connectionBasePtr->addHighSpeed(data, m_side);

    // The rotation most likely occurred before this reading, so calculate average power for the previous rotation and
    // use this reading as the first in the next rotation.
    m_updateAveragePower(data.timestamp);

    // Accumulate energy
    m_energy += data.velocity * data.torque * (data.timestamp - m_lastTime) * 1e-6;
    m_lastTime = data.timestamp;
}

inline void Side::enableADCOffsetCalibration()
{
    m_adcOffsetCalibration = true;
}

inline void Side::enableStrainOffsetCalibration()
{
    enableADCOffsetCalibration();
    taskENTER_CRITICAL(&spinlock);
    m_offsetSteps = OFFSET_COMPENSATION_SAMPLES;
    config.strain[m_side].offset = 0; // Reset the offset.
    taskEXIT_CRITICAL(&spinlock);
}

inline void Side::startAmp()
{
    enableADCOffsetCalibration();
    attachInterrupt(digitalPinToInterrupt(m_pinDout), m_irq, FALLING);
}

inline uint32_t Side::m_readADC()
{
    // Start reading
    uint32_t raw = 0;
    uint8_t clockBits = 24;
    if (m_adcOffsetCalibration)
    {
        // Add 2 additional clock pulses to enter offset calibrarion mode.
        clockBits = 26;
    }

    // Main reading loop
    for (uint8_t i = 0; i < clockBits; i++)
    {
        // Read 1 bit
        digitalWrite(m_pinSclk, HIGH);
        delayMicroseconds(1);
        raw = (raw << 1) | digitalRead(m_pinDout);
        digitalWrite(m_pinSclk, LOW);
        delayMicroseconds(1);
    }

    // Send the contents of raw somewhere as needed.
    if (m_adcOffsetCalibration)
    {
        // Remove the extra 2 bits
        raw >>= 2;
        m_adcOffsetCalibration = false; // Disable automatically.
    }
    return raw;
}

float Side::m_calculateTorque(uint32_t raw, float temperature)
{
    StrainConf &conf = config.strain[m_side];
    // There is a relatively linear relationship between the raw value and the torque.
    int32_t difference = raw - conf.offset;
    float torque = difference * conf.coefficient;

    // Thermal compensation. // TODO: In the strain gauge datasheet, this looks like it may be quadratic?
    torque *= (1 - conf.tempCoefficient * (temperature - conf.tempTest));
    return torque;
}

void Side::m_updateAveragePower(uint32_t timestamp)
{
    // Check if a full rotation occurred.
    if (powerMeter.imuManager.rotations != m_lastRotation)
    {
        // Rotation has occurred, calculate average power and reset accumulator.
        m_lastRotation = powerMeter.imuManager.rotations;

        // Calculate the average power over the rotation, aligned to the sample rate. This variable will remain
        // set until the next rotation.
        averagePower = m_energy / (timestamp - m_segStartTime);
        m_segStartTime = timestamp;

        // Reset accumulator.
        m_energy = 0;

        // Send the notification.
        xTaskNotify(lowSpeedTaskHandle, (2 << m_side), eSetBits);
    }
}

void taskAmp(void *pvParameters)
{
    Side *side = (Side *)pvParameters;
    side->readDataTask();
}

template <EnumSide sideEnum, uint8_t pinDout>
void irqAmp()
{
    // Notify the task for ADC 1. If the ADC 1 task has a higher priority than the one currently running, force a
    // context switch.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Disable this interrupt being called again until the data is retrieved.
    detachInterrupt(digitalPinToInterrupt(pinDout));

    // Give the notification and perform a context switch if necessary.
    uint32_t time = micros();
    xTaskNotifyFromISR(powerMeter.sides[sideEnum].taskHandle, time, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Instantiate the functions / irqs
template void irqAmp<SIDE_LEFT, PIN_AMP2_DOUT>();
template void irqAmp<SIDE_RIGHT, PIN_AMP1_DOUT>();

void PowerMeter::begin()
{
    LOGD("Power", "Starting hardware");
    // Initialise I2C for the temperature sensors
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_FREQ);

    // Initialise the strain gauges
    sides[SIDE_LEFT].begin();
    sides[SIDE_RIGHT].begin();

    // Initialise the IMU
    imuManager.begin();
}

void PowerMeter::powerDown()
{
    LOGI("Power", "Power down");
    digitalWrite(PIN_AMP_PWDN, LOW);
    digitalWrite(PIN_POWER_SAVE, LOW);
}

void PowerMeter::powerUp()
{
    LOGI("Power", "Power up");
    // Set pin modes. // TODO: OOP
    pinMode(PIN_POWER_SAVE, OUTPUT);
    pinMode(PIN_AMP1_SCLK, OUTPUT);
    pinMode(PIN_AMP2_SCLK, OUTPUT);
    pinMode(PIN_AMP_PWDN, OUTPUT);
    pinMode(PIN_AMP1_DOUT, INPUT);
    pinMode(PIN_AMP2_DOUT, INPUT);
    pinMode(PIN_ACCEL_INTERRUPT, INPUT);

    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);

    // Reset the strain gauge ADCs as per the manual (only needed first time, but should be ok later?).
    digitalWrite(PIN_POWER_SAVE, HIGH); // Turn on the strain gauges.
    delay(5);                           // Way longer than required, but should let the reference and strain gauge voltages settle.

    // Reset sequence specified in the ADS1232 datasheet.
    digitalWrite(PIN_AMP_PWDN, HIGH);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, LOW);
    delayMicroseconds(26);
    digitalWrite(PIN_AMP_PWDN, HIGH);

    // Enable interrupts for the amplifiers
    sides[SIDE_LEFT].startAmp();
    sides[SIDE_RIGHT].startAmp();

    // Start the IMU
    imuManager.startEstimating();
}

void PowerMeter::offsetCompensate()
{
    LOGI("Power", "Enabling offset compensation.");
    sides[SIDE_LEFT].enableStrainOffsetCalibration();
    sides[SIDE_RIGHT].enableStrainOffsetCalibration();
}

uint32_t PowerMeter::batteryVoltage()
{
    // return analogRead(PIN_BATTERY_VOLTAGE);
    return (analogRead(PIN_BATTERY_VOLTAGE) * SUPPLY_VOLTAGE) >> 12;
}

bool waitLowSpeedNofity(uint32_t timeout)
{
    uint32_t notifyBits = 0;
    bool success;
    do
    {
        // Don't clear anything now as we will loop until the bit for each side is set (2 notifications).
        success = xTaskNotifyWait(0x00, 0x00, &notifyBits, timeout);
    } while (notifyBits != ((2 << SIDE_LEFT) | (2 << SIDE_RIGHT)) && success);

    // Clear the bits ready for the next notification.
    if (success)
    {
        // No timeouts
        ulTaskNotifyValueClear(lowSpeedTaskHandle, 0xffffffff);
    }
    return success;
}

void taskLowSpeed(void *pvParameters)
{
    LOGI("LS", "Low speed task started");
    while (true)
    {
        LowSpeedData lowSpeed;
        // Wait for a rotation or once every 5 seconds, whatever comes first.
        if (waitLowSpeedNofity(pdMS_TO_TICKS(3000)))
        {
            // We had a rotation.
            powerMeter.imuManager.setLowSpeedData(lowSpeed);

            // Get powers and pedal balance.
            float left = powerMeter.sides[SIDE_LEFT].averagePower;
            float right = powerMeter.sides[SIDE_RIGHT].averagePower;
            lowSpeed.power = left + right;
            lowSpeed.balance = 100 * right / lowSpeed.power;
        }
        else
        {
            // No rotation. Get the last known values, set power to 0.
            powerMeter.imuManager.setLowSpeedData(lowSpeed);
            lowSpeed.balance = 50;
            lowSpeed.power = 0;
        }

        // Send data
        connectionBasePtr->addLowSpeed(lowSpeed);
    }
}

void debugMemory()
{
    SERIAL_TAKE();
    log_printf("Free memory: %lu\n", esp_get_free_heap_size());
    log_printf("  - LS:   %lu\n", uxTaskGetStackHighWaterMark(lowSpeedTaskHandle));
    log_printf("  - IMU:  %lu\n", uxTaskGetStackHighWaterMark(imuTaskHandle));
    log_printf("  - Conn: %lu\n", uxTaskGetStackHighWaterMark(connectionTaskHandle));
    log_printf("  - This: %lu\n", uxTaskGetStackHighWaterMark(NULL));
    SERIAL_GIVE();
}
