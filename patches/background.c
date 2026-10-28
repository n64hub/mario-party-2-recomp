// A board's background to a picture wider than 4:3. A board is a map of 64x48 tiles (src/51630.c's
// func_80050B20_51720 loads one), which the game streams in from the ROM as its camera moves: func_80051A74_52674 picks
// the 6x6 tiles under the camera's 320x240, func_800518B8_524B8 gives each a slot of the 50 it has and sends it to a
// thread that reads it (func_80051054_51C54) and one that decompresses it (func_800512F8_51EF8), and
// func_80052B4C_5374C draws them, each as a 64x32 and a 64x16 rectangle (func_80051BCC_527CC), before the board's 3D,
// zoomed in by D_800CC210_CCE10, mirrored while D_800DF9C0_E05C0 has 0x10, and with another bank's bytes for some
// tiles while D_800DFE5C_E0A5C is set. RT64's Aspect Ratio Expand widens the 3D but can't draw tiles the game never
// drew, and RT64 keeps a 4:3 picture's rectangles inside 4:3, so the board drew in a 4:3 box with its 3D spaces and
// characters out past it on black.
//
// Here the game picks the tiles under a 21:9 picture and a row more above and below (what a frame RT64 draws between
// two of the game's can reach), those under 4:3 first, from 138 slots (the game's 50 and 88 more here), through queues
// as long, and draws them through lib/mario-party-engine's board_back.c: as quads at their places on the map, in a list
// of their own with black behind them, kept to the picture's aspect and moved by an interpolated scroll, so the
// window's sides show more of the map, and the board pans with its 3D spaces. A tile past 4:3 is sent to be read only
// while fewer than EXTRA_IN_FLIGHT are on their way, since each holds its compressed bytes on the game's heap
// (HuMemMemoryAllocPerm) until it is decompressed, and the game sized that heap for its 50.
//
// The game keeps the board's camera where a 320x240 picture stays on the map (func_80050E98_51A98, which every board's
// camera goes through), so at the map's edge the wider picture ran past it, where there are no tiles, and a black
// strip opened at the window's side. Here the camera is kept where the wider picture stays on the map
// (lib/mario-party-engine's src/aspect_ratio.cpp gives its width), so it stops short of the edge by as much of the
// map as the window shows past 4:3, and all a 4:3 picture showed there is still on screen. A map narrower than the
// picture keeps its camera in its middle, with black at both sides. Everything else is the game's as it was.

#include "patches.h"
#include "game/malloc.h"
#include "../lib/mario-party-engine/patches/board_back.h"

#define TILE_WIDTH 64
#define TILE_HEIGHT 48
// The picture past 4:3 on each side at 21:9, in the game's pixels.
#define WIDE_SIDE 120
#define COLUMNS 11
#define ROWS 8
#define GAME_COLUMNS 6
#define GAME_ROWS 6
#define GAME_SLOTS 50
#define SLOTS (GAME_SLOTS + COLUMNS * ROWS)
#define SLOT_BYTES 0x1800
#define EXTRA_IN_FLIGHT 12

// src/51630.c's own: a tile's slot.
typedef struct {
    /* 0x00 */ u8 unk0;    // wanted this frame
    /* 0x01 */ u8 unk1;    // the bank its bytes are from, 0 for the map's own
    /* 0x02 */ s16 unk2;   // the tile, or -1 for none
    /* 0x04 */ u8* data;   // its 64x48 pixels, 16-bit, decompressed
    /* 0x08 */ void* archive; // its compressed bytes, while they are read and decompressed
    /* 0x0C */ u32 size;   // their size
    /* 0x10 */ u8* bytes;  // and place in the ROM
} BgSlot;

_Static_assert(sizeof(BgSlot) == 0x14, "BgSlot's size");

// The map's header (its first 0x3C bytes): a tile's size, and the map's in tiles.
typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
} MapSize;

// A bank of tiles that take other bytes while it is on: how many and which, and where their bytes are.
typedef struct {
    /* 0x00 */ s32 count;
    /* 0x04 */ s32* tiles;
    /* 0x08 */ s32* offsets;
} TileBank;

extern MapSize* D_800CC208_CCE08;
extern u8 D_800CC20C_CCE0C;
extern f32 D_800CC210_CCE10;
extern Gfx D_800CC228_CCE28[];
extern u16 D_800DF9C0_E05C0;
extern s16 D_800DF9C2_E05C2;
extern s16 D_800DF9C4_E05C4;
extern s16 D_800DF9C6_E05C6;
extern s16 D_800DF9C8_E05C8;
extern BgSlot D_800DF9D8_E05D8[GAME_SLOTS];
extern BgSlot* D_800DFDC0_E09C0[GAME_COLUMNS][GAME_ROWS];
extern s32 D_800DFE5C_E0A5C;
extern s32 D_800DFE60_E0A60;
extern TileBank* D_800DFE64_E0A64;
extern OSMesgQueue D_800DFE68_E0A68;
extern OSThread D_800DFF48_E0B48;
extern OSThread D_800E0900_E1500;
extern s32 D_800E0AB0_E16B0;
extern OSMesgQueue D_800E12C0_E1EC0;
extern OSMesgQueue D_800E13A0_E1FA0;
extern OSMesgQueue D_800E1480_E2080;
extern OSMesg D_800E1498_E2098[2];
// The board camera's viewport's place, in quarter pixels at a zoom of 1 (func_80053230_53E30's).
extern f32 D_800E18A8_E24A8[2];

void func_80051054_51C54(void);
void func_800512F8_51EF8(void);
void func_800514F0_520F0(void);
void func_80051510_52110(void);
void func_8005154C_5214C(void);
u16 func_80051834_52434(s32 column, s32 row);
u32 func_80051864_52464(u16 tile);
u8* func_8005188C_5248C(u16 tile);
u8* func_80053EB0_54AB0(u16 index);
u32 func_80053EEC_54AEC(u16 index);
void* func_80027138_27D38(u32 size);
void func_800271D8_27DD8(void* ptr);
void func_800A5530_A6130(void* ptr, s32 size);
s16 func_8007C058_7CC58(s32 arg0, s32 arg1, s32 arg2);
void func_8007C094_7CC94(s16 saved);
f32 mp_aspect_ratio(void);

// The slots past the game's 50, and their tiles.
static BgSlot extra_slots[SLOTS - GAME_SLOTS];
static u8 extra_slot_data[SLOTS - GAME_SLOTS][SLOT_BYTES] __attribute__((aligned(16)));
// The tiles to draw: [column][row], from (first_column, first_row) on the map.
static BgSlot* grid[COLUMNS][ROWS];
static s32 first_column, first_row;
// The three queues' messages, as many as there are slots.
static OSMesg read_messages[SLOTS];
static OSMesg decompress_messages[SLOTS];
static OSMesg done_messages[SLOTS];

static BgSlot* slot(s32 i) {
    return (i < GAME_SLOTS) ? &D_800DF9D8_E05D8[i] : &extra_slots[i - GAME_SLOTS];
}

RECOMP_PATCH void func_800513FC_51FFC(void) {
    // @recomp Queues as long as there are slots.
    osCreateMesgQueue(&D_800DFE68_E0A68, read_messages, SLOTS);
    osCreateMesgQueue(&D_800E12C0_E1EC0, decompress_messages, SLOTS);
    osCreateMesgQueue(&D_800E13A0_E1FA0, done_messages, SLOTS);
    osCreateMesgQueue(&D_800E1480_E2080, D_800E1498_E2098, 2);
    D_800E0AB0_E16B0 = 1;
    osCreateThread(&D_800E0900_E1500, 0x64, (void (*)(void*))func_800512F8_51EF8, NULL, &D_800E12C0_E1EC0, 1);
    osStartThread(&D_800E0900_E1500);
    osCreateThread(&D_800DFF48_E0B48, 0x65, (void (*)(void*))func_80051054_51C54, NULL, &D_800E0900_E1500, 4);
    osStartThread(&D_800DFF48_E0B48);
}

RECOMP_PATCH void func_8005159C_5219C(void) {
    s32 i;

    osJamMesg(&D_800DFE68_E0A68, NULL, OS_MESG_NOBLOCK);
    osJamMesg(&D_800E12C0_E1EC0, NULL, OS_MESG_NOBLOCK);
    func_800514F0_520F0();
    func_80051510_52110();
    osRecvMesg(&D_800E1480_E2080, NULL, OS_MESG_BLOCK);
    osRecvMesg(&D_800E1480_E2080, NULL, OS_MESG_BLOCK);
    // @recomp Every slot.
    for (i = 0; i < SLOTS; i++) {
        BgSlot* s = slot(i);
        if (s->archive != NULL) {
            HuMemMemoryFreeTemp(s->archive);
            s->archive = NULL;
        }
    }
}

RECOMP_PATCH void func_80051664_52264(void) {
    s32 i;

    for (i = 0; i < SLOTS; i++) {
        BgSlot* s = slot(i);
        s->unk0 = 0;
        s->unk1 = 0;
        s->unk2 = -1;
        // @recomp The game's 50 from its heap, as before, the others from here.
        s->data = (i < GAME_SLOTS) ? func_80027138_27D38(SLOT_BYTES) : extra_slot_data[i - GAME_SLOTS];
        s->archive = NULL;
    }
    func_800A5530_A6130(D_800DFDC0_E09C0, sizeof(D_800DFDC0_E09C0));
    func_800A5530_A6130(grid, sizeof(grid));
}

RECOMP_PATCH void func_800516DC_522DC(void) {
    s32 i;

    for (i = 0; i < SLOTS; i++) {
        BgSlot* s = slot(i);
        if ((s->archive == NULL) && (s->unk0 == 0)) {
            s->unk2 = -1;
        }
        s->unk0 = 0;
    }
}

RECOMP_PATCH void func_80051720_52320(void) {
    s32 i;

    for (i = 0; i < SLOTS; i++) {
        BgSlot* s = slot(i);
        s->unk0 = 0;
        s->unk2 = -1;
        // @recomp Only the game's 50 came from its heap.
        if ((i < GAME_SLOTS) && (s->data != NULL)) {
            func_800271D8_27DD8(s->data);
        }
        s->data = NULL;
    }
}

// func_800518B8_524B8: the slot holding a tile, or a free one it is sent to be read into, or NULL if there is none or
// it isn't under the 4:3 picture and too many are on their way. Only func_80051A74_52674 calls it.
static BgSlot* tile_slot(u16 tile, s32 games) {
    BgSlot* free_slot = NULL;
    u16 bank = 0;
    u16 bank_index = 0;
    s32 in_flight = 0;
    s32 i;

    if (D_800DFE5C_E0A5C != 0) {
        for (i = 0; i < D_800DFE64_E0A64[D_800DFE60_E0A60].count - 1; i++) {
            if (tile == D_800DFE64_E0A64[D_800DFE60_E0A60].tiles[i]) {
                bank = D_800DFE5C_E0A5C;
                bank_index = i;
            }
        }
    }

    for (i = 0; i < SLOTS; i++) {
        BgSlot* s = slot(i);
        if (s->unk2 == -1) {
            free_slot = s;
        } else if ((s->unk2 == tile) && (s->unk1 == bank)) {
            s->unk0 = 1;
            return s;
        }
        in_flight += (s->archive != NULL);
    }

    // @recomp A tile past 4:3 only while few are on their way.
    if ((free_slot != NULL) && (games || (in_flight < EXTRA_IN_FLIGHT))) {
        free_slot->unk0 = 1;
        free_slot->unk2 = tile;
        free_slot->unk1 = bank;
        if (bank != 0) {
            free_slot->bytes = func_80053EB0_54AB0(bank_index);
            free_slot->size = func_80053EEC_54AEC(bank_index);
        } else {
            free_slot->bytes = func_8005188C_5248C(tile);
            free_slot->size = func_80051864_52464(tile);
        }
        free_slot->archive = HuMemMemoryAllocPerm(free_slot->size);
        osSendMesg(&D_800DFE68_E0A68, free_slot, OS_MESG_NOBLOCK);
        return free_slot;
    }
    return NULL;
}

// How much of the map the window shows past each side of the 4:3 picture, at the zoom: a screen x is the map's
// (x - left) * zoom, and the window reaches 120 * aspect from the picture's middle, as far as the map has room for.
static f32 map_past_4x3(f32 zoom) {
    f32 side = 120.0f * mp_aspect_ratio() - 160.0f;

    if (side > D_800DF9C6_E05C6 * zoom - 160.0f) {
        side = D_800DF9C6_E05C6 * zoom - 160.0f;
    }
    if (side < 0.0f) {
        side = 0.0f;
    }
    return side / zoom;
}

// func_80050E98_51A98: the camera's place at the scale arg1, kept where the picture stays on the map; what it was kept
// from, a bit for each edge.
RECOMP_PATCH u16 func_80050E98_51A98(Vec2f* arg0, f32 arg1) {
    // @recomp Each x bound in by the map the window shows past 4:3.
    f32 side = map_past_4x3(D_800CC210_CCE10) / arg1;
    u16 ret = 0;

    if (arg0->unk_00 <= ((-D_800DF9C6_E05C6 + 160.0f) / arg1) + 160.0f + side) {
        arg0->unk_00 = ((-D_800DF9C6_E05C6 + 160.0f) / arg1) + 160.0f + side;
        ret = 1;
    }
    if (((D_800DF9C6_E05C6 - 160.0f) / arg1) + 160.0f + ((320.0f / arg1) - (320.0f / (arg1 * D_800CC210_CCE10))) - side <= arg0->unk_00) {
        arg0->unk_00 = ((D_800DF9C6_E05C6 - 160.0f) / arg1) + 160.0f + ((320.0f / arg1) - (320.0f / (arg1 * D_800CC210_CCE10))) - side;
        ret |= 2;
    }
    if (arg0->unk_04 <= ((-D_800DF9C8_E05C8 + 120.0f) / arg1) + 120.0f) {
        arg0->unk_04 = ((-D_800DF9C8_E05C8 + 120.0f) / arg1) + 120.0f;
        ret |= 4;
    }
    if (((D_800DF9C8_E05C8 - 120.0f) / arg1) + 120.0f + ((240.0f / arg1) - (240.0f / (arg1 * D_800CC210_CCE10))) <= arg0->unk_04) {
        arg0->unk_04 = ((D_800DF9C8_E05C8 - 120.0f) / arg1) + 120.0f + ((240.0f / arg1) - (240.0f / (arg1 * D_800CC210_CCE10)));
        ret |= 8;
    }
    return ret;
}

// Where the picture's left and top edges are on the map at a zoom of 1, as func_80051BCC_527CC draws: the scroll, less
// the part of a pixel the 3D's viewport (D_800E18A8, in quarter pixels) is past a whole one.
static f32 left_edge(void) {
    f32 quarters = D_800E18A8_E24A8[0] / 4.0f;
    return (f32)((D_800DF9C6_E05C6 + D_800DF9C2_E05C2) - 160) - (quarters - (f32)(s32)quarters);
}

static f32 top_edge(void) {
    f32 quarters = D_800E18A8_E24A8[1] / 4.0f;
    return (f32)((D_800DF9C8_E05C8 + D_800DF9C4_E05C4) - 120) - (quarters - (f32)(s32)quarters);
}

static s32 floor_div(f32 value, s32 by) {
    s32 whole = (s32)value;
    if ((f32)whole > value) {
        whole--;
    }
    return (whole >= 0) ? (whole / by) : -((by - 1 - whole) / by);
}

// Whether a tile's x (or y) on the map, from..to, reaches into [low, high), the 4:3 picture's.
static s32 inside(s32 from, s32 to, f32 low, f32 high) {
    return (to > low) && (from < high);
}

RECOMP_PATCH void func_80051A74_52674(void) {
    f32 zoom = D_800CC210_CCE10;
    f32 left = left_edge();
    f32 top = top_edge();
    // The map under the 4:3 picture, and under the wide one: a screen x is the map's (x - left) * zoom, or mirrored,
    // (320 - (x - left)) * zoom.
    f32 low_x = (D_800DF9C0_E05C0 & 0x10) ? (left + 320.0f - 320.0f / zoom) : left;
    f32 high_x = low_x + 320.0f / zoom;
    f32 low_y = top;
    f32 high_y = top + 240.0f / zoom;
    s32 pass, i, j;

    first_column = floor_div(low_x - WIDE_SIDE / zoom, TILE_WIDTH);
    first_row = floor_div(low_y - TILE_HEIGHT / zoom, TILE_HEIGHT);

    // @recomp The tiles under the 4:3 picture first, so that they come first when there are too few slots and are
    // never held back, then the others.
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < ROWS; i++) {
            s32 row = first_row + i;
            for (j = 0; j < COLUMNS; j++) {
                s32 column = first_column + j;
                s32 games = inside(column * TILE_WIDTH, (column + 1) * TILE_WIDTH, low_x, high_x) &&
                            inside(row * TILE_HEIGHT, (row + 1) * TILE_HEIGHT, low_y, high_y);
                if (games != (pass == 0)) {
                    continue;
                }
                if ((column < 0) || (row < 0) || (column >= D_800CC208_CCE08->unk8) ||
                    (row >= D_800CC208_CCE08->unkC)) {
                    grid[j][i] = NULL;
                } else {
                    grid[j][i] = tile_slot(func_80051834_52434(column, row), games);
                }
            }
        }
    }
}

// A command as the game writes it (the combiner and render modes func_80052B4C_5374C sets).
static void gfx_words(Gfx** gfx, u32 w0, u32 w1) {
    Gfx* g = (*gfx)++;
    g->words.w0 = w0;
    g->words.w1 = w1;
}

// The tile at (j, i) in the grid, or NULL past the grid or where there is none.
static u8* tile_data(s32 j, s32 i) {
    return ((j < COLUMNS) && (i < ROWS) && (grid[j][i] != NULL)) ? grid[j][i]->data : NULL;
}

RECOMP_PATCH void func_80052B4C_5374C(Gfx** game_gfx, s32 arg1, u8 arg2) {
    s16 saved;
    s32 i, j;
    Gfx** gfx;

    if ((arg2 != 0) || !(D_800DF9C0_E05C0 & 4)) {
        return;
    }
    func_8005154C_5214C();
    if (!(D_800DF9C0_E05C0 & 1)) {
        return;
    }
    func_800516DC_522DC();
    saved = func_8007C058_7CC58(0, 0, 0);
    func_80051A74_52674();
    func_8007C094_7CC94(saved);

    // @recomp In a list of the tiles' own, black behind them, to the window's edges.
    gfx = board_back_begin(game_gfx);
    gSPDisplayList((*gfx)++, D_800CC228_CCE28);
    gfx_words(gfx, 0xFB000000, 0xFFFFFF00 | D_800CC20C_CCE0C);
    if (D_800CC20C_CCE0C < 0xFF) {
        if (D_800DF9C0_E05C0 & 8) {
            gfx_words(gfx, 0xFC52FEA5, 0x11FFFBFD);
        } else {
            gfx_words(gfx, 0xFCFFFFFF, 0xFFFCFA7D);
        }
        gfx_words(gfx, 0xE200001C, 0x00504240);
    } else {
        if (D_800DF9C0_E05C0 & 8) {
            gfx_words(gfx, 0xFC52FEA5, 0x11FFF3F9);
        }
        gfx_words(gfx, 0xE200001C, 0x00552248);
    }

    // @recomp The tiles at their places on the map, moved by the scroll, zoomed and mirrored as the game draws them.
    board_back_view(gfx, left_edge(), top_edge(), D_800CC210_CCE10, (D_800DF9C0_E05C0 & 0x10) != 0,
                    D_800CC208_CCE08->unk8 * TILE_WIDTH);
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLUMNS; j++) {
            if (grid[j][i] != NULL) {
                // @recomp func_80051BCC_527CC's 64x32 and 64x16 parts, each loaded alone and filtered, met in a
                // seam at a high resolution: loaded with their neighbours' edge texels (board_back.c says why).
                board_back_tile(gfx, (first_column + j) * TILE_WIDTH, (first_row + i) * TILE_HEIGHT, grid[j][i]->data,
                                tile_data(j + 1, i), tile_data(j, i + 1), tile_data(j + 1, i + 1));
            }
        }
    }
    board_back_end(gfx);
    func_80051510_52110();
}
