#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogc/pad.h>
#include <ogc/video.h>
#include <ogc/system.h>
#include <fat.h>

#include "runner.h"
#include "runner_keyboard.h"
#include "vm.h"
#include "../data_win.h"
#include "../json_reader.h"
#include "wii_file_system.h"
#include "wii_audio_system.h"
#include "gx_renderer.h"
#include "noop_audio_system.h"
#include "wii_utils.h"
#include "utils.h"

// Wii Memory Architecture:
// - MEM1: 24 MB (0x80000000 - 0x81800000) - Fast access, used for code and frequently accessed data
// - MEM2: 64 MB (0x90000000 - 0x94000000) - Slower access, used for textures, audio, and large buffers
// Total: 88 MB usable for applications

// Memory limits for allocation tracking
static const uint32_t MEM1_SIZE = 24 * 1024 * 1024;      // 24 MB MEM1
static const uint32_t MEM2_SIZE = 64 * 1024 * 1024;      // 64 MB MEM2
static const uint32_t MAX_MEMORY_BYTES = 88 * 1024 * 1024; // Total available

// Track memory usage separately for MEM1 and MEM2
static uint32_t mem1Used = 0;
static uint32_t mem2Used = 0;

// Controller button to GML key mapping
typedef struct {
    uint16_t padButton;
    int32_t gmlKey;
} PadMapping;

static PadMapping* padMappings = NULL;
static int padMappingCount = 0;

// Previous frame's button state for detecting press/release edges
static uint16_t prevButtons = 0;

// Global runner instance for keyboard input handling
Runner* g_runner = NULL;

// Video variables
static void* gp_fifo = NULL;
static GXRModeObj* vmode = NULL;

static void initVideo(void) {
    VIDEO_Init();
    vmode = VIDEO_GetPreferredMode(NULL);
    
    void* xfb = SYS_AllocateFramebuffer(vmode);
    VIDEO_SetPostRetraceCallback(NULL);
    VIDEO_Configure(vmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(TRUE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (vmode->viTVMode >> 2) {
        VIDEO_WaitVSync();
    }
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    
    // Initialize GX
    gp_fifo = memalign(32, 256 * 1024);
    memset(gp_fifo, 0, 256 * 1024);
    GX_Init(gp_fifo, 256 * 1024);
    GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
    GX_CopyDisp(xfb, GX_FALSE);
    GX_DrawDone();
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (vmode->viTVMode >> 2) {
        VIDEO_WaitVSync();
    }
}

static void initPad(void) {
    PAD_Init();
    PAD_ScanPads();
}

static uint16_t getPadButtons(void) {
    PAD_ScanPads();
    return PAD_ButtonsHeld(0);
}

int main(int argc, char* argv[]) {
    // Initialize video and input
    initVideo();
    initPad();
    
    // Initialize keyboard support (USB HID via manual implementation + controller mappings)
    WiiKeyboard_init();
    
    // Initialize FAT for file access
    if (!fatInitDefault()) {
        printf("Failed to initialize FAT filesystem\n");
        return 1;
    }
    
    // Initialize memory tracking utilities
    WiiUtils_init();
    
    printf("Wiiscotch - Starting...\n");
    WiiUtils_printMemoryStatus("Startup");
    
    // Determine data.win path
    const char* dataWinPath = "sd:/data.win";
    if (argc > 1) {
        dataWinPath = argv[1];
    }
    
    // Check if data.win exists
    FILE* testFile = fopen(dataWinPath, "rb");
    if (!testFile) {
        printf("Error: Could not find %s\n", dataWinPath);
        while (1) {
            VIDEO_WaitVSync();
        }
    }
    fclose(testFile);
    
    // ===[ Parse data.win ]===
    printf("Loading data.win from %s...\n", dataWinPath);
    
    DataWin* dataWin = DataWin_parse(
        dataWinPath,
        (DataWinParserOptions) {
            .parseGen8 = true,
            .parseOptn = true,
            .parseLang = true,
            .parseExtn = true,
            .parseSond = true,
            .parseAgrp = true,
            .parseSprt = true,
            .parseBgnd = true,
            .parsePath = true,
            .parseScpt = true,
            .parseGlob = true,
            .parseShdr = true,
            .parseFont = true,
            .parseTmln = true,
            .parseObjt = true,
            .parseRoom = true,
            .parseTpag = true,
            .parseCode = true,
            .parseVari = true,
            .parseFunc = true,
            .parseStrg = true,
            .parseTxtr = false,
            .parseAudo = false,
            .skipLoadingPreciseMasksForNonPreciseSprites = true,
            .progressCallback = NULL,
            .progressCallbackUserData = NULL,
        }
    );
    
    if (!dataWin) {
        printf("Failed to parse data.win!\n");
        while (1) {
            VIDEO_WaitVSync();
        }
    }
    
    printf("\ndata.win loaded successfully!\n");
    WiiUtils_printMemoryStatus("After data.win parsing");
    
    // ===[ Load CONFIG.JSN ]===
    printf("Loading CONFIG.JSN...\n");
    
    FILE* configFile = fopen("sd:/CONFIG.JSN", "rb");
    JsonValue* configRoot = NULL;
    
    if (configFile != NULL) {
        fseek(configFile, 0, SEEK_END);
        long configSize = ftell(configFile);
        fseek(configFile, 0, SEEK_SET);
        
        char* configJsonText = malloc(configSize + 1);
        size_t configBytesRead = fread(configJsonText, 1, configSize, configFile);
        configJsonText[configBytesRead] = '\0';
        fclose(configFile);
        
        configRoot = JsonReader_parse(configJsonText);
        free(configJsonText);
    }
    
    if (configRoot == NULL) {
        printf("CONFIG.JSN invalid or not found! Using defaults.\n");
        // Create minimal default config
        configRoot = JsonReader_parse("{\"fileSystem\":{\"savePath\":\"sd:/saves\"}}");
    }
    
    // ===[ Create renderer ]===
    printf("Creating renderer...\n");
    Renderer* renderer = GxRenderer_create(vmode);
    
    // ===[ Create VM and runner ]===
    printf("Creating VM and runner...\n");
    
    FileSystem* fileSystem = WiiFileSystem_create(configRoot, dataWin->gen8.displayName);
    if (fileSystem == NULL) {
        printf("Failed to create file system!\n");
        while (1) {
            VIDEO_WaitVSync();
        }
    }
    
    VMContext* vm = VM_create(dataWin);
    Runner* runner = Runner_create(dataWin, vm, fileSystem);
    g_runner = runner;  // Set global runner for keyboard input handling
    
    WiiUtils_printMemoryStatus("After VM and runner creation");
    
    // Set up controller mappings
    printf("Initializing controller...\n");
    padMappings = NULL;
    padMappingCount = 0;
    
    // Default Wii controller mappings (will be expanded)
    // A button -> vk_space
    // B button -> vk_shift
    // etc.
    
    printf("Starting game loop...\n");
    
    // Main game loop
    while (1) {
        uint16_t currentButtons = getPadButtons();
        
        // Process input
        WiiKeyboard_processInput(currentButtons, prevButtons);
        
        // Update game
        Runner_step(runner);
        
        // Render - call renderer's draw methods through function pointers if needed
        // For now, we'll just wait for vsync as the renderer is called via callbacks
        
        prevButtons = currentButtons;
        
        VIDEO_WaitVSync();
    }
    
    // ralsei,,, 🤤🤤🤤
    Runner_free(runner);
    VM_free(vm);
    DataWin_free(dataWin);
    
    WiiUtils_printMemoryStatus("Shutdown");
    WiiUtils_shutdown();
    
    return 0;
}
