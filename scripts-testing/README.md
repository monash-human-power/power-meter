# Testing scripts and development
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

## Kalman filter development
The [`AccelGyroData.ipynb`](./kalman-filter/AccelGyroData.ipynb) jupyter notebook contains initial code used to test the Kalman filter algorithm with data recorded from a phone taped to a bicycle crank prior to the actual hardware being available. The data was recorded using the [Sensor Logger app](https://play.google.com/store/apps/details?id=com.kelvin.sensorapp&hl=en_US).

[`kalman_test.cpp`](./kalman-filter/kalman_test.cpp) tests the actual Kalman filter implementation whilst running on a computer. Compile this programme using `make kalman` and run it using `make run`.

The [BasicLinearAlgebra](https://github.com/tomstewart89/BasicLinearAlgebra/) library is included as a submodule to assist.