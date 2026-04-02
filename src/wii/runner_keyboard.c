#include "runner_keyboard.h"
#include "../runner.h"
#include <ogc/pad.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// USB Keyboard support is deprecated/removed in modern libogc.
// We rely on Wii Remote (WPAD) and GameCube Controller (PAD) instead.

// Wii controller button mappings to GML key codes
typedef struct {
    uint16_t padButton;
    int32_t gmlKey;
} PadMapping;

static PadMapping padMappings[] = {
    { PAD_BUTTON_A, 32 },      // Space
    { PAD_BUTTON_B, 304 },     // Shift
    { PAD_BUTTON_X, 90 },      // Z
    { PAD_BUTTON_Y, 88 },      // X
    { PAD_BUTTON_START, 13 },  // Enter
    { PAD_BUTTON_UP, 273 },    // Up arrow
    { PAD_BUTTON_DOWN, 274 },  // Down arrow
    { PAD_BUTTON_LEFT, 276 },  // Left arrow
    { PAD_BUTTON_RIGHT, 275 }, // Right arrow
    { PAD_TRIGGER_L, 306 },    // Ctrl
    { PAD_TRIGGER_R, 308 },    // Alt
    { 0, 0 }
};

// Classic Controller button mappings (expanded)
static PadMapping classicMappings[] = {
    { PAD_CLASSIC_BUTTON_A, 32 },      // Space
    { PAD_CLASSIC_BUTTON_B, 304 },     // Shift
    { PAD_CLASSIC_BUTTON_X, 90 },      // Z
    { PAD_CLASSIC_BUTTON_Y, 88 },      // X
    { PAD_CLASSIC_BUTTON_PLUS, 13 },   // Enter
    { PAD_CLASSIC_BUTTON_UP, 273 },    // Up arrow
    { PAD_CLASSIC_BUTTON_DOWN, 274 },  // Down arrow
    { PAD_CLASSIC_BUTTON_LEFT, 276 },  // Left arrow
    { PAD_CLASSIC_BUTTON_RIGHT, 275 }, // Right arrow
    { PAD_CLASSIC_BUTTON_ZL, 306 },    // Ctrl
    { PAD_CLASSIC_BUTTON_ZR, 308 },    // Alt
    { PAD_CLASSIC_BUTTON_HOME, 27 },   // Escape
    { PAD_CLASSIC_BUTTON_MINUS, 8 },   // Backspace
    { 0, 0 }
};

// USB Keyboard scan code to GML key mappings (subset of common keys)
static const uint16_t usbKeyToGml[] = {
    [0x04] = 65,   // A
    [0x05] = 66,   // B
    [0x06] = 67,   // C
    [0x07] = 68,   // D
    [0x08] = 69,   // E
    [0x09] = 70,   // F
    [0x0A] = 71,   // G
    [0x0B] = 72,   // H
    [0x0C] = 73,   // I
    [0x0D] = 74,   // J
    [0x0E] = 75,   // K
    [0x0F] = 76,   // L
    [0x10] = 77,   // M
    [0x11] = 78,   // N
    [0x12] = 79,   // O
    [0x13] = 80,   // P
    [0x14] = 81,   // Q
    [0x15] = 82,   // R
    [0x16] = 83,   // S
    [0x17] = 84,   // T
    [0x18] = 85,   // U
    [0x19] = 86,   // V
    [0x1A] = 87,   // W
    [0x1B] = 88,   // X
    [0x1C] = 89,   // Y
    [0x1D] = 90,   // Z
    [0x1E] = 49,   // 1
    [0x1F] = 50,   // 2
    [0x20] = 51,   // 3
    [0x21] = 52,   // 4
    [0x22] = 53,   // 5
    [0x23] = 54,   // 6
    [0x24] = 55,   // 7
    [0x25] = 56,   // 8
    [0x26] = 57,   // 9
    [0x27] = 48,   // 0
    [0x28] = 13,   // Enter
    [0x29] = 27,   // Escape
    [0x2A] = 8,    // Backspace
    [0x2B] = 9,    // Tab
    [0x2C] = 32,   // Space
    [0x2D] = 45,   // Minus
    [0x2E] = 61,   // Equal
    [0x2F] = 91,   // Left Bracket
    [0x30] = 93,   // Right Bracket
    [0x31] = 92,   // Backslash
    [0x33] = 59,   // Semicolon
    [0x34] = 39,   // Quote
    [0x35] = 96,   // Grave
    [0x36] = 44,   // Comma
    [0x37] = 46,   // Period
    [0x38] = 47,   // Slash
    [0x39] = 304,  // Left Shift
    [0x3A] = 304,  // Right Shift
    [0xE0] = 306,  // Left Control
    [0xE4] = 306,  // Right Control
    [0xE2] = 308,  // Left Alt
    [0xE6] = 308,  // Right Alt
    [0x52] = 273,  // Up Arrow
    [0x51] = 274,  // Down Arrow
    [0x50] = 276,  // Left Arrow
    [0x4F] = 275,  // Right Arrow
};

void WiiKeyboard_init(void) {
    // USB Keyboard initialization removed - not supported in modern libogc
    // GameCube and Wii Remote controllers are initialized in main.c
}

bool WiiKeyboard_isConnected(void) {
    // Always return true since we have GameCube controller support
    return true;
}

static int32_t mapPadButton(uint16_t button, bool isClassic) {
    PadMapping* mappings = isClassic ? classicMappings : padMappings;
    for (int i = 0; mappings[i].padButton != 0; i++) {
        if (button & mappings[i].padButton) {
            return mappings[i].gmlKey;
        }
    }
    return -1;
}

static int32_t mapUsbKey(uint8_t usbCode) {
    if (usbCode < sizeof(usbKeyToGml) / sizeof(usbKeyToGml[0])) {
        return usbKeyToGml[usbCode];
    }
    return -1;
}

void WiiKeyboard_processInput(uint16_t currentButtons, uint16_t prevButtons) {
    // Detect controller type (Classic Controller or standard Wiimote)
    uint32_t expType = PAD_Expansion(0);
    bool isClassic = (expType == PAD_EXP_CLASSIC);
    
    // Handle button presses for gamepad
    uint16_t pressed = currentButtons & ~prevButtons;
    uint16_t released = ~currentButtons & prevButtons;
    
    for (int i = 0; padMappings[i].padButton != 0; i++) {
        if (pressed & padMappings[i].padButton) {
            Runner_keyPressed(padMappings[i].gmlKey);
        }
        if (released & padMappings[i].padButton) {
            Runner_keyReleased(padMappings[i].gmlKey);
        }
    }
    
    // Handle Classic Controller buttons if connected
    if (isClassic) {
        for (int i = 0; classicMappings[i].padButton != 0; i++) {
            if (pressed & classicMappings[i].padButton) {
                Runner_keyPressed(classicMappings[i].gmlKey);
            }
            if (released & classicMappings[i].padButton) {
                Runner_keyReleased(classicMappings[i].gmlKey);
            }
        }
        
        // Handle Classic Controller analog sticks
        Stick_t stick = PAD_ClassicStick(0);
        
        // Left stick X axis
        if (stick.x < -30) {
            Runner_keyDown(276); // Left
        } else if (stick.x > 30) {
            Runner_keyDown(275); // Right
        }
        
        // Left stick Y axis
        if (stick.y < -30) {
            Runner_keyDown(274); // Down
        } else if (stick.y > 30) {
            Runner_keyDown(273); // Up
        }
        
        // Right stick (optional support)
        Stick_t rightStick = PAD_ClassicRightStick(0);
        if (rightStick.x < -30) {
            Runner_keyDown(276); // Left
        } else if (rightStick.x > 30) {
            Runner_keyDown(275); // Right
        }
        if (rightStick.y < -30) {
            Runner_keyDown(274); // Down
        } else if (rightStick.y > 30) {
            Runner_keyDown(273); // Up
        }
    } else {
        // Handle standard Wiimote analog stick (if available via Nunchuk)
        Stick_t stick = PAD_Stick(0);
        
        // Left stick X axis
        if (stick.x < -30) {
            Runner_keyDown(276); // Left
        } else if (stick.x > 30) {
            Runner_keyDown(275); // Right
        }
        
        // Left stick Y axis
        if (stick.y < -30) {
            Runner_keyDown(274); // Down
        } else if (stick.y > 30) {
            Runner_keyDown(273); // Up
        }
    }
    
    // Handle USB Keyboard input
    if (USB_IsKeyboardConnected()) {
        static uint8_t prevKeys[6] = {0};
        uint8_t currentKeys[6];
        uint8_t modifiers;
        
        if (USB_GetKeys(currentKeys, &modifiers) == 0) {
            // Check for newly pressed keys
            for (int i = 0; i < 6; i++) {
                if (currentKeys[i] != 0 && currentKeys[i] != prevKeys[i]) {
                    int32_t gmlKey = mapUsbKey(currentKeys[i]);
                    if (gmlKey >= 0) {
                        Runner_keyPressed(gmlKey);
                        Runner_keyDown(gmlKey);
                    }
                }
            }
            
            // Check for released keys
            for (int i = 0; i < 6; i++) {
                if (prevKeys[i] != 0 && prevKeys[i] != currentKeys[i]) {
                    int32_t gmlKey = mapUsbKey(prevKeys[i]);
                    if (gmlKey >= 0) {
                        Runner_keyReleased(gmlKey);
                    }
                }
            }
            
            // Copy current keys to previous
            memcpy(prevKeys, currentKeys, 6);
        }
    }
}
