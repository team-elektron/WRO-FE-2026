#include "sensors_tof.h"
#include "shared_state.h"    
#include "pins.h"
#include <Wire.h>
#include <VL53L0X.h>
#include <Arduino.h>

#define ADDR_AUX    0x30
#define ADDR_LEFT   0x31
#define ADDR_RIGHT  0x32

//#define MEASURE_BUDGET_US  33000   // up from 20000 — better accuracy
//#define MEASURE_WAIT_MS    38      // must be > budget, up from 25

#define MEASURE_BUDGET_AUX_US   33000
#define MEASURE_WAIT_AUX_MS     38

#define MEASURE_BUDGET_SIDE_US  20000
#define MEASURE_WAIT_SIDE_MS    25

#define TOF_MAX_VALID_MM_AUX    1250
#define TOF_MAX_VALID_MM_SIDE   1200

// Median filter buffer — 3 samples per sensor
static uint16_t medBuf[3][3] = {};
static uint8_t  medIdx[3]    = {};

static uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
    if (a > b) { uint16_t t = a; a = b; b = t; }
    if (b > c) { uint16_t t = b; b = c; c = t; }
    if (a > b) { uint16_t t = a; a = b; b = t; }
    return b;
}

VL53L0X sensors[3];

const uint8_t xshutPins[3] = { 9, 7, 8 };           // AUX, LEFT, RIGHT
const uint8_t addresses[3] = { ADDR_AUX, ADDR_LEFT, ADDR_RIGHT };
const char*   names[3]     = { "Aux", "Left", "Right" };

bool     connected[3]   = { false, false, false };
uint16_t distances[3]   = { 9999, 9999, 9999 };
bool     triggered[3]   = { false, false, false };
uint32_t triggerTime[3] = { 0, 0, 0 };

static void shutdownAll() {
    // Hold Front (pin 6) down too so it never wakes
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    for (int i = 0; i < 3; i++) {
        pinMode(xshutPins[i], OUTPUT);
        digitalWrite(xshutPins[i], LOW);
    }
    delay(20);
}

static bool initSensor(int idx) {
    pinMode(xshutPins[idx], INPUT);
    delay(20);
    sensors[idx].setBus(&Wire);
    sensors[idx].setTimeout(500);

    WIRE_LOCK();
    bool ok = sensors[idx].init();
    WIRE_UNLOCK();

    if (!ok) {
        Serial.print("[FAIL] "); Serial.println(names[idx]);
        pinMode(xshutPins[idx], OUTPUT);
        digitalWrite(xshutPins[idx], LOW);
        return false;
    }

    bool isSide = (idx == TOF_LEFT || idx == TOF_RIGHT);
    uint32_t budget = isSide ? MEASURE_BUDGET_SIDE_US : MEASURE_BUDGET_AUX_US;

    WIRE_LOCK();
    sensors[idx].setAddress(addresses[idx]);
    sensors[idx].setSignalRateLimit(0.25);
    sensors[idx].setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange,   18);
    sensors[idx].setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
    sensors[idx].setMeasurementTimingBudget(budget);
    WIRE_UNLOCK();

    Serial.print("[OK] "); Serial.print(names[idx]);
    Serial.print(isSide ? " [HIGH SPEED]" : " [HIGH ACCURACY]");
    Serial.print(" @ 0x"); Serial.println(addresses[idx], HEX);
    return true;
}

static void triggerSensor(int idx) {
    if (!connected[idx]) return;
    WIRE_LOCK();
    sensors[idx].writeReg(VL53L0X::SYSRANGE_START, 0x01);
    WIRE_UNLOCK();
    triggered[idx]   = true;
    triggerTime[idx] = millis();
}

static void pollSensor(int idx) {
    if (!connected[idx] || !triggered[idx]) return;

    uint8_t waitMs = (idx == TOF_LEFT || idx == TOF_RIGHT)
                     ? MEASURE_WAIT_SIDE_MS
                     : MEASURE_WAIT_AUX_MS;

    if (millis() - triggerTime[idx] < waitMs) return;

    WIRE_LOCK();
    uint8_t status = sensors[idx].readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07;
    WIRE_UNLOCK();

    if (status == 0) return;

    WIRE_LOCK();
    uint16_t raw = sensors[idx].readReg16Bit(VL53L0X::RESULT_RANGE_STATUS + 10);
    sensors[idx].writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);
    WIRE_UNLOCK();

    triggered[idx] = false;

    uint16_t maxMm = (idx == TOF_AUX) ? TOF_MAX_VALID_MM_AUX : TOF_MAX_VALID_MM_SIDE;

    // Clamp to max instead of discarding — out of range reads as max distance
    uint16_t clamped = (raw < maxMm) ? raw : maxMm;

    medBuf[idx][medIdx[idx]] = clamped;
    medIdx[idx] = (medIdx[idx] + 1) % 3;
    distances[idx] = median3(medBuf[idx][0], medBuf[idx][1], medBuf[idx][2]);

    triggerSensor(idx);
}

// public API

void tof_init() {
    shutdownAll();
    for (int i = 0; i < 3; i++) {
        connected[i] = initSensor(i);
    }
    for (int i = 0; i < 3; i++) {
        triggerSensor(i);
    }
}

void tof_update() {
    for (int i = 0; i < 3; i++) {
        pollSensor(i);
    }
}

uint16_t tof_get(uint8_t idx) {
    if (idx >= 3) return 9999;
    return distances[idx];
}

bool tof_connected(uint8_t idx) {
    if (idx >= 3) return false;
    return connected[idx];
}