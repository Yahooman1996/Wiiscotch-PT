#include "wii_file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>

typedef struct {
    FileSystem base;
    char savePath[256];
} WiiFileSystem;

static FILE* wii_fopen(const char* path, const char* mode, void* userData) {
    WiiFileSystem* fs = (WiiFileSystem*) userData;
    
    // Handle save file paths
    if (strncmp(path, "$SAVE", 5) == 0) {
        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "%s%s", fs->savePath, path + 5);
        return fopen(fullPath, mode);
    }
    
    // Default to SD card
    if (path[0] != '/') {
        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "sd:/%s", path);
        return fopen(fullPath, mode);
    }
    
    return fopen(path, mode);
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
    
    // Default save path
    strncpy(fs->savePath, "sd:/butterscotch_saves/", sizeof(fs->savePath) - 1);
    
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
    
    return &fs->base;
}
