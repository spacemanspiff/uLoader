#ifndef _FFS_EMU_H_
#define _FFS_EMU_H_

#include <string.h>

#include "fat.h"
#include "fat_wrapper.h"
#include "ipc.h"
#include "mem.h"
#include "module.h"
#include "sdio.h"
#include "syscalls.h"
#include "timer.h"
#include "types.h"
#include "usbstorage.h"

#include "tools.h"


#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "isfs.h"

#include "errors.h"
 
#define FFS_IOCTL_FORMAT	0x01
#define FFS_IOCTL_GETSTATS	0x02
#define FFS_IOCTL_CREATEDIR	0x03
#define FFS_IOCTLV_READDIR    0x04
#define FFS_IOCTL_SETATTR	0x05
#define FFS_IOCTL_GETATTR	0x06
#define FFS_IOCTL_DELETE	0x07
#define FFS_IOCTL_RENAME	0x08
#define FFS_IOCTL_CREATEFILE	0x09
#define FFS_IOCTL_UNKNOWN_0A	0x0A
#define FFS_IOCTL_GETFILESTATS	0x0B
#define FFS_IOCTLV_GETUSAGE	0x0C
#define FFS_IOCTL_SHUTDOWN	0x0D

#define FFS_IOCTL_SETNANDEMULATION 0x64

int ffs_Init(void);
int ffs_FileStats(int fd, void *filestat);
int ffs_VFSStats(const char *path, struct statvfs *vfsstats);
int ffs_Stat(const char *filename, struct stat *statdata);
int ffs_Rename(const char *oldname, const char *newname);
int ffs_DeleteDir(const char *filename);
int ffs_Delete(const char *filename);
int ffs_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen);
int ffs_MakeFile(const char *filename);
int ffs_MakeDir(const char *dirname);
s32 ffs_GetUsage(const char *dirpath, u32 *size, u32 *ionodes);

#endif
