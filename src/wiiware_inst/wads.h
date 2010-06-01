#ifndef __WADS_H__
#define __WADS_H__

#include "types.h"
/* 'WAD Header' structure */
typedef struct {
        /* Header length */
        u32 header_len;

        /* WAD type */
        u16 type;

        u16 padding;

        /* Data length */
        u32 certs_len;
        u32 crl_len;
        u32 tik_len;
        u32 tmd_len;
        u32 data_len;
        u32 footer_len;
} __attribute ( ( packed ) ) wadHeader;

int Install_from_wad ( const char *nandPath, const char *filename );

#endif
