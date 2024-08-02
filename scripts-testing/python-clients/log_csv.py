#!/usr/bin/env python3
"""log_csv.py
Receives data from MQTT and logs this to a CSV file.
"""

from __future__ import annotations
import paho.mqtt.client as mqtt
from typing import List
import struct
import argparse
from abc import ABC, abstractmethod

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
    """Class for storing and processing data from the IMU.
    """
    SIZE = 24

    def __init__(self, data:bytes) -> None:
        """Initialises the object.

        Args:
            data (bytes): The raw data (trimmed to just that containing this object).
        """
        self.timestamp, self.velocity, self.position, self.accel_x, self.accel_y, self.gyro_z = struct.unpack("<Lfffff", data)
    
    def __str__(self) -> str:
        return f"{self.timestamp:>10d}: {self.velocity:>8.2f}rad/s {self.position:>8.1f}rad {self.accel_x:>8.2f}x_m/s {self.accel_y:>8.2f}y_m/s {self.gyro_z:>8.2f}z_rad/s"

class DataHandler(ABC):
    """Class for accepting and processing data from the power meter.
    """
    @abstractmethod
    def add_imu(self, data:bytes) -> None:
        """Takes in bytes containing the data from the IMU and handles them.

        Args:
            data (bytes): The raw MQTT message containing IMU data.
        """
        pass

    def _process_imu(self, data:bytes) -> List[IMUData]:
        """Accepts a blob of bytes and converterts these into an array of
        IMUData objects.

        Args:
            data (bytes): The raw data (full MQTT message).

        Returns:
            List[IMUData]: A list of IMUData objects that were contained in the
                           data.
        """
        # For each data object, extract it and add it to an array.
        result = [IMUData(data[i:i+IMUData.SIZE]) for i in range(0, len(data), IMUData.SIZE)]
        return result

class IMUCSVHandler(DataHandler):
    """Class for accepting data and saving this to an excel spreadsheet."""
    def add_imu(self, data: bytes) -> None:
        # TODO
        converted = self._process_imu(data)
        for i in converted:
            print(i)
            file.write(f"{i.timestamp},{i.velocity},{i.position},{i.accel_x},{i.accel_y},{i.gyro_z}\n")

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
excel_handler = IMUCSVHandler()

def on_connect(client:mqtt.Client, userdata:None, flags, reason_code:int, properties) -> None:
    """Called when connected to the broker."""
    print("Connected to MQTT")
    mqtt_client.subscribe(MQTT_TOPIC_ABOUT)
    mqtt_client.subscribe(MQTT_TOPIC_HOUSEKEEPING)
    mqtt_client.subscribe(MQTT_TOPIC_LOW_SPEED)
    mqtt_client.subscribe(MQTT_TOPIC_LEFT)
    mqtt_client.subscribe(MQTT_TOPIC_RIGHT)
    mqtt_client.subscribe(MQTT_TOPIC_IMU)

def on_message(client:mqtt.Client, userdata:None, msg:mqtt.MQTTMessage) -> None:
    """Handles a received message from MQTT.

    Args:
        client (mqtt.Client): The MQTT client.
        userdata (None): The user data.
        msg (mqtt.MQTTMessage): The message structure.
    """
    if msg.topic == MQTT_TOPIC_ABOUT:
        print("About this device: " + msg.payload.decode())
    elif msg.topic == MQTT_TOPIC_IMU:
        excel_handler.add_imu(msg.payload)

if __name__ == "__main__":
    # Extract command line arguments
    parser = argparse.ArgumentParser(
        "Log power meter data to a file",
        "Subscribes to MQTT data from the power meter, decodes it and saves the data to a file.",
        conflict_handler="resolve"
    )
    parser.add_argument(
        "-h", "--host",
        help="The MQTT broker to connect to.",
        type=str,
        default="localhost"
    )
    parser.add_argument(
        "-o", "--output",
        help="The CSV file to write IMU data to.",
        type=str,
        default="imu.csv"
    )

    args = parser.parse_args()

    # Setup MQTT and loop forever
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(args.host)

    with open(args.output, "w") as file:
        file.write("Timestamp [us],Velocity [rad/s],Position [rad],Acceleration X [m/s^2],Acceleration Y [m/s^2],Gyro Z [rad/s]\n")
        mqtt_client.loop_forever()