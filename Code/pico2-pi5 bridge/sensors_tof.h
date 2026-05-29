#pragma once
#include <stdint.h>

#define TOF_AUX    0
#define TOF_LEFT   1
#define TOF_RIGHT  2
#define TOF_COUNT  3

void     tof_init();
void     tof_update();
uint16_t tof_get(uint8_t idx);
bool     tof_connected(uint8_t idx);