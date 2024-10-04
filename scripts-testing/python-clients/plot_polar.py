#!/usr/bin/env python3
"""plot_polar.py
usage: plot_polar.py [-h] -i INPUT [-g GLOBAL_OFFSET] [--start START] [--stop STOP] [-d]

Plots polar graphs showing instantaneous cadence, torque and power. This uses the same graphing functions as the live graphs in the logger.

options:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The folder containing the CSV files. (default: None)

Time:
  Parameters relating to time offsets and limiting the time periods plotted.

  -g GLOBAL_OFFSET, --global-offset GLOBAL_OFFSET
                        Global time offset in seconds. This is mainly for making the x axis look nicer. The start and stop times also depend on this.
                        (default: 0)
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

from common import (
    add_time_args,
    apply_time_args,
    apply_device_time_corrections,
    IMULiveChart,
    velocity_to_cadence,
    TorqueLiveChart,
    PowerLiveChart,
)


def plot_imu(df: pd.DataFrame) -> None:
    cadence = velocity_to_cadence(df["Velocity [rad/s]"].values)
    IMULiveChart(None, df["Position [rad]"].values, cadence, False)


def plot_torque(left_df: pd.DataFrame, right_df: pd.DataFrame) -> None:
    TorqueLiveChart(
        None,
        left_df["Position [rad]"].values,
        left_df["Torque [Nm]"].values,
        right_df["Position [rad]"].values,
        right_df["Torque [Nm]"].values,
        False,
    )


def plot_power(left_df: pd.DataFrame, right_df: pd.DataFrame) -> None:
    PowerLiveChart(
        None,
        left_df["Position [rad]"].values,
        left_df["Power [W]"].values,
        right_df["Position [rad]"].values,
        right_df["Power [W]"].values,
        False,
    )


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Plots polar graphs showing instantaneous cadence, torque and power. This uses the same graphing functions as the live graphs in the logger.",
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
    add_time_args(parser, add_device_compensate=True)
    args = parser.parse_args()

    # Load and process the data frames
    left_strain_df = pd.read_csv(f"{args.input}/left_strain.csv")
    if len(left_strain_df):
        left_strain_df = apply_time_args(left_strain_df, args)
        # apply_device_time_corrections(left_strain_df, args)

    right_strain_df = pd.read_csv(f"{args.input}/right_strain.csv")
    if len(right_strain_df):
        right_strain_df = apply_time_args(right_strain_df, args)
        # apply_device_time_corrections(right_strain_df, args)

    plot_torque(left_strain_df, right_strain_df)
    plot_power(left_strain_df, right_strain_df)

    imu_df = pd.read_csv(f"{args.input}/imu.csv")
    if len(imu_df):
        imu_df = apply_time_args(imu_df, args)
        # apply_device_time_corrections(imu_df, args)
        plot_imu(imu_df)

    # Show all figures
    plt.show()
