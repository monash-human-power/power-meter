#!/usr/bin/env python3
"""plot_imu.py
usage: plot_imu.py [-h] -i INPUT [-t TITLE] [-g GLOBAL_OFFSET] [--start START] [--stop STOP] [-d]

Plots IMU-related timeseries data. Alternative methods of calculating orientation are used to provide something to compare the inbuilt Kalman filter to.

options:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The folder containing the CSV files. (default: None)
  -t TITLE, --title TITLE
                        Title to put on the figure (default: IMU Timeseries data)

Time:
  Parameters relating to time offsets and limiting the time periods plotted.

  -g GLOBAL_OFFSET, --global-offset GLOBAL_OFFSET
                        Global time offset in seconds. This is mainly for making the x axis look nicer. The start and stop times also depend on this. (default: 0)
  --start START         The minimum time to show (after all offsets are applied). (default: None)
  --stop STOP           The maximum time to show (after all offsets are applied). (default: None)
  -d, --device-time     Does not correct for the device time potentially being reset. (default: True)

Written by Jotham Gates and Oscar Varney for MHP, 2024
"""
import pandas as pd
import numpy as np
from typing import Tuple
import matplotlib.pyplot as plt
import argparse

from common import add_time_args, apply_time_args, apply_device_time_corrections

def accel_to_angle(accel_x: np.ndarray, accel_y: np.ndarray) -> np.ndarray:
    """Converts X and Y acceleration to angles.

    Args:
        accel_x (np.ndarray): The X acceleration.
        accel_y (np.ndarray): The Y acceleration.

    Returns:
        np.ndarray: Angles between -pi and pi.
    """
    # Calculate the angle from the accelerometer
    small_angles = np.arctan(np.abs(accel_y / accel_x))
    angles = np.empty(accel_x.size)
    angles[(accel_x >= 0) & (accel_y >= 0)] = small_angles[
        (accel_x >= 0) & (accel_y >= 0)
    ]
    angles[(accel_x < 0) & (accel_y >= 0)] = (
        np.pi - small_angles[(accel_x < 0) & (accel_y >= 0)]
    )
    angles[(accel_x < 0) & (accel_y < 0)] = (
        -np.pi + small_angles[(accel_x < 0) & (accel_y < 0)]
    )
    angles[(accel_x >= 0) & (accel_y < 0)] = -small_angles[
        (accel_x >= 0) & (accel_y < 0)
    ]
    angles *= -1
    return angles


def kalman(
    time: np.ndarray,
    angles: np.ndarray,
    gyro_z: np.ndarray,
    x0: np.ndarray = np.array([[0], [0]]),
    p0: np.ndarray = np.array([[1e4, 1e4], [1e4, 1e4]]),
    env_uncertainty: np.ndarray = np.array([[0.002, 0], [0, 0.1]]),
    meas_uncertainty: np.ndarray = np.array([[100, 0], [0, 0.01]]),
) -> Tuple[np.ndarray, np.ndarray]:
    """Runs a Kalman filter on this computer to verify the kalman filter on the imu.

    Args:
        time (np.ndarray): The timestamps of each measurement.
        accel_x (np.ndarray): The X acceleration in m/s^2, already corrected for centripedal force.
        accel_y (np.ndarray): The Y acceleration in m/s^2, already corrected for centripedal force.
        gyro_z (np.ndarray): The rotation velocity in rad/s.

    Returns:
        Tuple[np.ndarray, np.ndarray]: The theta (position) vector and the omega (velocity) vector.
    """

    def limit_angles(value: float) -> float:
        """Limits the given angle to between -pi and pi.

        Args:
            value (float): The input angle

        Returns:
            float: The limited angle.
        """
        while value > np.pi or value <= -np.pi:
            if value > np.pi:
                value -= 2 * np.pi
            elif value <= -np.pi:
                value += 2 * np.pi

        return value

    def angle_diff(vector1: np.ndarray, vector2: np.ndarray) -> np.ndarray:
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
            difference = 2 * np.pi - difference

        result = np.array([difference, vector1[1] - vector2[1]])
        return result

    def kalman_step(
        x_prev: np.ndarray,
        p_prev: np.ndarray,
        time: float,
        env_uncertainty: np.ndarray,
        measured: np.ndarray,
        meas_uncertainty: np.ndarray,
    ) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
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
        measured = measured.reshape(2, 1)

        # Prediction
        fk = np.array([[1, time], [0, 1]])
        x_predict = np.matmul(fk, x_prev)  # Ignoring Bk u

        # Limit theta
        x_predict[0] = limit_angles(x_predict[0])

        p_predict = np.matmul(fk, p_prev)
        p_predict = np.matmul(p_predict, fk.transpose())
        p_predict = p_predict + env_uncertainty

        # Update
        hk = np.array([[1, 0], [0, 1]])
        k_prime = p_predict.dot(hk.transpose()).dot(
            np.linalg.inv(hk.dot(p_predict).dot(hk.transpose()) + meas_uncertainty)
        )
        x_prime = x_predict + k_prime.dot(angle_diff(measured, hk.dot(x_predict)))
        p_prime = p_predict - k_prime.dot(hk).dot(p_predict)

        x_prime[0] = limit_angles(x_prime[0])

        return x_predict, x_prime, p_prime

    # Run the code
    print("Running Kalman filter")
    # Lists to save data in
    thetas = np.zeros(shape=(len(time), 1))
    omegas = np.zeros(shape=(len(time), 1))

    # Initial states
    x = x0  # Starting position.
    p = p0  # Initially not confident where we are.

    # Run the kalmin filter
    for i in range(1, len(time)):  # // 3 for quicker testing
        # Calculate parameters
        timestep = time[i] - time[i - 1]
        meas = np.array([angles[i], gyro_z[i]])

        # Do the step
        _, x, p = kalman_step(x, p, timestep, env_uncertainty, meas, meas_uncertainty)

        # Save the results for analysis later
        thetas[i] = x[0]
        omegas[i] = x[1]
    print("Finished running Kalman filter")
    return thetas, omegas


def integrate_gyro(
    times: np.ndarray, gyro: np.ndarray, offset: float = 0
) -> np.ndarray:
    """Integrates the gyroscope velocity data to estimate position. This will drift over time.

    Args:
        times (np.ndarray): The times of the readings.
        gyro (np.ndarray): The gyroscope readings.
        offset (float): An offset to add to all angles.

    Returns:
        np.ndarray: The calculated positions.
    """
    # Integrate the gyroscope data to calculate position (with drift).
    time_step = np.concatenate((np.array([0]), np.diff(times)))
    impulse = gyro * time_step
    full_angle = impulse.cumsum()
    position = full_angle + offset
    position -= 0.2  # Roughly zero the start
    position %= 2 * np.pi
    position -= np.pi
    return position


def plot_imu(df: pd.DataFrame, name: str = ""):
    """Plots the IMU data as a timeseries.

    Args:
        df (pd.DataFrame): Dataframe containing the loaded imu.csv file.
        name (str): Name of the dataset to print in the figure title.
    """
    # Extract the data from the device.
    times = df["Time"]
    accel_x = df["Acceleration X [m/s^2]"].values
    accel_y = df["Acceleration Y [m/s^2]"].values
    gyro_z = df["Gyro Z [rad/s]"].values
    device_velocity = df["Velocity [rad/s]"].values
    device_position = df["Position [rad]"].values

    # Calculate some other things to compare with.
    accel_position = accel_to_angle(accel_x, accel_y)
    gyro_position = integrate_gyro(times, gyro_z)
    kalman_position, kalman_velocity = kalman(times, accel_position, gyro_z)

    # Plot everything
    cycle = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    # plt.close()
    fig = plt.figure()
    gs = fig.add_gridspec(3, height_ratios=[0.5, 0.5, 1])
    ax_accel, ax_omega, ax_theta = gs.subplots(sharex=True)

    # Acceleration
    ax_accel.plot(times, accel_x, "r", label="X")
    ax_accel.plot(times, accel_y, "g", label="Y")
    ax_accel.legend()
    ax_accel.set_ylabel("Acceleration [$m/s^2$]")
    ax_accel.set_title("Accelerometer readings, corrected for centripedal forces")

    # Velocity (omega)
    ax_omega.plot(times, gyro_z, label="Gyro", color=cycle[0])
    ax_omega.plot(times, kalman_velocity, label="Kalman", color=cycle[2])
    ax_omega.plot(times, device_velocity, label="Onboard Kalman", color=cycle[3])
    ax_omega.set_ylabel("Velocity [$rad/s$]")
    ax_omega.set_title("Velocity")
    ax_omega.legend()

    # Position (theta)
    ax_theta.axhline(-np.pi, color="black")
    ax_theta.axhline(np.pi, color="black")
    ax_theta.plot(times, gyro_position, label="Gyro", color=cycle[0])
    ax_theta.plot(times, accel_position, label="Accelerometer", color=cycle[1])
    ax_theta.plot(times, kalman_position, label="Kalman", color=cycle[2])
    ax_theta.plot(times, device_position, label="Onboard Kalman", color=cycle[3])
    ax_theta.set_xlabel("Times [$s$]")
    ax_theta.set_ylabel("Position [$rad$]")
    ax_theta.set_title("Position calculations using different methods")
    ax_theta.legend()

    plt.suptitle(name)
    plt.show()


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Plots IMU-related timeseries data.\n\nAlternative methods of calculating orientation are used to provide something to compare the inbuilt Kalman filter to.",
        conflict_handler="resolve",  # Cope with -h being used for host like mosquitto clients
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="Written by Jotham Gates and Oscar Varney for MHP, 2024",
    )
    parser.add_argument(
        "-i",
        "--input",
        help="The folder containing the CSV files.",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-t",
        "--title",
        help="Title to put on the figure",
        type=str,
        default="IMU Timeseries data",
    )
    add_time_args(parser, add_device_compensate=True)
    args = parser.parse_args()

    imu_df = pd.read_csv(f"{args.input}/imu.csv")
    if len(imu_df):
        imu_df = apply_time_args(imu_df, args)
        apply_device_time_corrections(imu_df, args)
        plot_imu(imu_df, args.title)
    else:
        print("The IMU file is empty. Please use another dataset.")
