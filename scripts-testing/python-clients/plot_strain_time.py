#!/usr/bin/env python3
"""plot_strain_time.py
usage: plot_strain_time.py [-h] -i INPUT [-t TITLE] [--no-raw]

Plots Strain gauge-related timeseries data.

options:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The folder containing the CSV files. (default: None)
  -t TITLE, --title TITLE
                        Title to put on the figure (default: Strain Timeseries data)
  --no-raw              Hides the raw valued subplot (default: True)

Written by Jotham Gates and Oscar Varney for MHP, 2024
"""
import pandas as pd
import numpy as np
from typing import Tuple
import matplotlib.pyplot as plt
import argparse

def left_raw_to_kg(raw:np.ndarray) -> np.ndarray:
    """Converts raw values to kg.

    Args:
        raw (np.ndarray): The raw inputs

    Returns:
        np.ndarray: Outputs scaled to kg
    """
    # Samples from testing.
    raw1, weight1 = 16620474, 0
    raw2, weight2 = 16730324, 38.23

    # Linear-relationship.
    m = (weight2 - weight1) / (raw2 - raw1)
    weight = m*(raw - raw1) + weight1
    return weight

def right_raw_to_kg(raw:np.ndarray) -> np.ndarray:
    """Converts raw values to kg.

    Args:
        raw (np.ndarray): The raw inputs

    Returns:
        np.ndarray: Outputs scaled to kg
    """
    # TODO: Actually measure / calibrate
    print("WARNING: Right side has not been calibrated")
    # Samples from testing.
    raw1, weight1 = 16620474, 0
    raw2, weight2 = 16730324, 38.23

    # Linear-relationship.
    m = (weight2 - weight1) / (raw2 - raw1)
    weight = m*(raw - raw1) + weight1
    return weight

def plot_strain(left_df:pd.DataFrame, right_df:pd.DataFrame, title:str, show_raw=True) -> None:
    """Plots strain over time.

    Args:
        left_df (pd.DataFrame): Left data
        right_df (pd.DataFrame): Right data
        title (str): The title to use.
    """

    # Convert the times to nicer units.
    left_df["Time"] = left_df["Timestamp [us]"]*1e-6
    right_df["Time"] = right_df["Timestamp [us]"]*1e-6

    if show_raw:
        # Create a figure with a raw subplot.
        fig = plt.figure()
        gs = fig.add_gridspec(2, height_ratios=[0.5, 1])
        ax_raw, ax_weight = gs.subplots(sharex=True)

        # Plot the raw values
        ax_raw.plot(left_df["Time"].values, left_df["Raw [uint24]"].values, label="Left side")
        ax_raw.plot(right_df["Time"].values, right_df["Raw [uint24]"].values, label="Right side")
        ax_raw.set_ylabel("Raw values")
        ax_raw.set_title("Raw readings")
        ax_raw.grid()
    else:
        # Create a figure without a raw subplot.
        fig = plt.figure()
        gs = fig.add_gridspec(1, height_ratios=[1])
        ax_weight = gs.subplots(sharex=True)

    # Plot the corrected weights
    left_weight = left_raw_to_kg(left_df["Raw [uint24]"].values)
    right_weight = right_raw_to_kg(right_df["Raw [uint24]"].values)
    ax_weight.plot(left_df["Time"].values, left_weight, label="Left side")
    ax_weight.plot(right_df["Time"].values, right_weight, label="Right side")
    ax_weight.set_ylabel("Weight [kg]")
    ax_weight.grid()

    # Check whether to show the legend if we have data for both sides
    if len(left_df) != 0 and len(right_df) != 0:
        # Data for both sides
        ax_weight.set_title("Calibrated weights for both sides")
        ax_weight.legend()
    elif len(left_df) != 0:
        # Left data only
        ax_weight.set_title("Calibrated weights on the left side")
    else:
        # Right data only
        ax_weight.set_title("Calibrated weights on the right side")

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
        "--no-raw",
        help="Hides the raw valued subplot",
        action="store_false"
    )

    args = parser.parse_args()
    left_strain_df = pd.read_csv(f"{args.input}/left_strain.csv")
    right_strain_df = pd.read_csv(f"{args.input}/right_strain.csv")
    plot_strain(left_strain_df, right_strain_df, args.title, args.no_raw)