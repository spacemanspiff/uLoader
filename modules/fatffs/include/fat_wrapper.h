#ifndef _FAT_WRAPPER_H_
#define _FAT_WRAPPER_H_

#include <sys/stat.h>
#include "types.h"

/* Filestats structure */
typedef struct _fstats {
	u32 file_length;
	u32 file_pos;
} fstats;

/* Prototypes */
s32 FAT_Open(const char *path, u32 mode);
s32 FAT_Close(s32 fd);
s32 FAT_Read(s32 fd, void *buffer, u32 len);
s32 FAT_Write(s32 fd, void *buffer, u32 len);
s32 FAT_Seek(s32 fd, u32 where, u32 whence);
s32 FAT_CreateDir(const char *dirpath);
s32 FAT_CreateFile(const char *filepath);
s32 FAT_ReadDir(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen);
s32 FAT_ReadDir_short(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen);
s32 FAT_Delete(const char *path);
s32 FAT_DeleteDir(const char *dirpath);
s32 FAT_Rename(const char *oldname, const char *newname);
s32 FAT_Stat(const char *path, void *stats);
s32 FAT_GetVfsStats(const char *path, void *stats);
s32 FAT_GetFileStats(s32 fd, fstats *stats);
s32 FAT_GetUsage(const char *dirpath, u64 *size, u32 *files);

#endif
