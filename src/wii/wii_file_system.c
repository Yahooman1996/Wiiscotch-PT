#include "wii_file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <ogc/dvd.h>
#include <ogc/usbstorage.h>

typedef struct {
    FileSystem base;
    char savePath[256];
    bool dvdInitialized;
    bool usbInitialized;
} WiiFileSystem;

// Initialize DVD subsystem
static void initDVD(void) {
#ifdef WII_ENABLE_DVD_SUPPORT
    DVD_Init();
    printf("DVD subsystem initialized\n");
#endif
}

// Initialize USB mass storage subsystem
static void initUSBStorage(void) {
#ifdef WII_ENABLE_USB_STORAGE
    if (USBStorage_Initialize()) {
        printf("USB mass storage initialized\n");
    } else {
        printf("USB mass storage initialization failed (no device connected?)\n");
    }
#endif
}

static FILE* wii_fopen(const char* path, const char* mode, void* userData) {
    WiiFileSystem* fs = (WiiFileSystem*) userData;
    
    // Handle save file paths
    if (strncmp(path, "$SAVE", 5) == 0) {
        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "%s%s", fs->savePath, path + 5);
        return fopen(fullPath, mode);
    }
    
    // Check for explicit device prefixes
    if (strncmp(path, "sd:", 3) == 0 || strncmp(path, "usb:", 4) == 0 || 
        strncmp(path, "dvd:", 4) == 0 || path[0] == '/') {
        return fopen(path, mode);
    }
    
    // Default to SD card for relative paths
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "sd:/%s", path);
    return fopen(fullPath, mode);
}

static void wii_fclose(FILE* file, void* userData) {
    if (file) {
        fclose(file);
    }
}

static size_t wii_fread(void* buffer, size_t size, size_t count, FILE* file, void* userData) {
    return fread(buffer, size, count, file);
}

static size_t wii_fwrite(const void* buffer, size_t size, size_t count, FILE* file, void* userData) {
    return fwrite(buffer, size, count, file);
}

static int wii_fseek(FILE* file, long offset, int whence, void* userData) {
    return fseek(file, offset, whence);
}

static long wii_ftell(FILE* file, void* userData) {
    return ftell(file);
}

static bool wii_exists(const char* path, void* userData) {
    WiiFileSystem* fs = (WiiFileSystem*) userData;
    
    char fullPath[512];
    if (strncmp(path, "$SAVE", 5) == 0) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", fs->savePath, path + 5);
    } else if (path[0] != '/') {
        snprintf(fullPath, sizeof(fullPath), "sd:/%s", path);
    } else {
        strncpy(fullPath, path, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = '\0';
    }
    
    FILE* f = fopen(fullPath, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

static void wii_destroy(FileSystem* fs, void* userData) {
    free(fs);
}

FileSystem* WiiFileSystem_create(JsonValue* config, const char* gameName) {
    WiiFileSystem* fs = (WiiFileSystem*) calloc(1, sizeof(WiiFileSystem));
    if (!fs) return NULL;
    
    // Initialize storage subsystems
    fs->dvdInitialized = false;
    fs->usbInitialized = false;
    
#ifdef WII_ENABLE_DVD_SUPPORT
    initDVD();
    fs->dvdInitialized = true;
#endif
    
#ifdef WII_ENABLE_USB_STORAGE
    initUSBStorage();
    fs->usbInitialized = true;
#endif
    
    // Default save path
    strncpy(fs->savePath, "sd:/wiiscotch_saves/", sizeof(fs->savePath) - 1);
    
    // Parse save path from config if available
    if (config) {
        JsonValue* fileSystemObj = JsonReader_getObject(config, "fileSystem");
        if (fileSystemObj) {
            JsonValue* savePathVal = JsonReader_getObject(fileSystemObj, "savePath");
            if (savePathVal && JsonReader_isString(savePathVal)) {
                strncpy(fs->savePath, savePathVal->stringValue, sizeof(fs->savePath) - 1);
            }
        }
    }
    
    fs->base.fopen = wii_fopen;
    fs->base.fclose = wii_fclose;
    fs->base.fread = wii_fread;
    fs->base.fwrite = wii_fwrite;
    fs->base.fseek = wii_fseek;
    fs->base.ftell = wii_ftell;
    fs->base.exists = wii_exists;
    fs->base.destroy = wii_destroy;
    fs->base.userData = fs;
    
    printf("Wii FileSystem created - SD: ready, USB: %s, DVD: %s\n",
           fs->usbInitialized ? "enabled" : "disabled",
           fs->dvdInitialized ? "enabled" : "disabled");
    
    return &fs->base;
}
