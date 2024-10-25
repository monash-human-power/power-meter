# Testing scripts and development
## Logging data to a file and plotting it.
The python scripts in the [`python-clients`](./python-clients/) directory allow data to be saved, unpacked into human-readable form and saved as needed.
- [`log_power_meter.py`](./python-clients/log_power_meter.py) logs data to a folder of files or displays it live.
- [`plot_imu_timeseries.py`](./python-clients/plot_imu_timeseries.py) plots IMU data.

## Board testing
These scripts help to verify that the hardware is working correctly. It is recommended to run these scripts in the following order. The next one usually depends on the previous working.

### 1. [LED testing](./led-testing/)
This script turns on and flashes the red and green LEDs (D1 and D2).

### 2. [Accelerometer polling](./accel-testing/)
> Relies on [LED testing](#1-led-testing) passing.

This script regularly polls the IMU and sets the colour of the LEDs based on the readings of the X axis accelerometer (tilt to change colour). The values read are also sent over the serial port.

This script is based off the [ICM42670P Arduino library `Polling_SPI`](https://github.com/tdk-invn-oss/motion.arduino.ICM42670P/blob/main/examples/Polling_SPI/Polling_SPI.ino) example.

### 3. [Accelerometer interrupts](./accel_testing-fifo/)
> Relies on [LED testing](#1-led-testing) and [Accelerometer polling](#2-accelerometer-polling) passing.

This script behaves the same as the polling script, however it uses the FIFO buffer and interrupts to obtain the data.

This script is based off the [ICM42670P Arduino library `FIFO_Interrupt`](https://github.com/tdk-invn-oss/motion.arduino.ICM42670P/blob/main/examples/FIFO_Interrupt/FIFO_Interrupt.ino) example.

### 4. [GPIO Testing for the ADC pins](./gpio-testing/)
> Relies on [LED testing](#1-led-testing) and USB / the ability to accept numbers from `Serial.parseInt()`.

This script toggles pins as given over the serial port. This can be used to check that they were soldered correctly and are not disconnected or shorted out.

Usage instructions:
- Red light indicates waiting for a pin number to be typed in the serial console.
- Flashing green indicates that a pin is being tested (on is high, off is low).
- Press 'q' to got back and select another pin.
- All pins other than LEDs are left in high impedance states after testing that pin concludes.

### 5. Temperature sensor and LED testing
> Relies on [LED testing](#1-led-testing) for pretty lights and a serial connection to show temperatures.

This script reads the temperatures on both I2C sensors and flashes the temperature sensor and built in LEDs.

## Kalman filter development
[`kalman-compare.py`](./kalman-filter/kalman-compare.py) takes a CSV file from the power meter and plots the reported orientation over time. This script also calculates orientation based on the accelerometer alone, gyroscope alone and a python implementation of the Kalman filter to compare the results.

The [`AccelGyroData.ipynb`](./kalman-filter/AccelGyroData.ipynb) Jupyter notebook contains initial code used to test the Kalman filter algorithm with data recorded from a phone taped to a bicycle crank prior to the actual hardware being available. The data was recorded using the [Sensor Logger app](https://play.google.com/store/apps/details?id=com.kelvin.sensorapp&hl=en_US).

[`kalman_test.cpp`](./kalman-filter/kalman_test.cpp) tests the actual Kalman filter implementation whilst running on a computer. Compile this programme using `make kalman` and run it using `make run`.

The [BasicLinearAlgebra](https://github.com/tomstewart89/BasicLinearAlgebra/) library is included as a submodule to assist.

## Python libraries and environments
The [`requirements.txt`](../requirements.txt) file in the root of the repository contains all necessary libraries for all python scripts and Jupyter notebooks in this repository. As usual, it is recommended to use a python virtual environment. This can be created and the libraries installed (running from the repository-root directory) using:
```bash
python3 -v venv .venv
source .venv/bin/activate
pip3 -r requirements.txt
```

To save / update [`requirements.txt`](../requirements.txt), run [`./tools/update_requirements.sh`](../tools/update_requirements.sh) or run:
```bash
source .venv/bin/activate
pip3 freeze > requirements.txt
```

