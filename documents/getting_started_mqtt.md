# Getting started (connecting over WiFi and MQTT) <!-- omit in toc -->

- [What you will need](#what-you-will-need)
- [Setting up to log and display data](#setting-up-to-log-and-display-data)
- [Turning the power meter on](#turning-the-power-meter-on)

## What you will need
- A WiFi network / mobile phone hotspot (2.4GHz) that the power meter is configurred to connect to (see [configurations](../configs/README.md)).
- An MQTT broker that the power meter is configurred to connect to (see [configurations](../configs/README.md)).
- A computer with:
  - A copy of this repository cloned to it.
  - [Python3](https://www.python.org/) installed.
  - The MQTT broker and WiFi network may be running or hosted on this computer (optional).

## Setting up to log and display data
1. Open a terminal and change the working directory to the [repository root](../).
2. **Optional:** Create a python virtual environment to keep the packages installed in this project separate from the rest of the system.
    ```bash
    pip3 -m venv .venv
    ```
3. Activate the virtual environment if using it.
    ```bash
    source .venv/bin/activate
    ```
    You will notive that each line in the terminal now starts with `(.venv)`.
4. Install the required packages:
    ```bash
    pip3 install -r requirements.txt
    ```
5. Change directory to the [`scripts-testing/python-clients`](../scripts-testing/python-clients/) folder.
6. Run the [logging script](../scripts-testing/python-clients/log_power_meter.py):
    ```bash
    ./log_power_meter.py
    ```
    By default, this will attempt to subscribe to a broker running on the same computer the script is being run from. Pass `-h HOST_OR_IP` to change this. To see other options available, run the script with `--help`.
7. It may be worthwhile opening another terminal to subscribe to the low speed topics to display messages as they arrive with:
    ```bash
    mosquitto_sub -t "/power/about" -t "/power/power" -t "/power/housekeeping" -v
    ```
    Again use `-h HOST_OR_IP` to specify an alternate MQTT broker if needed.
8. Use `Ctrl` + `C` to close both of these programmes.

## Turning the power meter on
1. Install the battery and plug it into the board. Make sure the end cap on the axle is done up sufficiently to stop the crank arm falling off.
2. The V1.1.1 power meters will flash the yellow LEDs on each side followed by a blue flash on the RGB LED when they are trying to connect to WiFi. Make sure that the yellow LEDs on both sides are flashing (sometimes power needs to be removed and re-applied to get the temperature sensors they are connected to, to initialise correctly).
3. If the power meter is stuck in this connecting to WiFi state, make sure that the WiFi network is running with the same SSID and password as the power meter is configured to connect to. Make sure that it uses 2.4GHz WiFi as the built in microcontroller does not support the 5GHz band. If none of these work, use a [USB cable to check and update the configs on the device as needed](../configs/README.md#through-serial).
4. Once connected to WiFi, the yellow LEDs on each side will continue to flash and the RGB LED will flash green. In this state, the device is trying to connect to the broker. If this does not happen successfully, make sure the broker is configurred correctly and the broker is running and reachable from other computers.
5. The RGB LED will flicker green when the power meter is connected and sending data.
6. To manually measure and apply offsets, publish a message to the `/power/offset` topic using the following command:
    ```bash
    mosquitto_pub -t "/power/offset" -m ""
    ```