// What a picture wider than 4:3 needs besides RT64's Aspect Ratio Expand, which widens the 3D: a camera's view drawn to
// the window's edges. Mario Party's patches/widescreen.c, on Mario Party 2's camera.
//
// Each camera has a scissor (src/20C70.c's func_8002064C_2124C sets it before the camera's view is drawn, from the
// rectangle CameraScissorSet gave it, four floats in the camera), and RT64 keeps a scissor that stops short of 4:3's
// edges as far in from the window's, so a board, whose camera keeps the console's overscan border (16, 12, 304, 228),
// drew in a box with black at its sides. A camera whose scissor is the whole screen or that border is drawn to the
// window's edges (gEXSetScissor, from the left and right): from top to bottom for the border, and between the game's
// own top and bottom otherwise. A screen split between players (a minigame's halves and quarters) keeps the game's.

#include "patches.h"
#include "rt64_extended_gbi.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define BORDER_LEFT 16
#define BORDER_TOP 12
#define BORDER_RIGHT 304
#define BORDER_BOTTOM 228

// The cameras (0x2B8 bytes each) and, in each, its scissor's corners.
typedef struct {
    u8 unk_00[0x98];
    f32 scissor[4];
    u8 unk_A8[0x2B8 - 0xA8];
} Camera;

extern Camera* D_800C9B80_CA780;

RECOMP_PATCH void func_8002064C_2124C(s16 index, Gfx** gfx) {
    Camera* camera = &D_800C9B80_CA780[index];
    // The game's 10.2 fixed point, as it writes the scissor.
    s32 x0 = (s32)(camera->scissor[0] * 4.0f) & 0xFFF;
    s32 y0 = (s32)(camera->scissor[1] * 4.0f) & 0xFFF;
    s32 x1 = (s32)(camera->scissor[2] * 4.0f) & 0xFFF;
    s32 y1 = (s32)(camera->scissor[3] * 4.0f) & 0xFFF;

    // @recomp The whole screen, or the border, to the window's edges.
    if (((x0 == 0) && (x1 == SCREEN_WIDTH * 4)) || ((x0 == BORDER_LEFT * 4) && (x1 == BORDER_RIGHT * 4))) {
        if ((y0 == BORDER_TOP * 4) && (y1 == BORDER_BOTTOM * 4)) {
            y0 = 0;
            y1 = SCREEN_HEIGHT * 4;
        }
        gEXSetScissor((*gfx)++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 0, y0 / 4, 0, y1 / 4);
        return;
    }

    gDPSetScissorFrac((*gfx)++, G_SC_NON_INTERLACE, x0, y0, x1, y1);
}
