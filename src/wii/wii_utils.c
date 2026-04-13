#include "wii_utils.h"
#include <gccore.h>
#include <stdio.h>
#include <string.h>

// Memory tracking for MEM1 and MEM2
static uint32_t g_mem1Used = 0;
static uint32_t g_mem2Used = 0;

// Memory limits
#define MEM1_SIZE (24 * 1024 * 1024)  // 24 MB
#define MEM2_SIZE (64 * 1024 * 1024)  // 64 MB

void WiiUtils_init(void) {
    g_mem1Used = 0;
    g_mem2Used = 0;
    printf("Wii Memory initialized: MEM1=%d MB, MEM2=%d MB\n", MEM1_SIZE / (1024*1024), MEM2_SIZE / (1024*1024));
}

void WiiUtils_shutdown(void) {
    printf("Memory usage at shutdown: MEM1=%u bytes (%.2f MB), MEM2=%u bytes (%.2f MB)\n", 
           g_mem1Used, g_mem1Used / (1024.0f * 1024.0f),
           g_mem2Used, g_mem2Used / (1024.0f * 1024.0f));
}

void* WiiUtils_alloc(size_t size, u32 align) {
    void* ptr = memalign(align, size);
    if (ptr) {
        // Determine which memory region based on address
        u32 addr = (u32)ptr;
        if (addr >= 0x80000000 && addr < 0x81800000) {
            // MEM1 allocation
            g_mem1Used += size;
        } else if (addr >= 0x90000000 && addr < 0x94000000) {
            // MEM2 allocation
            g_mem2Used += size;
        }
    }
    return ptr;
}

void WiiUtils_free(void* ptr) {
    if (ptr) {
        u32 addr = (u32)ptr;
        size_t size = 0; // In a real implementation, we'd track allocation sizes
        
        // For now, just free without adjusting counters
        // A more sophisticated implementation would track allocation sizes
        
        free(ptr);
    }
}

uint32_t WiiUtils_getMem1Used(void) {
    return g_mem1Used;
}

uint32_t WiiUtils_getMem2Used(void) {
    return g_mem2Used;
}

uint32_t WiiUtils_getMem1Free(void) {
    return MEM1_SIZE - g_mem1Used;
}

uint32_t WiiUtils_getMem2Free(void) {
    return MEM2_SIZE - g_mem2Used;
}

void WiiUtils_printMemoryStatus(const char* context) {
    printf("[%s] Memory: MEM1=%u/%u bytes (%.2f/%.2f MB), MEM2=%u/%u bytes (%.2f/%.2f MB)\n",
           context ? context : "Unknown",
           g_mem1Used, MEM1_SIZE, g_mem1Used / (1024.0f * 1024.0f), MEM1_SIZE / (1024.0f * 1024.0f),
           g_mem2Used, MEM2_SIZE, g_mem2Used / (1024.0f * 1024.0f), MEM2_SIZE / (1024.0f * 1024.0f));
}
