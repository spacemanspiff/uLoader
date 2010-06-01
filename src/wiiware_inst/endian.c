/*
	Copyright (C) 2010 Spaceman Spiff.

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
#include "endian.h"

u16 be_u16 ( u16 n )
{
        u8 * v= ( u8 * ) &n;
        return ( v[0] << 8 ) | v[1];
}

/*
static s16 be_s16 ( s16 n )
{
        u8 * v= ( u8 * ) &n;
        return ( s16 ) ( v[0] << 8 ) | v[1];
}

static s32 be_s32 ( s32 n )
{
        u8 * v= ( u8 * ) &n;
        return ( s32 ) ( ( v[0] << 24 ) | ( v[1] << 16 ) | ( v[2] << 8 ) | v[3] );
}
*/

static u32 be_u32 ( u32 n )
{
        u8 * v= ( u8 * ) &n;
        return ( ( v[0] << 24 ) | ( v[1] << 16 ) | ( v[2] << 8 ) | v[3] );
}


static u64 be_u64 ( u64 n )
{
        u32 * v= ( u32 * ) &n;
        return ( u64 ) ( ( u64 ) be_u32 ( v[0] ) << 32 ) | be_u32 ( v[1] );
}

static u64 be_u64_titleId ( u64 n )
{
        u32 * v= ( u32 * ) &n;
        return ( u64 ) ( ( u64 ) be_u32 ( v[1] ) << 32 ) | be_u32 ( v[0] );
}

void fix_shared_entry_endian ( shared_entry *entry )
{
        entry->title_id = be_u32 ( entry->title_id );
        entry->cindex = be_u32 ( entry->cindex );
        entry->n_shared = be_u16 ( entry->n_shared );
}

void fix_wad_header_endian ( wadHeader *h )
{
        h->header_len = be_u32 ( h->header_len );
        h->type = be_u16 ( h->type );
        h->padding = be_u16 ( h->padding );
        h->certs_len = be_u32 ( h->certs_len );
        h->crl_len = be_u32 ( h->crl_len );
        h->tik_len = be_u32 ( h->tik_len );
        h->tmd_len = be_u32 ( h->tmd_len );
        h->data_len = be_u32 ( h->data_len );
        h->footer_len = be_u32 ( h->footer_len );

        /*
          printf("header_len: %lx\n", h->header_len);
          printf("type: %x\n", h->type);
          printf("h->padding: %x\n", h->padding );
          printf("h->certs_len: %lx\n", h->certs_len );
          printf("h->crl_len: %lx\n", h->crl_len );
          printf("h->tik_len: %lx\n", h->tik_len );
          printf("h->tmd_len: %lx\n", h->tmd_len );
          printf("h->data_len: %lx\n", h->data_len );
          printf("h->footer_len: %lx\n", h->footer_len );
        */
}

void fix_tmd_endian ( tmd *Tmd )
{
        int i;

        Tmd->sys_version = be_u64 ( Tmd->sys_version );
        Tmd->title_id = be_u64_titleId ( Tmd->title_id );
        Tmd->title_type = be_u32 ( Tmd->title_type );
        Tmd->group_id = be_u16 ( Tmd->group_id );
        Tmd->zero = be_u16 ( Tmd->zero );
        Tmd->region = be_u16 ( Tmd->region );
        Tmd->access_rights = be_u32 ( Tmd->access_rights );
        Tmd->title_version = be_u16 ( Tmd->title_version );
        Tmd->num_contents = be_u16 ( Tmd->num_contents );
        Tmd->boot_index = be_u16 ( Tmd->boot_index );
        Tmd->fill3 = be_u16 ( Tmd->fill3 );

        for ( i = 0; i < Tmd->num_contents; i++ ) {
                Tmd->contents[i].cid = be_u32 ( Tmd->contents[i].cid );
                Tmd->contents[i].index = be_u16 ( Tmd->contents[i].index );
                Tmd->contents[i].type = be_u16 ( Tmd->contents[i].type );
                Tmd->contents[i].size = be_u64 ( Tmd->contents[i].size );
        }
}


