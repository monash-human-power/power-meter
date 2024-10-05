"""common.py
Utility functions common between several scripts."""

import argparse
import pandas as pd
import numpy as np
from typing import Union, List
from abc import ABC, abstractmethod
from enum import Enum
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.lines import Line2D
from multiprocessing import Queue
from typing import TypeVar, Tuple
from dataclasses import dataclass
import struct

T = TypeVar("U")


def add_time_args(
    parser: argparse.ArgumentParser, add_device_compensate: bool = False
) -> argparse._ArgumentGroup:
    """Adds time options to the arguments

    Args:
        args (argparse.ArgumentParser): Argparser to add to.
        add_device_compensate (bool): Compensates for the device time reseting (uses unix time of each batch being received as a guide). Call convert_unix_time to perform the compensation.

    Returns:
        argparse._ArgumentGroup: Group containing the time arguments (so more stuff can be added to it if needed).
    """
    time_group = parser.add_argument_group(
        "Time",
        "Parameters relating to time offsets and limiting the time periods plotted.",
    )
    time_group.add_argument(
        "-g",
        "--global-offset",
        help="Global time offset in seconds. This is mainly for making the x axis look nicer. The start and stop times also depend on this.",
        type=float,
        default=0,
    )
    time_group.add_argument(
        "--start",
        help="The minimum time to show (after all offsets are applied).",
        type=float,
        default=None,
    )
    time_group.add_argument(
        "--stop",
        help="The maximum time to show (after all offsets are applied).",
        type=float,
        default=None,
    )
    if add_device_compensate:
        time_group.add_argument(
            "-d",
            "--device-time",
            help="Does not correct for the device time potentially being reset.",
            action="store_false",
        )
    return time_group


def convert_to_unix_time(df: pd.DataFrame) -> None:
    """Correctly assigns a unix timestamp to each record based off the unix timestamp when the first message was received and the device timestamp.

    This correctly handles the device being reset / overflow.

    Args:
        df (pd.DataFrame): The dataframe containing "Unix Timestamp [s]" and "Device Timestamp [us]" columns. A column "Calculated Time [s]" will be created with the result.
    """
    # Calculate the offsets between units.
    first_in_batch = df.groupby("Unix Timestamp [s]").first()
    offsets = first_in_batch.index - first_in_batch["Device Timestamp [us]"] * 1e-6

    # Work out times that were reset.
    steps = np.diff(offsets.values, prepend=[-np.inf])
    step_starts = first_in_batch[(steps > 10)]

    # Apply the offset for each step start
    for i in range(len(step_starts)):
        # Calculate the time range to apply and the offset
        start_time = step_starts.iloc[i].name
        offset = start_time - step_starts.iloc[i]["Device Timestamp [us]"] * 1e-6

        # Apply the offset
        if i < len(step_starts) - 1:
            # Apply to the next restart
            stop_time = step_starts.iloc[i + 1].name
            selection = (df["Unix Timestamp [s]"] >= start_time) & (
                df["Unix Timestamp [s]"] < stop_time
            )
        else:
            # Apply to the end of the data.
            selection = df["Unix Timestamp [s]"] >= start_time

        # Apply
        df.loc[selection, "Calculated Time [s]"] = (
            df[selection]["Device Timestamp [us]"] * 1e-6 + offset
        )


def truncate_times(
    df: pd.DataFrame, start_time: Union[float, None], stop_time: Union[float, None]
) -> pd.DataFrame:
    """Removes times outside the given range.

    Args:
        df (pd.DataFrame): Input dataframe.
        start_time (Union[float, None]): Start time.
        stop_time (Union[float, None]): Stop time.

    Returns:
        pd.DataFrame: Adjusted dataframe.
    """
    if start_time is not None:
        df = df.drop(df[df["Unix Timestamp [s]"] < start_time].index)

    if stop_time is not None:
        df = df.drop(df[df["Unix Timestamp [s]"] > stop_time].index)

    return df


def apply_time_args(df: pd.DataFrame, args: argparse.Namespace) -> pd.DataFrame:
    """Applies time arguments to a dataframe, filtering using "Unix Timestamp [s]".

    Args:
        df (pd.DataFrame): _description_
        args (argparse.Namespace): _description_

    Returns:
        pd.DataFrame: _description_
    """
    df["Unix Timestamp [s]"] += args.global_offset - df["Unix Timestamp [s]"][0]
    df = truncate_times(df, args.start, args.stop)
    return df


def apply_device_time_corrections(df: pd.DataFrame, args: argparse.Namespace) -> None:
    """Applies device timestamp corrections if requruired, otherwise copies the given device timestamp without correction.

    Args:
        df (pd.DataFrame): The outputs are placed in the Time column.
        args (argparse.Namespace): _description_
    """
    # Convert the times to nicer units.
    if args.device_time:
        # Correct for the device being reset.
        convert_to_unix_time(df)
        df["Time"] = df["Calculated Time [s]"]
    else:
        # No corrections, naively use device time (could be reset to 0).
        df["Time"] = df["Device Timestamp [us]"] * 1e-6


def velocity_to_cadence(av: float) -> float:
    """Converts the angular velocity to cadence.

    Args:
        av (float): The angular velocity in radians per second.

    Returns:
        float: Cadence in RPM.
    """
    rad_per_min = av * 60
    rpm = rad_per_min / (2 * np.pi)
    return rpm


class Side(Enum):
    LEFT = "left"
    RIGHT = "right"


class IMUData:
    """Class for storing and processing data from the IMU."""

    SIZE = 36

    def __init__(self, data: bytes) -> None:
        """Initialises the object.

        Args:
            data (bytes): The raw data (trimmed to just that containing this object).
        """
        (
            self.timestamp,
            self.velocity,
            self.position,
            self.accel_x,
            self.accel_y,
            self.accel_z,
            self.gyro_x,
            self.gyro_y,
            self.gyro_z,
        ) = struct.unpack("<Lffffffff", data)

    def cadence(self) -> float:
        """Calculates the current cadence from the velocity.

        Returns:
            float: The cadence in RPM.
        """
        return abs(self.velocity) / (2 * np.pi) * 60

    def __str__(self) -> str:
        return f"{self.timestamp:>10d}: {self.velocity:>8.2f}rad/s {self.position:>8.1f}rad [{self.accel_x:>8.2f}, {self.accel_y:>8.2f}, {self.accel_z:>8.2f}]m/s [{self.gyro_x:>8.2f}, {self.gyro_y:>8.2f}, {self.gyro_z:>8.2f}]rad/s"


class StrainData:
    """Class for storing and processing data from a strain gauge"""

    SIZE = 24

    def __init__(self, data: bytes) -> None:
        """Initialises the object.

        Args:
            data (bytes): The raw data (trimmed to just that containing this object).
        """
        (
            self.timestamp,
            self.velocity,
            self.position,
            self.raw,
            self.torque,
            self.power,
        ) = struct.unpack("<LffLff", data)

    def __str__(self) -> str:
        return f"{self.timestamp:>10d}: {self.velocity:>8.2f}rad/s {self.position:>8.1f}rad {self.raw:>11d}raw {self.torque:>8.2f}Nm {self.power:>8.2f}W"


class LiveChart(ABC):
    def __init__(
        self, fig: Figure, ax: Axes, max_history: int = None, title: str = ""
    ) -> None:
        """Initialises the figure and sets it up to call update.

        Args:
            max_history (int, optional): The maximum history to show. If None, shows all history. Defaults to None.
            ymax (int, optional): The maximum y value. Defaults to None.
            title (str, optional): Title to use. Defaults to "".
        """
        self.max_history = max_history
        fig.suptitle(title)
        ax.set_title("")
        fig.tight_layout()
        self.fig, self.ax = fig, ax
        # Start the process to show the graph.
        self.queue = Queue()
        self.title_queue = Queue()

    @abstractmethod
    def update(self, data: T) -> Tuple[Line2D]:
        """Updates the graph.

        Args:
            data (T): The data to add (or list of data points).
        """
        pass

    def add_data(self, data: T) -> None:
        """Adds data to be passed to the update method eventually."""
        self.queue.put(data)

    def setup_animation(self) -> None:
        self.ani = animation.FuncAnimation(
            fig=self.fig, func=self.update_graph, interval=80, cache_frame_data=False
        )

    def update_graph(self, frame: int) -> Tuple[Line2D]:
        """Gets the latest data from the queue and calls update"""
        # Update the title as needed
        if not self.title_queue.empty():
            self.ax.set_title(self.title_queue.get())

        # Update the lines
        if not self.queue.empty():
            data = self.queue.get()
            return self.update(data)
        else:
            return None

    def limit_length(self, input: list) -> list:
        """Limits the length of a list to the given max history.

        Args:
            input (list): The input data.

        Returns:
            list: The latest with old data removed.
        """
        if self.max_history and len(input) > self.max_history:
            return input[len(input) - self.max_history :]
        else:
            return input

    def update_title(self, new_title: str) -> None:
        """Adds the new title to the queue."""
        self.title_queue.put(new_title)


class PolarLiveChart(LiveChart):
    def __init__(
        self, max_history: int, title: str, ymax: int, show_current_angle: bool = True
    ) -> None:
        """Parent class for polar charts.

        Args:
            max_history (int): Maximum history to show.
            title (str): Title to use.
            ymax (int): Maximum Y value.
            show_current_angle (bool, optional): Add a red line showing the current position. Defaults to True.
        """
        fig, ax = plt.subplots(subplot_kw={"projection": "polar"})
        ax.set_ylim(0, ymax)
        if show_current_angle:
            self.latest = ax.axvline(0, color="r")
        super().__init__(fig, ax, max_history, title)


class IMULiveChart(PolarLiveChart):
    """Live chart for the IMU."""

    def __init__(
        self,
        max_history: int = None,
        initial_theta: np.ndarray = [],
        initial_cadence: np.ndarray = [],
        show_current_angle: bool = True,
    ):
        self.theta = []
        self.cadence = []
        super().__init__(
            max_history,
            "Cadence [rpm] vs pedal angle [$^\circ$]",
            160,
            show_current_angle,
        )
        self.line = self.ax.plot(initial_theta, initial_cadence)[0]

    def update(self, converted: T) -> Tuple[Line2D]:
        for conv in converted:
            self.theta.append(conv.position)
            self.cadence.append(conv.cadence())

        # Remove old data
        self.cadence = self.limit_length(self.cadence)
        self.theta = self.limit_length(self.theta)

        # Update the data
        self.line.set_xdata(self.theta)
        self.line.set_ydata(self.cadence)
        self.latest.set_xdata([self.theta[-1]])

        return self.line, self.latest

    def update_cadence_subtitle(self, cadence: float) -> None:
        self.update_title(f"Currently ${cadence:>.1f}$ rpm average")


@dataclass
class SideDataPair:
    side: Side
    data: List[StrainData]


class TorqueLiveChart(PolarLiveChart):
    """Live chart for the torque."""

    def __init__(
        self,
        max_history: int = None,
        initial_left_theta: np.ndarray = [],
        initial_left_torque: np.ndarray = [],
        initial_right_theta: np.ndarray = [],
        initial_right_torque: np.ndarray = [],
        show_current_angle: bool = True,
    ):
        # Lists to hold data
        self.thetas = {Side.LEFT: [], Side.RIGHT: []}
        self.torques = {Side.LEFT: [], Side.RIGHT: []}

        # Initialise the graph.
        super().__init__(
            max_history, "Torque [Nm] vs pedal angle [$^\circ$]", 80, show_current_angle
        )
        self.lines = {
            Side.LEFT: self.ax.plot(initial_left_theta, initial_left_torque)[0],
            Side.RIGHT: self.ax.plot(initial_right_theta, initial_right_torque)[0],
        }
        # self.points = {
        #     Side.LEFT: self.ax.plot([], [], marker=".", markersize=10)[0],
        #     Side.RIGHT: self.ax.plot([], [], marker=".", markersize=10)[0]
        # }

    def update(self, converted: T) -> Tuple[Line2D]:
        # Extract the data
        side: Side = converted.side
        data: List[StrainData] = converted.data

        # Append to list.
        theta_list = self.thetas[side]
        torque_list = self.torques[side]
        for conv in data:
            theta_list.append(conv.position)
            torque_list.append(conv.torque)

        # Remove old data
        self.thetas[side] = self.limit_length(self.thetas[side])
        self.torques[side] = self.limit_length(self.torques[side])

        # Update the data
        self.lines[side].set_xdata(self.thetas[side])
        self.lines[side].set_ydata(self.torques[side])
        # self.points[side].set_xdata([self.thetas[side][-1]])
        # self.points[side].set_ydata([self.torques[side][-1]])
        self.latest.set_xdata([self.thetas[side][-1]])

        return self.lines[Side.LEFT], self.lines[Side.RIGHT], self.latest
        # return self.lines[Side.LEFT], self.lines[Side.RIGHT], self.latest, self.points[Side.LEFT], self.points[Side.RIGHT]


class PowerLiveChart(PolarLiveChart):
    """Live chart for the power."""

    def __init__(
        self,
        max_history: int = None,
        initial_left_theta: np.ndarray = [],
        initial_left_power: np.ndarray = [],
        initial_right_theta: np.ndarray = [],
        initial_right_power: np.ndarray = [],
        show_current_angle: bool = True,
    ):
        # Lists to hold data
        self.thetas = {Side.LEFT: [], Side.RIGHT: []}
        self.powers = {Side.LEFT: [], Side.RIGHT: []}

        # Initialise the graph.
        super().__init__(
            max_history,
            "Instantaneous power [W] vs pedal angle [$^\circ$]",
            800,
            show_current_angle,
        )
        self.lines = {
            Side.LEFT: self.ax.plot(
                initial_left_theta, initial_left_power, label="Left"
            )[0],
            Side.RIGHT: self.ax.plot(
                initial_right_theta, initial_right_power, label="Right"
            )[0],
        }
        self.ax.legend()

    def update(self, converted: T) -> Tuple[Line2D]:
        # Extract the data
        side: Side = converted.side
        data: List[StrainData] = converted.data

        # Append to list.
        theta_list = self.thetas[side]
        power_list = self.powers[side]
        for conv in data:
            theta_list.append(conv.position)
            power_list.append(conv.power)

        # Remove old data
        self.thetas[side] = self.limit_length(self.thetas[side])
        self.powers[side] = self.limit_length(self.powers[side])

        # Update the data
        self.lines[side].set_xdata(self.thetas[side])
        self.lines[side].set_ydata(self.powers[side])
        self.latest.set_xdata([self.thetas[side][-1]])

        return self.lines[Side.LEFT], self.lines[Side.RIGHT], self.latest

    def update_power_subtitle(self, power: float, balance: float) -> None:
        self.update_title(
            f"${power:.0f}$ W, ${balance:.0f}$ % balance over the last rotation"
        )
