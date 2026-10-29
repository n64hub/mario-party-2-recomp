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

// The controllers' setup and what it uses (src/169B0.c, all of it asm in the decomp).
extern s16 D_800D819C_D8D9C;
extern OSMesgQueue D_800D81A0_D8DA0;
extern OSMesg D_800D81B8_D8DB8;
extern s16 gNumOfControllers;
s32 func_80016840_17440(u16* channel);
s16 func_800169AC_175AC(s16 ch, s32 flags);
void func_80016BBC_177BC(s8 x, s8 y);
void func_80017244_17E44(void);

#define PAD_INIT func_800169AC_175AC
#define PAD_INIT_SI func_80016840_17440
#define PAD_REQUEST func_8007EE0C_7FA0C
#define PAD_REQUEST_T unkfunc_8007EE0C
#define PAD_AUTO_READ func_80017244_17E44
#define PAD_AUTO_READING D_800D819C_D8D9C
#define PAD_STICK_LIMIT func_80016BBC_177BC
#define PAD_LOCK D_800D81A0_D8DA0
#define PAD_LOCK_MESG D_800D81B8_D8DB8
#define PAD_CONTROLLERS gNumOfControllers

#endif
