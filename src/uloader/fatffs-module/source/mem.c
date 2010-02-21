/*   
	Custom IOS Module (FAT)

	Copyright (C) 2008 neimod.
	Copyright (C) 2009 WiiGator.
	Copyright (C) 2009 Waninkoko.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "syscalls.h"
#include "timer.h"

int swi_mload(u32 arg0, u32 arg1, u32 arg2, u32 arg3);

#if 1
/* Heapspace */
static u32 heapspace[0x7000] ATTRIBUTE_ALIGN(32);

/* Variables */
static s32 hid = -1;


s32 Mem_Init(void)
{
	/* Heap already created */
	if (hid >= 0)
		return 0;

	/* Create heap */
	hid = os_heap_create(heapspace, sizeof(heapspace));

	return (hid >= 0) ? 0 : -1;
}
#else
s32 hid=-1;


s32 Mem_Init(void)
{
	hid=0;
	return 0;
}

#endif

void *Mem_Alloc(u32 size)
{
void *p;
//hid=0;
	/* Allocate memory */
	p= os_heap_alloc_aligned(hid, size, 32);
    if(!p)
		{
		while(1) {swi_mload(128,0,0,0);Timer_Sleep(500*1000);swi_mload(129,0,0,0);Timer_Sleep(500*1000);}
		}

return p;

}

void Mem_Free(void *ptr)
{
//hid=0;
	/* Free memory */
	os_heap_free(hid, ptr);
}

