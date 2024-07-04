/**
 * @file constants.example.h
 * @brief Constants and settings specific to individual power meters.
 * 
 * Copy this file and rename it to `constants.h`. Tweak the values to calibrate the power meter.
 *
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-04
 */
#pragma once

/**
 * @brief Constants for the Kalman filter used to estimate the current rotation angle and angular velocity.
 * 
 */
#define KALMAN_Q {0.002, 0, 0, 0.01} // Environmental covariance matrix.
#define KALMAN_R {1e4, 0, 0, 0.1} // Measurement covariance matrix.
#define KALMAN_X0 {0, 0} // Initial state.
#define KALMAN_P0 {1e6, 1e6, 1e6, 1e6} // Initial covariance. High numbers mean we don't know to start with.