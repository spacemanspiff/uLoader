#ifndef __FAT_H__
#define __FAT_H__

#include "types.h"

int FAT_copy_file ( const char *filepath_ori, const char *filepath_dest );
int FAT_write_file ( const char *filepath, void *buffer, int len );
int FAT_read_file ( const char *filepath, void **buffer, int *len );
s32 FAT_DeleteDir ( const char *dirpath );
void FAT_Install ( const char *nandPath, const char **install_paths, int count );
int init_nand ( const char *nandPath );

#endif
