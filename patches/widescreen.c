// What a picture wider than 4:3 needs besides RT64's Aspect Ratio Expand, which widens the 3D: a camera's view drawn to
// the window's edges. Mario Party's patches/widescreen.c, on Mario Party 2's camera.
//
// Each camera has a scissor (src/20C70.c's func_8002064C_2124C sets it before the camera's view is drawn, from the
// rectangle CameraScissorSet gave it, four floats in the camera), and RT64 keeps a scissor that stops short of 4:3's
// edges as far in from the window's, so a board, whose camera keeps the console's overscan border (16, 12, 304, 228),
// drew in a box with black at its sides. A camera whose scissor is the whole screen or that border is drawn to the
// window's edges (gEXSetScissor, from the left and right): from top to bottom for the border, and between the game's
// own top and bottom otherwise. A screen split between players (a minigame's halves and quarters) keeps the game's.
//
// A board's ground is a picture in tiles, each a sprite placed in 3D (Expand widens them with the rest), and the game
// draws only the sprites DispCheck finds on the screen: a corner of the sprite's projected quad inside 320x240, or an
// edge across it. So a board seen from close drew its ground in 4:3 with nothing at the sides. DispCheck here keeps a
// sprite whose quad's bounds reach a screen half as wide again at each side (x from -160 to 480: a picture up to 8:3,
// wider than 21:9); a sprite past 4:3's sides is drawn only where the window is wider than 4:3. A board seen from
// afar and the minigames reach the window's edges; a board's close view, the players' panels at its foot, still
// draws in 4:3.

#include "patches.h"
#include "rt64_extended_gbi.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define BORDER_LEFT 16
#define BORDER_TOP 12
#define BORDER_RIGHT 304
#define BORDER_BOTTOM 228
#define WIDE_LEFT -160.0f
#define WIDE_RIGHT 480.0f

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

// A sprite's projected quad, its corners in screen pixels: on the screen (1) or not (0), against the wide screen above.
RECOMP_PATCH s32 DispCheck(Vec2f* corners) {
    f32 left = corners[0].unk_00;
    f32 right = left;
    f32 top = corners[0].unk_04;
    f32 bottom = top;
    s32 i;

    for (i = 1; i < 4; i++) {
        f32 x = corners[i].unk_00;
        f32 y = corners[i].unk_04;
        if (x < left) left = x;
        if (x > right) right = x;
        if (y < top) top = y;
        if (y > bottom) bottom = y;
    }
    return (left < WIDE_RIGHT) && (right >= WIDE_LEFT) && (top < SCREEN_HEIGHT) && (bottom >= 0.0f);
}
