/*
> Relies on [LED testing](#1-led-testing) passing.

This script regularly polls the IMU and sets the colour of the LEDs based on the readings of the X axis accelerometer (tilt to change colour). The values read are also sent over the serial port.

This script is based off the [ICM42670P Arduino library `Polling_SPI`](https://github.com/tdk-invn-oss/motion.arduino.ICM42670P/blob/main/examples/Polling_SPI/Polling_SPI.ino) example.

 */
#include "ICM42670P.h"

// Accelerometer
#define PIN_ACCEL_INTERRUPT 38
#define PIN_SPI_SDO 39
#define PIN_SPI_SDI 40
#define PIN_SPI_SCLK 41
#define PIN_SPI_AC_CS 42
#define IMU_SAMPLE_RATE 800 // Options are 12, 25, 50, 100, 200, 400, 800, 1600 Hz (any other value defaults to 100 Hz).
#define IMU_ACCEL_RANGE 4   // Options are 2, 4, 8, 16 G (any other value defaults to 16 G).
#define IMU_GYRO_RANGE 1000 // Options are 250, 500, 1000, 2000 dps (any other value defaults to 2000 dps).

// Buttons and LEDs
#define PIN_LED1 8
#define PIN_LED2 9
#define PIN_BOOT 0

// Instantiate an ICM42670 with SPI interface and CS on pin 8
ICM42670 IMU(SPI,PIN_SPI_AC_CS);
void setup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED1, OUTPUT);
  setLEDState(true);
  Serial.begin(115200);
  SPI.begin(PIN_SPI_SCLK, PIN_SPI_SDO, PIN_SPI_SDI, PIN_SPI_AC_CS);
    int result = IMU.begin();
    if (result)
    {
        Serial.print("Cannot connect to IMU, error ");
        Serial.println(result);
        flashLEDS();
    }
  Serial.println("Accelerometer test started");
  int ret;
  // // Serial.begin(115200);
  // while(!Serial) {}

  // // Initializing the ICM42670
  // ret = IMU.begin();
  // if (ret != 0) {
  //   Serial.print("ICM42670 initialization failed: ");
  //   Serial.println(ret);
  //   while(1);
  // }
  // Accel ODR = 100 Hz and Full Scale Range = 16G
  IMU.startAccel(100,16);
  // Gyro ODR = 100 Hz and Full Scale Range = 2000 dps
  IMU.startGyro(100,2000);
  // Wait IMU to start
  delay(100);
}

void loop() {

  inv_imu_sensor_event_t imu_event;

  // Get last event
  IMU.getDataFromRegisters(imu_event);

  // Format data for Serial Plotter
  // Serial.print("AccelX:");
  Serial.print(imu_event.accel[0]);
  Serial.print(" ");
  Serial.print(imu_event.accel[1]);
  Serial.print(" ");
  Serial.print(imu_event.accel[2]);
  Serial.print(" ");
  Serial.print(imu_event.gyro[0]);
  Serial.print(" ");
  Serial.print(imu_event.gyro[1]);
  Serial.print(" ");
  Serial.print(imu_event.gyro[2]);
  Serial.print(" ");
  Serial.println(imu_event.temperature);

  // Set LEDs
  if (imu_event.accel[0] > 500)
  {
    setLEDState(true);
  }
  else if (imu_event.accel[0] < -500)
  {
    setLEDState(false);
  }

  // Run @ ODR 100Hz
  delay(10);
}

void setLEDState(bool state)
{
  digitalWrite(8, state);
  digitalWrite(9, !state);
}

void flashLEDS()
{
  while (1)
  {
    setLEDState(true);
  delay(500);
  setLEDState(false);
  delay(500);
  }
}