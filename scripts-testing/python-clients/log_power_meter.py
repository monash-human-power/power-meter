#!/usr/bin/env python3
"""log_power_meter.py
usage: log_power_meter.py [--help] [-h HOST] [-m {graph,csv,both}] [-r MAX_RECORDS] [-o OUTPUT]

Subscribes to MQTT data from the power meter, decodes it and saves the data to a file and or draws it on a live graph.

options:
  --help                show this help message and exit
  -h HOST, --host HOST  The MQTT broker to connect to. (default: localhost)
  -m {graph,csv,both}, --method {graph,csv,both}
                        The output method to use. (default: both)
  -r MAX_RECORDS, --max-records MAX_RECORDS
                        The maximum number of records to show at a time on the graph. Set as 0 to show everything (may slow down over time) (default: 250)
  -o OUTPUT, --output OUTPUT
                        The folder to create and write CSV files into. (default: DATETIME_PowerMeter)

Written by Jotham Gates and Oscar Varney for MHP, 2024
"""

from __future__ import annotations
import paho.mqtt.client as mqtt
from typing import List
import struct
import argparse
from abc import ABC, abstractmethod
from datetime import datetime
import os
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.lines import Line2D
from multiprocessing import Process, Queue, Semaphore
import math
from enum import Enum
import time
import json
import traceback
from typing import TypeVar, Tuple
from dataclasses import dataclass

T = TypeVar("U")

# Sides
class Side(Enum):
    LEFT = "left"
    RIGHT = "right"


# Topics
MQTT_TOPIC_PREFIX = "/power/"
MQTT_TOPIC_ABOUT = MQTT_TOPIC_PREFIX + "about"
MQTT_TOPIC_HOUSEKEEPING = MQTT_TOPIC_PREFIX + "housekeeping"
MQTT_TOPIC_LOW_SPEED = MQTT_TOPIC_PREFIX + "power"
MQTT_TOPIC_HIGH_SPEED = MQTT_TOPIC_PREFIX + "fast/"
MQTT_TOPIC_IMU = MQTT_TOPIC_PREFIX + "imu"
MQTT_TOPIC_LEFT = MQTT_TOPIC_HIGH_SPEED + Side.LEFT.value
MQTT_TOPIC_RIGHT = MQTT_TOPIC_HIGH_SPEED + Side.RIGHT.value


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
        return abs(self.velocity) / (2 * math.pi) * 60

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


class DataHandler(ABC):
    """Class for accepting and processing data from the power meter."""

    @abstractmethod
    def add_imu(self, data: bytes) -> None:
        """Takes in bytes containing the data from the IMU and handles them.

        Args:
            data (bytes): The raw MQTT message containing IMU data.
        """
        pass

    @abstractmethod
    def add_about(self, data: str) -> None:
        """Accepts an about message and handles it.

        Args:
            data (str): The MQTT message string containing the about message.
        """
        pass

    def add_housekeeping(self, unix_time:float, data: str) -> None:
        """Accepts housekeeping data and handles it.

        Args:
            data (str): The MQTT message string containing the housekeeping data.
        """
        pass

    def add_fast(self, unix_time:float, data: str, side: Side) -> None:
        """Accepts a message from a side and handles it.

        Args:
            data (str): The MQTT message string containing the about message.
            side (Side): The side the data applies to.
        """
        pass

    def add_slow(self, unix_time:float, data: str) -> None:
        """Adds slow speed data (power, balance, cadence...)

        Args:
            unix_time (float): The time the message was received.
            data (str): The data in the message.
        """
        pass

    def close(self) -> None:
        """Closes the handler safely."""

    def _process_imu(self, data: bytes) -> List[IMUData]:
        """Accepts a blob of bytes and converterts these into an array of
        IMUData objects.

        Args:
            data (bytes): The raw data (full MQTT message).

        Returns:
            List[IMUData]: A list of IMUData objects that were contained in the
                           data.
        """
        # For each data object, extract it and add it to an array.
        result = [
            IMUData(data[i : i + IMUData.SIZE])
            for i in range(0, len(data), IMUData.SIZE)
        ]
        return result

    def _process_strain(self, data: bytes) -> List[StrainData]:
        """Accepts a blob of bytes and converterts these into an array of
        StrainData objects.

        Args:
            data (bytes): The raw data (full MQTT message).

        Returns:
            List[StrainData]: A list of StrainData objects that were contained
            in the data.
        """
        # For each data object, extract it and add it to an array.
        result = [
            StrainData(data[i : i + StrainData.SIZE])
            for i in range(0, len(data), StrainData.SIZE)
        ]
        return result


class CSVSide:
    def __init__(self, side: Side, output_dir: str):
        """Opens a file ready to write with the given name.

        Args:
            side (Side): The side this will represent.
            output_dir (str): The output directory to place the file in.
        """
        # Open the file and write a heading.
        self.file = open(f"{output_dir}/{side.value}_strain.csv", "w")
        self.file.write(
            "Unix Timestamp [us],Device Timestamp [us],Timestep[us],Velocity [rad/s],Position [rad],Raw [uint24],Torque [Nm],Power [W]\n"
        )

        # Initialise time step calculation
        self.last_timestamp = 0
        self.side = side

    def add_fast(self, unix_time:float, data: List[StrainData]) -> None:
        """Adds high speed data to the side.

        Args:
            data (List[StrainData]): The data to add.

        Returns:
            None
        """
        # Select which file to write to
        raw_sum = 0
        for i in data:
            timestep = i.timestamp - self.last_timestamp
            self.last_timestamp = i.timestamp
            # print(f"{self.side.name}: ({timestep:>10d}) {i}")
            raw_sum += i.raw
            self.file.write(
                f"{unix_time},{i.timestamp},{timestep},{i.velocity},{i.position},{i.raw},{i.torque},{i.power}\n"
            )
        
        # Print out the average
        print(f"{self.side.name:<10s}: {raw_sum//len(data):>10d}")

    def close(self) -> None:
        self.file.close()


class CSVHandler(DataHandler):
    """Class for accepting data and saving this to a folder of CSVs."""

    def __init__(self, output: str):
        # Create the folder
        print(f"Creating folder '{output}'")
        if not os.path.exists(output):
            os.makedirs(output)

        # Create the about file
        json_str_header = "Unix Timestamp [us],Message\n"
        self.about_file = open(f"{output}/about.csv", "w", buffering=1)
        self.about_file.write(json_str_header)

        # Create the housekeeping file
        self.housekeeping_file = open(f"{output}/housekeeping.csv", "w", buffering=1)
        self.housekeeping_file.write("Unix Timestamp [us],Left Temperature [C],Right Temperature [C],IMU Temperature [C],Battery [mV],Left Offset [raw],Right Offset [raw]\n")

        # Create the IMU file
        self.imu_file = open(f"{output}/imu.csv", "w")
        self.imu_file.write(
            "Unix Timestamp [us],Device Timestamp [us],Timestep[us],Velocity [rad/s],Position [rad],Acceleration X [m/s^2],Acceleration Y [m/s^2],Acceleration Z [m/s^2],Gyro A [rad/s],Gyro B [rad/s],Gyro Z [rad/s]\n"
        )
        self.last_imu_timestamp = 0

        # Create the strain gauge files
        self.left = CSVSide(Side.LEFT, output)
        self.right = CSVSide(Side.RIGHT, output)

        # Create the slow file
        self.slow = open(f"{output}/slow.csv", "w")
        self.slow.write(
            "Unix Timestamp [us],Device Timestamp [us],Cadence [rpm],Rotations [#],Power [W],Balance [%]\n"
        )

    def add_imu(self, unix_time:float, data: bytes) -> None:
        converted = self._process_imu(data)
        for i in converted:
            timestep = i.timestamp - self.last_imu_timestamp
            self.last_imu_timestamp = i.timestamp
            # print(f"({timestep:>10d}) {i}")
            self.imu_file.write(
                f"{unix_time},{i.timestamp},{timestep},{i.velocity},{i.position},{i.accel_x},{i.accel_y},{i.accel_z},{i.gyro_x},{i.gyro_y},{i.gyro_z}\n"
            )

    def add_about(self, unix_time:float, data: str) -> None:
        print(f"About: {data}")
        self.about_file.write(f"{unix_time},'{data}'\n")

    def add_housekeeping(self, unix_time:float, data: str) -> None:
        print(f"Housekeeping: {data}")
        data = json.loads(data)
        self.housekeeping_file.write(f"{unix_time},{data['temps']['left']},{data['temps']['right']},{data['temps']['imu']},{data['battery']},{data['left-offset']},{data['right-offset']}\n")

    def add_fast(self, unix_time:float, data: str, side: Side) -> None:
        # Process the data.
        converted = self._process_strain(data)

        if side == Side.LEFT:
            self.left.add_fast(unix_time, converted)
        else:
            self.right.add_fast(unix_time, converted)
    
    def add_slow(self, unix_time: float, data: json) -> None:
        self.slow.write(f"{unix_time},{data['timestamp']},{data['cadence']},{data['rotations']},{data['power']},{data['balance']}\n")

    def close(self):
        print("Closing CSV Handler")
        self.imu_file.close()
        self.about_file.close()
        self.housekeeping_file.close()
        self.slow.close()

class LiveChart(ABC):
    def __init__(self, fig:Figure, ax:Axes, max_history:int=None, title:str="") -> None:
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
    def update(self, data:T) -> Tuple[Line2D]:
        """Updates the graph.

        Args:
            data (T): The data to add (or list of data points).
        """
        pass

    def add_data(self, data:T) -> None:
        """Adds data to be passed to the update method eventually."""
        self.queue.put(data)
    
    def setup_animation(self) -> None:
        self.ani = animation.FuncAnimation(
            fig=self.fig, func=self.update_graph, interval=80, cache_frame_data=False
        )

    def update_graph(self, frame:int) -> Tuple[Line2D]:
        """Gets the latest data from the queue and calls update
        """
        # Update the title as needed
        if not self.title_queue.empty():
            self.ax.set_title(self.title_queue.get())

        # Update the lines
        if not self.queue.empty():
            data = self.queue.get()
            return self.update(data)
        else:
            return None
    
    def limit_length(self, input:list) -> list:
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
    
    def update_title(self, new_title:str) -> None:
        """Adds the new title to the queue."""
        self.title_queue.put(new_title)

class PolarLiveChart(LiveChart):
    def __init__(self, max_history:int=None, title:str="", ymax:int=None) -> None:
        fig, ax = fig, ax = plt.subplots(subplot_kw={"projection": "polar"})
        ax.set_ylim(0, ymax)
        super().__init__(fig, ax, max_history, title)

class IMULiveChart(PolarLiveChart):
    """Live chart for the IMU."""
    def __init__(self, max_history:int=None):
        self.theta = []
        self.cadence = []
        super().__init__(max_history, "Cadence [rpm] vs pedal angle [$^\circ$]", 160)
        self.line = self.ax.plot([], [])[0]
        self.latest = self.ax.axvline(0, color="r")
    
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
    def __init__(self, max_history:int=None):
        # Lists to hold data
        self.thetas = {
            Side.LEFT: [],
            Side.RIGHT: []
        }
        self.torques = {
            Side.LEFT: [],
            Side.RIGHT: []
        }

        # Initialise the graph.
        super().__init__(max_history, "Torque [Nm] vs pedal angle [$^\circ$]", 50)
        self.lines = {
            Side.LEFT: self.ax.plot([], [])[0],
            Side.RIGHT: self.ax.plot([], [])[0]
        }
        self.latest = self.ax.axvline(0, color="r")
    
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
        self.latest.set_xdata([self.thetas[side][-1]])

        return self.lines[Side.LEFT], self.lines[Side.RIGHT], self.latest
    
class PowerLiveChart(PolarLiveChart):
    """Live chart for the power."""
    def __init__(self, max_history:int=None):
        # Lists to hold data
        self.thetas = {
            Side.LEFT: [],
            Side.RIGHT: []
        }
        self.powers = {
            Side.LEFT: [],
            Side.RIGHT: []
        }

        # Initialise the graph.
        super().__init__(max_history, "Instantaneous power [W] vs pedal angle [$^\circ$]", 150)
        self.lines = {
            Side.LEFT: self.ax.plot([], [], label="Left")[0],
            Side.RIGHT: self.ax.plot([], [], label="Right")[0]
        }
        self.latest = self.ax.axvline(0, color="r")
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

    def update_power_subtitle(self, power: float, balance:float) -> None:
        self.update_title(f"${power:.0f}$ W, ${balance:.0f}$ % balance over the last rotation")

class GraphHandler(DataHandler):
    """Class that shows data on matplotlib."""
    def __init__(self, max_history=None):
        self.imu_graph = IMULiveChart(max_history)
        self.torque_graph = TorqueLiveChart(max_history)
        self.power_graph = PowerLiveChart(max_history)
        self.animate_process = Process(target=self.animate, daemon=True)
        self.animate_process.start()

    def add_imu(self, unix_time:float, data: bytes) -> None:
        converted = self._process_imu(data)
        self.imu_graph.add_data(converted)

    def add_about(self, unix_time:float, data: str) -> None:
        return super().add_about(data)

    def add_fast(self, unix_time: float, data: str, side: Side) -> None:
        converted = self._process_strain(data)
        self.torque_graph.add_data(SideDataPair(side, converted))
        self.power_graph.add_data(SideDataPair(side, converted))

    def close(self) -> None:
        self.animate_process.kill()
    
    def add_slow(self, unix_time: float, data: str) -> None:
        self.imu_graph.update_cadence_subtitle(data["cadence"])
        self.power_graph.update_power_subtitle(data["power"], data["balance"])
    
    def animate(self) -> None:
        self.imu_graph.setup_animation()
        self.torque_graph.setup_animation()
        self.power_graph.setup_animation()
        plt.show()

class MultiHandler(DataHandler):
    def __init__(self, handlers: List[DataHandler]) -> None:
        """Initialises the handler with a list of other handlers to pass data to.

        Args:
            handlers (List[DataHandler]): The handlers
        """
        self.handlers = handlers

    def add_imu(self, unix_time:float, data: bytes) -> None:
        for h in self.handlers:
            h.add_imu(unix_time, data)

    def add_about(self, unix_time:float, data: str) -> None:
        for h in self.handlers:
            h.add_about(unix_time, data)

    def add_housekeeping(self, unix_time: float, data: str) -> None:
        for h in self.handlers:
            h.add_housekeeping(unix_time, data)

    def add_fast(self, unix_time:float, data: str, side: Side) -> None:
        for h in self.handlers:
            h.add_fast(unix_time, data, side)

    def close(self) -> None:
        for h in self.handlers:
            h.close()


mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)


def on_connect(
    client: mqtt.Client, userdata: None, flags, reason_code: int, properties
) -> None:
    """Called when connected to the broker."""
    print("Connected to MQTT")
    if not args.no_about:
        mqtt_client.subscribe(MQTT_TOPIC_ABOUT)
    else:
        print("Not subscribing to about messages.")

    if not args.no_housekeeping:
        mqtt_client.subscribe(MQTT_TOPIC_HOUSEKEEPING)
    else:
        print("Not subscribing to housekeeping messages.")

    if not args.no_power:
        mqtt_client.subscribe(MQTT_TOPIC_LOW_SPEED)
    else:
        print("Not subscribing to low-speed power messages.")

    if not args.no_left:
        mqtt_client.subscribe(MQTT_TOPIC_LEFT)
    else:
        print("Not subscribing to high-speed left ADC messages.")

    if not args.no_right:
        mqtt_client.subscribe(MQTT_TOPIC_RIGHT)
    else:
        print("Not subscribing to high-speed right ADC messages.")

    if not args.no_imu:
        mqtt_client.subscribe(MQTT_TOPIC_IMU)
    else:
        print("Not subscribing to high-speed IMU messages.")


def on_message(client: mqtt.Client, userdata: None, msg: mqtt.MQTTMessage) -> None:
    """Handles a received message from MQTT.

    Args:
        client (mqtt.Client): The MQTT client.
        userdata (None): The user data.
        msg (mqtt.MQTTMessage): The message structure.
    """
    t = time.time()
    if msg.topic == MQTT_TOPIC_ABOUT:
        print("About this device: " + msg.payload.decode())
        handler.add_about(t, msg.payload.decode())
    elif msg.topic == MQTT_TOPIC_IMU:
        handler.add_imu(t, msg.payload)
    elif msg.topic == MQTT_TOPIC_HOUSEKEEPING:
        handler.add_housekeeping(t, msg.payload.decode())
    elif msg.topic == MQTT_TOPIC_LEFT:
        handler.add_fast(t, msg.payload, Side.LEFT)
    elif msg.topic == MQTT_TOPIC_RIGHT:
        handler.add_fast(t, msg.payload, Side.RIGHT)
    elif msg.topic == MQTT_TOPIC_LOW_SPEED:
        data = json.loads(msg.payload)
        handler.add_slow(t, data)


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Subscribes to MQTT data from the power meter, decodes it and saves the data to a file and or draws it on a live graph.",
        conflict_handler="resolve",  # Cope with -h being used for host like mosquitto clients
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="Written by Jotham Gates and Oscar Varney for MHP, 2024. For more information, please see here: https://github.com/monash-human-power/power-meter",
    )
    parser.add_argument(
        "-h",
        "--host",
        help="The MQTT broker to connect to.",
        type=str,
        default="localhost",
    )
    parser.add_argument(
        "-m",
        "--method",
        help="The output method to use.",
        type=str,
        choices=["graph", "csv", "both"],
        default="both",
    )
    parser.add_argument(
        "-r",
        "--max-records",
        help="The maximum number of records to show at a time on the graph. Set as 0 to show everything (may slow down over time)",
        type=int,
        default=250,
    )
    parser.add_argument(
        "-o",
        "--output",
        help="The folder to create and write CSV files into.",
        type=str,
        default=datetime.now().strftime("%Y%m%d_%H%M%S_PowerMeter"),
    )

    # What to include
    group = parser.add_argument_group(
        title="Topics to include",
        description="All topics are included by default, set these to False to not subscribe to a particular topic.",
    )
    group.add_argument(
        "--no-about",
        help="If present, does not subscribe to about messages.",
        action="store_true",
    )
    group.add_argument(
        "--no-housekeeping",
        help="If present, does not subscribe to housekeeping messages.",
        action="store_true",
    )
    group.add_argument(
        "--no-imu",
        help="If present, does not subscribe to messages containing high speed IMU data.",
        action="store_true",
    )
    group.add_argument(
        "--no-left",
        help="If present, does not subscribe to messages containing high speed data from the left ADC.",
        action="store_true",
    )
    group.add_argument(
        "--no-right",
        help="If present, does not subscribe to messages containing high speed data from the right ADC.",
        action="store_true",
    )
    group.add_argument(
        "--no-power",
        help="If present, does not subscribe to messages containing slow power data.",
        action="store_true",
    )
    args = parser.parse_args()

    # Setup the data handler
    if args.method == "csv":
        handler = CSVHandler(args.output)
    elif args.method == "graph":
        handler = GraphHandler(args.max_records)
    else:
        # Both
        handler = MultiHandler(
            (CSVHandler(args.output), GraphHandler(args.max_records))
        )

    # Setup MQTT and loop forever
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(args.host)
    try:
        mqtt_client.loop_forever()
    except Exception as e:
        print(f"Exception causing data recording to stop: '{e}'")
        traceback.print_exc()
        handler.close()
