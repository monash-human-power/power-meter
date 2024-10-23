#!/usr/bin/env python3
"""recalculate.py
"""

import argparse
import shutil
import os
import pandas as pd
from abc import ABC, abstractmethod
from collections import deque

from common import PowerMeterConfig, StrainConfig

class StrainProcessor(ABC):
    """Base class for processing strain data into torque."""
    def __init__(self, conf:StrainConfig) -> None:
        """Initialises the processor.

        Args:
            conf (StrainConfig): The config and calibration data to use.
        """
        self.conf = conf
        self.cur_temp = 0
    
    def add_temp(self, temperature:float) -> None:
        """Adds the current temperature

        Args:
            temperature (float): The current temperature for the side in celcius
        """
        self.cur_temp = temperature

    @abstractmethod
    def step(self, data:pd.DataFrame) -> float:
        """Takes the input strain data and calculates the torque.

        Args:
            data (pd.DataFrame): The data containing the position, speed and raw values.

        Returns:
            float: The torque in Nm.
        """
        pass

    def process(self, strain:pd.DataFrame, housekeeping:pd.DataFrame, temp_col:str) -> None:
        # Add the first temperature reading to start with.
        housekeeping_index = 0
        self.add_temp(housekeeping.iloc[housekeeping_index][temp_col])
        housekeeping_index += 1

        # Iterate over all fields and process it.
        for index, row in strain.iterrows():
            # Check if the next housekeeping data is older than the current timestamp.
            if housekeeping_index < len(housekeeping) and row["Unix Timestamp [s]"] >= housekeeping.iloc[housekeeping_index]["Unix Timestamp [s]"]:
                self.add_temp(housekeeping.iloc[housekeeping_index][temp_col])
                housekeeping_index += 1
            
            # Calculate the torque and power
            torque = self.step(row)
            velocity = row["Velocity [rad/s]"]
            strain.loc[index, ["Torque [Nm]", "Power [W]"]] = torque, torque * velocity

class SimpleStrainProcessor(StrainProcessor):
    def step(self, data):
        return self.conf.apply(data["Raw [uint24]"], self.cur_temp)

class MovingAverage:
    def __init__(self, max_size:int) -> None:
        self.queue = deque()
        self.sum = 0
        self.max_size = max_size
    
    def add(self, value:float) -> None:
        self.sum += value
        self.queue.append(value)
        if len(self.queue) > self.max_size:
            self.sum -= self.queue.popleft()
    
    def calculate(self) -> float:
        return self.sum / len(self.queue)

class AutoZeroProcessor(StrainProcessor):
    def __init__(self, conf:StrainConfig):
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
            self.last_move_time = timestamp # Stop this being called too often.

        # Calculate the average 
        return self.conf.apply(raw, self.cur_temp)

def process_strain(strain_file:str, temp_col:str, conf:StrainConfig, housekeeping:pd.DataFrame, out_strain_file:str) -> pd.DataFrame:
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
    processor = AutoZeroProcessor(conf)
    processor.process(strain, housekeeping, temp_col)
    strain.to_csv(out_strain_file, index=False)

def run(in_dir:str, out_dir:str, conf_name:str) -> None:
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
    process_strain(f"{in_dir}/left_strain.csv", "Left Temperature [C]", conf.left_strain, housekeeping, f"{out_dir}/left_strain.csv")
    process_strain(f"{in_dir}/right_strain.csv", "Right Temperature [C]", conf.right_strain, housekeeping, f"{out_dir}/right_strain.csv")


    

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
        required=True
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Folder to create to contain the new CSV files.",
        type=str,
        default=None
    )

    args = parser.parse_args()
    output = args.output
    if output is None:
        output = f"{args.input.rstrip('/')}_recalculated"
    run(args.input, output, args.cal_in)