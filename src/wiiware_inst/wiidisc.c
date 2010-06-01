// Copyright 2009 Kwiirk based on negentig.c:
// Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Modified by Hermes
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <string.h>

#include "types.h"
#include "rijndael.h"

void _decrypt_title_key ( u8 *tik, u8 *title_key )
{
        u8 common_key[16]={
                0xeb, 0xe4, 0x2a, 0x22, 0x5e, 0x85, 0x93, 0xe4, 0x48, 0xd9, 0xc5, 0x45,
                0x73, 0x81, 0xaa, 0xf7
        };
        u8 iv[16];

        memset ( iv, 0, sizeof iv );
        memcpy ( iv, tik + 0x01dc, 8 );
        aes_set_key ( common_key );
        //_aes_cbc_dec(common_key, iv, tik + 0x01bf, 16, title_key);
        aes_decrypt ( iv, tik + 0x01bf,title_key,16 );
}
