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
// While one plays, RT64 is told the game's rate is OWN_RATE, above any display's, so it draws no frames between the
// game's, as at Original. The game sends a frame as several display lists, a task each (its models' for each of their
// microcode groups, its 2D layers' at each group change), then the scheduler sends one more of its own, a few commands
// that draw nothing (func_8007C254_7CE54, after the frame's last task). RT64 reads a frame's rate from one list: on a
// desktop build the one its present follows, on the web the frame's last. So every list is told:
// - each task's: us.toml's hook at func_8001D054_1DC54, which sends every task to the graphics thread, calls
//   frame_rate_task, which writes the rate in front of the list's gDPFullSync and gSPEndDisplayList, the last two
//   commands at the frame's display list pointer (D_800FDBCC_FE7CC).
// - the scheduler's: us.toml's hook in func_8007C254_7CE54, before it writes its gDPFullSync, calls frame_rate_list
//   with the list's next command, which writes the rate there. The list is rebuilt each frame at 0x800E2450 and ends
//   16 bytes later than the game's own, in memory nothing else in the game reads or writes (up to 0x800E2850).
// RT64 forgets the rate, as all of its extended state, as each display list starts, so a list after the scene's is
// drawn at the rate it reads from the VI. It reads extended commands only after a gEXEnable in the same display list.

// Above the display rates RT64 interpolates to (its Framerate's target is at most 1000).
#define OWN_RATE 1000

// The commands a task's display list ends with: gDPFullSync and gSPEndDisplayList.
#define FULL_SYNC 0xE9000000
#define END_DISPLAY_LIST 0xDF000000

extern Gfx* D_800FDBCC_FE7CC;

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

void frame_rate_task(void) {
    Gfx* gfx = D_800FDBCC_FE7CC;
    Gfx sync;
    Gfx end;

    if (!own_rate || (gfx[-2].words.w0 != FULL_SYNC) || (gfx[-1].words.w0 != END_DISPLAY_LIST)) {
        return;
    }
    sync = gfx[-2];
    end = gfx[-1];
    gfx -= 2;
    gEXEnable(gfx++);
    gEXSetRefreshRate(gfx++, OWN_RATE);
    *gfx++ = sync;
    *gfx++ = end;
    D_800FDBCC_FE7CC = gfx;
}

Gfx* frame_rate_list(Gfx* gfx) {
    if (own_rate) {
        gEXEnable(gfx++);
        gEXSetRefreshRate(gfx++, OWN_RATE);
    }
    return gfx;
}
