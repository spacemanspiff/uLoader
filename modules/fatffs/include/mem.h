#ifndef _MEM_H_
#define _MEM_H_

#include "types.h"

/* Prototypes */
s32   Mem_Init(void);
void *Mem_Alloc(u32 size);
void  Mem_Free(void *ptr);

void *Mem_Alloc0(u32 size);
void  Mem_Free0(void *ptr);

#endif
