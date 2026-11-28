// Models the game reads from its ROM, mended as DataRead hands them over (the hook at its return, us.toml), before
// Hu3DModelCreate builds them.
//
// Crazy Cutters' drill (file 0x33001E, a FORM model of one quad). Each player has two copies of it, the drill and the
// chips flying off it, and the game puts a texture animation on each (files 0x330002 and 0x330001), whose frames are
// 32x32 texels, one frame loaded at a time. The quad's face reaches V 0.3359375 (0x3EAC0011) of its 96-row picture:
// 32.25 rows, a quarter texel past the frame's last row, into texels that are not the frame's (what the texture memory
// holds past the load). The filter takes a texel's middle at its whole number, so every pixel past row 31 draws part of
// row 32. On a console that band is under a sixth of a pixel at the quad's bottom, which a pixel's middle seldom falls
// in, but drawn at a higher resolution it is a row or more of the window's pixels: a thin dark line under each player's
// drill, the whole minigame (issue #118). Row 31 is empty in every frame of both animations, so the quad stops at row
// 31 instead: its two bottom corners move up from 0 to 62 of its 1600 units of height (1600 x 1.25 / 32.25) and its
// face's V to 31 rows (0.32291666, 0x3EA55555), which keeps the picture's size and place and leaves out only the empty
// row and the quarter texel past it.

#include <cstdint>

#include "recomp.h"

namespace {
    // The file, by what it holds: DataRead's return has only the file's number in its directory, not the directory.
    // "FORM" and its size, the vertex (VTX1) chunk and the face (FAC1) chunk.
    constexpr uint32_t FORM_SIZE = 0x904;
    constexpr int VTX_CHUNK = 0x1A2;
    constexpr uint8_t VTX[] = {
        0x56, 0x54, 0x58, 0x31, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x04, 0x00, 0x01, 0x3D, 0x80, 0x00, 0x00, 0xFE, 0x70,
        0x06, 0x40, 0x02, 0x30, 0x00, 0x00, 0x77, 0xFE, 0x70, 0x00, 0x00, 0x02, 0x30, 0x00, 0x00, 0x77, 0x01, 0x90,
        0x00, 0x00, 0x02, 0x30, 0x00, 0x00, 0x77, 0x01, 0x90, 0x06, 0x40, 0x02, 0x30, 0x00, 0x00, 0x77,
    };
    constexpr int FACE_CHUNK = 0x1D6;
    constexpr uint8_t FACE[] = {
        0x46, 0x41, 0x43, 0x31, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00,
        0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x00,
        0x00, 0x3E, 0xAC, 0x00, 0x11, 0x00, 0x02, 0xFF, 0xFF, 0x3F, 0x80, 0x00, 0x00, 0x3E, 0xAC, 0x00, 0x11, 0x00,
        0x03, 0xFF, 0xFF, 0x3F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    struct Mend {
        int at;
        uint8_t bytes[4];
        int count;
    };
    constexpr Mend MENDS[] = {
        { 0x1BD, { 0x00, 0x3E }, 2 },             // vertex 1's y (bottom left): 0 -> 62
        { 0x1C6, { 0x00, 0x3E }, 2 },             // vertex 2's y (bottom right): 0 -> 62
        { 0x1FB, { 0x3E, 0xA5, 0x55, 0x55 }, 4 }, // vertex 1's V: 32.25 rows -> 31
        { 0x207, { 0x3E, 0xA5, 0x55, 0x55 }, 4 }, // vertex 2's V: 32.25 rows -> 31
    };

    bool matches(uint8_t* rdram, gpr data, int at, const uint8_t* bytes, int count) {
        for (int i = 0; i < count; i++) {
            if (MEM_BU(at + i, data) != bytes[i]) {
                return false;
            }
        }
        return true;
    }
}

// mp2_data_read(file, data): the file's number in its directory and what DataRead decoded (0 if nothing).
extern "C" void mp2_data_read(uint8_t* rdram, int32_t file, gpr data) {
    if ((file != 0x1E) || (data == 0)) {
        return;
    }
    if ((MEM_W(0, data) != 0x464F524D) || ((uint32_t)MEM_W(4, data) != FORM_SIZE)) {
        return;
    }
    if (!matches(rdram, data, VTX_CHUNK, VTX, (int)sizeof(VTX)) || !matches(rdram, data, FACE_CHUNK, FACE, (int)sizeof(FACE))) {
        return;
    }
    for (const Mend& mend : MENDS) {
        for (int i = 0; i < mend.count; i++) {
            MEM_B(mend.at + i, data) = (int8_t)mend.bytes[i];
        }
    }
}
