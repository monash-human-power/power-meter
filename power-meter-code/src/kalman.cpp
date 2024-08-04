/**
 * @file kalman.cpp
 * @brief Implementation of a Kalman filter for predicting orientation and angular velocity.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-08-05
 */
#include "kalman.h"

extern portMUX_TYPE spinlock;

template <typename T>
void Kalman<T>::update(Matrix<2, 1, T> &measurement, T timestep)
{
    // Prediction step.
    Matrix<2, 2, T> fPrediction = {1, 0, 0, 1};
    fPrediction(0, 1) = timestep;
    Serial.println(fPrediction);
    Matrix<2, 1, T> x = fPrediction * m_xState;
    x(0, 0) = limitAngle(x(0, 0)); // Make sure we wrap around if needed.

    // P[k] = F * P_prev * Transpose(F) + Q
    Matrix<2, 2, T> p = fPrediction * m_pCovariance;
    // Manually transpose F
    fPrediction(0, 1) = 0;
    fPrediction(1, 0) = timestep;
    p = p * fPrediction + m_qEnvCovariance;

    // Refinement step.
    // Assuming that the measurements match the state (h = [[1, 0], [0, 1]]).
    Matrix<2, 2, T> kPrime = p * Inverse(p + m_rMeasCovariance);
    p = p - kPrime * p;
    x = x + kPrime * subtractStates(measurement, x);

    // Save the new state.
    TAKE_KALMAN_PROTECT();
    m_xState = x;
    m_pCovariance = p;
    GIVE_KALMAN_PROTECT();
}

template <typename T>
inline T Kalman<T>::limitAngle(T input)
{
    input = fmod(input, 2 * M_PI);
    if (input > M_PI)
    {
        input -= 2 * M_PI;
    }
    return input;
}

template <typename T>
inline Matrix<2, 1, T> Kalman<T>::subtractStates(Matrix<2, 1, T> state1, Matrix<2, 1, T> state2)
{
    Matrix<2, 1, T> result = state1 - state2;
    if (abs(result(0, 0)) > M_PI)
    {
        result(0, 0) -= 2 * M_PI;
    }
    return result;
}

template <typename T>
inline void Kalman<T>::resetState(Matrix<2, 1, T> xInitialState, Matrix<2, 2, T> pInitialCovariance)
{
    m_xState = xInitialState;
    m_pCovariance = pInitialCovariance;
}

template <typename T>
inline Matrix<2, 2, T> Kalman<T>::getCovariance()
{
    TAKE_KALMAN_PROTECT();
    Matrix<2, 2, T> result = m_pCovariance;
    GIVE_KALMAN_PROTECT();
    return result;
}

template <typename T>
inline Matrix<2, 1, T> Kalman<T>::getState()
{
    TAKE_KALMAN_PROTECT();
    Matrix<2, 1, T> result = m_xState;
    GIVE_KALMAN_PROTECT();
    return result;
}

template class Kalman<float>;
template class Kalman<double>;