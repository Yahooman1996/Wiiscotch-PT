#include "runner_keyboard.h"
#include "../runner.h"
#include <ogc/pad.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Keyboard input handling for Wii using libwiikeyboard (part of libogc)
// Handles USB Keyboard via libwiikeyboard and GameCube/Wii controllers via PAD/WPAD

extern Runner* g_runner;  // Global runner instance for accessing keyboard state

// Convert libwiikeyboard keycode to GML key code
static int32_t convertKeycodeToGml(uint8_t key) {
    // Direct ASCII characters (printable)
    if (key >= 32 && key <= 126) {
        return key;
    }
    
    // Special keys based on libwiikeyboard keycodes
    switch (key) {
        case 13:  return 13;   // Enter
        case 27:  return 27;   // Escape
        case 8:   return 8;    // Backspace
        case 9:   return 9;    // Tab
        case 127: return 46;   // Delete
        case 128: return 273;  // Up arrow
        case 129: return 274;  // Down arrow
        case 130: return 276;  // Left arrow
        case 131: return 275;  // Right arrow
        case 132: return 278;  // Insert
        case 133: return 279;  // Home
        case 134: return 280;  // End
        case 135: return 281;  // Page Up
        case 136: return 282;  // Page Down
        case 137: return 112;  // F1
        case 138: return 113;  // F2
        case 139: return 114;  // F3
        case 140: return 115;  // F4
        case 141: return 116;  // F5
        case 142: return 117;  // F6
        case 143: return 118;  // F7
        case 144: return 119;  // F8
        case 145: return 120;  // F9
        case 146: return 121;  // F10
        case 147: return 122;  // F11
        case 148: return 123;  // F12
        case 149: return 304;  // Left Shift
        case 150: return 304;  // Right Shift
        case 151: return 306;  // Left Control
        case 152: return 306;  // Right Control
        case 153: return 308;  // Left Alt
        case 154: return 308;  // Right Alt
        default:  return -1;   // Unknown key
    }
}

void WiiKeyboard_init(void) {
    // Initialize USB keyboard using libwiikeyboard
    KEYBOARD_Init(NULL);
    printf("USB Keyboard initialized via libwiikeyboard\n");
    // GameCube and Wii Remote controllers are initialized in main.c
}

bool WiiKeyboard_isConnected(void) {
    // libwiikeyboard doesn't provide a direct connection check
    // Assume connected if initialized successfully
    return true;
}

void WiiKeyboard_processInput(uint16_t currentButtons, uint16_t prevButtons) {
    RunnerKeyboardState* kb = g_runner->keyboard;
    
    // Process USB keyboard input via libwiikeyboard
    keyboard_event ke;
    s32 res = KEYBOARD_GetEvent(&ke);
    while (res) {
        if (ke.type == KEYBOARD_PRESSED) {
            int32_t gmlKey = convertKeycodeToGml(ke.keycode);
            if (gmlKey > 0) {
                RunnerKeyboard_onKeyDown(kb, gmlKey);
            }
        } else if (ke.type == KEYBOARD_RELEASED) {
            int32_t gmlKey = convertKeycodeToGml(ke.keycode);
            if (gmlKey > 0) {
                RunnerKeyboard_onKeyUp(kb, gmlKey);
            }
        }
        res = KEYBOARD_GetEvent(&ke);
    }
    
    // Handle button presses for GameCube controller
    uint16_t pressed = currentButtons & ~prevButtons;
    uint16_t released = ~currentButtons & prevButtons;
    
    // Standard GameCube/Wiimote button mappings
    static const struct { uint16_t button; int32_t gmlKey; } padMappings[] = {
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
    
    for (int i = 0; padMappings[i].button != 0; i++) {
        if (pressed & padMappings[i].button) {
            RunnerKeyboard_onKeyDown(kb, padMappings[i].gmlKey);
        }
        if (released & padMappings[i].button) {
            RunnerKeyboard_onKeyUp(kb, padMappings[i].gmlKey);
        }
    }
    
    // Check for Classic Controller attachment
    WPAD_ScanPads();
    struct expansion_t exp;
    WPAD_Expansion(0, &exp);
    bool isClassic = (exp.type == WPAD_EXP_CLASSIC);
    
    if (isClassic) {
        // Classic Controller button mappings - use u32 for buttons
        static const struct { u32 button; int32_t gmlKey; } classicMappings[] = {
            { WPAD_CLASSIC_BUTTON_A, 32 },      // Space
            { WPAD_CLASSIC_BUTTON_B, 304 },     // Shift
            { WPAD_CLASSIC_BUTTON_X, 90 },      // Z
            { WPAD_CLASSIC_BUTTON_Y, 88 },      // X
            { WPAD_CLASSIC_BUTTON_PLUS, 13 },   // Enter
            { WPAD_CLASSIC_BUTTON_UP, 273 },    // Up arrow
            { WPAD_CLASSIC_BUTTON_DOWN, 274 },  // Down arrow
            { WPAD_CLASSIC_BUTTON_LEFT, 276 },  // Left arrow
            { WPAD_CLASSIC_BUTTON_RIGHT, 275 }, // Right arrow
            { WPAD_CLASSIC_BUTTON_ZL, 306 },    // Ctrl
            { WPAD_CLASSIC_BUTTON_ZR, 308 },    // Alt
            { WPAD_CLASSIC_BUTTON_HOME, 27 },   // Escape
            { WPAD_CLASSIC_BUTTON_MINUS, 8 },   // Backspace
            { 0, 0 }
        };
        
        // Get WPAD data with correct type
        WPADData* wdata = WPAD_Data(0);
        u32 classicPressed = wdata->btns.held & ~wdata->btns.prev;
        u32 classicReleased = ~wdata->btns.held & wdata->btns.prev;
        
        for (int i = 0; classicMappings[i].button != 0; i++) {
            if (classicPressed & classicMappings[i].button) {
                RunnerKeyboard_onKeyDown(kb, classicMappings[i].gmlKey);
            }
            if (classicReleased & classicMappings[i].button) {
                RunnerKeyboard_onKeyUp(kb, classicMappings[i].gmlKey);
            }
        }
        
        // Handle Classic Controller analog sticks
        if (wdata) {
            // Left stick
            if (wdata->exp.classic.ljs.x < -30) {
                RunnerKeyboard_onKeyDown(kb, 276); // Left
            } else if (wdata->exp.classic.ljs.x > 30) {
                RunnerKeyboard_onKeyDown(kb, 275); // Right
            }
            
            if (wdata->exp.classic.ljs.y < -30) {
                RunnerKeyboard_onKeyDown(kb, 274); // Down
            } else if (wdata->exp.classic.ljs.y > 30) {
                RunnerKeyboard_onKeyDown(kb, 273); // Up
            }
            
            // Right stick
            if (wdata->exp.classic.rjs.x < -30) {
                RunnerKeyboard_onKeyDown(kb, 276); // Left
            } else if (wdata->exp.classic.rjs.x > 30) {
                RunnerKeyboard_onKeyDown(kb, 275); // Right
            }
            
            if (wdata->exp.classic.rjs.y < -30) {
                RunnerKeyboard_onKeyDown(kb, 274); // Down
            } else if (wdata->exp.classic.rjs.y > 30) {
                RunnerKeyboard_onKeyDown(kb, 273); // Up
            }
        }
    } else {
        // Handle Nunchuk analog stick if attached
        struct expansion_t nunchukExp;
        WPAD_Expansion(0, &nunchukExp);
        if (nunchukExp.type == WPAD_EXP_NUNCHUK) {
            WPADData* wdata = WPAD_Data(0);
            if (wdata) {
                if (wdata->exp.nunchuk.js.x < -30) {
                    RunnerKeyboard_onKeyDown(kb, 276); // Left
                } else if (wdata->exp.nunchuk.js.x > 30) {
                    RunnerKeyboard_onKeyDown(kb, 275); // Right
                }
                
                if (wdata->exp.nunchuk.js.y < -30) {
                    RunnerKeyboard_onKeyDown(kb, 274); // Down
                } else if (wdata->exp.nunchuk.js.y > 30) {
                    RunnerKeyboard_onKeyDown(kb, 273); // Up
                }
            }
        }
    }
}
