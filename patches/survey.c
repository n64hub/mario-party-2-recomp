// The survey: lets a script take the game to any overlay (a board, a minigame, a menu) by itself, for the page's shader
// list (src/rt64_shaders.txt, recorded by tools/shader_plan.json) and its play plans, without anyone playing a board
// to its minigames. It does nothing unless a script asks. Mario Party's and Mario Party 3's patches/survey.c, with
// Mario Party 2's names.
//
// A script finds this struct in RDRAM by its two magic words. It writes an overlay's index, an event and a stat (as
// omOvlGotoEx takes them; -1 keeps the game's own), the overlay to take the place of in `from` (-1 for any), then
// `request`. The next time the game goes to that overlay (with a request of 1; every time, with 2, until a script
// clears it), it goes to the one asked for instead: a hook at omOvlGotoEx's start (us.toml) changes its arguments
// (src/game/survey.cpp). A board takes 2: the game goes to the board again with each part of a game (its turns, back
// from each minigame). So the game leaves the overlay it is in its own way, with what it frees and sets up as it does,
// which a call of omOvlGotoEx from elsewhere skips. A request also carries `groups`, a byte a player from player 1's in
// the top byte (GwPlayer's group, a minigame's sides; all ones: leave them), `computers`, a bit a player from bit 0
// (the computer plays that player), set only when bit 31 is, and `minigame`, a minigame's ID (-1: leave it), which
// GwSystem's chosenMinigameIndex takes: all three as a board sets them before it opens a minigame, by the minigame's
// explanation screen (0x5F for IDs below 30, 0x60 for the others), which reads the ID to know which one it explains and
// then goes to. `board` (-1: leave it) is GwSystem's current_board_index as a request is taken, 0 (Western Land) to 5
// (Bowser Land), which a board's shared code reads for its own tables: set as the game leaves, not before, since the
// overlay it leaves reads it too (Rules Land never leaves with another board's).
//
// `warps` counts the requests taken, and every frame the struct carries the overlay playing (the one omOvlGotoEx last
// went to) and a count of the scheduler's frames (HuPrcCall, lib/mario-party-engine/patches/process.c, calls
// survey_tick once a frame), which plans hold their waits on.

#include "patches.h"

// omOvlGotoEx's (src/objmain.c, not matched C): the overlay it last went to.
extern s32 D_800FA63C_FB23C;

void recomp_survey_register(void* survey);

struct Survey {
    u32 magic[2];
    volatile s32 request;
    volatile s32 from;
    volatile s32 overlay;
    volatile s32 event;
    volatile s32 stat;
    volatile u32 warps;
    volatile s32 cur_overlay;
    volatile u32 frames;
    volatile u32 groups;
    volatile u32 computers;
    volatile s32 minigame;
    volatile s32 board;
};

struct Survey survey = { { 0x4D505254, 0x53525659 }, 0, -1, 0, 0, 0, 0, -1, 0, 0xFFFFFFFF, 0, -1, -1 }; // "MPRT" "SRVY"

void survey_tick(void) {
    if (survey.frames == 0) {
        recomp_survey_register(&survey);
    }
    survey.cur_overlay = D_800FA63C_FB23C;
    survey.frames++;
}
