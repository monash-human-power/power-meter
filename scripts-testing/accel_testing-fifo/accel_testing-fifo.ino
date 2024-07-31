/*
> Relies on [LED testing](#1-led-testing) and [Accelerometer polling](#2-accelerometer-polling) passing.

This script behaves the same as the polling script, however it uses the FIFO buffer and interrupts to obtain the data.

This script is based off the [ICM42670P Arduino library `FIFO_Interrupt`](https://github.com/tdk-invn-oss/motion.arduino.ICM42670P/blob/main/examples/FIFO_Interrupt/FIFO_Interrupt.ino) example.
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

uint8_t irq_received = 0;

void irq_handler(void) {
  irq_received = 1;
}

void setup() {
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
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
  // Enable interrupt on pin 2, Fifo watermark=10
  IMU.enableFifoInterrupt(PIN_ACCEL_INTERRUPT,irq_handler,10);

  // Accel ODR = 100 Hz and Full Scale Range = 16G
  IMU.startAccel(100,16);
  // Gyro ODR = 100 Hz and Full Scale Range = 2000 dps
  IMU.startGyro(100,2000);
  // Wait IMU to start
  delay(100);
}

void event_cb(inv_imu_sensor_event_t *evt) {
  // Format data for Serial Plotter
  if(IMU.isAccelDataValid(evt)&&IMU.isGyroDataValid(evt)) {
    Serial.print(evt->accel[0]);
    Serial.print(" ");
    Serial.print(evt->accel[1]);
    Serial.print(" ");
    Serial.print(evt->accel[2]);
    Serial.print(" ");
    Serial.print(evt->gyro[0]);
    Serial.print(" ");
    Serial.print(evt->gyro[1]);
    Serial.print(" ");
    Serial.print(evt->gyro[2]);
    Serial.print(" ");
    Serial.println(evt->temperature);

    // Set LEDs
    if (evt->accel[0] > 500)
    {
      setLEDState(true);
    }
    else if (evt->accel[0] < -500)
    {
      setLEDState(false);
    }
  }
}

void loop() {

  inv_imu_sensor_event_t imu_event;

  // Wait for interrupt to read data from fifo
  if(irq_received) {
      irq_received = 0;
      IMU.getDataFromFifo(event_cb);
  }
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