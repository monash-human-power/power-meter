# Config files <!-- omit in toc -->
These JSON files contain calibration data and settings for the power meters.

- [Config format](#config-format)
  - [Example](#example)
  - [Key value pairs](#key-value-pairs)
  - [Notes](#notes)
    - [(1) More on Kalman filters](#1-more-on-kalman-filters)
    - [(2) Matrix representation](#2-matrix-representation)
    - [(3) Converting ADC values to torque](#3-converting-adc-values-to-torque)
- [Obtaining the current config from the device](#obtaining-the-current-config-from-the-device)
  - [Over MQTT](#over-mqtt)
  - [Through serial](#through-serial)
- [Uploading new configs to the device](#uploading-new-configs-to-the-device)
  - [Uploading over MQTT](#uploading-over-mqtt)
  - [Uploading over serial](#uploading-over-serial)

## Config format
The data is formatted into key-value pairs using JSON.

### Example
```json
{
    "connection": 0,
    "kalman": {
        "Q": [
            [2e-3, 0],
            [0, 0.1]
        ],
        "R": [
            [100, 0],
            [0, 1e-2]
        ]
    },
    "imuHowOften": 1,
    "sleep-time": 0,
    "left-strain": {
        "offset": 0,
        "coef": -3.679656e-4,
        "temp-test": 24.25,
        "temp-coef": 0
    },
    "right-strain": {
        "offset": 0,
        "coef": 2.246571e-4,
        "temp-test": 24.25,
        "temp-coef": 0
    },
    "mqtt": {
        "length": 20,
        "broker": "mhp-chase-car.local"
    },
    "wifi": {
        "ssid": "",
        "psk": "",
        "redacted": true
    }
}
```

### Key value pairs
|               Key                |                         Data type                         | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                         | When changes are applied |
| :------------------------------: | :-------------------------------------------------------: | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :----------------------- |
|          `"connection"`          | [`EnumSide`](../power-meter-code/src/defines.h#EnumSide)  | What connection method to use. `0` is MQTT and WiFi, `1` is Bluetooth Low Energy (BLE). Pressing the boot button after the power meter has started will also toggle this option and restart the power meter to apply it.                                                                                                                                                                                                                                            | On boot                  |
|            `"kalman"`            |                        JSON object                        | Configurration values for the Kalman filter used to process data from the IMU. See [(1)](#1-more-on-kalman-filters) for more info.                                                                                                                                                                                                                                                                                                                                  |                          |
|        `"kalman"` - `"Q"`        | $2 \times 2$ matrix (see [(2)](#2-matrix-representation)) | The covariance matrix representing environmental uncertainty. This increases the uncertainty of the historical predictions over time. Without going into details, increasing the top left number will make the system more uncertain about the previously predicted position, whilst increasing the bottom right number will make it more uncertain about the previously predicted velocity.                                                                        | Instantly                |
|        `"kalman"` - `"R"`        | $2 \times 2$ matrix (see [(2)](#2-matrix-representation)) | The covariance matrix representing measurement uncertainty. This represents the uncertainty of the most recent measurements from the IMU, making the filter rely more on predictions based off historical data. Without going into details, increasing the top left number will make the system more uncertain about the position measured by the accelerometer, whilst increasing the bottom right number will make it more uncertain about the measured velocity. | Instantly                |
|         `"imuHowOften"`          |                          Integer                          | How often to save and transmit data from the IMU. `1` is every time.                                                                                                                                                                                                                                                                                                                                                                                                | Instantly                |
|          `"sleep-time"`          |                          Integer                          | The number of seconds after the last forwards rotation occurred to go into sleep mode to save power. Setting this to 0 disables sleep mode entirely. For safety reasons / reducing the pain to unbrick if set to too short a value, this value will not be updated if it is between 0 and 20 seconds (inclusive).                                                                                                                                                   | Instantly                |
| `"left-strain"` `"right-strain"` |                        JSON object                        | Calibration data for the strain gauges on each side. See [(3)](#3-converting-adc-values-to-torque) for more information on how these values are used to calculate torque.                                                                                                                                                                                                                                                                                           |                          |
|    `"*-strain"` - `"offset"`     |                  Unsigned 24 bit integer                  | Reading from the ADC when there is no torque applied. This is the 0 value.                                                                                                                                                                                                                                                                                                                                                                                          | Instantly                |
|     `"*-strain"` - `"coef"`      |                           float                           | This is the coeficient used to scale the ADC reading to obtain the torque in Nm from the raw ADC reading. It needs to have the correct sign depending on the wheatstone bridge wiring and side of the meter.                                                                                                                                                                                                                                                        | Instantly                |
|   `"*-strain"` - `"temp-test"`   |                           float                           | The temperature at which calibration occurred on that side.                                                                                                                                                                                                                                                                                                                                                                                                         |
|   `"*-strain"` - `"temp-coef"`   |                           float                           | The temperature coefficient.                                                                                                                                                                                                                                                                                                                                                                                                                                        | Instantly                |

### Notes
#### (1) More on Kalman filters
[This](https://www.bzarg.com/p/how-a-kalman-filter-works-in-pictures/) is a really useful website that nicely describes with pictures how a multidimensional Kalman filter works and can be implemented.

The main aims / reasoning of using a Kalman filter are:
- The accelerometer's angle / measurement of the direction of gravity is stable over long periods of time. It is unreliable over short periods due to factors like rocks, vibration and falling off a cliff.
- The gyroscope is highly accurate at measuring instantaneous angular velocity. Finding position from this requires integration, which accrues error and drift over time.
- The filter combines the short term accuracy of the gyroscope with long term stability of the accelerometer to produce a nice, relatively noise free output of angular position and angular velocity.
- 
#### (2) Matrix representation
$2 \times 2$ matrices like the one shown below are represented using 2 dimensional arrays (same way as in numpy).
$$
A = \left[\begin{matrix} a & b \\ c & d\end{matrix}\right]
$$
Will be translated to:
```json
{
    "A": [[a, b], [c, d]]
}
```

#### (3) Converting ADC values to torque
The current formula is:
$$
\tau (x, t) = S_c \times (x - S_o) \times (1 - T_c \times (t - T_o))
$$

##### Where <!-- omit in toc -->
- $\tau(x, t)$ is the calculated torque, $\tau$ for a given ADC reading, $x$ and temperature in Celcius, $t$.
- $S_c$ is the coefficient for the strain gauges and ADC (`"coef"`).
- $S_o$ is the offset (what the ADC resports when no torque is applied, `"offset"`).
- $T_c$ is the temperature coefficient (set to 0 to disable temperature compensation, `"temp-coef"`).
- $T_o$ is the temperature offset (temperature that the calibration to measure $S_c$ took place, `"temp-test"`).

## Obtaining the current config from the device
### Over MQTT
The config is published as part of a message on the [`/power/about`](../documents/mqtt_topics.md#about-this-device-powerabout) topic when the power meter connects to the broker.

### Through serial
1. Connect a USB cable and open a serial terminal.
2. Press `g` to print out the current config stored on the device. Make changes as necessary.
3. Typing `h` lists other potentially useful commands.

## Uploading new configs to the device
### Uploading over MQTT
These can be uploaded over MQTT using:
```
mosquitto_pub -t "/power/conf" -h "10.42.0.140" -f FirstPrototype_130.json
```

### Uploading over serial
They can also be uploaded over the serial port:
1. Connect a USB cable and open a serial terminal.
2. Press `g` to print out the current config stored on the device. Make changes as necessary.
3. Press `s` to set the config. Remove all new lines and paste the json document. Press enter if needed afterwards to apply. There is a 30 second time limit.
4. Confirm that the config is successfully set.