#pragma once

#include <stdint.h>
#include <stdbool.h>

// Process Wii controller input (Wiimote, Classic Controller, and USB Keyboard via libwiikeyboard)
void WiiKeyboard_processInput(uint16_t currentButtons, uint16_t prevButtons);

// Map Wii buttons to GML key codes (deprecated, use internal mapping)
int32_t WiiKeyboard_mapButton(uint16_t button);

// Initialize keyboard support (USB HID via libwiikeyboard and controller mappings)
void WiiKeyboard_init(void);

// Check if any keyboard input device is connected
bool WiiKeyboard_isConnected(void);
