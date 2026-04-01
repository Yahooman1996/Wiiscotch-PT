#include "wii_utils.h"

void WiiUtils_init(void) {
    // Initialize Wii hardware
}

void WiiUtils_shutdown(void) {
    // Shutdown Wii hardware
}

void* WiiUtils_alloc(size_t size, u32 align) {
    return memalign(align, size);
}

void WiiUtils_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}
