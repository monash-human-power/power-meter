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
from multiprocessing import Process, Queue
import math


# Topics
MQTT_TOPIC_PREFIX = "/power/"
MQTT_TOPIC_ABOUT = MQTT_TOPIC_PREFIX + "about"
MQTT_TOPIC_HOUSEKEEPING = MQTT_TOPIC_PREFIX + "housekeeping"
MQTT_TOPIC_LOW_SPEED = MQTT_TOPIC_PREFIX + "power"
MQTT_TOPIC_HIGH_SPEED = MQTT_TOPIC_PREFIX + "fast/"
MQTT_TOPIC_IMU = MQTT_TOPIC_PREFIX + "imu"
MQTT_TOPIC_LEFT = MQTT_TOPIC_HIGH_SPEED + "left"
MQTT_TOPIC_RIGHT = MQTT_TOPIC_HIGH_SPEED + "right"


class IMUData:
    """Class for storing and processing data from the IMU."""

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
            self.accel_x,
            self.accel_y,
            self.gyro_z,
        ) = struct.unpack("<Lfffff", data)

    def cadence(self) -> float:
        """Calculates the current cadence from the velocity.

        Returns:
            float: The cadence in RPM.
        """
        return abs(self.velocity) / (2 * math.pi) * 60

    def __str__(self) -> str:
        return f"{self.timestamp:>10d}: {self.velocity:>8.2f}rad/s {self.position:>8.1f}rad {self.accel_x:>8.2f}x_m/s {self.accel_y:>8.2f}y_m/s {self.gyro_z:>8.2f}z_rad/s"


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


class CSVHandler(DataHandler):
    """Class for accepting data and saving this to a folder of CSVs."""

    def __init__(self, output: str):
        # Create the folder
        print(f"Creating folder '{output}'")
        if not os.path.exists(output):
            os.makedirs(output)

        # Create the about file
        self.about_file = open(f"{output}/about.txt", "w")

        # Create the IMU file
        self.imu_file = open(f"{output}/imu.csv", "w")
        self.imu_file.write(
            "Timestamp [us],Timestep[us],Velocity [rad/s],Position [rad],Acceleration X [m/s^2],Acceleration Y [m/s^2],Gyro Z [rad/s]\n"
        )
        self.last_imu_timestamp = 0

    def add_imu(self, data: bytes) -> None:
        converted = self._process_imu(data)
        for i in converted:
            timestep = i.timestamp - self.last_imu_timestamp
            self.last_imu_timestamp = i.timestamp
            print(f"({timestep:>10d}) {i}")
            self.imu_file.write(
                f"{i.timestamp},{timestep},{i.velocity},{i.position},{i.accel_x},{i.accel_y},{i.gyro_z}\n"
            )

    def add_about(self, data: str) -> None:
        print(f"About: {data}")
        self.about_file.write(data)

    def close(self):
        print("Closing CSV Handler")
        self.imu_file.close()
        self.about_file.close()


class GraphHandler(DataHandler):
    """Class that shows data on matplotlib."""

    def __init__(self, max_history=None):
        self.max_history = max_history
        fig, ax = plt.subplots(subplot_kw={"projection": "polar"})
        self.theta = []
        self.cadence = []
        self.line = ax.plot(self.theta, self.cadence)[0]
        self.latest = ax.axvline(0, color="r")
        ax.set_ylim(0, 160)
        ax.set_title(f"Cadence [rpm] vs pedal angle [$^\circ$]")

        def format_radians_label(float_in):
            """Converts a float value in radians into a string representation of that float.
            From https://stackoverflow.com/a/63667851
            """
            if (float_in > math.pi):
                float_in = -(2*math.pi - float_in)
            string_out = str(float_in / (math.pi))+"π"
            
            return string_out

        def convert_polar_xticks_to_radians(ax):
            """Converts x-tick labels from degrees to radians
            From https://stackoverflow.com/a/63667851
            """
            
            # Get the x-tick positions (returns in radians)
            label_positions = ax.get_xticks()
            
            # Convert to a list since we want to change the type of the elements
            labels = list(label_positions)
            
            # Format each label (edit this function however you'd like)
            labels = [format_radians_label(label) for label in labels]
            
            ax.set_xticklabels(labels)

        def animate():
            """Animate and block in a function to allow for multiple processing."""
            ani = animation.FuncAnimation(
                fig=fig, func=self.update_graph, interval=100, cache_frame_data=False
            )
            plt.show()

        convert_polar_xticks_to_radians(ax)
        self.queue = Queue()
        self.animate_process = Process(target=animate)
        self.animate_process.start() # TODO: Handle keyboard interrupts.

    def update_graph(self, frame):
        print(f"Frame {frame}")
        # Get the new data
        converted = self.queue.get()
        for conv in converted:
            self.theta.append(conv.position)
            self.cadence.append(conv.cadence())

        # Remove old data
        if self.max_history and len(self.cadence) > self.max_history:
            self.cadence = self.cadence[len(self.cadence) - self.max_history :]
            self.theta = self.theta[len(self.theta) - self.max_history :]

        # Update the data
        self.line.set_xdata(self.theta)
        self.line.set_ydata(self.cadence)
        self.latest.set_xdata([self.theta[-1]])
        # self.latest.set_ydata([self.cadence[-1]])
        print(f"{self.theta[-1]=}\t{self.cadence[-1]=}")
        return self.line, self.latest

    def add_imu(self, data: bytes) -> None:
        converted = self._process_imu(data)
        self.queue.put(converted)

    def add_about(self, data: str) -> None:
        return super().add_about(data)
    
    def close(self) -> None:
        self.animate_process.kill()


class MultiHandler(DataHandler):
    def __init__(self, handlers: List[DataHandler]) -> None:
        """Initialises the handler with a list of other handlers to pass data to.

        Args:
            handlers (List[DataHandler]): The handlers
        """
        self.handlers = handlers

    def add_imu(self, data: bytes) -> None:
        for h in self.handlers:
            h.add_imu(data)

    def add_about(self, data: str) -> None:
        for h in self.handlers:
            h.add_about(data)

    def close(self) -> None:
        for h in self.handlers:
            h.close()


mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)


def on_connect(
    client: mqtt.Client, userdata: None, flags, reason_code: int, properties
) -> None:
    """Called when connected to the broker."""
    print("Connected to MQTT")
    mqtt_client.subscribe(MQTT_TOPIC_ABOUT)
    mqtt_client.subscribe(MQTT_TOPIC_HOUSEKEEPING)
    mqtt_client.subscribe(MQTT_TOPIC_LOW_SPEED)
    mqtt_client.subscribe(MQTT_TOPIC_LEFT)
    mqtt_client.subscribe(MQTT_TOPIC_RIGHT)
    mqtt_client.subscribe(MQTT_TOPIC_IMU)


def on_message(client: mqtt.Client, userdata: None, msg: mqtt.MQTTMessage) -> None:
    """Handles a received message from MQTT.

    Args:
        client (mqtt.Client): The MQTT client.
        userdata (None): The user data.
        msg (mqtt.MQTTMessage): The message structure.
    """
    if msg.topic == MQTT_TOPIC_ABOUT:
        print("About this device: " + msg.payload.decode())
        handler.add_about(msg.payload.decode())
    elif msg.topic == MQTT_TOPIC_IMU:
        handler.add_imu(msg.payload)


if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        description="Subscribes to MQTT data from the power meter, decodes it and saves the data to a file and or draws it on a live graph.",
        conflict_handler="resolve",  # Cope with -h being used for host like mosquitto clients
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="Written by Jotham Gates and Oscar Varney for MHP, 2024",
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
    except:
        handler.close()
