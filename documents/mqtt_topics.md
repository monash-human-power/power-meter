# MQTT Topics <!-- omit in toc -->
When running in WiFi and MQTT mode, the power meter publishes data to several topics. All topics are prefixed by `/power/` to distinguish data sent to and from the power meter with other traffic on the broker.

- [Published topics](#published-topics)
  - [About this device (`/power/about`)](#about-this-device-powerabout)
    - [Example](#example)
    - [Key value pairs](#key-value-pairs)
  - [Housekeeping data (`/power/housekeeping`)](#housekeeping-data-powerhousekeeping)
    - [Example](#example-1)
    - [Key value pairs](#key-value-pairs-1)
  - [Slow speed data (`/power/power`)](#slow-speed-data-powerpower)
    - [Example](#example-2)
    - [Key value pairs](#key-value-pairs-2)
  - [High speed IMU data (`/power/imu`)](#high-speed-imu-data-powerimu)
    - [Record format](#record-format)
  - [High speed strain gauge data (`/power/fast/left`, `/power/fast/right`)](#high-speed-strain-gauge-data-powerfastleft-powerfastright)
    - [Record format](#record-format-1)
- [Subscribed topics](#subscribed-topics)
  - [Set a new configurration (`/power/conf`)](#set-a-new-configurration-powerconf)
  - [Calculate and apply new offsets on the strain gauge ADCs (`/power/offset`)](#calculate-and-apply-new-offsets-on-the-strain-gauge-adcs-poweroffset)
    - [Example usage](#example-usage)
  - [Save configs (`/power/save`)](#save-configs-powersave)
  - [Reboot (`/power/reboot`)](#reboot-powerreboot)
  - [Enter sleep mode (`/power/sleep`)](#enter-sleep-mode-powersleep)
- [Additional notes](#additional-notes)


## Published topics
These topics contain data from the power meter. Messages sent less frequently are formatted using JSON for easier reading and processing. Messages containing high frequency data (IMU and strain gauges) are packed into bytes.

### About this device (`/power/about`)
This message is sent whenever the power meter connects to the broker. It contains information relating to the software and hardware versions, as well as the current configurrations.

#### Example
An example message after pretty-printing is:
```json
{
    "name": "Power meter prototype",
    "compiled": "Sep 30 2024, 22:38:28",
    "sw_version": "s0.1.0",
    "hw_version": "v1.1.1",
    "connect-time": 7336,
    "calibration": {
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
    },
    "mac": "XX:XX:XX:XX:XX:XX"
}
```

#### Key value pairs
|       Key        |        Data type        | Description                                                                                                                                                            |
| :--------------: | :---------------------: | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|     `"name"`     |         string          | The device name, set in [constants.h](../power-meter-code/src/constants.example.h).                                                                                    |
|   `"compiled"`   |         string          | The date and time that the firmware was compiled according to the computer that did the compiling.                                                                     |
|  `"sw_version"`  |         string          | The firmware version (set in [defines.h](../power-meter-code/src/defines.h)).                                                                                          |
|  `"hw_version"`  |         string          | The targeted hardware version, set in [constants.h](../power-meter-code/src/constants.example.h).                                                                      |
| `"connect-time"` | unsigned 32 bit integer | The number of microseconds since the microcontroller was reset. This rills over approximately every hour.                                                              |
| `"calibration"`  |       JSON object       | The current config value loaded into memory. See the [`/power/conf`](#set-a-new-configurration-powerconf) topic or [this page](../configs/README.md) for more details. |
|     `"mac"`      |         string          | The MAC address of the microcontroller in the power meter. This is aking to a serial number and is useful for identifying which power meter recorded the data.         |

### Housekeeping data (`/power/housekeeping`)
This message is sent once every few seconds and contains information on the device health and environmental conditions. It is designed for information that updates faster than that sent in the [about message](#about-this-device-powerabout), but is not time critical like that sent in the power and high frequency messages.

#### Example
An example message after pretty-printing is:
```json
{
    "temps": {
        "left": 24.00,
        "right": -1000.00,
        "imu": 35.00
    },
    "battery": 3299.00,
    "left-offset": 9848390,
    "right-offset": 6252516
}
```

#### Key value pairs
|                Key                |        Data type        | Description                                                                                                                                                                                                                         |
| :-------------------------------: | :---------------------: | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|             `"temps"`             |       JSON object       | This field contains the temperatures for each side and that reported by the IMU. All values within this are floating point in Celcius. A value of `-1000.00` represents not being able to successfully communicate with the sensor. |
|            `"battery"`            |          float          | The battery voltage in mV.                                                                                                                                                                                                          |
| `"left-offset"`, `"right-offset"` | unsigned 32 bit integer | The current offsets being used by each ADC. This value represents 0 torque on each side.                                                                                                                                            |

### Slow speed data (`/power/power`)
This message contains information averaged over the last complete rotation. This message is sent every rotation or every few seconds, whichever comes sooner. Most of the useful data that the riders care about mid-ride will come from this message.

#### Example
An example message after pretty-printing is:
```json
{
    "timestamp": 311280713,
    "cadence": 70.1,
    "rotations": 185,
    "power": 390.6,
    "balance": 50.8
}
```

#### Key value pairs
|      Key      |        Data type        | Description                                                                                                                                                                                                                                                                   |
| :-----------: | :---------------------: | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"timestamp"` | unsigned 32 bit integer | The system time in microseconds when the last rotation occurred.                                                                                                                                                                                                              |
|  `"cadence"`  |          float          | The average cadence over the last complete rotation in RPM.                                                                                                                                                                                                                   |
| `"rotations"` | unsigned 32 bit integer | The total number of rotations since the device was reset.                                                                                                                                                                                                                     |
|   `"power"`   |          float          | The power over the last rotation in W. If this message is being sent according to a schedule rather than a rotation completing, this will be 0.                                                                                                                               |
|  `"balance"`  |          float          | The pedal balance in percent. A value of 50% means that both sides are recording equal power. A value of 0% means that all power is being generated by one side and a value of 100% means all power is being generated by the other side. TODO: Work out which side is which. |

### High speed IMU data (`/power/imu`)
This message contains multiple records of the 100Hz sampled IMU data. Because a lot of this data needs to be sent over the network, structures of bytes are used rather than converting to ASCII and JSON formats.

Each message is comprised of many individual records. The oldest records are arranged at the start of the message, whilst the most recent are at the back. Each message will be a multiple of the size of each record's length. The number of records per message is set in the [configurration](../configs/README.md).
| Byte offset in record |        Data type        | Size in bytes | 

#### Record format
| Byte offset in record |        Data type        | Size in bytes | Description                                                                                                                                                        |
| :-------------------: | :---------------------: | :-----------: | :----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|           0           | unsigned 32 bit integer |       4       | The device time in microseconds when the sample was captured.                                                                                                      |
|           4           |          float          |       4       | The angular velocity in radians per second of the power meter. This is an output from the on-board Kalman filter.                                                  |
|           8           |          float          |       4       | The position in radians of the power meter. This is an output from the on-board Kalman filter.                                                                     |
|          12           |          float          |       4       | The acceleration in m/s^2 for the X axis. This has been corrected for centripedal forces caused by the IMU not being at the center of rotation of the power meter. |
|          16           |          float          |       4       | The acceleration in m/s^2 for the Y axis. This has been corrected for centripedal forces caused by the IMU not being at the center of rotation of the power meter. |
|          20           |          float          |       4       | The acceleration in m/s^2 for the Z axis.                                                                                                                          |
|          24           |          float          |       4       | The angular velocity of the X axis in radians per second as measured by the gyroscope.                                                                             |
|          28           |          float          |       4       | The angular velocity of the Y axis in radians per second as measured by the gyroscope.                                                                             |
|          32           |          float          |       4       | The angular velocity of the Z axis in radians per second as measured by the gyroscope.                                                                             |


### High speed strain gauge data (`/power/fast/left`, `/power/fast/right`)
This message contains multiple records of the 80Hz sampled strain gauge data for a side. Because a lot of this data needs to be sent over the network, structures of bytes are used rather than converting to ASCII and JSON formats.

Each message is comprised of many individual records. The oldest records are arranged at the start of the message, whilst the most recent are at the back. Each message will be a multiple of the size of each record's length. The number of records per message is set in the [configurration](../configs/README.md).

#### Record format
| Byte offset in record |        Data type        | Size in bytes | Description                                                                                                                                                     |
| :-------------------: | :---------------------: | :-----------: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|           0           | unsigned 32 bit integer |       4       | The device time in microseconds when the sample was captured.                                                                                                   |
|           4           |          float          |       4       | The angular velocity in radians per second of the power meter. This is an output from the on-board Kalman filter that has been predicted at the current time.   |
|           8           |          float          |       4       | The position in radians of the power meter. This is an output from the on-board Kalman filter that has been predicted at the current time.                      |
|          12           | unsigned 24 bit integer |       4       | The raw reading from the ADC. Whilst the output from the ADCs is a 24 bit number, the value is being sent as an unsigned 32 bit integer to simplify processing. |
|          16           |          float          |       4       | The calculated torque in Nm.                                                                                                                                    |
|          20           |          float          |       4       | The calculate power in W. This is the torque multiplied by the angular velocity.                                                                                |

## Subscribed topics
### Set a new configurration (`/power/conf`)
See [here](../configs/README.md) for more information on the message format and alternative ways to set configs.

### Calculate and apply new offsets on the strain gauge ADCs (`/power/offset`)
Messages sent to this topic will cause the device to take many samples from the ADC, average them and use them as the offset from now on. These offsets are not stored in flash memory and will be restored to the defaults or those loaded into flash memory upon reboot.

#### Example usage
```bash
mosquitto_pub -t "/power/offset" -m ""
```

### Save configs (`/power/save`)
*Not implemented yet.*

### Reboot (`/power/reboot`)
This would be useful to load new configs / start from a defined base.
*Not implemented yet.*

### Enter sleep mode (`/power/sleep`)
This would be useful to shut the system down and save power manually. It would also be useful to cleanly disconnect from the current connection so that the router / device managing the connection doesn't throw a wobbly if power is removed and reapplied with no warning.

*Not implemented yet.*

## Additional notes
- Integers are sent using little endian.
- Floats are single precision for improved processing performance. They are stored and sent in the IEEE 754 binary32 format (native on the ESP32 and most computers).