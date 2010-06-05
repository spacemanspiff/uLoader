/* uLoader- forwarder by Hermes */

/*   
	Copyright (C) 2009 Hermes
   
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


#include <stdio.h>
#include <string.h>

#include <gccore.h>
#include <ogcsys.h>

typedef struct _dolheader {
	u32 section_pos[18];
	u32 section_start[18];
	u32 section_size[18];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;

void *dol_data = NULL;
int   dol_len  = 0;

extern void patch_dol(void *Address, int Section_Size, int sel);

#if 0

extern void _start(void);

#include "gfx.h"

u32 check_dol() 
{
	int i;
	dolheader *dol_header;

	dol_header = (dolheader *)dol_data;

	PX=0;
	PY=50;
	letter_size(12,24);
	bkcolor = 0xff000000;
	color = 0xffffffff;

	s_printf("entry: %x\n", dol_header->entry_point);
	s_printf("bss: %x %x\n", dol_header->bss_start,
		 dol_header->bss_size);

	if(dol_header->bss_start)
		memset ((void *) dol_header->bss_start, 0, dol_header->bss_size);

	for (i = 0; i < 18; i++) {
		if ((!dol_header->section_size[i]) || 
		    (dol_header->section_start[i] < 0x100))
			continue;
		s_printf("section: %x %x\n", dol_header->section_start[i], 
			 dol_header->section_size[i]);
	}

	Screen_flip();
	sleep(60);
	return dol_header->entry_point;

}
#endif

void wipreset();
void wipregisteroffset(u32 dst, u32 len);

u32 load_dol() 
{
	int i;
	dolheader *dol_header;
	u32 current_addr = 0;

	wipreset();

	dol_header = (dolheader *) dol_data;

	if(dol_header->bss_start)
		memset ((void *) dol_header->bss_start, 0, dol_header->bss_size);

	for (i = 0; i < 18; i++) {
		if ((!dol_header->section_size[i]) || 
		    (dol_header->section_start[i] < 0x100)) 
			continue;

		current_addr = dol_header->section_start[i];
		if (!(current_addr & 0x80000000)) 
			dol_header->section_start[i] |= 0x80000000;

		if (i < 7) {
			ICInvalidateRange ((void *) dol_header->section_start[i], 
					            dol_header->section_size[i]);
		}
			
		memcpy ((void *) dol_header->section_start[i], 
			 dol_data+dol_header->section_pos[i], 
			dol_header->section_size[i]);

		if (i >= 7 || 1) {
			DCFlushRange ((void *) dol_header->section_start[i], 
				      dol_header->section_size[i]);
		}

		if (current_addr & 0x80000000) {
			patch_dol((void *) dol_header->section_start[i], 
				           dol_header->section_size[i],1);
			/*
			if (i >= 3)
			        wipregisteroffset((u32)dol_header->section_start[i], 
				                       dol_header->section_size[i]);
			*/
		}
	}

	if (dol_header->bss_start)
		DCFlushRange((void *) dol_header->bss_start, 
			              dol_header->bss_size);

	return dol_header->entry_point;
}



