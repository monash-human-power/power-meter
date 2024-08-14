/**
 * @file data_points.cpp
 * @brief Classes that represent data from a single point.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-14
 */
#include "data_points.h"

inline float HousekeepingData::averageTemp()
{
    return (temperatures[SIDE_LEFT] + temperatures[SIDE_RIGHT]) / 2;
}

float LowSpeedData::cadence()
{
    return 60e6/lastRotationDuration;
}

inline float BaseData::cadence()
{
    return VELOCITY_TO_CADENCE(velocity);
}

void BaseData::toBytes(uint8_t *buffer)
{
    // We can't just copy the entire struct due to the potential for byte packing.
    ADD_TO_BYTES(timestamp, buffer, 0); // Add the timestamp
    ADD_TO_BYTES(velocity, buffer, 4); // Add the velocity
    ADD_TO_BYTES(position, buffer, 8); // Add the position
}


void IMUData::toBytes(uint8_t *buffer)
{
    BaseData::toBytes(buffer); // 12 bytes
    ADD_TO_BYTES(xAccel, buffer, 12);
    ADD_TO_BYTES(yAccel, buffer, 16);
    ADD_TO_BYTES(zGyro, buffer, 20);
}

inline float HighSpeedData::power()
{
    return velocity * torque;
}

void HighSpeedData::toBytes(uint8_t *buffer)
{
    BaseData::toBytes(buffer); // 12 bytes
    ADD_TO_BYTES(raw, buffer, 12);
    float calcTorque = torque;
    ADD_TO_BYTES(calcTorque, buffer, 16);
    float calcPower = power();
    ADD_TO_BYTES(calcPower, buffer, 20);
}
