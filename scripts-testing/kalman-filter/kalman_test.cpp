/**
 * @file kalman_test.cpp
 * @brief Testing script for the Kalman filter. Compare it against the jupyter notebook implementation.
 * 
 * Note that line 10 of `BasicLinearAlgebra/tests/Arduino.h` needs to be modified to change the definition
 * `struct Print` to `static struct Print` to avoid linking errors.
 * 
 * @version 0.0.0
 * @date 2024-07-02
 */
#include <iostream>
#include "kalman.h"

using namespace BLA;

typedef double T;

// Create the kalman filter.
// const Matrix<2, 2, T> qEnvCovariance = {0.03, 0, 0, 0.03};
const Matrix<2, 2, T> qEnvCovariance = {0, 0, 0, 0};
const Matrix<2, 2, T> rMeasCovariance = {0.02, 0, 0, 0.03};
Matrix<2, 1, T> xInitialState = {1.97, 0.2};
Matrix<2, 2, T> pInitialCovariance = {1, 0, 0, 1};
Kalman<T> kalman(qEnvCovariance, rMeasCovariance, xInitialState, pInitialCovariance);

// Readings
Matrix<2, 1, T> measurements[] = {
    {2, 0.19},
    {2.04, 0.2},
    {2.08, 0.21},
    {2.12, 0.19},
    {2.16, 0.21},
    {2.2, 0.2},
    {2.24, 0.2},
    {2.3, 0.2},
    {2.5, 0.6}
};
const int measurementCount = sizeof(measurements) / sizeof(Matrix<2, 1, T>);

int main()
{
    for (int i = 0; i < measurementCount; i++)
    {
        kalman.update(measurements[i], 0.2);
        Matrix<2, 1, T> x = kalman.getState();
        Matrix<2, 2, T> p = kalman.getCovariance();
        printf("Step %d:\n%12s %1.3frad %2.3frad/sec\n%12s %1.3frad %2.3frad/sec\n\n", i, "Measured:", measurements[i](0, 0), measurements[i](1, 0), "Output:",  x(0, 0), x(1, 0));

    }
    return 0;
}