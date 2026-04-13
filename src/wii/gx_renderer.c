#include "gx_renderer.h"
#include "wii_utils.h"
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

static void gx_init(Renderer* renderer, DataWin* dataWin) {
    renderer->dataWin = dataWin;
    renderer->drawColor = 0xFFFFFF;  // White
    renderer->drawAlpha = 1.0f;
    renderer->drawFont = -1;
    renderer->drawHalign = 0;  // Left
    renderer->drawValign = 0;  // Top
}

static void gx_destroy(Renderer* renderer) {
    GxRenderer* gr = (GxRenderer*) renderer;
    free(gr);
}

static void gx_begin_frame(Renderer* renderer, int32_t gameW, int32_t gameH, int32_t windowW, int32_t windowH) {
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

static void gx_begin_view(Renderer* renderer, int32_t viewX, int32_t viewY, int32_t viewW, int32_t viewH, 
                          int32_t portX, int32_t portY, int32_t portW, int32_t portH, float viewAngle) {
    GxRenderer* gr = (GxRenderer*) renderer;
    gr->viewX = viewX;
    gr->viewY = viewY;
    gr->scaleX = (float)portW / (float)viewW;
    gr->scaleY = (float)portH / (float)viewH;
    
    // Set viewport
    GX_SetViewport(portX, portY, portW, portH, 0, 1);
    GX_SetScissor(portX, portY, portW, portH);
}

static void gx_end_view(Renderer* renderer) {
    // Reset to full screen
    GxRenderer* gr = (GxRenderer*) renderer;
    GX_SetViewport(0, 0, gr->vmode->fbWidth, gr->vmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, gr->vmode->fbWidth, gr->vmode->efbHeight);
}

static void gx_draw_sprite(Renderer* renderer, int32_t tpagIndex, float x, float y, 
                           float originX, float originY, float xscale, float yscale, 
                           float angleDeg, uint32_t color, float alpha) {
    // TODO: Implement sprite drawing with GX
    (void)tpagIndex; (void)x; (void)y; (void)originX; (void)originY;
    (void)xscale; (void)yscale; (void)angleDeg; (void)color; (void)alpha;
}

static void gx_draw_sprite_part(Renderer* renderer, int32_t tpagIndex, 
                                int32_t srcOffX, int32_t srcOffY, int32_t srcW, int32_t srcH,
                                float x, float y, float xscale, float yscale, 
                                uint32_t color, float alpha) {
    // TODO: Implement sprite part drawing with GX
    (void)tpagIndex; (void)srcOffX; (void)srcOffY; (void)srcW; (void)srcH;
    (void)x; (void)y; (void)xscale; (void)yscale; (void)color; (void)alpha;
}

static void gx_draw_rectangle(Renderer* renderer, float x1, float y1, float x2, float y2, 
                              uint32_t color, float alpha, bool outline) {
    // TODO: Implement rectangle drawing with GX
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color; (void)alpha; (void)outline;
}

static void gx_draw_line(Renderer* renderer, float x1, float y1, float x2, float y2, 
                         float width, uint32_t color, float alpha) {
    // TODO: Implement line drawing with GX
    (void)x1; (void)y1; (void)x2; (void)y2; (void)width; (void)color; (void)alpha;
}

static void gx_draw_triangle(Renderer *renderer, float x1, float y1, float x2, float y2, 
                             float x3, float y3, bool outline) {
    // TODO: Implement triangle drawing with GX
    (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)outline;
}

static void gx_draw_line_color(Renderer* renderer, float x1, float y1, float x2, float y2, 
                               float width, uint32_t color1, uint32_t color2, float alpha) {
    // TODO: Implement colored line drawing with GX
    (void)x1; (void)y1; (void)x2; (void)y2; (void)width; (void)color1; (void)color2; (void)alpha;
}

static void gx_draw_text(Renderer* renderer, const char* text, float x, float y, 
                         float xscale, float yscale, float angleDeg) {
    // TODO: Implement text drawing with GX
    (void)text; (void)x; (void)y; (void)xscale; (void)yscale; (void)angleDeg;
}

static void gx_draw_text_color(Renderer* renderer, const char* text, float x, float y, 
                               float xscale, float yscale, float angleDeg, 
                               int32_t c1, int32_t c2, int32_t c3, int32_t c4, float alpha) {
    // TODO: Implement colored text drawing with GX
    (void)text; (void)x; (void)y; (void)xscale; (void)yscale; (void)angleDeg;
    (void)c1; (void)c2; (void)c3; (void)c4; (void)alpha;
}

static void gx_flush(Renderer* renderer) {
    // No-op for GX - drawing is immediate
    (void)renderer;
}

static int32_t gx_create_sprite_from_surface(Renderer* renderer, int32_t x, int32_t y, 
                                             int32_t w, int32_t h, bool removeback, 
                                             bool smooth, int32_t xorig, int32_t yorig) {
    // TODO: Implement texture creation from surface
    (void)x; (void)y; (void)w; (void)h; (void)removeback; (void)smooth; (void)xorig; (void)yorig;
    return -1;  // Not implemented
}

static void gx_delete_sprite(Renderer* renderer, int32_t spriteIndex) {
    // TODO: Implement texture deletion
    (void)renderer; (void)spriteIndex;
}

static void gx_draw_tile(Renderer* renderer, RoomTile* tile, float offsetX, float offsetY) {
    // TODO: Implement tile drawing with GX
    (void)tile; (void)offsetX; (void)offsetY;
}

static RendererVtable gx_vtable = {
    .init = gx_init,
    .destroy = gx_destroy,
    .beginFrame = gx_begin_frame,
    .endFrame = gx_end_frame,
    .beginView = gx_begin_view,
    .endView = gx_end_view,
    .drawSprite = gx_draw_sprite,
    .drawSpritePart = gx_draw_sprite_part,
    .drawRectangle = gx_draw_rectangle,
    .drawLine = gx_draw_line,
    .drawTriangle = gx_draw_triangle,
    .drawLineColor = gx_draw_line_color,
    .drawText = gx_draw_text,
    .drawTextColor = gx_draw_text_color,
    .flush = gx_flush,
    .createSpriteFromSurface = gx_create_sprite_from_surface,
    .deleteSprite = gx_delete_sprite,
    .drawTile = gx_draw_tile
};

Renderer* GxRenderer_create(GXRModeObj* vmode) {
    GxRenderer* gr = (GxRenderer*) calloc(1, sizeof(GxRenderer));
    if (!gr) return NULL;
    
    // Initialize base renderer with vtable
    gr->base.vtable = &gx_vtable;
    
    gr->vmode = vmode;
    gr->whichXfb = 0;
    
    // Allocate framebuffers in MEM2 for better performance with GX
    gr->xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
    gr->xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
    
    WiiUtils_printMemoryStatus("After framebuffer allocation");
    
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
    
    return &gr->base;
}
