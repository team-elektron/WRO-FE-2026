#include "sensors_imu.h"
#include "shared_state.h"    // for STATE_LOCK / STATE_UNLOCK
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

static Adafruit_MPU6050 mpu;
static bool     g_ready      = false;
static float    g_yaw        = 0.0f;
static float    g_pitch      = 0.0f;
static float    g_roll       = 0.0f;
static float    g_yawOffset  = 0.0f;
static float    g_yaw_out    = 0.0f;
static uint32_t g_lastTime   = 0;

static float g_bias_gx = 0.0f;
static float g_bias_gy = 0.0f;
static float g_bias_gz = 0.0f;

#define GYRO_DEADBAND_DEG   0.08f
#define STILL_ACCEL_BAND    0.04f
#define STILL_PAUSE_S       1.5f
#define YAW_LP_ALPHA        0.6f

static float g_last_accel_mag = 9.81f;
static float g_still_timer    = 0.0f;
static bool  g_still          = false;

static void calibrate_gyro() {
    Serial.println("[IMU] Calibrating gyro — keep robot still...");
    const int samples = 500;
    double gx_sum = 0, gy_sum = 0, gz_sum = 0;

    for (int i = 0; i < samples; i++) {
        sensors_event_t accel, gyro, temp;
        WIRE_LOCK();
        mpu.getEvent(&accel, &gyro, &temp);
        WIRE_UNLOCK();
        gx_sum += gyro.gyro.x;
        gy_sum += gyro.gyro.y;
        gz_sum += gyro.gyro.z;
        delay(4);
    }

    g_bias_gx = (float)(gx_sum / samples);
    g_bias_gy = (float)(gy_sum / samples);
    g_bias_gz = (float)(gz_sum / samples);

    Serial.printf("[IMU] Gyro bias: gx=%.4f gy=%.4f gz=%.4f (rad/s)\n",
                  g_bias_gx, g_bias_gy, g_bias_gz);
}

void imu_init() {
    if (!mpu.begin()) {
        Serial.println("[FAIL] MPU6050 not found");
        return;
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    calibrate_gyro();

    g_lastTime = millis();
    g_ready    = true;
    Serial.println("[OK] MPU6050 ready");
}

void imu_update() {
    if (!g_ready) return;

    uint32_t now = millis();
    float dt = (now - g_lastTime) / 1000.0f;
    if (dt <= 0.0f) return;
    g_lastTime = now;

    sensors_event_t accel, gyro, temp;
    WIRE_LOCK();
    mpu.getEvent(&accel, &gyro, &temp);
    WIRE_UNLOCK();

    float gx = (gyro.gyro.x - g_bias_gx) * 180.0f / M_PI;
    float gy = (gyro.gyro.y - g_bias_gy) * 180.0f / M_PI;
    float gz = (gyro.gyro.z - g_bias_gz) * 180.0f / M_PI;

    if (fabsf(gx) < GYRO_DEADBAND_DEG) gx = 0.0f;
    if (fabsf(gy) < GYRO_DEADBAND_DEG) gy = 0.0f;
    if (fabsf(gz) < GYRO_DEADBAND_DEG) gz = 0.0f;

    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;

    float accel_pitch = atan2f(ay, az) * 180.0f / M_PI;
    float accel_roll  = atan2f(-ax, az) * 180.0f / M_PI;

    float accel_mag   = sqrtf(ax*ax + ay*ay + az*az);
    float accel_delta = fabsf(accel_mag - g_last_accel_mag);
    g_last_accel_mag  = accel_mag;

    if (accel_delta < STILL_ACCEL_BAND && fabsf(gz) == 0.0f) {
        g_still_timer += dt;
    } else {
        g_still_timer = 0.0f;
        g_still       = false;
    }
    if (g_still_timer >= STILL_PAUSE_S) g_still = true;

    g_pitch = 0.98f * (g_pitch + gy * dt) + 0.02f * accel_pitch;
    g_roll  = 0.98f * (g_roll  + gx * dt) + 0.02f * accel_roll;

    if (!g_still) {
        g_yaw += gz * dt;
        while (g_yaw >  180.0f) g_yaw -= 360.0f;
        while (g_yaw < -180.0f) g_yaw += 360.0f;
    }

    float raw_out = g_yaw - g_yawOffset;
    while (raw_out >  180.0f) raw_out -= 360.0f;
    while (raw_out < -180.0f) raw_out += 360.0f;
    g_yaw_out = YAW_LP_ALPHA * raw_out + (1.0f - YAW_LP_ALPHA) * g_yaw_out;
}

bool  imu_ready()    { return g_ready; }
float imu_get_yaw()  { return g_yaw_out; }
float imu_get_pitch(){ return g_pitch; }
float imu_get_roll() { return g_roll; }

void imu_reset_yaw() {
    g_yawOffset = g_yaw;
    g_yaw_out   = 0.0f;
    Serial.println("[IMU] Yaw zeroed");
}