// What a picture wider than 4:3 needs besides RT64's Aspect Ratio Expand, which widens the 3D: a camera's view drawn to
// the window's edges, its 3D moving with the board under it. The engine's patches/camera_view.c says how; here are
// Mario Party 2's two functions that start a camera's view, src/20C70.c's func_800204C0_210C0 (its viewport and
// projection) and func_8002064C_2124C (its scissor). A board pans its 3D by moving its camera's viewport
// (src/51630.c's func_80053230_53E30, which sets camera 0's with CameraViewportSet and the background's scroll with
// func_80050E84_51A84).
//
// A camera keeps its scissor as four floats, which the game writes in 10.2 fixed point, and camera_view.c takes whole
// pixels. A camera whose scissor's sides are whole pixels, the whole screen's or the border's, is widened, its top and
// bottom to the pixel at or above the game's where they have a part of one; any other keeps the game's scissor as it
// writes it.
//
// Some of what the game draws in 2D, its sprites (SprDisp), it draws only where DispCheck finds them on the screen: a
// corner of the sprite's quad (SprDispCheck's, in screen pixels) inside 320x240, or an edge across it. DispCheck here
// keeps a sprite whose quad's bounds reach a screen half as wide again at each side (x from -160 to 480: a picture up
// to 8:3, wider than 21:9); a sprite past 4:3's sides is drawn only where the window is wider than 4:3. A board's
// ground is no sprite but its background's tiles (background.c).

#include "patches.h"
#include "../lib/mario-party-engine/patches/camera_view.h"

#define SCREEN_HEIGHT 240
#define WIDE_LEFT -160.0f
#define WIDE_RIGHT 480.0f
// The frames in flight, each with its own viewport and matrices in a camera.
#define FRAMES 4

typedef struct {
    Mtx projection;
    Mtx modelview;
} CameraMatrices;

// A camera (src/20C70.c's, 0x2B8 bytes): what func_800204C0_210C0 and func_8002064C_2124C read.
typedef struct {
    /* 0x000 */ u16 persp_norm; // guPerspective's, from func_800203DC_20FDC
    /* 0x002 */ u8 unk_002[0x30 - 0x02];
    /* 0x030 */ f32 viewport_scale[3]; // CameraViewportSet's, in quarter pixels
    /* 0x03C */ f32 viewport_trans[3];
    /* 0x048 */ u8 unk_048[0x58 - 0x48];
    /* 0x058 */ Vp viewports[FRAMES]; // the frame's (D_800FFFD6_100BD6) as func_800204C0_210C0 writes it
    /* 0x098 */ f32 scissor[4];       // CameraScissorSet's: left, top, right, bottom
    /* 0x0A8 */ u8 unk_0A8[0xB8 - 0xA8];
    /* 0x0B8 */ CameraMatrices matrices[FRAMES];
} Camera;

_Static_assert(__builtin_offsetof(Camera, viewports) == 0x58, "Camera's viewports");
_Static_assert(__builtin_offsetof(Camera, scissor) == 0x98, "Camera's scissor");
_Static_assert(sizeof(Camera) == 0x2B8, "Camera's size");

extern Camera* D_800C9B80_CA780;
// The frame being built, of FRAMES.
extern s16 D_800FFFD6_100BD6;

// A scissor's edge as the game writes it, in 10.2 fixed point.
static s32 fixed(f32 edge) {
    return (s32)(edge * 4.0f) & 0xFFF;
}

static s32 widened(Camera* camera) {
    s32 left = fixed(camera->scissor[0]);
    s32 right = fixed(camera->scissor[2]);

    return !((left | right) & 3) && camera_view_widened(left >> 2, right >> 2);
}

RECOMP_PATCH void func_800204C0_210C0(s16 index, Gfx** gfx) {
    Camera* camera = &D_800C9B80_CA780[index];
    CameraMatrices* matrices = &camera->matrices[D_800FFFD6_100BD6];
    Vp* viewport = &camera->viewports[D_800FFFD6_100BD6];

    viewport->vp.vscale[0] = (s16)camera->viewport_scale[0];
    viewport->vp.vscale[1] = (s16)camera->viewport_scale[1];
    viewport->vp.vscale[2] = (s16)camera->viewport_scale[2];
    viewport->vp.vtrans[0] = (s16)camera->viewport_trans[0];
    viewport->vp.vtrans[1] = (s16)camera->viewport_trans[1];
    viewport->vp.vtrans[2] = (s16)camera->viewport_trans[2];

    // @recomp A widened camera's viewport in the middle, what it was moved by in its projection.
    camera_view_projection(gfx, index, viewport, camera->persp_norm, &matrices->projection, widened(camera));
    gSPMatrix((*gfx)++, (u32)&matrices->modelview + 0x80000000, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
}

RECOMP_PATCH void func_8002064C_2124C(s16 index, Gfx** gfx) {
    Camera* camera = &D_800C9B80_CA780[index];
    s32 left = fixed(camera->scissor[0]);
    s32 top = fixed(camera->scissor[1]);
    s32 right = fixed(camera->scissor[2]);
    s32 bottom = fixed(camera->scissor[3]);

    // @recomp The whole screen, or the border, to the window's edges.
    if (widened(camera)) {
        camera_view_scissor(gfx, left >> 2, top >> 2, right >> 2, bottom >> 2);
        return;
    }

    gDPSetScissorFrac((*gfx)++, G_SC_NON_INTERLACE, left, top, right, bottom);
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
