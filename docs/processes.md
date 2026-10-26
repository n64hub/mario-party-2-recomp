# The process scheduler

Hudson's engine, which Mario Party, Mario Party 2 and 3 and Bomberman 64 are built on, runs most of a game as
processes: small functions, each with a stack of its own in the engine's heap, which a scheduler (`HuPrcCall`, called
once a frame from the game's main thread) runs in turn. On a console they switch with the game's own setjmp and
longjmp, which recompiled code can't do.

Mario Party 2 runs them as [Mario Party: Recompiled](https://github.com/n64hub/mario-party-recomp) does, each live
process on a native thread of its own and exactly one of them running at a time, from the same code. That repo's
`docs/processes.md` says how it works and why; this one says only what is Mario Party 2's.

## What is Mario Party 2's

- `src/game/process_threads.cpp` is Mario Party: Recompiled's file as it is. A change to it is made there first and
  copied here.
- `patches/process.c` is that repo's, on Mario Party 2's names: the decomp (`src/game/process.c`) has the same five
  functions, `Process` and jmp_buf, and the scheduler frees an ended process's heap with `HuMemMemoryFreePerm` where
  Mario Party frees it with `HuMemDirectFree`. The decomp declares the scheduler's globals and its `EXEC_PROCESS_*`
  values in `process.c` itself, so the patch declares them too.
- The game calls setjmp and longjmp nowhere else (only `HuPrcCall`, `HuPrcSleep`, `HuPrcChildWatch` and
  `HuPrcTerminate`, which the patches replace), and `src/game/libultra.cpp`'s setjmp aborts.
- Its swap chain draws each frame in several RSP tasks as Mario Party's does, so the game presents at the VI
  (`src/main/main.cpp`).

## How much it costs

Built with `-DMP_PROCESS_STATS=ON`, the program prints every 5 seconds how many processes are alive, how many threads
it has made, how many resumes a second the scheduler makes and what each costs beyond the process's own run (the two
handoffs, from the scheduler's thread to the process's and back). With `MP_PROCESS_LOG` set it also prints each
process's start and each overlay the game loads.

On the desktop, on a busy host, from the logos through Mario Land's greeting into the board Toad teaches the rules on,
which four computer players play: the title screens keep up to 24 processes alive, and the board 19 to 21 (up to 24
as a turn's screens come and go), with about 570 to 620 resumes a second. 39 threads are made in all. Each resume costs 11 to 23 us beyond the process, which is 7 to 13 ms a second, about 1% of a
core. A board here keeps a few more processes alive than Mario Party's (15 to 18, about 450 resumes a second).
