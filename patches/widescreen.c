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
#include "../lib/mario-party-engine/patches/board_back.h"

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

// The scenes the game draws at 4:3 whatever the window's width, by their overlay (OvlLoad, overlays.c, calls
// widescreen_overlay as each loads): the engine keeps black at the window's sides while one plays (board_back.h's
// board_back_scene_4x3), its src/wipe.cpp drawing it over whatever the scene's 3D reaches past 4:3 at the end of each
// frame (us.toml's hook at func_800286FC_292FC's gDPFullSync), and the camera's view and the HUD stay inside 4:3.
// - 0x05, Give Me a Brake (Western Land's item minigame): its desert, the cacti and fence with it, is a picture of the
//   screen's size, and the frame's white clear showed past it.
// - 0x09, Bumper Balloon Cars: its arena, the sand and the fence and stands around it, is a 2D picture of the screen's
//   size, and the cars that drive on the sand are 3D, kept to it as it is at 4:3. Stretched over a wide window, the
//   sand went on far past where the cars turn back.
// - 0x0A, Rakin' 'Em In: the ground around the 3D table, the dark pit it stands in and four empty stumps, is a 2D
//   picture of the screen's size, the stumps behind the players' at 4:3. Stretched over a wide window, the pit was
//   nearly three times the table's width and the empty stumps stood out at the window's sides.
// - 0x17, Archerival: the counter, the bushes and the brick wall the targets stand behind are a 2D picture of the
//   screen's size. Stretched over a wide window, only the wall's middle was drawn, blank grey boxes beside it, and the
//   targets bunched over the middle.
// - 0x1B, Balloon Burst: each team's platform and its gauges are in a 2D picture of the screen's size, and the
//   players stand on it in 3D. Stretched over a wide window, the platforms moved out from under the players.
// - 0x18, Toad Bandstand: a stage of the screen's size, and past 4:3 the leaves around Toad, seen close as he conducts.
// - 0x1E, Cake Factory: its wall is a 2D picture of the screen's size, and its conveyor belts ran on past it.
// - 0x1F, Dungeon Dash: two views side by side, each a team's, and their walls and lanterns past 4:3.
// - 0x20, Magnet Carta: the coin room, the star floor the carts drive on and each team's scoreboard are a 2D picture of
//   the screen's size, and a board's score is drawn on it apart. Stretched over a wide window, the boards stood empty
//   at the window's sides, their scores still at 4:3's edges, and the floor went on past where the carts turn back.
// - 0x21, Face Lift: four faces on a 2D picture of the screen's size, and the pieces they are made of past it.
// - 0x23, Crazy Cutters: its picture is 80 tiles of 32x32, each a copy of the frame, which stay at 4:3, and its stage
//   past 4:3, the logs and spiked bars, was drawn over the black.
// - 0x24, Toad in the Box: a stage of the screen's size, and its curtains and floor past it.
// - 0x2C, Tipsy Tourney: four quarters on a starry picture of the screen's size, and past it the boards' wooden frames
//   over white.
// - 0x3A, Torpedo Targets: two views, one over the other, each a team's: the bottom one was widened past 4:3 and the
//   top one not, its subs drawn past its sides into the black.
static const u8 overlays_4x3[] = { 0x05, 0x09, 0x0A, 0x17, 0x18, 0x1B, 0x1E, 0x1F, 0x20, 0x21, 0x23, 0x24,
                                   0x2C, 0x3A };

void widescreen_overlay(s32 overlay) {
    s32 picture_4x3 = FALSE;
    u32 i;

    for (i = 0; i < sizeof(overlays_4x3); i++) {
        if (overlays_4x3[i] == overlay) {
            picture_4x3 = TRUE;
        }
    }
    board_back_scene_4x3(picture_4x3);
}
