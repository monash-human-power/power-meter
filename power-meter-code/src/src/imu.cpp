/**
 * @file imp.cpp
 * @brief Class and function to process the IMU data on the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-28
 */

#include "imu.h"
// Power meter requires that the IMUManager class is defined, so put the include here.
#include "power_meter.h"
extern SemaphoreHandle_t serialMutex;
extern TaskHandle_t imuTaskHandle;
extern PowerMeter powerMeter;
extern portMUX_TYPE spinlock;

#include "connections.h"
extern Connection *connectionBasePtr;

#include <esp_sleep.h>
#include <driver/rtc_io.h>

volatile uint32_t imuTime;

void IMUManager::begin()
{
    LOGD("IMU", "Starting IMU");
    // Initialise the IMU
#ifdef ACCEL_RTC_CAPABLE
    rtc_gpio_deinit((gpio_num_t)PIN_ACCEL_INTERRUPT);
#endif
    // Manually initialise the SPI bus beforehand so that the pins can be specified. The call to `SPI.begin()` in the
    // IMU library should have no effect / returned early.
    SPI.begin(PIN_SPI_SCLK, PIN_SPI_SDI, PIN_SPI_SDO, PIN_SPI_AC_CS);
    int result = imu.begin();
    if (result)
    {
        LOGE("IMU", "Cannot connect to IMU, error %d.", result);
    }
}

void IMUManager::startEstimating()
{
    // Setup the IMU
    // Set FIFO watermark to 1 so that we get a constant update rate instead of in batches.
    imu.enableFifoInterrupt(PIN_ACCEL_INTERRUPT, irqIMUActive, 1);
    imu.startAccel(IMU_SAMPLE_RATE, IMU_ACCEL_RANGE);
    imu.startGyro(IMU_SAMPLE_RATE, IMU_GYRO_RANGE);
}

void IMUManager::enableMotion()
{
    // TODO: Interrupt handler to wake up.
    imu.startWakeOnMotion(PIN_ACCEL_INTERRUPT, irqIMUWake);
}

void IMUManager::processIMUEvent(inv_imu_sensor_event_t *evt)
{
    if (imu.isAccelDataValid(evt) && imu.isGyroDataValid(evt))
    {
        // Extract the data we need from the gyro.
        float zGyro = SCALE_GYRO(evt->gyro[2]); // TODO: Work out direction and axis.
        float xAccel = m_correctCentripedal(SCALE_ACCEL(evt->accel[0]), IMU_OFFSET_X, zGyro);
        float yAccel = m_correctCentripedal(SCALE_ACCEL(evt->accel[1]), IMU_OFFSET_Y, zGyro);

        // Do stuff with timestamps
        IMUData data;
        data.timestamp = imuTime;

        // Save the temperature.
        taskENTER_CRITICAL(&spinlock);
        m_lastTemperature = evt->temperature;
        taskEXIT_CRITICAL(&spinlock);

        // Add to the Kalman filter
        float theta = m_calculateAngle(xAccel, yAccel);
        // LOGD("Accel", "%f", theta);
        // log_printf("Angle measured: %f\n", theta);
        Matrix<2, 1, float> measurement;
        measurement(0, 0) = -theta;
        measurement(1, 0) = zGyro;
        kalman.update(measurement, data.timestamp);

        // Get ready to send the data
        Matrix<2, 1, float> state = kalman.getState();
        data.position = state(0, 0);
        data.velocity = state(1, 0);

        // Check whether it should be sent.
        if (m_sendCount >= config.imuHowOften)
        {
            // We should send this time.
            data.xAccel = xAccel;
            data.yAccel = yAccel;
            data.zAccel = SCALE_ACCEL(evt->accel[2]);
            data.xGyro = SCALE_GYRO(evt->gyro[0]);
            data.yGyro = SCALE_GYRO(evt->gyro[1]);
            data.zGyro = zGyro;
            connectionBasePtr->addIMU(data);
            m_sendCount = 0;
        }
        m_sendCount++;

        // Calculate the number of rotations.
        // Calculate the current sector.
        int8_t rotationSector = m_angleToSector(data.position);

        // Arm trigger if crossing from sector 0 to sector 1
        if (rotationSector == 1 and m_lastRotationSector == 0)
        {
            m_armRotationCounter = true;
        }

        // If trigger is armed and we crossed from sector 2 back to sector 0, increase the count.
        if (m_armRotationCounter && rotationSector == 0 && m_lastRotationSector == 2)
        {
            // We have a complete rotation. // TODO: Confirm direction.
            m_armRotationCounter = false;

            // Write to variables that need to be protected
            taskENTER_CRITICAL(&spinlock);
            rotations++;
            m_lastRotationDuration = data.timestamp - m_lastRotationTime;
            m_lastRotationTime = data.timestamp;
            taskEXIT_CRITICAL(&spinlock);
        }
        m_lastRotationSector = rotationSector;
    }
    else
    {
        LOGE("IMU", "Accel or Gyro data invalid");
    }
}

void IMUManager::setLowSpeedData(LowSpeedData &data)
{
    // Safely copy the data in to the object.
    taskENTER_CRITICAL(&spinlock);
    data.lastRotationDuration = m_lastRotationDuration;
    data.timestamp = m_lastRotationTime;
    data.rotationCount = rotations;
    taskEXIT_CRITICAL(&spinlock);
}

float IMUManager::getLastTemperature()
{
    // Safely copy the data in to the object.
    taskENTER_CRITICAL(&spinlock);
    float temp = m_lastTemperature / 2 + 25;
    taskEXIT_CRITICAL(&spinlock);
    return temp;
}

inline float const IMUManager::m_correctCentripedal(float reading, float radius, float velocity)
{
    return reading + radius * velocity * velocity;
}

float const IMUManager::m_calculateAngle(float x, float y)
{
    if (x != 0)
    {
        // Most cases, avoiding any chance of divide by 0 errors.
        float angle = atanf(y / x);
        if (x > 0)
        {
            // First or fourth quadrants
            return angle;
        }
        else
        {
            // Second or third quadrants
            if (y >= 0)
            {
                // Second
                return M_PI + angle;
            }
            else
            {
                // Third
                return -M_PI + angle;
            }
        }
    }
    else
    {
        // Very rare case when x is 0 (avoid division by 0).
        if (y >= 0)
        {
            // Straight up.
            return M_PI_2;
        }
        else
        {
            // Straight down.
            return -M_PI_2;
        }
    }
}

inline int8_t IMUManager::m_angleToSector(float angle)
{
    if (angle < -M_PI / 3)
    {
        return 0;
    }
    else if (angle < M_PI / 3)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

void taskIMU(void *pvParameters)
{
    LOGD("IMU", "Starting the IMU task");
    while (true)
    {
        // Wait for the interrupt to occur and we get a notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // Get data from the accelerometer. An anonymous function is used as callback to call the correct class method.
        powerMeter.imuManager.imu.getDataFromFifo(
            [](inv_imu_sensor_event_t *evt)
            {
                powerMeter.imuManager.processIMUEvent(evt);
            });
    }
}

void irqIMUActive()
{
    // Notify the IMU task. If the IMU task has a higher priority than the one currently running, force a context
    // switch.
    imuTime = micros();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(imuTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void irqIMUWake()
{
    // TODO
    LOGI("Wake", "Wakeup interrupt received");
}