#include "patches.h"
#include "rt64_extended_gbi.h"

// A scene drawn at the game's own rate whatever the Framerate option, where RT64's frames between the game's would
// draw it wrong. RT64 draws those frames by interpolating the matrices of the game's last two, and never moves what a
// texture's pixels show, so what a game draws by rewriting a picture each frame stands at the newest frame while the
// 3D around it is drawn between the last two.
//
// - 0x15, Lights Out: the room's darkness is one 320x240 picture the overlay draws into each frame on the CPU
//   (func_8010B648_178FD8_LightsOut), with a hole cut at each player's place projected to the screen, drawn as texture
//   rectangles over the scene. The holes, the players' lights, led the players by up to a game frame, a player at a
//   light's trailing edge as they ran.
//
// While one plays, each camera's view (widescreen.c's func_800204C0_210C0 calls frame_rate_view) tells RT64 the game's
// rate is OWN_RATE, above any display's, so it draws no frames between the game's, as at Original. RT64 forgets the
// rate, as all of its extended state, as each frame's display list starts, so it is told again each frame, and a frame
// after the scene's is drawn at the rate it reads from the VI. It reads extended commands only after a gEXEnable in the
// same display list.

// Above the display rates RT64 interpolates to (its Framerate's target is at most 1000).
#define OWN_RATE 1000

static const u8 overlays_own_rate[] = { 0x15 };

static s32 own_rate;

void frame_rate_overlay(s32 overlay) {
    u32 i;

    own_rate = 0;
    for (i = 0; i < sizeof(overlays_own_rate); i++) {
        if (overlays_own_rate[i] == overlay) {
            own_rate = 1;
        }
    }
}

void frame_rate_view(Gfx** gfx) {
    if (!own_rate) {
        return;
    }
    gEXEnable((*gfx)++);
    gEXSetRefreshRate((*gfx)++, OWN_RATE);
}
