#pragma once

#include <stdint.h>

// Process Wii controller input
void WiiKeyboard_processInput(uint16_t currentButtons, uint16_t prevButtons);

// Map Wii buttons to GML key codes
int32_t WiiKeyboard_mapButton(uint16_t button);
