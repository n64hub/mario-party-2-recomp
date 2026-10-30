# CLAUDE.md

A static recompilation of the N64 *Mario Party 2* (USA) with N64Recomp, N64ModernRuntime, RecompFrontend and RT64, from
the mariopartyrd/marioparty2 symbols. Almost all of it is written by AI agents, so this file is what the next one
needs: the rules, where things are, and the facts about the game the code can't say on its own.

## What lives where

This repo is the game: its recompilation, the patches over it, the desktop build and its web page.

- `syms/`: the symbols N64Recomp reads, generated (never edited by hand) by `tools/build_decomp.sh` then
  `tools/gen_syms.sh` (`lib/mario-party-engine/tools/gen_syms.py` says how).
- `us.toml`: N64Recomp's configuration; `tools/recompile.sh` writes `RecompiledFuncs/` with it, and the audio
  microcode with `aspMain.us.toml`.
- `patches/` and `patches.toml`: MIPS C against `lib/marioparty2`'s headers, recompiled into PatchesLib. Its
  `ui_funcs.h`, `patch_helpers.h` and `recompui_event_structs.h` are RecompFrontend's, which includes the first from
  the game's `patches/`.
- `src/`: RecompFrontend's main (`src/main/main.cpp`, with the GameEntry), the natives the patches call
  (`src/game/recomp_api.cpp`; the process scheduler's are `lib/mario-party-engine`'s), the libultra functions the
  runtime lacks (`src/game/libultra.cpp`) and the settings.
- `lib/`: `marioparty2` (the decomp, for the symbols and the patches' headers), `mario-party-engine` (the code the
  Mario Party recomps share, below), upstream `rt64`, `N64ModernRuntime` and
  `RecompFrontend` at the commits N64Recomp-browser's web forks are built on, and `N64Recomp-browser` at a release
  tag. The web support every recomp's page shares is developed and documented there; `tools/build_n64recomp.sh` and
  `CMakeLists.txt` apply its `patches/` to the other three.
- `docs/processes.md`: what is this game's in the engine's process scheduler, whose code and how it runs on threads
  are `lib/mario-party-engine`'s. `docs/web.md`: the web page, its shader list (`src/rt64_shaders.txt`, recorded by
  `tools/shader_plan.json`) and the survey (`patches/survey.c`, `src/game/survey.cpp`, `tools/survey.js`) that takes a
  plan to any board or minigame.
- Comments beside the code, for how and why: each file's header. Read the header of the file you change and update
  it in the same commit.

## Rules

- No ROMs, ROM-derived assets or extracted game data in git, CI or PRs (`tools/ci/check_repo.py` checks). Nothing
  from the decomp's build is committed but `syms/`. The launcher's art is text only (`icons/`): no logo, character or
  picture from the game.
- The decomp is built only where the ROM is, never in CI.
- The desktop releases are built where the ROM is too, by `tools/release.sh` (its header says how), and published to
  GitHub Releases with their notes in `releases/`.
- The scope is the game as it plays, plus what a web page needs (RT64 interpolation to the display's rate, resolution
  Auto), plus modern enhancements: each an option with an Original value that plays as the console does, none
  changing how the game plays. USA only.
- The code the Mario Party recomps share is `lib/mario-party-engine` (n64hub/mario-party-engine): the process
  scheduler's patches and natives, a camera's view, a board's background and its HUD past 4:3 and `gen_syms.py`,
  written once against this game's names in `patches/engine_names.h` (the HUD's against the addresses its `us.toml`
  hooks pass). A change to it is a commit there that each game's submodule moves onto, and code this game
  would copy from another Mario Party recomp goes there instead.
- Write for a stranger, in the present tense: nothing about a contributor's machine, servers, paths or accounts, and
  nothing about where anyone keeps ROMs.
- GPL-3.0, as the recompilations it follows.

## How it fits together

- **The code.** `main` is stored raw in the ROM after the boot code, and the runtime maps what lies in ROM 0x1000
  plus 1 MB at start. Every other code section is one of the 115 overlays (a minigame, a board, a menu), all of them
  at VRAM 0x80102800, which the game loads through `OvlLoad` (src/util.c) by its index in `_ovltbl`;
  `patches/overlays.c` patches it to tell the runtime.
- **The process scheduler.** Most of the game runs as the engine's processes, which switch with its own setjmp and
  longjmp; here each runs on a thread of its own, one at a time (`docs/processes.md`). The game has no other setjmp
  or longjmp, and `src/game/libultra.cpp`'s setjmp aborts.
- **The game's threads.** The main thread (id 3) runs the scheduler once a frame; the RSP tasks (5), the VI and its
  swaps (0x10), the cartridge's reads (0x14) and audio (0x64, 0x65) have threads of their own, and 0x11 to 0x13 are
  more task and VI threads (`src/main/main.cpp` names them).
- **No 64DD.** The game asks whether one is attached by reading the PI's and the drive's registers directly
  (`func_800989C0_995C0`, libleo's check); `patches/leo.c` answers no, as a console without one does. `us.toml` stubs
  the 64DD's boot of a disk game, and
  `src/game/libultra.cpp` has the 64DD's and the Controller Pak's libultra functions N64Recomp leaves out, which only
  that hardware reaches.
- **Microcodes.** Graphics go to RT64. Audio: aspMain, recompiled by RSPRecomp from `aspMain.us.toml` (its header
  says how its layout was found; it is Bomberman 64's microcode, byte for byte).
- **Overlays by number.** The debug menu's table in the ROM (`ovl_00_Debug`'s `debug_ovl_table`) names every overlay
  with its minigame's ID, and its groups table each minigame's teams; `docs/web.md` (The plan) lists the ones plans
  use. A board opens a minigame by its explanation screen (0x5F for IDs below 30, 0x60 for the others), which reads
  GwSystem's chosenMinigameIndex; the board code reads GwSystem's current_board_index for its own tables.
-  **Drawing.** The game draws a frame in several RSP tasks, one a camera (`func_800204C0_210C0`), which RT64
  interpolates as one game frame with RT64_GAME_FRAME_WORKLOADS, on with Emscripten and in `CMakeLists.txt` on the
  desktop too. It draws a picture wider than 4:3 with Aspect Ratio Expand: `patches/widescreen.c` takes a camera's view
  to the window's edges and `patches/background.c` a board's background, through lib/mario-party-engine's
  `camera_view.c` and `board_back.c` (their headers say how), and keeps the sprites DispCheck would cull outside 4:3.
  `us.toml`'s hooks in the sprite manager draw the board's HUD, the players' panels, at the window's left and right
  edges, and a picture as wide as the screen (Abandon Ship's sky) stretched over the window wherever the game puts it
  (kept to 4:3 in a scene the game draws at 4:3), through lib/mario-party-engine's `src/hud.cpp`, and its hooks in the
  wipe module draw a wipe's colour (an iris's black, a fade) to them too, through its `src/wipe.cpp`. Two hooks in
  `us.toml` keep the scissor the game's text sets inside the picture. The board's camera stops where the wider picture
  reaches the map's edge (`patches/background.c`, with the engine's `src/aspect_ratio.cpp`). A map no wider than 320
  (Rules Land's START, 5x5 tiles) keeps black at its sides, the game having nothing beside it to draw, and its 3D and
  the HUD keep inside 4:3 with it (`board_back.c`). So does a scene `patches/widescreen.c` lists as drawn at 4:3 (Crazy
  Cutters and nineteen more minigames), black drawn over the window's sides at each frame's end (`us.toml`, the engine's
  `src/wipe.cpp`). A minigame whose 3D ends short of a wide window's edges (Bumper Balls' lava, Sneak 'n' Snore's
  walls) is drawn no wider than where it ends, `patches/widescreen.c`'s second list, with black bars only past that.
- **Saves.** EEPROM, 4 Kbit. `main.cpp` declares `SaveType::Eep4k`; the runtime keeps it in `saves/`.
- **Input.** RecompFrontend's default bindings. Keyboard: WASD the stick, Space A, Left Shift B, Q Z, R R, E L,
  Enter Start, the arrows the C buttons.
