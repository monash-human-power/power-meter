"""common.py
Utility functions common between several scripts."""

import argparse
import pandas as pd
import numpy as np
from typing import Union


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
        parser.add_argument(
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
    steps = np.diff(offsets.values, prepend=[0])
    step_starts = first_in_batch[(steps > 10)]

    # Apply the offset for each step start
    for i in range(1, len(step_starts)):
        # Calculate the time range to apply and the offset
        start_time = step_starts.iloc[i - 1].name
        offset = start_time - step_starts.iloc[i - 1]["Device Timestamp [us]"] * 1e-6

        # Apply the offset
        stop_time = step_starts.iloc[i].name
        selection = (df["Unix Timestamp [s]"] >= start_time) & (
            df["Unix Timestamp [s]"] < stop_time
        )
        df.loc[selection, "Calculated Time [s]"] = (
            df[selection]["Device Timestamp [us]"] * 1e-6 + offset
        )

    # Apply from the last iteration to the end of all samples
    start_time = stop_time
    offset = (
        start_time
        - step_starts.iloc[len(step_starts) - 1]["Device Timestamp [us]"] * 1e-6
    )
    selection = df["Unix Timestamp [s]"] >= start_time
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

def apply_time_args(df:pd.DataFrame, args:argparse.Namespace) -> pd.DataFrame:
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

def apply_device_time_corrections(df:pd.DataFrame, args:argparse.Namespace) -> None:
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