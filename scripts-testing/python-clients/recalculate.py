#!/usr/bin/env python3
"""recalculate.py
"""

import argparse
import shutil
import os
from typing import Tuple
import numpy as np
import pandas as pd
from abc import ABC, abstractmethod
from collections import deque
from multiprocessing import Process

from common import PowerMeterConfig, StrainConfig


class StrainProcessor(ABC):
    """Base class for processing strain data into torque."""

    def __init__(self, conf: StrainConfig) -> None:
        """Initialises the processor.

        Args:
            conf (StrainConfig): The config and calibration data to use.
        """
        self.conf = conf
        self.cur_temp = 0

    def add_temp(self, temperature: float) -> None:
        """Adds the current temperature

        Args:
            temperature (float): The current temperature for the side in celcius
        """
        self.cur_temp = temperature

    @abstractmethod
    def step(self, data: pd.DataFrame) -> float:
        """Takes the input strain data and calculates the torque.

        Args:
            data (pd.DataFrame): The data containing the position, speed and raw values.

        Returns:
            float: The torque in Nm.
        """
        pass

    def is_outlier(self, row: pd.DataFrame) -> bool:
        return False

    def process(
        self, strain: pd.DataFrame, housekeeping: pd.DataFrame, temp_col: str
    ) -> None:
        # Add the first temperature reading to start with.
        housekeeping_index = 0
        self.add_temp(housekeeping.iloc[housekeeping_index][temp_col])
        housekeeping_index += 1

        # Iterate over all fields and process it.
        for index, row in strain.iterrows():
            # Check if the next housekeeping data is older than the current timestamp.
            if (
                housekeeping_index < len(housekeeping)
                and row["Unix Timestamp [s]"]
                >= housekeeping.iloc[housekeeping_index]["Unix Timestamp [s]"]
            ):
                self.add_temp(housekeeping.iloc[housekeeping_index][temp_col])
                housekeeping_index += 1

            # Calculate the torque and power
            torque = self.step(row)
            velocity = row["Velocity [rad/s]"]
            strain.loc[index, ["Torque [Nm]", "Power [W]", "Outlier"]] = (
                torque,
                torque * velocity,
                self.is_outlier(row),
            )


class SimpleStrainProcessor(StrainProcessor):
    def step(self, data):
        return self.conf.apply(data["Raw [uint24]"], self.cur_temp)


class MovingAverage:
    def __init__(self, max_size: int) -> None:
        self.queue = deque()
        self.sum = 0
        self.max_size = max_size

    def add(self, value: float) -> None:
        self.sum += value
        self.queue.append(value)
        if len(self.queue) > self.max_size:
            self.sum -= self.queue.popleft()

    def calculate(self) -> float:
        return self.sum / len(self.queue)


class AutoZeroProcessor(StrainProcessor):
    def __init__(self, conf: StrainConfig):
        super().__init__(conf)
        self.adc_ave = MovingAverage(1000)
        self.last_move_time = 0

    def step(self, data):
        # Add to averages
        timestamp = data["Unix Timestamp [s]"]
        raw = data["Raw [uint24]"]
        self.adc_ave.add(raw)

        # If we are moving, update the last known time we weere moving.
        if abs(data["Velocity [rad/s]"]) > 0.1:
            self.last_move_time = timestamp

        # Check if a sufficient time has occurred without movement being detected. Automatic offset will be applied.
        elif timestamp - self.last_move_time > 10:
            # Not moved for a while. Apply the offset
            self.conf.strain_offset = self.adc_ave.calculate()
            print(timestamp, self.conf.strain_offset, timestamp - self.last_move_time)
            self.last_move_time = timestamp  # Stop this being called too often.

        # Calculate the average
        return self.conf.apply(raw, self.cur_temp)


class RemoveOutliersProcessor(AutoZeroProcessor):
    def __init__(self, conf):
        super().__init__(conf)
        self.prev_raw = 0
        self.prev_change = 0
        self.discard_count = 0

    def is_outlier(self, row: pd.DataFrame) -> bool:
        # Calculate the change in raw value from the last reading (f'(x)).
        cur_raw = row["Raw [uint24]"]
        cur_change = cur_raw - self.prev_raw
        self.prev_raw = cur_raw

        # Calculate the change in change from the last reading (f''(x))
        change_change = cur_change - self.prev_change
        self.prev_change = cur_change

        # If this change is too much, consider it an outlier.
        if self.discard_count > 0:
            self.discard_count -= 1
            return True
        elif cur_change < -20000:
            self.discard_count = 3
            return True
        # TODO: Make following ones also change. Make the threshold proportional to the amplitude of the last pedal stroke?
        else:
            return False


def kalman(
    time: np.ndarray,
    values: np.ndarray,
    x0: np.ndarray,
    p0: np.ndarray,
    env_uncertainty: np.ndarray,
    meas_uncertainty: np.ndarray,
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
        x_prev = x_prev.reshape(3, 1)
        measured = measured.reshape(3, 1)

        # Prediction
        fk = np.array([[1, time, 0], [0, 1, time], [0, 0, 1]])
        x_predict = np.matmul(fk, x_prev)  # Ignoring Bk u

        p_predict = np.matmul(fk, p_prev)
        p_predict = np.matmul(p_predict, fk.transpose())
        p_predict = p_predict + env_uncertainty

        # Update
        hk = np.eye(3)
        k_prime = p_predict.dot(hk.transpose()).dot(
            np.linalg.inv(hk.dot(p_predict).dot(hk.transpose()) + meas_uncertainty)
        )
        x_prime = x_predict + k_prime.dot(measured - hk.dot(x_predict))
        p_prime = p_predict - k_prime.dot(hk).dot(p_predict)

        return x_predict, x_prime, p_prime

    # Run the code
    print("Running Kalman filter")
    # Lists to save data in
    out_values = np.zeros(shape=(len(time), 3))

    # Calculate derivatives.
    in_values = np.empty(shape=(len(time), 3))
    in_values[:, 0] = values
    in_values[:, 1] = np.concat([[0], np.diff(in_values[:, 0])]) / time
    in_values[:, 2] = np.concat([[0], np.diff(in_values[:, 1])]) / time

    # Initial states
    x = x0  # Starting position.
    p = p0  # Initially not confident where we are.

    # Run the kalman filter
    for i in range(1, len(time)):  # // 3 for quicker testing
        # Calculate parameters
        timestep = time[i] - time[i - 1]
        meas = in_values[i, :]

        # Do the step
        _, out, p = kalman_step(
            x, p, timestep, env_uncertainty, meas, meas_uncertainty
        )

        # Save the results for analysis later
        out_values[i, :] = out.squeeze()
    print("Finished running Kalman filter")
    return out_values


def process_strain(
    strain_file: str,
    temp_col: str,
    conf: StrainConfig,
    housekeeping: pd.DataFrame,
    out_strain_file: str,
) -> pd.DataFrame:
    """Processes the strain values to recalculate torque and power.

    Args:
        input (pd.DataFrame): The input dataframe containing the raw values.
        conf (StrainConfig): The calibration to use.

    Returns:
        pd.DataFrame: The processed dataframe.
    """
    print(f"Processing {temp_col.split()[0].lower()} side.")
    strain = pd.read_csv(strain_file)
    # processor = SimpleStrainProcessor(conf)
    # processor = AutoZeroProcessor(conf)
    # processor = RemoveOutliersProcessor(conf)
    # processor.process(strain, housekeeping, temp_col)
    times = strain["Unix Timestamp [s]"].values
    values = strain["Raw [uint24]"].values
    output = kalman(
        time=times,
        values=values,
        x0=np.array([2**23, 1, 1]),
        p0=np.array([[100, 0, 0], [0, 200, 0], [0, 0, 300]]),
        env_uncertainty=np.array([[0, 0, 0], [0, 0, 0], [0, 0, 0]]),
        meas_uncertainty=np.array([[0, 0, 0], [0, 0, 0], [0, 0, 0]])
    )
    strain["KalmanRaw"] = output[:, 0]
    strain["KalmanDRaw"] = output[:, 1]
    strain["KalmanDDRaw"] = output[:, 2]
    print("Saving")
    strain.to_csv(out_strain_file, index=False)


def run(in_dir: str, out_dir: str, conf_name: str) -> None:
    """Runs the code.

    Args:
        in_dir (str): The input directory.
        out_dir (str): The output directory.
        conf (PowerMeterConfig): The config to use when processing.
    """
    # Create the output folder and copy relevant, but unused files to it.
    print("Creating the output directory and copying relevant files.")
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    shutil.copy2(f"{in_dir}/about.csv", f"{out_dir}/about.csv")
    shutil.copy2(f"{in_dir}/imu.csv", f"{out_dir}/imu.csv")
    shutil.copy2(f"{in_dir}/housekeeping.csv", f"{out_dir}/housekeeping.csv")
    shutil.copy2(conf_name, f"{out_dir}/{os.path.basename(conf_name)}")

    # Create a note to say that this has been processed.
    with open(f"{out_dir}/README.md", "w") as f:
        f.write("This dataset was reprocessed after the fact.")

    # Load the config ready for processing
    print(f"Loading the config from '{conf_name}'.")
    conf = PowerMeterConfig()
    conf.load_file(conf_name)
    # print(conf.left_strain.as_dict())
    # Process each side.
    housekeeping = pd.read_csv(f"{out_dir}/housekeeping.csv")

    def process_left_strain():
        process_strain(
            f"{in_dir}/left_strain.csv",
            "Left Temperature [C]",
            conf.left_strain,
            housekeeping,
            f"{out_dir}/left_strain.csv",
        )

    def process_right_strain():
        process_strain(
            f"{in_dir}/right_strain.csv",
            "Right Temperature [C]",
            conf.right_strain,
            housekeeping,
            f"{out_dir}/right_strain.csv",
        )

    # left_process = Process(target=process_left_strain)
    # right_process = Process(target=process_right_strain)
    # print("Starting strain processes")
    # left_process.start()
    # right_process.start()
    # left_process.join()
    # right_process.join()
    # print("Finished strain processes")
    # process_left_strain()
    process_right_strain()


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Takes the raw ADC readings and re-calculates torque and power. The IMU / Kalman filter data is not modified.",
        conflict_handler="resolve",  # Cope with -h being used for host like mosquitto clients
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="Written by Jotham Gates and Oscar Varney for MHP, 2024",
    )
    parser.add_argument(
        "-i",
        "--input",
        help="The folder containing the CSV files to input.",
        type=str,
        required=True,
    )
    parser.add_argument(
        "--cal-in",
        help="Calibration to use when processing the data",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Folder to create to contain the new CSV files.",
        type=str,
        default=None,
    )

    args = parser.parse_args()
    output = args.output
    if output is None:
        output = f"{args.input.rstrip('/')}_recalculated"
    run(args.input, output, args.cal_in)
