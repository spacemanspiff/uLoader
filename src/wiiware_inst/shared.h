#ifndef __SHARED_H__
#define __SHARED_H__

#include "types.h"

typedef struct {
        u32 title_id;
        u32 cindex;
        u8 ios;
        u8 minor_ios;
        u16 n_shared;
        u8 hash[20];
} __attribute ( ( packed ) ) shared_entry;

void init_shared_list ( const char *nandPath );
void release_shared_list ( void );
int update_shared_list ( const char *nandPath );
int write_shared_list ( const char *nandPath );
#endif
