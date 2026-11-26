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
// writes it, and, but for a cut of nearly the whole screen's width (OVERSCAN), starts a pass of RT64's own
// (new_pass). RT64 widens a 3D view with the window only where it spans the width of all that the pass it is drawn in
// draws, and the frame's first pass holds the clear of the whole screen to the background's colour
// (func_800295D4_2A1D4, after the depth buffer's): the first camera's first models, drawn in it, were fitted to 4:3
// under a scissor widened with the rest. Shell Shocked's top-left quarter, the first of its four
// cameras, drew its floor and sky three quarters as wide as the quarter, at its right side, with black at its left
// where the fence, drawn in a later pass, went on across. The other quarters, and the first one's later models, were
// each in a pass of their own already. A scene drawn at 4:3 (board_back.c) starts none, so its 3D stays inside 4:3,
// and nor does overlay 0x48's camera, 8 pixels inside the screen at each side, which camera_view.c keeps at 4:3.
//
// Some of what the game draws in 2D, its sprites (SprDisp), it draws only where DispCheck finds them on the screen: a
// corner of the sprite's quad (SprDispCheck's, in screen pixels) inside 320x240, or an edge across it. DispCheck here
// keeps a sprite whose quad's bounds reach a screen half as wide again at each side (x from -160 to 480: a picture up
// to 8:3, wider than 21:9), and on the right as far again (to 640) for one the engine's src/hud.cpp has moved right
// by as far as the window reaches past 4:3 (us.toml's SprDisp hook, for PutSprite, which cuts a texture rectangle off
// at x 0); a sprite past 4:3's sides is drawn only where the window is wider than 4:3. A board's
// ground is no sprite but its background's tiles (background.c).

#include "patches.h"
#include "../lib/mario-party-engine/patches/camera_view.h"
#include "../lib/mario-party-engine/patches/board_back.h"

#define SCREEN_WIDTH 320
// A scissor at least this close to both of the screen's sides, in pixels: a cut of the whole screen, not one view of
// several.
#define OVERSCAN 8
#define SCREEN_HEIGHT 240
#define WIDE_LEFT -160.0f
#define WIDE_RIGHT 640.0f
// The depth buffer, where the game points the RDP's depth image every time it sets it (func_800295D4_2A1D4).
#define DEPTH_IMAGE 0x003D0800
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
void mp_model_camera(s32 camera, Mtx* view, Mtx* projection);
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
    // @recomp A cut of the camera to another shot, its models not interpolated across it.
    mp_model_camera(index, &matrices->modelview, &matrices->projection);
}

// The pass RT64 draws in ended, and a new one started at the next thing drawn: RT64 starts one where the depth image
// changes, as it does where the game clears the depth buffer. The depth image, set away and back, is the same.
static void new_pass(Gfx** gfx) {
    gDPPipeSync((*gfx)++);
    gDPSetDepthImage((*gfx)++, 0);
    gDPSetDepthImage((*gfx)++, DEPTH_IMAGE);
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

    // @recomp Its 3D widened as its own, not fitted to 4:3 with what the frame drew before it.
    if (!board_back_4x3() && (left > OVERSCAN * 4 || right < (SCREEN_WIDTH - OVERSCAN) * 4)) {
        new_pass(gfx);
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
// board_back_scene_widest, at 4:3), its src/wipe.cpp drawing it over whatever the scene's 3D reaches past 4:3 at the end of each
// frame (us.toml's hook at func_800286FC_292FC's gDPFullSync), and the camera's view and the HUD stay inside 4:3.
// - 0x05, Give Me a Brake (Western Land's item minigame): its desert, the cacti and fence with it, is a picture of the
//   screen's size, and the frame's white clear showed past it.
// - 0x09, Bumper Balloon Cars: its arena, the sand and the fence and stands around it, is a 2D picture of the screen's
//   size, and the cars that drive on the sand are 3D, kept to it as it is at 4:3. Stretched over a wide window, the
//   sand went on far past where the cars turn back.
// - 0x0A, Rakin' 'Em In: the ground around the 3D table, the dark pit it stands in and four empty stumps, is a 2D
//   picture of the screen's size, the stumps behind the players' at 4:3. Stretched over a wide window, the pit was
//   nearly three times the table's width and the empty stumps stood out at the window's sides.
// - 0x0E, Bowl Over: two views, one over the other, and a sky drawn as rectangles 320 wide behind the top one's 3D:
//   the bottom view was widened past 4:3 and the top one's sky not, black beside it.
// - 0x11, Move to the Music: the night sky, the moon and the towers are a 2D picture of the screen's size, and the 3D
//   steps ran on past it over the black.
// - 0x16, Filet Relay: two views side by side, each a team's, inside 4:3, and past them the frame's clear, a grey blue.
// - 0x17, Archerival: the counter, the bushes and the brick wall the targets stand behind are a 2D picture of the
//   screen's size. Stretched over a wide window, only the wall's middle was drawn, blank grey boxes beside it, and the
//   targets bunched over the middle.
// - 0x19, Bobsled Run: two views, one over the other, each a team's: the bottom one was widened nearly to the window's
//   edges and the top one not, its walls ending at 4:3 over the frame's clear, a dark blue.
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
// - 0x2E, Hexagon Heat: the lava's far edge is narrower than 4:3, and past 4:3 the lava's near part went on beside
//   black corners as wide as the window.
// - 0x38, Quicksand Cache: the rock ring around the sand is built about as wide as 4:3, its corners short of it.
// - 0x39, Bowser's Big Blast: the room's side walls end at 4:3's edges.
// - 0x3A, Torpedo Targets: two views, one over the other, each a team's: the bottom one was widened past 4:3 and the
//   top one not, its subs drawn past its sides into the black.
// - 0x3C, Deep Sea Salvage: the sky, the sea and the reef far off are a 2D picture of the screen's size, and the sea
//   the players swim in, tinted, and its rocks are 3D that ends at 4:3's edges. Stretched over a wide window, the
//   picture's sea went on past them beside the water's tinted box.
// - 0x70, a board's results after each minigame: the board dimmed behind the table is a map of the screen's size
//   (func_801042D8_410738_Results), which the engine judges 4:3 by its edges each frame; it is 4:3 by its overlay too,
//   so nothing the HUD aligns to the window's edges is drawn over the black at its sides in any frame.
static const u8 overlays_4x3[] = { 0x05, 0x09, 0x0A, 0x0E, 0x11, 0x16, 0x17, 0x18, 0x19, 0x1B, 0x1E, 0x1F, 0x20,
                                   0x21, 0x23, 0x24, 0x2C, 0x2E, 0x38, 0x39, 0x3A, 0x3C, 0x70 };

// The minigames whose 3D ends short of a wide window's edges, and the widest each is drawn, its width over its height:
// where its stage ends at its narrowest across the minigame's play, at a 32:9 window. The engine draws one to the
// window's edges up to that width and black past it (board_back_scene_widest), the camera's view stopping there, so a
// window no wider shows it as before, and a wider one shows black bars rather than the frame's clear around the stage.
// - 0x02, Roll Out the Barrels: the side walls' outer edges, 2.1, past them the clear's teal.
// - 0x25, Mecha-Marathon: the grass field's edges, 2.05 (its far corner, a wedge of black at the top left, shows at
//   4:3 too).
// - 0x2A, Bumper Balls: the lava's far edge, 1.65, the lava widened past it as it plays (widescreen_bumper_lava).
// - 0x2F, Skateboard Scamper: the start room's end wall, 1.8; the hall the players run fills a 32:9 window.
// - 0x32, Sneak 'n' Snore: the outer wall's bottom edge, 1.45, past it the clear's grey.
// - 0x3B, Destruction Duet: the cave's arch, 1.5.
typedef struct {
    u8 overlay;
    f32 widest;
} ShortScene;

static const ShortScene short_scenes[] = {
    { 0x02, 2.1f }, { 0x25, 2.05f }, { 0x2A, 1.65f }, { 0x2F, 1.8f }, { 0x32, 1.45f }, { 0x3B, 1.5f },
};

void widescreen_overlay(s32 overlay) {
    f32 widest = 0.0f;
    u32 i;

    for (i = 0; i < sizeof(overlays_4x3); i++) {
        if (overlays_4x3[i] == overlay) {
            widest = 4.0f / 3.0f;
        }
    }
    for (i = 0; i < sizeof(short_scenes) / sizeof(short_scenes[0]); i++) {
        if (short_scenes[i].overlay == overlay) {
            widest = short_scenes[i].widest;
        }
    }
    board_back_scene_widest(widest);
}

// The seas of short_scenes that are widened in x so that they reach a wide window's edges (lib/mario-party-engine's
// board_back_stage_widened, which moves the scene's widest out with them): a sea with nothing standing at its edges,
// which reads the same wider. A room's walls, a stage built for 4:3 and a painted backdrop keep their cap, but for the
// opening's theater (widescreen_theater_stage), whose curtains end in a plain colour and hide what is widened.

// The model instances (0xC0 bytes each), their scale at 0x34, which the object manager copies an object's onto only for
// its first model.
typedef struct {
    u8 unk_00[0x34];
    Vec scale;
    u8 unk_40[0xC0 - 0x40];
} ModelInstance;
extern ModelInstance* D_800FC834_FD434;

// Bumper Balls' lava (file 0x3A000C), the stage's second model, under the arena and as wide as its far edge at 1.65:
// us.toml's hook calls this each frame from the stage's frame function, which scrolls the lava's textures and never
// writes its scale. The lava is the first of the minigame's three arenas, which the game picks by a byte at
// 0x800FA656 as it starts (func_80102800_2111A0_BumperBalls); the other two keep the cap: the snowy peak's mountains
// are a backdrop, and the island's sea is a part inside one of its models that the game finds by name and rewrites
// each frame, not a model of its own to scale.
void widescreen_bumper_lava(omObjData* stage) {
    D_800FC834_FD434[stage->model[1]].scale.x = board_back_stage_widened(1.65f);
}

// The opening's theater, where Toad tells the story (func_80105AB8_3D7FD8_TitleScreenAndIntro), widened to a wide
// window's edges up to 16:9. Its set, the red curtains, the pink borders above and below them and the floor's front
// (file 0xE0000, built about the middle), ends at 1.57 wide at the curtains' outer edges, and the characters waiting in
// the wings showed past it. The camera never moves, so the set is made as wide as the window and no wider (not
// board_back_stage_widened's spare): the curtains, wider, hide more of the wings, and their inner edges move out by as
// much. The painted backdrop between them is a sprite, each picture of the story a sprite group made anew, 236 wide
// about the middle with the curtains' inner edges 221 apart at 4:3, so it is widened by as much less, to meet them with
// a little to spare under them, and no slit of black opens beside it. Past 16:9 the set keeps that width, black beside
// it, so the painting is stretched by no more than 10%.
#define THEATER_WIDEST 1.55f
#define THEATER_MOST (16.0f / 9.0f / THEATER_WIDEST)
#define THEATER_PICTURE (226.0f / 236.0f)

f32 mp_aspect_ratio(void);
extern void SprScaleSet(s16 group, s16 index, f32 x, f32 y);

static f32 theater_widened(void) {
    f32 times = mp_aspect_ratio() / THEATER_WIDEST;

    if (times < 1.0f) {
        times = 1.0f;
    }
    if (times > THEATER_MOST) {
        times = THEATER_MOST;
    }
    board_back_scene_widest(THEATER_WIDEST * times);
    return times;
}

// The set, from us.toml's hook as the theater is made: the game never writes its model's scale again.
void widescreen_theater_stage(s16 model) {
    D_800FC834_FD434[model].scale.x = theater_widened();
}

// A picture the theater shows, from us.toml's hook as func_80104448_3D6968_TitleScreenAndIntro has made and placed it,
// its sprite group at picture[2]: the backdrop is the one at the screen's middle, x 160 and y 100, the others lower.
void widescreen_theater_picture(s32* picture, s32 x, s32 y) {
    f32 times;

    if ((x == 160) && (y == 100)) {
        times = theater_widened() * THEATER_PICTURE;
        SprScaleSet(picture[2], 0, (times < 1.0f) ? 1.0f : times, 1.0f);
    }
}

// The Koopa who comes on as Bowser attacks waits in front of the curtains, out past 4:3 at x -650, and a wide window
// showed him standing there before he walked on: from us.toml's hook as the theater places him, the x (a float's bits)
// is moved out by as much as the set, past the window's edge.
s32 widescreen_theater_wing(s32 x) {
    union {
        s32 bits;
        f32 x;
    } at;

    at.bits = x;
    at.x *= theater_widened();
    return at.bits;
}
