#pragma once

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Memory limits for Wii
#define WII_MEM1_SIZE (24 * 1024 * 1024)  // 24 MB MEM1
#define WII_MEM2_SIZE (64 * 1024 * 1024)  // 64 MB MEM2

// Wii utility functions
void WiiUtils_init(void);
void WiiUtils_shutdown(void);
void* WiiUtils_alloc(size_t size, u32 align);
void WiiUtils_free(void* ptr);

// Memory tracking functions
uint32_t WiiUtils_getMem1Used(void);
uint32_t WiiUtils_getMem2Used(void);
uint32_t WiiUtils_getMem1Free(void);
uint32_t WiiUtils_getMem2Free(void);
void WiiUtils_printMemoryStatus(const char* context);

