/**
 * @file imp.cpp
 * @brief Class and function to process the IMU data on the power meter.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-31
 */

#include "imu.h"
// Power meter requires that the IMUManager class is defined, so put the include here.
#include "power_meter.h"
extern SemaphoreHandle_t serialMutex;
extern TaskHandle_t imuTaskHandle;
extern PowerMeter powerMeter;

void IMUManager::begin()
{
    LOGD("IMU", "Starting IMU");
    // Initialise the IMU
    // Manually initialise the SPI bus beforehand so that the pins can be specified. The call to `SPI.begin()` in the
    // IMU library should have no effect / returned early.
    SPI.begin(PIN_SPI_SCLK, PIN_SPI_SDI, PIN_SPI_SDO, PIN_SPI_AC_CS);
    int result = imu.begin();
    if (!result)
    {
        LOGE("IMU", "Cannot connect to IMU, error %d.", result);
    }
}

void IMUManager::startEstimating()
{
    // Setup the IMU
    imu.enableFifoInterrupt(PIN_ACCEL_INTERRUPT, irqIMUActive, 10);
    imu.startAccel(IMU_SAMPLE_RATE, IMU_ACCEL_RANGE);
    imu.startGyro(IMU_GYRO_RANGE, IMU_GYRO_RANGE);
}

void IMUManager::enableMotion()
{
    // TODO: Interrupt handler.
    imu.startWakeOnMotion(PIN_ACCEL_INTERRUPT, irqIMUWake);
}

void IMUManager::processIMUEvent(inv_imu_sensor_event_t *evt)
{
    if (imu.isAccelDataValid(evt) && imu.isGyroDataValid(evt))
    {
        // Extract the data we need from the gyro.
        float zGyro = SCALE_GYRO(evt->gyro[2]); // TODO: Work out direction and axis.
        float xAccel = m_correctCentripedal(SCALE_ACCEL(evt->accel[0]), IMU_OFFSET_X, zGyro);
        float yAccel = m_correctCentripedal(SCALE_ACCEL(evt->accel[0]), IMU_OFFSET_X, zGyro);
        LOGV("X Accel", "%f", xAccel);
        // Calculate the time step (given in 4us resolution).
        float timeStep = (evt->timestamp_fsync - m_lastTimestamp) * 4e-6;
        m_lastTimestamp = evt->timestamp_fsync;

        // Add to the Kalman filter
        Matrix<2, 1, float> measurement;
        measurement(0, 0) = m_calculateAngle(xAccel, yAccel);
        measurement(0, 1) = zGyro;
        m_kalman.update(measurement, timeStep);
    }
    else
    {
        LOGE("IMU", "Accel or Gyro data invalid");
    }
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
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(imuTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void irqIMUWake()
{
    // TODO
    LOGI("Wake", "Wakeup interrupt received");
}