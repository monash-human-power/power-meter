/**
 * @file kalman.cpp
 * @brief Implementation of a Kalman filter for predicting orientation and angular velocity.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.1.0
 * @date 2024-09-04
 */
#include "kalman.h"

#ifdef PROTECT_KALMAN_STATES
extern portMUX_TYPE spinlock;
#endif

template <typename T>
void Kalman<T>::update(Matrix<2, 1, T> &measurement, uint32_t time)
{
    // Run the predition
    Matrix<2, 1, T> x;
    Matrix<2, 2, T> p;
    predict(time, x, p);
    m_lastTimestamp = time; // Should be atomic?

    // Refinement step.
    // Assuming that the measurements match the state (h = [[1, 0], [0, 1]]).
    Matrix<2, 2, T> kPrime = p * Inverse(p + m_rMeasCovariance);
    p = p - (kPrime * p);
    x = x + (kPrime * subtractStates(measurement, x));
    x(0,0) = limitAngle(x(0,0));

    // Save the new state.
    TAKE_KALMAN_PROTECT();
    m_xState = x;
    m_pCovariance = p;
    GIVE_KALMAN_PROTECT();
}

template <typename T>
void Kalman<T>::predict(uint32_t time, Matrix<2, 1, T> &xState)
{
    Matrix<2, 2, T> pState; // Matrix that can be chucked away afterwards.
    predict(time, xState, pState);
}

template <typename T>
inline T Kalman<T>::limitAngle(T input)
{
    // Do many times to make sure in case we are moving fast.
    while (input > M_PI || input <= -M_PI)
    {
        if (input > M_PI)
        {
            input -= 2*M_PI;
        } else if (input <= -M_PI)
        {
            input += 2*M_PI;
        }
    }
    return input;
}

template <typename T>
inline Matrix<2, 1, T> Kalman<T>::subtractStates(Matrix<2, 1, T> state1, Matrix<2, 1, T> state2)
{
    // https://stackoverflow.com/a/27308346
    float angleDiff = fmodf(state1(0,0) - state2(0,0) + 2*M_PI, 2*M_PI);
    if (angleDiff > M_PI)
    {
        angleDiff = -(2*M_PI - angleDiff);
    }
    Matrix<2, 1, T> result;
    result(0,0) = angleDiff;
    result(1,0) = state1(1,0) - state2(1,0);
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

template <typename T>
void Kalman<T>::predict(uint32_t time, Matrix<2, 1, T> &xState, Matrix<2, 2, T> &pCovariance)
{
    // Get the current states.
    TAKE_KALMAN_PROTECT();
    xState = m_xState;
    pCovariance = m_pCovariance;
    GIVE_KALMAN_PROTECT();

    // Prediction step.
    Matrix<2, 2, T> fPrediction = {1, 0, 0, 1};
    T timestep = (time - m_lastTimestamp) * 1e-6; // Calculate the timestep.
    fPrediction(0, 1) = timestep;
    // log_printf("fPrediction: {%f, %f, %f, %f}\n", fPrediction(0, 0), fPrediction(0, 1), fPrediction(1, 0), fPrediction(1, 1));
    xState = fPrediction * xState;
    xState(0, 0) = limitAngle(xState(0, 0)); // Make sure we wrap around if needed.
    // log_printf("X: {%f, %f}\n", x(0,0), x(1,0));
    // P[k] = F * P_prev * Transpose(F) + Q
    pCovariance = ((fPrediction * pCovariance) * ~fPrediction) + m_qEnvCovariance;
}

template class Kalman<float>;
template class Kalman<double>;