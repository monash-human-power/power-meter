# Config files
These JSON files contain calibration data and settings for the power meters.
These can be uploaded over MQTT using:
```
mosquitto_pub -t "/power/conf" -h "10.42.0.140" -f FirstPrototype_130.json
```

They can also be uploaded over the serial port:
1. Connect a USB cable and open a serial terminal.
2. Press `g` to print out the current config stored on the device. Make changes as necessary.
3. Press `s` to set the config. Remove all new lines and paste the json document. Press enter if needed afterwards to apply. There is a 30 second time limit.
4. Confirm that the config is successfully set.