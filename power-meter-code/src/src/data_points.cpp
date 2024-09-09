/**
 * @file data_points.cpp
 * @brief Classes that represent data from a single point.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-10
 */
#include "data_points.h"

inline float HousekeepingData::averageTemp()
{
    return (temperatures[SIDE_LEFT] + temperatures[SIDE_RIGHT]) / 2;
}

float LowSpeedData::cadence()
{
    if (lastRotationDuration != 0)
    {
        return 60e6/lastRotationDuration;
    }
    else
    {
        return 0;
    }
}

inline float BaseData::cadence()
{
    return VELOCITY_TO_CADENCE(velocity);
}

void BaseData::baseBytes(uint8_t *buffer)
{
    // We can't just copy the entire struct due to the potential for byte packing.
    ADD_TO_BYTES(timestamp, buffer, 0); // Add the timestamp
    ADD_TO_BYTES(velocity, buffer, 4); // Add the velocity
    ADD_TO_BYTES(position, buffer, 8); // Add the position
}


void IMUData::toBytes(uint8_t *buffer)
{
    BaseData::baseBytes(buffer); // 12 bytes
    ADD_TO_BYTES(xAccel, buffer, BASE_BYTES_SIZE);
    ADD_TO_BYTES(yAccel, buffer, BASE_BYTES_SIZE + 4);
    ADD_TO_BYTES(zAccel, buffer, BASE_BYTES_SIZE + 8);
    ADD_TO_BYTES(xGyro, buffer, BASE_BYTES_SIZE + 12);
    ADD_TO_BYTES(yGyro, buffer, BASE_BYTES_SIZE + 16);
    ADD_TO_BYTES(zGyro, buffer, BASE_BYTES_SIZE + 20);
}

inline float HighSpeedData::power()
{
    return velocity * torque;
}

void HighSpeedData::toBytes(uint8_t *buffer)
{
    baseBytes(buffer); // 12 bytes
    ADD_TO_BYTES(raw, buffer, BASE_BYTES_SIZE);
    float calcTorque = torque;
    ADD_TO_BYTES(calcTorque, buffer, BASE_BYTES_SIZE + 4);
    float calcPower = power();
    ADD_TO_BYTES(calcPower, buffer, BASE_BYTES_SIZE + 8);
}
