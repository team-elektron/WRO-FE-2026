#pragma once
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

void  imu_init();
void  imu_update();       // call every loop on Core 0, non-blocking
float imu_get_yaw();      // degrees, -180 to +180
float imu_get_pitch();
float imu_get_roll();
void  imu_reset_yaw();    // zero yaw at current heading
bool  imu_ready();
