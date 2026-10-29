#include "patches.h"

// The runtime registers the code inside ROM 0x1000 plus 1 MB at startup, which is main, and any other code only when
// it is told it was loaded (recomp_load_overlays). Every overlay (a minigame, a board, a menu, all 115 of them at VRAM
// 0x80102800) comes in through OvlLoad (src/util.c), by its index in _ovltbl.

// The decomp declares these in src/util.c itself.
typedef struct OverlayTable {
/* 0x00 */ u8* romStart;
/* 0x04 */ u8* romEnd;
/* 0x08 */ u8* vramStart;
/* 0x0C */ u8* textVramStart;
/* 0x10 */ u8* textVramEnd;
/* 0x14 */ u8* dataVramStart;
/* 0x18 */ u8* dataVramEnd;
/* 0x1C */ u8* bssVramStart;
/* 0x20 */ u8* bssVramEnd;
} OverlayTable;

extern OverlayTable _ovltbl[];
extern u8 D_800C8870_C9470;
void HuRomDmaCodeRead(u8* src, u8* dest, s32 size);
// widescreen.c's: whether the scene an overlay plays is drawn at 4:3.
void widescreen_overlay(s32 overlay);

RECOMP_PATCH void OvlLoad(s32 overlayIndex) {
    u8* romStart;
    u8* romEnd;
    u8* bssStart;
    u8* bssEnd;
    u8* curBssAddr;

    romStart = _ovltbl[overlayIndex].romStart;
    romEnd = _ovltbl[overlayIndex].romEnd;
    bssStart = _ovltbl[overlayIndex].bssVramStart;
    bssEnd = _ovltbl[overlayIndex].bssVramEnd;

    // @recomp Register the code this brings in.
    recomp_load_overlays((u32) romStart, _ovltbl[overlayIndex].vramStart, romEnd - romStart);

    HuRomDmaCodeRead(romStart, _ovltbl[overlayIndex].vramStart, romEnd - romStart);
    curBssAddr = bssStart;
    while (bssStart < bssEnd) {
        *curBssAddr++ = 0;
        bssStart++;
    }

    D_800C8870_C9470 = 0;

    // @recomp Whether the scene is drawn at 4:3 (widescreen.c).
    widescreen_overlay(overlayIndex);
}
