# The process scheduler

Hudson's engine runs most of a game as processes, small functions with stacks of their own that a scheduler runs in
turn once a frame and that switch with the game's own setjmp and longjmp, which recompiled code can't do. Here each
live process runs on a native thread of its own, one at a time, from the code the Mario Party recomps share:
`lib/mario-party-engine` ([its docs/processes.md](https://github.com/n64hub/mario-party-engine/blob/main/docs/processes.md)
says how it works and why). This file says what is Mario Party 2's.

## What is Mario Party 2's

- `patches/engine_names.h` gives the engine's patches the decomp's names (`src/game/process.c`), which are Mario
  Party's but for the heap a process leaves, freed with `HuMemMemoryFreePerm` where Mario Party frees it with
  `HuMemDirectFree`. The decomp declares the scheduler's globals and its `EXEC_PROCESS_*` values in `process.c`
  itself, so the header declares them too.
- The natives' lines are in `patches/syms.ld`.
- The game calls setjmp and longjmp nowhere else (only `HuPrcCall`, `HuPrcSleep`, `HuPrcChildWatch` and
  `HuPrcTerminate`, which the patches replace), and `src/game/libultra.cpp`'s setjmp aborts.
- Its swap chain draws each frame in several RSP tasks as Mario Party's does, so the game presents at the VI
  (`src/main/main.cpp`).

## How much it costs

Built with `-DMP_PROCESS_STATS=ON`, the program prints every 5 seconds how many processes are alive, how many threads
it has made, how many resumes a second the scheduler makes and what each costs beyond the process's own run (the two
handoffs, from the scheduler's thread to the process's and back).

On the desktop, on a busy host, from the logos through Mario Land's greeting into the board Toad teaches the rules on,
which four computer players play: the title screens keep up to 24 processes alive, and the board 19 to 21 (up to 24
as a turn's screens come and go), with about 570 to 620 resumes a second. 39 threads are made in all. Each resume
costs 11 to 23 us beyond the process, which is 7 to 13 ms a second, about 1% of a core. A board here keeps a few more
processes alive than Mario Party's (15 to 18, about 450 resumes a second).
