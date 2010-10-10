#ifndef _FAT_UTIL_
#define _FAT_UTIL_

#include "global.h"

extern u32 ERR_FATFSS_APP;

extern char temp_read_buffer[16384];

s32 FAT_DeleteDir(const char *dirpath);

s32 FFS_to_FAT_Copy(const char *ffsdirpath, const char *fatdirpath); // copy directory contents

s32 FFS_to_FAT_File_Copy(const char *ffsdirpath, const char *fatdirpath); // copy files

void create_FAT_FFS_Directory(struct discHdr *header); // creates directories for DVD games (00010000 or 00010004)

int test_FAT_game(char * directory);

char *get_FAT_directory1(void);

char *get_FAT_directory2(void);

char *get_FFS_directory1(void);

char *get_FFS_directory2(void);

int FAT_read_file(const char *filepath, void **buffer, int *len);

int FAT_write_file(const char *filepath, void *buffer, int len);

int FAT_copy_file(const char *filepath_ori, const char *filepath_dest);

extern u32 title_ios;

int FAT_get_title(u64 id, void **dol, u8 *str_id, int dont_use_boot); // get the dol in MEM2 memory

int FAT_Identify(void); // create ticket and use system cert to pass ES_Identify in the new IOS

void FAT_Install(int is_sd); // install from sd/ud wad files (sd:/nand/install)

void scan_for_shared(int is_sd); // scan sd:/nand/title/00010001 and get the newest shared content in sd:/nand/shared folder

int Force_Update(u64 title, int is_sd); // use the hash to identify the content and update scanning all installed titles 

#endif

