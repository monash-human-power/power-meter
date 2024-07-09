/**
 * @file kalman.h
 * @brief Implementation of a Kalman filter for predicting orientation and angular velocity.
 *
 * @author Jotham Gates and Oscar Varney, MHP
 * @version 0.0.0
 * @date 2024-07-09
 */
#pragma once
#include <BasicLinearAlgebra.h>

#ifdef PROTECT_KALMAN_STATES
    // All access operations are very quick. For efficiency, making this a critical section should be ok.
    #define TAKE_KALMAN_PROTECT() taskENTER_CRITICAL()
    #define GIVE_KALMAN_PROTECT() taskEXIT_CRITICAL()
#else
    #define TAKE_KALMAN_PROTECT()
    #define GIVE_KALMAN_PROTECT()
#endif

using namespace BLA;
/**
 * @brief Class that implements a Kalman filter to predict the current angle and angular velocity.
 * 
 * Accelerometer and gyroscope data are used as inputs. The update step has been modified to account for continuous
 * rotation  - i.e. accepting 180 degrees i the same as -180 degrees. 
 * 
 * @tparam T is the type to use for calculations (`float` or `double`).
 */
template <typename T>
class Kalman
{
public:
    /**
     * @brief Construct a new Kalman object with the given constants and initial state estimates.
     * 
     * @param qEnvCovariance 2x2 matrix representing the covariance of noise from the environment that increases the
     *                       uncertainty of the prediction over time.
     * @param rMeasCovariance 2x2 matrix representing the covariance of the noise / uncertainty from the measurement
     *                        device.
     * @param xInitialState The initial state extimate.
     * @param pInitialCovariance The covariance matrix of the initial state. Large values in this will allow wildy
     *                           inaccurate initial guesses to be quickly forgotten.
     */
    Kalman(const Matrix<2, 2, T> qEnvCovariance, const Matrix<2, 2, T> rMeasCovariance, Matrix<2, 1, T> xInitialState,
        Matrix<2, 2, T> pInitialCovariance) : m_qEnvCovariance(qEnvCovariance), m_rMeasCovariance(rMeasCovariance), m_xState(xInitialState), m_pCovariance(pInitialCovariance) {}
    
    /**
     * @brief Adds a new set of measurements to the model.
     * 
     * @param measurement 2x1 matrix representing the new measurements (position on top, omega on bottom).
     * @param timestep The timestep in seconds from the previous measurement.
     */
    void update(Matrix<2, 1, T> &measurement, T timestep);

    /**
     * @brief Resets the state to the given values.
     * 
     * @param xInitialState The initial state extimate.
     * @param pInitialCovariance The covariance matrix of the initial state. Large values in this will allow wildy
     *                           inaccurate initial guesses to be quickly forgotten.
     */
    void resetState(Matrix<2, 1, T> xInitialState, Matrix<2, 2, T> pInitialCovariance);

    /**
     * @brief Get the covariance matrix (thread safe).
     * 
     * @return Matrix<2, 2, T> 
     */
    Matrix<2, 2, T> getCovariance();

    /**
     * @brief Get the state vector (thread safe).
     * 
     * @return Matrix<2, 1, T> 2x1 matrix, (0,0) is the angle, (0,1) is the angular velocity.
     */
    Matrix<2, 1, T> getState();

private:
    /**
     * @brief Limits an angle to between -pi and pi.
     * 
     * @param input the input angle.
     * @return T the angle limited (modulo) to a single rotation.
     */
    T limitAngle(T input);

    /**
     * @brief Subtracts two position-velocity vectors from each other.
     * 
     * The shortest angle around the circle is taken for the position.
     * 
     * @param state1 A 2x1 matrix with position on top, velocity on the bottom.
     * @param state2 A 2x1 matrix with position on top, velocity on the bottom.
     * @return Matrix<2, 1, T> The difference between the matrices.
     */
    Matrix<2, 1, T> subtractStates(Matrix<2, 1, T> state1, Matrix<2, 1, T> state2);

    const Matrix<2, 2, T> m_qEnvCovariance, m_rMeasCovariance;

    /**
     * @brief Current state variables.
     * 
     */
    Matrix<2, 1, T> m_xState;
    Matrix<2, 2, T> m_pCovariance;
};