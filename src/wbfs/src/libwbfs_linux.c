#ifdef WIN32
#include <windows.h>

#else

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>

#endif

#include "libwbfs.h"

#include "os.h"

static int wbfs_fread_sector(HANDLE_TYPE handle,u32 lba,u32 count,void*buf)
{
        u64 off = lba;
        off*=512ULL;
	if (os_seek(handle, off))
        {
                fprintf(stderr,"\n\n%lld %d\n",off,count);
		wbfs_error("error seeking in disc partition");
                return 1;
        }
        if (os_read(handle, buf, count*512ULL, NULL) != 1){
                wbfs_error("error reading disc");
                return 1;
        }
        return 0;
  
}

static int wbfs_fwrite_sector(HANDLE_TYPE handle,u32 lba,u32 count,void*buf)
{
        u64 off = lba;
        off*=512ULL;
	if (os_seek(handle, off))
        {
		wbfs_error("error seeking in disc file");
                return 1;
        }
        if (os_write(handle, buf, count*512ULL, NULL) ){
                wbfs_error("error writing disc");
                return 1;
        }
        return 0;
  
}

static void close_handle(HANDLE_TYPE handle)
{
	os_close(handle);
}

static int get_capacity(char *file,u32 *sector_size,u32 *n_sector)
{
	return os_get_capacity(file, sector_size, n_sector);
}

wbfs_t *wbfs_try_open_hd(char *fn,int reset)
{
        u32 sector_size, n_sector;
        if(!get_capacity(fn,&sector_size,&n_sector))
                return NULL;
	HANDLE_TYPE f = os_open(fn, FILE_READWRITE);
        if (f == INVALID_HANDLE_VALUE)
                return NULL;
        return wbfs_open_hd(wbfs_fread_sector,	wbfs_fwrite_sector,
			close_handle, f, sector_size, n_sector,reset);
}
wbfs_t *wbfs_try_open_partition(char *fn,int reset)
{
        u32 sector_size, n_sector;
	wbfs_t * ret;

        if(!get_capacity(fn,&sector_size,&n_sector))
                return NULL;
	HANDLE_TYPE f = os_open(fn, FILE_READWRITE);
        if (f == INVALID_HANDLE_VALUE)
                return NULL;
        ret= wbfs_open_partition(wbfs_fread_sector,wbfs_fwrite_sector,close_handle,f,
                                   sector_size ,n_sector,0,reset);

	if(!ret) os_close(f);
	return ret;
}
wbfs_t *wbfs_try_open(char *disc,char *partition, int reset)
{
        wbfs_t *p = 0;
        if(partition)
                p = wbfs_try_open_partition(partition,reset);
        if (!p && !reset && disc)
                p = wbfs_try_open_hd(disc,0);
        else if(!p && !reset){
                char buffer[32];
                int i;
                for (i='c';i<'z';i++)
                {
                        snprintf(buffer,32,"/dev/sd%c",i);
                        p = wbfs_try_open_hd(buffer,0);
                        if(p)
                        {
                                fprintf(stderr,"using %s\n",buffer);
                                return p;
                        }
                        snprintf(buffer,32,"/dev/hd%c",i);
                        p = wbfs_try_open_hd(buffer,0);
                        if(p)
                        {
                                fprintf(stderr,"using %s\n",buffer);
                                return p;
                        }                        
                }
                wbfs_error("cannot find any wbfs partition (verify permissions))");
        }
        return p;
        
}

