// The survey's natives (patches/survey.c says what it is for): where its struct is, and the hook at omOvlGotoEx's start
// (us.toml) that sends the game to the overlay a script asked for in place of the one it was going to.

#include <cstdio>
#include <cstdlib>

#include "recomp.h"
#include "librecomp/helpers.hpp"

namespace {
    // The struct's place in RDRAM, and its words' offsets (patches/survey.c's struct Survey).
    gpr survey_vram = 0;
    constexpr int REQUEST = 0x08;
    constexpr int FROM = 0x0C;
    constexpr int OVERLAY = 0x10;
    constexpr int EVENT = 0x14;
    constexpr int STAT = 0x18;
    constexpr int WARPS = 0x1C;
    constexpr int GROUPS = 0x28;
    constexpr int COMPUTERS = 0x2C;
    constexpr int MINIGAME = 0x30;
    constexpr int BOARD = 0x34;

    // gPlayers (0x800FD2C0, 0x34 bytes a player): its group (a minigame's side) at 0 and its flags at 6, bit 0 of the
    // low byte (7) the computer playing it (ovl_70_Results reads it so); GwSystem's chosenMinigameIndex (0x800F93C8), a
    // minigame's ID, which the explanation screen turns into its overlay; GwSystem's current_board_index (0x800F93AA).
    constexpr gpr GW_PLAYER = (gpr)(int32_t)0x800FD2C0;
    constexpr int PLAYER_SIZE = 0x34;
    constexpr gpr MINIGAME_INDEX = (gpr)(int32_t)0x800F93C8;
    constexpr gpr BOARD_INDEX = (gpr)(int32_t)0x800F93AA;
}

// recomp_survey_register(survey): the survey's struct, once, at the first frame.
extern "C" void recomp_survey_register(uint8_t* rdram, recomp_context* ctx) {
    survey_vram = ctx->r4;
}

// The hook: omOvlGotoEx(overlay, event, stat)'s arguments, replaced by the request's if there is one and the game is
// going to the overlay it names in `from` (or `from` is -1). An event or stat of -1 keeps the game's own. A request of
// 1 is taken once; one of 2 every time until a script clears it. With MP_SURVEY_LOG set it prints each call, as the
// game makes it and the minigame GwSystem has chosen, which is how a game's trips between overlays were read.
extern "C" void mp_survey_goto(uint8_t* rdram, gpr* overlay, gpr* event, gpr* stat) {
    static const bool log = getenv("MP_SURVEY_LOG") != nullptr;
    if (log) {
        fprintf(stderr, "[survey] omOvlGotoEx(0x%02X, %d, 0x%X) minigame %d groups %d %d %d %d flags %04X %04X %04X %04X\n",
            (int32_t)*overlay, (int16_t)*event, (uint16_t)*stat, (int16_t)MEM_H(0, MINIGAME_INDEX),
            MEM_B(0, GW_PLAYER), MEM_B(PLAYER_SIZE, GW_PLAYER), MEM_B(2 * PLAYER_SIZE, GW_PLAYER),
            MEM_B(3 * PLAYER_SIZE, GW_PLAYER), (uint16_t)MEM_H(6, GW_PLAYER), (uint16_t)MEM_H(PLAYER_SIZE + 6, GW_PLAYER),
            (uint16_t)MEM_H(2 * PLAYER_SIZE + 6, GW_PLAYER), (uint16_t)MEM_H(3 * PLAYER_SIZE + 6, GW_PLAYER));
    }
    if (survey_vram == 0 || MEM_W(REQUEST, survey_vram) == 0) {
        return;
    }
    int32_t from = MEM_W(FROM, survey_vram);
    if (from != -1 && from != (int32_t)*overlay) {
        return;
    }
    *overlay = (int32_t)MEM_W(OVERLAY, survey_vram);
    if ((int32_t)MEM_W(EVENT, survey_vram) != -1) {
        *event = (int32_t)MEM_W(EVENT, survey_vram);
    }
    if ((int32_t)MEM_W(STAT, survey_vram) != -1) {
        *stat = (int32_t)MEM_W(STAT, survey_vram);
    }
    uint32_t groups = MEM_W(GROUPS, survey_vram);
    uint32_t computers = MEM_W(COMPUTERS, survey_vram);
    for (int player = 0; player < 4; player++) {
        gpr base = GW_PLAYER + player * PLAYER_SIZE;
        if (groups != 0xFFFFFFFF) {
            MEM_B(0, base) = (groups >> (24 - player * 8)) & 0xFF;
        }
        if (computers & 0x80000000) {
            MEM_B(7, base) = (MEM_B(7, base) & ~1) | ((computers >> player) & 1);
        }
    }
    int32_t minigame = MEM_W(MINIGAME, survey_vram);
    if (minigame != -1) {
        MEM_H(0, MINIGAME_INDEX) = (int16_t)minigame;
    }
    int32_t board = MEM_W(BOARD, survey_vram);
    if (board != -1) {
        MEM_H(0, BOARD_INDEX) = (int16_t)board;
    }
    if (MEM_W(REQUEST, survey_vram) == 1) {
        MEM_W(REQUEST, survey_vram) = 0;
    }
    MEM_W(WARPS, survey_vram) = MEM_W(WARPS, survey_vram) + 1;
}
