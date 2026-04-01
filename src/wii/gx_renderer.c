#include "gx_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    Renderer base;
    GXRModeObj* vmode;
    void* xfb[2];
    int whichXfb;
    
    // View transform state
    float scaleX;
    float scaleY;
    float offsetX;
    float offsetY;
    int32_t viewX;
    int32_t viewY;
    
    // GX state
    Mtx44 projection;
    Mtx modelview;
} GxRenderer;

static void gx_set_viewport(Renderer* renderer, int32_t x, int32_t y, int32_t width, int32_t height, float scale, float scaleY) {
    GxRenderer* gr = (GxRenderer*) renderer;
    gr->viewX = x;
    gr->viewY = y;
    gr->scaleX = scale;
    gr->scaleY = scaleY;
}

static void gx_clear(Renderer* renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    GxRenderer* gr = (GxRenderer*) renderer;
    
    // Clear framebuffers
    for (int i = 0; i < 2; i++) {
        VIDEO_ClearFrameBuffer(gr->vmode, gr->xfb[i], 
            GX_COLOR((r << 24) | (g << 16) | (b << 8) | a));
    }
}

static void gx_begin_frame(Renderer* renderer) {
    GxRenderer* gr = (GxRenderer*) renderer;
    
    // Wait for video sync
    VIDEO_WaitVSync();
    
    // Clear current framebuffer
    VIDEO_ClearFrameBuffer(gr->vmode, gr->xfb[gr->whichXfb], GX_COLOR0);
}

static void gx_end_frame(Renderer* renderer) {
    GxRenderer* gr = (GxRenderer*) renderer;
    
    // Copy to XFB
    GX_CopyDisp(gr->xfb[gr->whichXfb], GX_TRUE);
    GX_DrawDone();
    
    // Swap buffers
    VIDEO_SetNextFramebuffer(gr->xfb[gr->whichXfb]);
    VIDEO_Flush();
    gr->whichXfb = !gr->whichXfb;
}

static void gx_draw_sprite(Renderer* renderer, uint32_t textureId, 
                           float x, float y, float width, float height,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                           float rotation, float originX, float originY) {
    // TODO: Implement sprite drawing with GX
}

static void gx_draw_tile(Renderer* renderer, uint32_t tileId,
                         float x, float y, float width, float height,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                         int32_t bgDef, uint16_t srcX, uint16_t srcY,
                         uint16_t srcW, uint16_t srcH) {
    // TODO: Implement tile drawing with GX
}

static void gx_draw_text(Renderer* renderer, const char* text,
                         float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                         int32_t fontId, float scale) {
    // TODO: Implement text drawing with GX
}

static void gx_destroy(Renderer* renderer) {
    GxRenderer* gr = (GxRenderer*) renderer;
    free(gr);
}

Renderer* GxRenderer_create(GXRModeObj* vmode) {
    GxRenderer* gr = (GxRenderer*) calloc(1, sizeof(GxRenderer));
    if (!gr) return NULL;
    
    gr->vmode = vmode;
    gr->whichXfb = 0;
    
    // Allocate framebuffers
    gr->xfb[0] = MEM_K0_TO_K1(MEM_AllocFramebuffer(vmode));
    gr->xfb[1] = MEM_K0_TO_K1(MEM_AllocFramebuffer(vmode));
    
    // Initialize GX
    GX_Init(NULL, 0);
    GX_SetCopyClear((GXColor){0, 0, 0, 0xFF}, GX_MAX_Z24);
    GX_CopyDisp(gr->xfb[0], GX_FALSE);
    GX_DrawDone();
    
    // Set up viewport
    GX_SetViewport(0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, vmode->fbWidth, vmode->efbHeight);
    
    // Enable blending
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_ENABLE);
    
    gr->base.setViewport = gx_set_viewport;
    gr->base.clear = gx_clear;
    gr->base.beginFrame = gx_begin_frame;
    gr->base.endFrame = gx_end_frame;
    gr->base.drawSprite = gx_draw_sprite;
    gr->base.drawTile = gx_draw_tile;
    gr->base.drawText = gx_draw_text;
    gr->base.destroy = gx_destroy;
    
    return &gr->base;
}
