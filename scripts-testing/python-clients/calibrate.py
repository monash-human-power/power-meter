#!/usr/bin/env python3
"""calibrate.py
"""
import argparse
import pandas as pd
import numpy as np
from typing import Tuple, Union, List
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.lines import Line2D
from sklearn.linear_model import LinearRegression

from common import none_empty_list, Side

GRAVITY = 9.81
CRANK_LENGTH = 0.13

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

def weight_to_torque(weight:np.ndarray) -> np.ndarray:
    """Converts the given weight to a torque.

    Args:
        weight (np.ndarray): The mass applied in kg (assuming G = 9.81ms^-2)

    Returns:
        np.ndarray: Torque in Nm
    """
    force = weight * GRAVITY
    torque = force * CRANK_LENGTH
    return torque

def linear_regress_side(sheet:pd.DataFrame) -> LinearRegression:
    forces = (weight_to_torque(sheet["Weight"].values)).reshape(-1, 1)
    return LinearRegression().fit(forces, sheet["Raw"].values.reshape(-1, 1))

def score_linear_regress(sheet:pd.DataFrame, reg:LinearRegression) -> float:
    forces = (weight_to_torque(sheet["Weight"].values)).reshape(-1, 1)
    return reg.score(forces, sheet["Raw"].values.reshape(-1, 1))

def separate_scientific(num:float) -> Tuple[float, int]:
    # https://stackoverflow.com/a/13490731
    exp = int(np.log10(abs(num)))
    return num / (10**exp), exp

def format_scientific(num:float) -> str:
    # return '{0}^{{{1:+03}}}'.format(*separate_scientific(-1.234e9))
    mantissa, exp = separate_scientific(num)
    return f"{mantissa:.2f} \\times 10^{{{exp}}}"

def plot_side_calibration(row:List[Axes], sheets:List[pd.DataFrame], sheet_names:List[str], side:Side, max_weight:float):
    # ax, lax = row
    ax = row
    for i, sheet in enumerate(sheets):
        # Perform linear regression
        reg = linear_regress_side(sheet)
        # reg_label = f"$f(x) = {reg.coef_[0][0]:.0f} x + {format_scientific(reg.intercept_[0])} , R^2={score_linear_regress(sheet, reg):.3f}$"
        reg_label = None
        # Plot the raw data
        # Date
        #label = f"{get_average_temp(sheet):.1f}°C measured ({sheet_names[i][6:8]}/{sheet_names[i][4:6]}/{sheet_names[i][0:4]})"
        # Temperature only
        label = f"${get_average_temp(sheet):.1f}^\circ C$, $R^2={score_linear_regress(sheet, reg):.3f}$"

        ax.scatter(weight_to_torque(sheet["Weight"].values), sheet["Raw"].values, label=label)

        # Perform linear regression and display the results
        x = np.array([0, weight_to_torque(max_weight)])
        y = reg.predict(x.reshape(-1, 1))
        ax.plot(x, y, ":", label=reg_label)
    
    ax.set_ylabel("Reading from ADC")
    ax.set_title(f"{side.value.title()} side")

    # Set the legend on the right with its own axis
    ax.legend()
    # h, l = ax.get_legend_handles_labels()
    # lax.legend(h, l, borderaxespad=0, frameon=False)
    # lax.axis("off")

def plot_calibration(left_sheets:List[pd.DataFrame], left_sheet_names:List[str], right_sheets:List[pd.DataFrame], right_sheet_names:List[str], max_weight:float) -> None:
    fig = plt.figure()
    # gs = fig.add_gridspec(2, 2, height_ratios=[1, 1], width_ratios=[1, 1])
    # row_left, row_right = gs.subplots(sharex=True)
    # row_left:List[Axes]
    # row_right:List[Axes]
    # ax_right = row_right[0]

    gs = fig.add_gridspec(2, 1)
    row_left, row_right = gs.subplots(sharex=True)
    row_left:List[Axes]
    row_right:List[Axes]
    ax_right = row_right

    plot_side_calibration(row_left, left_sheets, left_sheet_names, Side.LEFT, max_weight)
    plot_side_calibration(row_right, right_sheets, right_sheet_names, Side.RIGHT, max_weight)
    
    ax_right.set_xlabel("Torque applied [Nm]")
    ax_right.set_xlim(right=weight_to_torque(max_weight))
    plt.suptitle("Reported ADC measuremets vs applied torque during calibration")
    plt.tight_layout()
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