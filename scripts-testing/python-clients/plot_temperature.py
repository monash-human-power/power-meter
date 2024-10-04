#!/usr/bin/env python3
import pandas as pd
import numpy as np
from typing import Tuple, Union
import matplotlib.pyplot as plt
import argparse

from common import add_time_args, apply_time_args

def remove_invalid_temps(housekeeping_df: pd.DataFrame) -> None:
    """Removes invalid temperatures from the data frame.

    Args:
        housekeeping_df (pd.DataFrame): The dataframe to remove from.
    """

    def remove_invalid_side_temp(side: str):
        housekeeping_df.loc[housekeeping_df[side] == -1000, side] = None

    remove_invalid_side_temp("Left Temperature [C]")
    remove_invalid_side_temp("Right Temperature [C]")
    remove_invalid_side_temp("IMU Temperature [C]")


def plot_housekeeping(housekeeping_df: pd.DataFrame, title: str) -> None:
    """Plots housekeeping data over time.

    Args:
        housekeeping_df (pd.DataFrame): Dataframe containing housekeeping data.
        title (str): The title to use.
    """
    # Create a figure with a raw subplot.
    fig = plt.figure()
    gs = fig.add_gridspec(1, height_ratios=[1])
    ax_temperature = gs.subplots(sharex=True)

    # Plot the raw values
    ax_temperature.plot(
        housekeeping_df["Unix Timestamp [s]"].values,
        housekeeping_df["Left Temperature [C]"].values,
        label="Left side",
    )
    ax_temperature.plot(
        housekeeping_df["Unix Timestamp [s]"].values,
        housekeeping_df["Right Temperature [C]"].values,
        label="Right side",
    )
    ax_temperature.plot(
        housekeeping_df["Unix Timestamp [s]"].values,
        housekeeping_df["IMU Temperature [C]"].values,
        label="IMU (right side under MCU)"
    )
    # ax_temperature.set_ylabel("Raw values")
    ax_temperature.set_title("Temperature on each side")
    # ax_raw.grid()
    ax_temperature.set_xlabel("Time [s]")
    ax_temperature.legend()

    plt.suptitle(title)
    plt.show()


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Plots Strain gauge-related timeseries data.",
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
        default="Temperature timeseries data",
    )
    add_time_args(parser)
    args = parser.parse_args()
    housekeeping_df = pd.read_csv(f"{args.input}/housekeeping.csv")
    remove_invalid_temps(housekeeping_df)

    # Adjust time displayed
    if len(housekeeping_df) == 0:
        print("No housekeeping data present in the file. Please use a different set of data.")
    else:
        housekeeping_df = apply_time_args(housekeeping_df, args)
        plot_housekeeping(housekeeping_df, args.title)
