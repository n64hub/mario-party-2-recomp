#ifndef ENGINE_NAMES_H
#define ENGINE_NAMES_H

// Mario Party 2's names for what lib/mario-party-engine's patches use (its README lists them).

#include "process.h"
#include "game/malloc.h"

// The decomp declares these in src/game/process.c itself.
extern s16 processcnt;
extern Process* processtop;
extern Process* processcur;

#define EXEC_PROCESS_DEFAULT 0
#define EXEC_PROCESS_SLEEPING 1
#define EXEC_PROCESS_WATCH 2
#define EXEC_PROCESS_DEAD 3

void HuUnlinkPrc(Process** root, Process* process);

#define PRC_SYS_INIT HuPrcInit
#define PRC_CHILD_WAIT HuPrcChildWatch
#define PRC_END HuPrcEnd
#define PRC_UNLINK HuUnlinkPrc
#define PRC_HEAP_FREE HuMemMemoryFreePerm
#define PRC_TOP processtop
#define PRC_CURRENT processcur
#define PRC_COUNT processcnt

#endif
