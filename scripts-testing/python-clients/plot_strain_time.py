#!/usr/bin/env python3
"""plot_strain_time.py
usage: plot_strain_time.py [-h] -i INPUT [-t TITLE] [--no-raw] [-c] [-l] [--local-calibration] [-g GLOBAL_OFFSET] [--start START] [--stop STOP] [-d]

Plots Strain gauge-related timeseries data.

options:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The folder containing the CSV files. (default: None)
  -t TITLE, --title TITLE
                        Title to put on the figure (default: Strain Timeseries data)
  --no-raw              Hides the raw valued subplot (default: True)
  -c, --compensate      If provided, uses the first reading for each side to offset compensate the raw values. (default: False)
  -l, --raw-limits      If provided, shows 0, (2^24)-1 and (2^23)-1 on the raw plot. (default: False)
  --local-calibration   If provided, calculates the torques using the raw values. If not provided, uses the torque provided by the power meter. (default: False)

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


def left_raw_to_nm(raw: np.ndarray) -> np.ndarray:
    """Converts raw values to kg.

    Args:
        raw (np.ndarray): The raw inputs

    Returns:
        np.ndarray: Outputs scaled to kg
    """
    # Linear-relationship.
    # m = -3532.9388551007 # Old
    m = -3189.14464910197
    # c = 9958806.90678679 # Old
    c = 0
    coef = 10 * 0.13 / m
    print(f"Left coefficient: {coef}")
    return (raw - c) * coef


def right_raw_to_nm(raw: np.ndarray) -> np.ndarray:
    """Converts raw values to kg.

    Args:
        raw (np.ndarray): The raw inputs

    Returns:
        np.ndarray: Outputs scaled to kg
    """
    # Linear-relationship.
    # m = 5786.5965078533 # Old
    m = 5549.22937585228
    # c = 6275241.9683 # Old
    c = 0
    coef = 10 * 0.13 / m
    print(f"Right coefficient: {coef}")
    return (raw - c) * coef


def plot_strain(
    left_df: pd.DataFrame,
    right_df: pd.DataFrame,
    title: str,
    show_raw: bool,
    use_start_compensate: bool,
    show_raw_limits: bool,
    local_calibration: bool,
) -> None:
    """Plots strain over time.

    Args:
        left_df (pd.DataFrame): Left data
        right_df (pd.DataFrame): Right data
        title (str): The title to use.
    """
    # Strategically place the dodgy (right) side at the back and adjust the colours to match the other graphs
    colour_cycle = plt.rcParams['axes.prop_cycle'].by_key()['color']

    if use_start_compensate:
        # Use the first reading for offset compensation if requested.
        if len(left_df) > 0:
            left_df["Raw [uint24]"] -= left_df["Raw [uint24]"][0]

        if len(right_df) > 0:
            right_df["Raw [uint24]"] -= right_df["Raw [uint24]"][0]

    if show_raw:
        # Create a figure with a raw subplot.
        fig = plt.figure()
        gs = fig.add_gridspec(2, height_ratios=[0.5, 1])
        ax_raw, ax_weight = gs.subplots(sharex=True)

        # Plot the raw limits if desired
        if show_raw_limits:
            ax_raw.axhline(y=0, color="r", linestyle="dotted")
            ax_raw.axhline(y=(2**24) - 1, color="r", linestyle="dotted")
            ax_raw.axhline(y=(2**23) - 1, color="r", linestyle="dotted")

        # Plot the raw values
        if len(left_df):
            ax_raw.plot(
                left_df["Time"].values, left_df["Raw [uint24]"].values, color=colour_cycle[0], label="Left side"
            )
        
        if len(right_df):
            ax_raw.plot(
                right_df["Time"].values, right_df["Raw [uint24]"].values, color=colour_cycle[1], label="Right side"
            )
        ax_raw.set_ylabel("Raw values")
        ax_raw.set_title("Raw readings")
        ax_raw.grid()
        ax_raw.legend()
    else:
        # Create a figure without a raw subplot.
        fig = plt.figure()
        gs = fig.add_gridspec(1, height_ratios=[1])
        ax_weight = gs.subplots(sharex=True)

    # Plot the corrected weights
    if local_calibration:
        left_weight = left_raw_to_nm(left_df["Raw [uint24]"].values)
        right_weight = right_raw_to_nm(right_df["Raw [uint24]"].values)
    else:
        left_weight = left_df["Torque [Nm]"].values
        right_weight = right_df["Torque [Nm]"].values
    
    if len(right_df):
        ax_weight.plot(right_df["Time"].values, right_weight, color=colour_cycle[1], label="Right side")
    
    if len(left_df):
        ax_weight.plot(left_df["Time"].values, left_weight, color=colour_cycle[0], label="Left side")
    ax_weight.set_ylabel("Torque [Nm]")
    ax_weight.grid()

    # Check whether to show the legend if we have data for both sides
    if len(left_df) != 0 and len(right_df) != 0:
        # Data for both sides
        ax_weight.set_title("Calibrated torque for both sides")
        if not show_raw:
            ax_weight.legend()
    elif len(left_df) != 0:
        # Left data only
        ax_weight.set_title("Calibrated torque on the left side")
    else:
        # Right data only
        ax_weight.set_title("Calibrated torque on the right side")

    ax_weight.set_xlabel("Time [s]")

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
        default="Strain Timeseries data",
    )
    parser.add_argument(
        "--no-raw", help="Hides the raw valued subplot", action="store_false"
    )
    parser.add_argument(
        "-c",
        "--compensate",
        help="If provided, uses the first reading for each side to offset compensate the raw values.",
        action="store_true",
    )
    parser.add_argument(
        "-l",
        "--raw-limits",
        help="If provided, shows 0, (2^24)-1 and (2^23)-1 on the raw plot.",
        action="store_true",
    )
    parser.add_argument(
        "--local-calibration",
        help="If provided, calculates the torques using the raw values. If not provided, uses the torque provided by the power meter.",
        action="store_true",
    )
    add_time_args(parser, add_device_compensate=True)
    args = parser.parse_args()

    # Load and process the data frames
    left_strain_df = pd.read_csv(f"{args.input}/left_strain.csv")
    if len(left_strain_df):
        left_strain_df = apply_time_args(left_strain_df, args)
        apply_device_time_corrections(left_strain_df, args)

    right_strain_df = pd.read_csv(f"{args.input}/right_strain.csv")
    if len(right_strain_df):
        right_strain_df = apply_time_args(right_strain_df, args)
        apply_device_time_corrections(right_strain_df, args)

    # Plot
    plot_strain(
        left_strain_df,
        right_strain_df,
        args.title,
        args.no_raw,
        args.compensate,
        args.raw_limits,
        args.local_calibration,
    )
