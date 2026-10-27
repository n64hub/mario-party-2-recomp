# The web page

The game plays in a browser: built with Emscripten on N64Recomp-browser's `frontend/` into
`MarioParty2Recompiled.html` (the launcher, the recomp's menus, SDL2's audio and keyboard, the ROM from Load ROM, and
the saves kept in IndexedDB; `lib/N64Recomp-browser/frontend/CMakeLists.txt`'s header says what each part does), drawn
through WebGPU. The page is built from your ROM like the desktop program, so it is never built in CI and never
published with the game in it.

What is the game's own stays here: the `EMSCRIPTEN` branches of `CMakeLists.txt` that make the game a page,
`tools/build_web.sh`, the shader list (`src/rt64_shaders.txt`, `tools/shader_plan.json`, `tools/record_shaders.sh`),
the play plan (`tools/play_plan.json`) and the survey (`patches/survey.c`, `src/game/survey.cpp`, `tools/survey.js`).
The rest is N64Recomp-browser's: `docs/porting.md`
(WebGPU and the shaders, prebuilt shader lists, Emscripten), `docs/playing.md` (playing and measuring a page),
`docs/forks.md` and `docs/findings.md`. Every play of this game's page is a line in N64Recomp-browser's `plays.jsonl`
(game `mp2`), never a record here.

The engine's process scheduler runs on threads on the page as on the desktop (`docs/processes.md`): the page has
threads and C++ exceptions anyway, so it needs no Asyncify.

## Build it

On Linux with Docker, with the ROM at `roms/marioparty2.us.z64`:

```bash
tools/build_web.sh                  # [cpus] [cmake arguments]
```

It builds `build/web/MarioParty2Recompiled.html` with its `.js`, `.wasm` (about 20 MB) and `.data` (about 9 MB: the
recomp's fonts and menus, nothing of the game) in `tools/docker/Dockerfile.web`, the image every N64Recomp-browser page
is built in, with ccache in `build/ccache-web`. The script's header says each step. The patches library (`patches/`,
built for MIPS) is recompiled by the native N64Recomp that `tools/recompile.sh` builds, not the runtime's, which
Emscripten would build for the browser.

Serve `build/web` over HTTP from localhost (any static server; the page brings its own `coi_serviceworker.js` for the
headers threads need) and open `MarioParty2Recompiled.html` in Chrome or Firefox with WebGPU, then Load ROM.

## The shader list

`src/rt64_shaders.txt` is the page's prebuilt shader list, in sections: the boot's, built before the first frame, then
the rules' board's, one a board and one a run of minigames, each built as the game first draws from it. It is recorded
by code, not by playing (`tools/record_shaders.sh`'s header says how to run it): `tools/shader_plan.json` reaches Mario
Land as the play plan does, then the survey takes Rules Land's trip to the rules' board to each of the six boards, with
four computer players, and to every minigame in turn, the duels and Chance Time among them, each played to its end by
four computer players. A section the game moves past without drawing from it is recorded again alone with `--only`.

## The survey

A plan can take the game to any overlay by itself, without anyone playing a board to its minigames: `patches/survey.c`
asks, from the engine's process loop, and `src/game/survey.cpp`, hooked onto `omOvlGotoEx` (`us.toml`), makes the next
time the game leaves for the overlay a script names go to the one asked for instead, leaving where it was in its own
way. `tools/survey.js` is what a plan evaluates in the page to use it (its header lists the calls). Going to a
minigame, a request also sets what a board sets as it starts one: the teams (GwPlayer's group), which players the
computer plays, and the minigame's ID, which the explanation screen (overlay 0x5F for IDs below 30, 0x60 for the
others) reads to load it. A minigame is left as it ends, never by a call of `omOvlGotoEx` from elsewhere: a plan asks
for the next one while the current one plays, so its own ending goes there. The debug menu's table in the ROM
(`ovl_00_Debug`'s `debug_ovl_table`) names every overlay with its minigame's ID, and the menu's groups for each.

## The plan

`tools/play_plan.json` reaches a board as a player does, by the keyboard's defaults (Enter is Start, Space A): Start
through the logos, the opening and the title into Mario Land, then A through Toad's greeting, the warp pipe and Rules
Land, on a new save, to where Rules Land leaves for the board Toad teaches the rules on, which the survey turns into
Western Land, where four computer players play with the board engine. Which screen is up is the overlay `omOvlGotoEx` last went to, a word at 0x800FA63C: Mario Land is 0x5B, Rules Land
0x4D and its board 0x4C, the six boards 0x3E (Western Land), 0x41, 0x43, 0x45, 0x47 and 0x49 (Bowser Land), their
duels 0x3F to 0x4A between them, the minigames 0x01 to 0x3C, a minigame's explanation 0x5F or 0x60 and its results
0x6F.
