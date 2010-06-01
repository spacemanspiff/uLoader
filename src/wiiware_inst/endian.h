#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include "types.h"
#include "es.h"
#include "wads.h"
#include "shared.h"

u16 be_u16 ( u16 n );
void fix_shared_entry_endian ( shared_entry *entry );
void fix_wad_header_endian ( wadHeader *h );
void fix_tmd_endian ( tmd *Tmd );
#endif
