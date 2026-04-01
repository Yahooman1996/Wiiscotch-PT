#pragma once

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Wii utility functions
void WiiUtils_init(void);
void WiiUtils_shutdown(void);
void* WiiUtils_alloc(size_t size, u32 align);
void WiiUtils_free(void* ptr);

