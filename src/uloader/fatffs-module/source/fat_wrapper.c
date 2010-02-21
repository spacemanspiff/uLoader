/*   
	Custom IOS Module (FAT)

	Copyright (C) 2008 neimod.
	Copyright (C) 2009 WiiGator.
	Copyright (C) 2009 Waninkoko.

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

#include <string.h>
#include <fcntl.h>

#include "errors.h"
#include "fat.h"
#include "fat_wrapper.h"
#include "ipc.h"
#include "mem.h"
#include "types.h"

#include "libfat/fatdir.h"
#include "libfat/fatfile.h"

/* Variables */
static struct _reent fReent;

s32 __FAT_GetError(void)
{
	/* Return error code */
	return fReent._errno;
}

s32 __FAT_OpenDir(const char *dirpath, DIR_ITER *dir)
{
	DIR_ITER         *result = NULL;
	DIR_STATE_STRUCT *state  = NULL;

	/* Allocate memory */
	state = Mem_Alloc(sizeof(DIR_STATE_STRUCT));
	if (!state)
		{
		return IPC_ENOMEM;
		}
	/* Clear buffer */
	memset(state, 0, sizeof(DIR_STATE_STRUCT));

	/* Prepare dir iterator */
	dir->device    = 0;
	dir->dirStruct = state;

	/* Clear error code */
	fReent._errno = 0;

	/* Open directory */
	result = _FAT_diropen_r(&fReent, dir, dirpath);

	if (!result) {
		/* Free memory */
		Mem_Free(state);

		/* Return error */
		return __FAT_GetError();
	}

	return 0;
}

void __FAT_CloseDir(DIR_ITER *dir)
{
	/* Close directory */
	_FAT_dirclose_r(&fReent, dir);

	/* Free memory */
	Mem_Free(dir->dirStruct);
}

FILE_STRUCT *global_fs = NULL;

s32 FAT_Open(const char *path, u32 mode)
{
	FILE_STRUCT *fs = NULL;
	int n;
	s32 ret;

	if(!global_fs)
	{
	/* Allocate memory */
		global_fs = Mem_Alloc(sizeof(FILE_STRUCT)*32);
		if(!global_fs) return -101;

	/* Clean memory */
		memset(global_fs, 0, sizeof(FILE_STRUCT)*32);
	}

	for(n=0;n<32;n++)
	{
		if(global_fs[n].inUse==0) fs=&global_fs[n];
	}
	if (!fs)
		return IPC_ENOMEM;

	/* Set mode */
	/*
	if (mode > 0)
		mode--;
*/
	/* Clear error code */
	fReent._errno = 0;

	/* Open file */
	ret = _FAT_open_r(&fReent, fs, path, mode, 0);	
	if (ret < 0) {
		/* Free memory */
		Mem_Free(fs);

		return __FAT_GetError();
	}

	return ret;
}

s32 FAT_Close(s32 fd)
{
s32 ret;
	/* Close file */
	ret=_FAT_close_r(&fReent, fd);

	return ret;
}

s32 FAT_Read(s32 fd, void *buffer, u32 len)
{
	s32 ret;

		/* Clear error code */
		fReent._errno = 0;
		
		/* Read file */
		ret = _FAT_read_r(&fReent, fd, buffer, len);
		if (ret < 0)
			{ret = __FAT_GetError();}

	return ret;
}

s32 FAT_Write(s32 fd, void *buffer, u32 len)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Write file */
	ret = _FAT_write_r(&fReent, fd, buffer, len);
	if (ret < 0)
		ret = __FAT_GetError();
	if(len>=16384)
		_FAT_syncToDisc ((FILE_STRUCT*) fd);
	return ret;
}

s32 FAT_Seek(s32 fd, u32 where, u32 whence)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Seek file */
	ret = _FAT_seek_r(&fReent, fd, where, whence);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}
#ifndef TINY_FAT

s32 FAT_CreateDir(const char *dirpath)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Create directory */
	ret = _FAT_mkdir_r(&fReent, dirpath, 0);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

#endif

s32 FAT_CreateFile(const char *filepath)
{
	s32         ret;

	/* Clear error code */
	fReent._errno = 0;
	
	ret=FAT_Open(filepath, O_CREAT | O_RDWR);
	if (ret < 0) return ret;

	FAT_Close(ret);

	return 0;
}

s32 FAT_ReadDir(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen)
{
	DIR_ITER dir;

	u32 cnt = 0, pos = 0;
	s32 ret;
   
   fReent._errno = 0;
	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;
   

	/* Read entries */
	while (!maxlen || (maxlen > cnt)) {
		char namefile[256];
		char *filename = outbuf + pos;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, namefile, NULL)!=0)
			break;

		/* Non valid entry */
		if (namefile[0]=='.' || namefile[0]=='#')
			continue;
		
		if(outbuf)
		    {
			int len=strlen(namefile);
			memcpy(filename, namefile, len); filename[len]=0;
			}

		/* Increase counter */
		cnt++;

		/* Update position */
		pos += (outbuf) ? strlen(filename) + 1 : 0;
	}

	/* Output values */
	*outlen = cnt;

	if(outbuf) os_sync_after_write(outbuf, pos);

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}

s32 FAT_ReadDir_short(const char *dirpath, char *outbuf, u32 *outlen, u32 maxlen)
{
	DIR_ITER dir;

	u32 cnt = 0, pos = 0;
	s32 ret;
	int len;
    char *temp_buffer=NULL;
	
	if(outbuf) temp_buffer=Mem_Alloc(13 * *outlen +32);

	if(!temp_buffer) temp_buffer=outbuf;

   fReent._errno = 0;
	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	
	if (ret < 0)
		{
		if(outbuf && outbuf!=temp_buffer)
			{
			Mem_Free(temp_buffer);
			}
		return ret;
		}
   

	/* Read entries */
	while (!outbuf || /*!maxlen || */(maxlen > cnt)) {
		static char namefile[256];
		
		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, namefile, NULL)!=0)
			break;

		/* Non valid entry */
		if (namefile[0]=='.' || namefile[0]=='#')
			continue;
		
		if(outbuf)
		    {
			
			len=strlen(namefile); if(len>12) continue; // ignore invalid filename len>12 //len=12;
			
			strcpy(temp_buffer+pos, namefile);
			
			/* Update position */
			pos += len + 1;
			}

		/* Increase counter */
		cnt++;
		
	}

	/* Output values */
	*outlen = cnt;

	if(outbuf) 
	  {
	  if(outbuf!=temp_buffer)
		  {
		  memcpy(outbuf, temp_buffer, pos);Mem_Free(temp_buffer);
		  }
	  os_sync_after_write(outbuf, pos);
	  }

	/* Close directory */
	__FAT_CloseDir(&dir);

	//if(cnt==0) return ENOENT;

	return 0;
}

#ifndef TINY_FAT

s32 FAT_Delete(const char *path)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Delete file/directory */
	ret = _FAT_unlink_r(&fReent, path);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_DeleteDir(const char *dirpath)
{
	DIR_ITER dir;

	s32 ret;

	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;

	/* Read entries */
	for (;;) {
		char   filename[96], newpath[96]; // this is possible becuse i use strcpy and not strncpy in libfat
		struct stat filestat;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;

		/* Generate entry path */
		strcpy(newpath, dirpath);
		strcat(newpath, "/");
		strcat(newpath, filename);

		/* Delete directory contents */
		if (filestat.st_mode & S_IFDIR)
			FAT_DeleteDir(newpath);

		/* Delete object */
		ret = FAT_Delete(newpath);

		/* Error */
		if (ret < 0)
			break;
	}

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}

s32 FAT_Rename(const char *oldname, const char *newname)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Rename file/directory */
	ret = _FAT_rename_r(&fReent, oldname, newname);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
} 

s32 FAT_Stat(const char *path, void *stats)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Get stats */
	ret = _FAT_stat_r(&fReent, path, stats);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}

s32 FAT_GetVfsStats(const char *path, void *stats)
{
	s32 ret;

	/* Clear error code */
	fReent._errno = 0;

	/* Get filesystem stats */
	ret = _FAT_statvfs_r(&fReent, path, stats);
	if (ret < 0)
		ret = __FAT_GetError();

	return ret;
}
#endif 

s32 FAT_GetFileStats(s32 fd, fstats *stats)
{
	FILE_STRUCT *fs = (FILE_STRUCT *)fd;

	if (fs && fs->inUse) {
		/* Fill file stats */
		stats->file_length = fs->filesize;
		stats->file_pos    = fs->currentPosition;

		return 0;
	}

	return EINVAL;
}


s32 FAT_GetUsage(const char *dirpath, u64 *size, u32 *files)
{
	DIR_ITER dir;

	u64 totalSz  = 0;
	u32 totalCnt = 0;
	s32 ret;

	/* Open directory */
	ret = __FAT_OpenDir(dirpath, &dir);
	if (ret < 0)
		return ret;
    totalCnt++;
	totalSz += 0x4000;

	/* Read entries */
	for (;;) {
		char   filename[96];
		struct stat filestat;

		/* Read entry */
		if (_FAT_dirnext_r(&fReent, &dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.' || filename[0]=='#')
			continue;

		 if(strlen(filename)>12) continue; // prevent large names

		/* Directory or file */
		if (filestat.st_mode & S_IFDIR) {
			char newpath[96];

			u64  dirsize;
			u32  dirfiles;

			/* Generate directory path */
			strcpy(newpath, dirpath);
			strcat(newpath, "/");
			strcat(newpath, filename);

			/* Get directory usage */
			ret = FAT_GetUsage(newpath, &dirsize, &dirfiles);
			if (ret >= 0) {
				/* Update variables */
				totalSz  += dirsize;
				totalCnt += dirfiles;
			}
		} else
			totalSz += filestat.st_size;

		/* Increment counter */
		totalCnt++;
	}

	/* Output values */
	*size  = totalSz;
	*files = totalCnt;

	/* Close directory */
	__FAT_CloseDir(&dir);

	return 0;
}

