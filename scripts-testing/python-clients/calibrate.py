#!/usr/bin/env python3
"""calibrate.py
"""
import argparse
import pandas as pd
import numpy as np
from typing import Union, List
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.lines import Line2D
from sklearn.linear_model import LinearRegression

from common import none_empty_list, Side

GRAVITY = 9.81

def remove_nan(sheets:List[pd.DataFrame]) -> None:
    """Removes rows with invalid weights and readings.

    Args:
        sheets (List[pd.DataFrame]): _description_
    """
    for s in sheets:
        s.dropna(how="any", subset=["Weight", "Raw"], inplace=True)

def join_sheets_by_weight(sheets:List[pd.DataFrame], sheet_names:List[str]) -> pd.DataFrame:
    sheets_grouped: List[pd.Datzrame] = []
    for i, sheet in enumerate(sheets):
        # sheet["Sheet"] = sheet_names[i]
        sheets_grouped.append(sheet) # .groupby("Weight").mean(True))
        sheets_grouped[-1]["Sheet"] = sheet_names[i]

    concat = pd.concat(sheets_grouped)
    return concat

def load_sheets(file:str, sheet_args:Union[List[str], None]) -> List[pd.DataFrame]:
    sheets = [pd.read_excel(file, sheet) for sheet in none_empty_list(sheet_args)]
    remove_nan(sheets)
    return sheets

def linear_regress_side(sheet:pd.DataFrame) -> LinearRegression:
    forces = (sheet["Weight"].values * GRAVITY).reshape(-1, 1)
    return LinearRegression().fit(forces, sheet["Raw"].values.reshape(-1, 1))

def score_linear_regress(sheet:pd.DataFrame, reg:LinearRegression) -> float:
    forces = (sheet["Weight"].values * GRAVITY).reshape(-1, 1)
    return reg.score(forces, sheet["Raw"].values.reshape(-1, 1))

def plot_side_calibration(ax:Axes, sheets:List[pd.DataFrame], sheet_names:List[str], side:Side, max_weight:float):
    for i, sheet in enumerate(sheets):
        # Plot the raw data
        ax.scatter(sheet["Weight"].values * GRAVITY, sheet["Raw"].values, label=sheet_names[i])

        # Perform linear regression and display the results
        reg = linear_regress_side(sheet)
        x = np.array([0, max_weight*GRAVITY])
        y = reg.predict(x.reshape(-1, 1))
        ax.plot(x, y, ":", label=f"$f(x) = {reg.coef_[0][0]} x + {reg.intercept_[0]} , R^2={score_linear_regress(sheet, reg)}$")
    
    ax.set_ylabel("Reading from ADC")
    ax.set_title(f"{side.value.title()} side")
    ax.legend()

def plot_calibration(left_sheets:List[pd.DataFrame], left_sheet_names:List[str], right_sheets:List[pd.DataFrame], right_sheet_names:List[str], max_weight:float) -> None:
    fig = plt.figure()
    gs = fig.add_gridspec(2, height_ratios=[1, 1])
    ax_left, ax_right = gs.subplots(sharex=False)
    ax_left:Axes
    ax_right:Axes

    plot_side_calibration(ax_left, left_sheets, left_sheet_names, Side.LEFT, max_weight)
    plot_side_calibration(ax_right, right_sheets, right_sheet_names, Side.RIGHT, max_weight)
    ax_right.set_xlabel("Force applied [N]")
    ax_right.set_xlim(right=max_weight*GRAVITY)
    # plt.show()

def plot_raw_vs_temp_side(ax:Axes, sheets:List[pd.DataFrame], sheet_names:List[str], side:Side):
    joined = join_sheets_by_weight(sheets, sheet_names)
    for weight in joined["Weight"].unique():
        print(weight)
        # print(weight)
        selection = joined["Weight"] == weight
        if sum(selection) > 4:
            temps = joined[selection]["Temp"].values
            readings = joined[selection]["Raw"].values
            ax.scatter(temps, readings, label=f"{weight}kg")
    
    ax.set_ylabel("Reading from ADC")
    ax.set_title(f"{side.value.title()} side")
    ax.legend()

def plot_raw_vs_temp(left_sheets:List[pd.DataFrame], left_sheet_names:List[str], right_sheets:List[pd.DataFrame], right_sheet_names:List[str]) -> None:
    fig = plt.figure()
    gs = fig.add_gridspec(2, height_ratios=[1, 1])
    ax_left, ax_right = gs.subplots(sharex=True)
    ax_left:Axes
    ax_right:Axes

    plot_raw_vs_temp_side(ax_left, left_sheets, left_sheet_names, Side.LEFT)
    plot_raw_vs_temp_side(ax_right, right_sheets, right_sheet_names, Side.RIGHT)
    ax_right.set_xlabel("Temperature [°C]")
    # ax_right.set_xlim(rig)
    plt.show()

def get_average_temp(sheet:pd.DataFrame) -> float:
    """Gets the average temperature of a sheet.

    Args:
        sheet (pd.DataFrame): The sheet to find the average temperature of.

    Returns:
        float: The average temperature.
    """
    return sheet["Temp"].values.mean()

def plot_coefs_vs_temp(sheets:List[pd.DataFrame], sheet_names:List[str], side:Side):
    regs = [linear_regress_side(s) for s in sheets]
    temps = [get_average_temp(s) for s in sheets]

    fig = plt.figure()
    gs = fig.add_gridspec(2, height_ratios=[1, 1])
    ax_m, ax_c = gs.subplots(sharex=True)
    ax_m:Axes
    ax_c:Axes

    plt.title(f"{side.value.title()} side calibration constants vs temperature")
    ax_m.scatter(temps, [r.coef_[0][0] for r in regs])
    ax_m.set_ylabel("Gradient [raw/N]")
    ax_m.set_title("Gradient")
    ax_c.scatter(temps, [r.intercept_[0] for r in regs])
    ax_c.set_ylabel("Offset [raw]")
    ax_c.set_title("Offset")
    ax_c.set_xlabel("Temperature [°C]")


if __name__ == "__main__":
     # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Plots the weight vs calibration data and calculates the required coeficients and offsets.",
        conflict_handler="resolve",  # Cope with -h being used for host like mosquitto clients
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="Written by Jotham Gates and Oscar Varney for MHP, 2024",
    )
    parser.add_argument(
        "-i",
        "--input",
        help="The spreadsheet containing the calibration data.",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-l",
        "--left",
        help="The sheets in the spreadsheet containing the left data.",
        nargs="+",
        type=str
    )
    parser.add_argument(
        "-r",
        "--right",
        help="The sheets in the spreadsheet containing the right data.",
        nargs="+",
        type=str
    )
    parser.add_argument(
        "-m",
        "--max-weight",
        help="The maximum x value to show (weight in kg).",
        default=50,
        type=float
    )

    args = parser.parse_args()
    # pd.read_excel(args.input, sheet_name=
    left_sheet_names = none_empty_list(args.left)
    left_sheets = load_sheets(args.input, left_sheet_names)
    right_sheet_names = none_empty_list(args.right)
    right_sheets = load_sheets(args.input, args.right)
    plot_calibration(left_sheets, left_sheet_names, right_sheets, right_sheet_names, args.max_weight)
    # plot_raw_vs_temp(left_sheets, left_sheet_names, right_sheets, right_sheet_names)
    plot_coefs_vs_temp(left_sheets, left_sheet_names, Side.LEFT)
    # plot_coefs_vs_temp(right_sheets, right_sheet_names, Side.RIGHT)
    plt.show()