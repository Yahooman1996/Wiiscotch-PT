#include "runner_keyboard.h"
#include "../runner.h"
#include <ogc/pad.h>

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

int32_t WiiKeyboard_mapButton(uint16_t button) {
    for (int i = 0; padMappings[i].padButton != 0; i++) {
        if (button & padMappings[i].padButton) {
            return padMappings[i].gmlKey;
        }
    }
    return -1;
}

void WiiKeyboard_processInput(uint16_t currentButtons, uint16_t prevButtons) {
    // Handle button presses
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
    
    // Handle analog stick as arrow keys
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
