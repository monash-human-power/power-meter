#!/usr/bin/env python3
# Compares the onboard Kalman filter to a calculated one.
# Cut down version of the jupyter notebook.
# Imports and setup
import matplotlib.pyplot as plt
import numpy as np
import sympy as sp
from sympy.stats import Normal, cdf, density, Binomial, ContinuousRV, DiscreteRV, P, E
import cmath as cm
from IPython.display import display, Markdown
from enum import Enum
import pandas as pd

# Load data from the actual unit
data = pd.read_csv("../python-clients/imu.csv")
offset = data["Timestamp [us]"].values[0]
data["Timestamp [us]"] = (data["Timestamp [us]"] - offset) / 1e6
data["z_accel"] = 0
data["x_gyro"] = 0
data["y_gyro"] = 0
data.rename(
    columns={
        "Timestamp [us]": "time",
        "Acceleration X [m/s^2]": "x_accel",
        "Acceleration Y [m/s^2]": "y_accel",
        "Gyro Z [rad/s]": "z_gyro"
    },
    inplace=True
)

# Offsets of the IMU from center of rotation
# In metres. Phone
# imu_length_offset = 0.115
# imu_width_offset = 0.02
# No offsets
imu_length_offset = 0
imu_width_offset = 0
imu_radius_offset = np.sqrt(imu_length_offset**2 + imu_width_offset**2)

# Correct for centripidal forces.
data["x_corrected"] = data["x_accel"] - imu_width_offset*(data["z_gyro"]**2)
data["y_corrected"] = data["y_accel"] + imu_length_offset*(data["z_gyro"]**2)
data["centripedal_accel"] = imu_radius_offset*(data["z_gyro"]**2)

# Calculate the angle from the accelerometer
angle = np.arctan(np.abs(data["y_corrected"] / data["x_corrected"]))
data.loc[(data["x_corrected"] >= 0) & (data["y_corrected"] >= 0), "theta_accel"] = angle # First quadrant
data.loc[(data["x_corrected"] < 0) & (data["y_corrected"] >= 0), "theta_accel"] = np.pi - angle # Second quadrant
data.loc[(data["x_corrected"] < 0) & (data["y_corrected"] < 0), "theta_accel"] = -np.pi + angle # Third quadrant
data.loc[(data["x_corrected"] >= 0) & (data["y_corrected"] < 0), "theta_accel"] = -angle # Fourth quadrant
data["theta_accel"] *= -1




def limit_angles(value):
    while value > np.pi or value <= -np.pi:
        if value > np.pi:
            value -= 2*np.pi
        elif value <= -np.pi:
            value += 2*np.pi
    
    return value
    # value = value % (2*np.pi)
    # if value > np.pi:
    #     value -= 2*np.pi
    
    return value

def angle_diff(vector1, vector2):
    """Subtracts two thera-omega vectors from each other, wrapping around to use the shortest side of the circle.

    Args:
        vector1 (np.ndarray): The first vector to subtract.
        vector2 (np.ndarray): The second vector to subtract

    Returns:
        np.ndarray: vector1-vector2 taking into accound the cyclical nature of theta.
    """
    difference = vector1[0] - vector2[0]
    if abs(difference[0]) > np.pi:
        # Over 1/2 circle, can go around the other way.
        difference = 2*np.pi - difference
        
    result = np.array([
        difference,
        vector1[1] - vector2[1]
    ])
    return result


def kalman_step(x_prev:np.ndarray, p_prev:np.ndarray, time:float, env_uncertainty:np.ndarray, measured:np.ndarray, meas_uncertainty:np.ndarray):
    """A Kalman filter modified to work with angles.

    Args:
        x_prev (np.ndarray): The previous state as a 2 row, 1 column matrix (position on top, angular velocity below).
        p_prev (np.ndarray): The previous covariance matrix.
        time (float): The time step from the last call.
        env_uncertainty (np.ndarray): The environmental uncertainty covariance matrix.
        measured (np.ndarray): The measured values at this step.
        meas_uncertainty (np.ndarray): The covariance matrix representing the measured values' uncertainty.

    Returns:
        Tuple: the current position and current covariance matrix.
    """
    x_prev = x_prev.reshape(2, 1)
    # p_prev = p_prev.reshape(2, 1)
    # env_uncertainty = env_uncertainty.reshape(2, 1)
    measured = measured.reshape(2, 1)
    # meas_uncertainty = meas_uncertainty.reshape(2, 1)
    # Prediction
    fk = np.array([
        [1, time],
        [0, 1]
    ])
    x_predict = np.matmul(fk, x_prev) # Ignoring Bk u

    # Limit theta
    x_predict[0] = limit_angles(x_predict[0])
    
    p_predict = np.matmul(fk, p_prev)
    p_predict = np.matmul(p_predict, fk.transpose())
    p_predict = p_predict + env_uncertainty
    # return x_predict, p_predict
    # print(f"{p_predict=}")
    # Update
    hk = np.array([
        [1, 0],
        [0, 1]
    ])
    k_prime = p_predict.dot(hk.transpose()).dot(
        np.linalg.inv(
            hk.dot(p_predict).dot(hk.transpose()) + meas_uncertainty
        )
    )
    # temp = np.dot(np.dot(hk, p_predict), hk.T) + meas_uncertainty
    # k_prime = np.dot(np.dot(p_predict, hk.T), np.linalg.inv(temp))   
    # x_prime = x_predict + k_prime.dot(measured - hk.dot(x_predict))
    x_prime = x_predict + k_prime.dot(angle_diff(measured, hk.dot(x_predict)))
    p_prime = p_predict - k_prime.dot(hk).dot(p_predict)

    # temp2 = measured - np.dot(hk, x_predict)  
    # x_prime = x_predict + np.dot(k_prime, temp2)

    # I = np.eye(p_predict.shape[0])
    # p_prime = np.dot(np.dot(I - np.dot(k_prime, hk), p_predict), (I - np.dot(k_prime, hk)).T) + np.dot(np.dot(k_prime, meas_uncertainty), k_prime.T)

    x_prime[0] = limit_angles(x_prime[0])

    return x_predict, x_prime, p_prime




# Run the code
print("Running")
# Lists to save data in
thetas = np.zeros(shape=(len(data), 1))
omegas = np.zeros(shape=(len(data), 1))

# Initial states
x = np.array([[0], [0]]) # Starting position.
p = np.array([[1e4, 1e4], [1e4, 1e4]]) # Initially not confident where we are.

# Constants
# Good baseline with the phone
env_uncertainty = np.array([[0.002, 0], [0, 0.1]]) # Probably will be somewhere near here, allows a bit to account for changing in speed.
meas_uncertainty = np.array([[100, 0], [0, 0.01]]) # Really not confident in the accelerometer, very confident in the gyroscope.
# env_uncertainty = np.array([[1, 0], [0, 1]])
# meas_uncertainty = np.array([[1, 0], [0, 1]])

# Run the kalmin filter
for i in range(1, len(data)): # // 3 for quicker testing
    # Calculate parameters
    prev_row = data.loc[i-1]
    cur_row = data.loc[i]
    time = cur_row["time"] - prev_row["time"]
    meas = np.array([cur_row["theta_accel"], cur_row["z_gyro"]])

    # Do the step
    _, x, p = kalman_step(x, p, time, env_uncertainty, meas, meas_uncertainty)

    # Save the results for analysis later
    thetas[i] = x[0]
    omegas[i] = x[1]

data["theta"] = thetas
data["omega"] = omegas


# Integrate the gyroscope data to calculate position (with drift).
time_step = np.concatenate((np.array([0]), np.diff(data["time"])))
impulse = data["z_gyro"] * time_step
full_angle = impulse.cumsum()
data["theta_gyro"] = full_angle
data["theta_gyro"] -= 0.2 # Roughly zero the start
data["theta_gyro"] %= (2*np.pi)
data["theta_gyro"] -= np.pi

# Plot everything
cycle = plt.rcParams['axes.prop_cycle'].by_key()['color']
# plt.close()
fig = plt.figure()
gs = fig.add_gridspec(2, height_ratios=[0.5, 1])
ax_omega, ax_theta = gs.subplots(sharex=True)

# Velocity (omega)
ax_omega.plot(data["time"].values, data["z_gyro"].values, label="Gyro", color=cycle[0])
ax_omega.plot(data["time"].values, data["omega"].values, label="Kalman", color=cycle[2])
ax_omega.plot(data["time"].values, data["Velocity [rad/s]"].values, label="Onboard Kalman", color=cycle[3])
ax_omega.set_ylabel("Velocity [rad/s]")
ax_omega.set_title("Velocity")
ax_omega.legend()

# Position (theta)
ax_theta.axhline(-np.pi, color="black")
ax_theta.axhline(np.pi, color="black")
ax_theta.plot(data["time"].values, data["theta_gyro"].values, label="Gyro", color=cycle[0])
ax_theta.plot(data["time"].values, data["theta_accel"].values, label="Accelerometer", color=cycle[1])
ax_theta.plot(data["time"].values, data["theta"].values, label="Kalman", color=cycle[2])
ax_theta.plot(data["time"].values, data["Position [rad]"].values, label="Onboard Kalman", color=cycle[3])
ax_theta.set_xlabel("Times [s]")
ax_theta.set_ylabel("Position [rad]")
ax_theta.set_title("Position calculations using different methods")
ax_theta.legend()
plt.show()