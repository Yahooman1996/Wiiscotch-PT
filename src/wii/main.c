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

// Maximum memory available on Wii (approximately 88MB usable for applications)
static const uint32_t MAX_MEMORY_BYTES = 88 * 1024 * 1024;

// Controller button to GML key mapping
typedef struct {
    uint16_t padButton;
    int32_t gmlKey;
} PadMapping;

static PadMapping* padMappings = NULL;
static int padMappingCount = 0;

// Previous frame's button state for detecting press/release edges
static uint16_t prevButtons = 0;

// Video variables
static void* gp_fifo = NULL;
static GXRModeObj* vmode = NULL;

// ===[ Loading Screen ]===

#define MAX_CHUNK_STATS 24

typedef struct {
    char label[16];
    uint32_t count;
} ChunkStat;

typedef struct {
    ChunkStat stats[MAX_CHUNK_STATS];
    int statCount;
} LoadingScreenState;

static void drawCreditsText(GXRModeObj* vmode, float yPos, const char* text) {
    // Placeholder - actual implementation would use GX or libwiigui
    printf("%s\n", text);
}

static void loadingScreenCallback(const char* chunkName, int chunkIndex, int totalChunks, DataWin* dataWin, void* userData) {
    LoadingScreenState* state = (LoadingScreenState*) userData;
    
    const char* gameName = dataWin->gen8.displayName ? dataWin->gen8.displayName : "Unknown Game";
    
    // Update progress display
    float progress = (float)(chunkIndex + 1) / (float)totalChunks;
    printf("\rLoading %s... %d/%d (%.0f%%)", chunkName, chunkIndex + 1, totalChunks, progress * 100);
    fflush(stdout);
    
    // Record item counts for already-parsed chunks
    typedef struct { uint32_t* countPtr; const char* label; } CountSource;
    CountSource sources[] = {
        { &dataWin->sond.count, "sounds" },
        { &dataWin->sprt.count, "sprites" },
        { &dataWin->bgnd.count, "backgrounds" },
        { &dataWin->font.count, "fonts" },
        { &dataWin->objt.count, "objects" },
        { &dataWin->room.count, "rooms" },
        { &dataWin->code.count, "code entries" },
        { &dataWin->txtr.count, "textures" },
    };
    
    int arrayLength = sizeof(sources) / sizeof(CountSource);
    
    for (int i = 0; i < arrayLength; i++) {
        if (*sources[i].countPtr == 0)
            continue;
        
        // Check if we already recorded this label
        bool found = false;
        for (int j = 0; j < state->statCount; j++) {
            if (strcmp(state->stats[j].label, sources[i].label) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found && MAX_CHUNK_STATS > state->statCount) {
            ChunkStat* stat = &state->stats[state->statCount++];
            snprintf(stat->label, sizeof(stat->label), "%s", sources[i].label);
            stat->count = *sources[i].countPtr;
        }
    }
}

static void initVideo(void) {
    VIDEO_Init();
    vmode = VIDEO_GetPreferredMode(NULL);
    
    void*xfb = MEM_K0_TO_K1(MEM_AllocFramebuffer(vmode));
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
    
    // Initialize FAT for file access
    if (!fatInitDefault()) {
        printf("Failed to initialize FAT filesystem\n");
        return 1;
    }
    
    printf("Butterscotch Wii - Starting...\n");
    
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
    
    // ===[ Loading Screen State ]===
    LoadingScreenState loadingState = { .statCount = 0 };
    
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
            .progressCallback = loadingScreenCallback,
            .progressCallbackUserData = &loadingState,
        }
    );
    
    if (!dataWin) {
        printf("Failed to parse data.win!\n");
        while (1) {
            VIDEO_WaitVSync();
        }
    }
    
    printf("\ndata.win loaded successfully!\n");
    
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
        Runner_update(runner);
        
        // Render
        Renderer_render(renderer, runner);
        
        prevButtons = currentButtons;
        
        VIDEO_WaitVSync();
    }
    
    // Cleanup (unreachable in normal operation)
    Runner_destroy(runner);
    VM_destroy(vm);
    DataWin_free(dataWin);
    
    return 0;
}
