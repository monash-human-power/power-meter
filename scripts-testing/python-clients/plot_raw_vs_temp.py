#!/usr/bin/env python3
import argparse
import pandas as pd
import matplotlib.pyplot as plt

def add_temp_to_raw(raw_df:pd.DataFrame, housekeeping:pd.DataFrame, temp_col:str, title:str) -> None:
    result = pd.merge_asof(raw_df, housekeeping, "Unix Timestamp [s]")
    # print(result)
    # result.to_csv("Hello.csv")
    # plt.figure()
    plt.plot(result[temp_col].values, result["Raw [uint24]"].values)
    plt.xlabel("Temperature [C]")
    plt.ylabel("Raw value reported")
    plt.title(title)
    plt.legend()

if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Plots the raw dat vs temperature.",
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

    args = parser.parse_args()
    left_df = pd.read_csv(f"{args.input}/left_strain.csv")
    right_df = pd.read_csv(f"{args.input}/right_strain.csv")
    housekeeping_df = pd.read_csv(f"{args.input}/housekeeping.csv")
    add_temp_to_raw(left_df, housekeeping_df, "Left Temperature [C]", "Left offset vs temperature")
    add_temp_to_raw(right_df, housekeeping_df, "Right Temperature [C]", "Right offset vs temperature")
    plt.show()