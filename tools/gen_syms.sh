#!/usr/bin/env bash
# Writes syms/marioparty2.us.syms.toml and syms/marioparty2.us.datasyms.toml from the ELF tools/build_decomp.sh brought back
# (build/decomp/marioparty2.elf) and your ROM: builds the pinned N64Recomp (tools/build_n64recomp.sh) and
# runs lib/mario-party-engine/tools/gen_syms.py, which says how.
#
# Usage: tools/gen_syms.sh [cmake arguments for tools/build_n64recomp.sh]
set -euo pipefail
cd "$(dirname "$0")/.."
elf=build/decomp/marioparty2.elf
rom=roms/marioparty2.us.z64
[[ -f "$elf" ]] || { echo "no $elf: run tools/build_decomp.sh first" >&2; exit 1; }
[[ -f "$rom" ]] || { echo "no $rom: put your Mario Party 2 (USA) .z64 there" >&2; exit 1; }
bash tools/build_n64recomp.sh "$@"
if command -v py > /dev/null; then python=(py -3); else python=(python3); fi
"${python[@]}" -I lib/mario-party-engine/tools/gen_syms.py build/n64recomp/N64Recomp "$elf" "$rom" syms/marioparty2.us
# The decomp leaves libultra's osViGetNextFramebuffer (0x800A6A70) unnamed. Named, N64Recomp calls the runtime's in its
# place, as it does osViGetCurrentFramebuffer's: the game's own copy reads the next framebuffer from libultra's VI state,
# which the runtime never writes. The graphics thread picks each frame's framebuffer as one that is neither shown nor
# next (0x8001CC50), so with that stale answer it could take the one waiting to be shown and draw the next frame into
# it: a new scene's first frame, its star wipe not drawn yet, flashed on screen.
sed -i 's/{ name = "func_800A6A70_A7670", /{ name = "osViGetNextFramebuffer", /' syms/marioparty2.us.syms.toml
