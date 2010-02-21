/*
 * FFS plugin for Custom IOS.
 *
 * Copyright (C) 2010 Spaceman Spiff
 * Copyright (C) 2010 Hermes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */



#include <stdio.h>
#include "ffs_emu.h"

#include "swi_mload.h"

#include <fcntl.h>

#include <sys/stat.h>
#include <sys/statvfs.h>


//#define READ_FROM_FILE_ATTR
#define IOCTL_ES_LAUNCH	0x08

// flag emu =0->disable 1->enable

extern int flag_emu;
extern int light_on;
extern int verbose_level;

// device selection
extern s32 index_dev;
extern const char dev_names[8][12];

#define FAT_DATA_ALIGN	0x20
#define FAT_DATA_SIZE 	(sizeof(fat_data)) 

#define FILENAME_SIZE 		0x60
#define STAT_DATA_SIZE		(sizeof(struct stat))
#define VFSSTAT_DATA_SIZE	(sizeof(struct statvfs))
#define FILESTAT_DATA_SIZE	0x08

#define MAX_FILENAME_SIZE 0x100


static s32 fatHandle = -1;

////////////////////

// Make Aligned...
typedef struct {
	ioctlv vector[8];
	union {
		struct {
			char filename[FILENAME_SIZE];
		} basic;
		struct {
			char filename[FILENAME_SIZE];
			int mode;
		} open;
		struct {
			char filename[FILENAME_SIZE];
			int outlen;
		} readdir;
		struct {
			char oldfilename[FILENAME_SIZE];
			char newfilename[FILENAME_SIZE];
		} rename;
		struct {
			char filename[FILENAME_SIZE];
			struct stat data_stat;
		} stat;
		struct {
			char filename[FILENAME_SIZE];
			struct statvfs stat_vfs;
		} statvfs;
		struct {
			char data[FILESTAT_DATA_SIZE];
		} filestats;
		struct {
			char filename[FILENAME_SIZE];
			u32 size;
			u32 ionodes;
		} getusage;
	};
} ATTRIBUTE_PACKED fat_data;

static fat_data *fatDataPtr = NULL;


// es_plugin

void debug_printf(const char *format, ...);

int restore_ffs(u32 ios, u32 none);

extern int out_ES_ioctlv(void *);


int ES_ioctlv(ipcmessage *msg)
{
	s32 ret = 0;
	
//os_sync_before_read(msg, sizeof(ipcmessage));
if(verbose_level>1) debug_printf("ES ioctlv: 0x%x\n",msg->ioctlv.command);

	switch(msg->ioctlv.command) {

    case IOCTL_ES_LAUNCH:
		{
		u64 title;
		title=*((u64 *) msg->ioctlv.vector[0].data);
		if(verbose_level) debug_printf("Launch Title: 0x%x-0x%x\n",(u32) (title>>32), (u32) (title & 0xffffffff));
		
		os_open(DEVICE_FAT":shutdown", 0);

		swi_mload_call_func((void *) restore_ffs, NULL, NULL);

		if(verbose_level) debug_printf("FFS Shutdown\n");

		goto original_ioctlv;
		}

	default:
	original_ioctlv:
		ret = out_ES_ioctlv(msg);
	}
	return ret;
}



int ffs_Init(void)
{

	if (fatHandle<0) {
		if (!fatDataPtr) 
			fatDataPtr = os_heap_alloc_aligned(0, FAT_DATA_SIZE, FAT_DATA_ALIGN);

		s32 res = os_open("fat", 0);

		if (res < 0)        // ERROR ?
			return res;

		fatHandle = res;
	}
	return 0;
}

void preappend_nand_dev_name(const char *origname, char *newname)
{
	
	FFS_Strcpy(newname, &dev_names[index_dev][0]);
	FFS_Strcat(newname, origname);
}

static char filename1[MAX_FILENAME_SIZE] ATTRIBUTE_ALIGN(32);
static char filename2[MAX_FILENAME_SIZE] ATTRIBUTE_ALIGN(32);


char * title_to_folder(void)
{
u32 title=*((u32 *) 0); // ADDRESS 0
static u8 n_title[]="/00000000";

	int n;

	for(n=0;n<8;n++) 
		{
		u8 dat=(title>>(28-(n<<2))) & 15;
		if(dat>=10) dat+=87; else dat+=48;
		n_title[n+1]=dat;
		}

return (char *) n_title;
}

u32 get_group_id(void)
{
u16 title_id=*((u16 *) 4); // ADDRESS 4

return title_id;
}

#if 0
int MakeDirGame(void)
{

	FFS_Strcpy(filename1, &dev_names[index_dev][0]);
	ffs_MakeDir(filename1);

	FFS_Strcat(filename1, "/title");
	ffs_MakeDir(filename1);
	FFS_Strcat(filename1, "/00010000");
	ffs_MakeDir(filename1);
	FFS_Strcat(filename1, title_to_folder());
	ffs_MakeDir(filename1);
//FFS_Strcat(filename1, "/data");
//ffs_MakeDir(filename1);

return 0;

}
#endif

int cmp_string( char *s,const char *c) // this is not exactly a strcmp funtion...
{
int n=0;

	while(1)
		{
		if(s[n]==0 && c[n]==0) return 0;
		if(s[n]==0) return -1;
		if(c[n]==0) return 0;
		if(c[n]!='#' && s[n]!=c[n]) return -1;
		n++;
		}

return -1;
}

int test_string(char *s)
{

	if(!cmp_string(s, "/title/00010000")) return 0;
	if(!cmp_string(s, "/title/00010001")) return 0;
	if(!cmp_string(s, "/title/00010004")) return 0;
	if(!cmp_string(s, "/tmp/launch.sys")) return -1; // es launch use this
	if(!cmp_string(s, "/tmp")) return 0;
	if(!cmp_string(s, "/ticket/00010000")) return 0;
	if(!cmp_string(s, "/ticket/00010001")) return 0;
	if(!cmp_string(s, "/ticket/00010004")) return 0;
	if(!cmp_string(s, "/sys/disc.sys")) return 0;
	if(!cmp_string(s, "/sys/uid.sys")) return 0;

return -1;
}


void nand_emu_device(void)
{

	if(fatHandle<0)
		{
		// initialize the FAT device
		fatHandle=ffs_Init();

		if(fatHandle<0) return;

		// create FAT directories

		preappend_nand_dev_name("", filename1);
		ffs_MakeDir(filename1);
	
		preappend_nand_dev_name("/tmp", filename1);
		
		ffs_DeleteDir(filename1);
		ffs_MakeDir(filename1);

		preappend_nand_dev_name("/ticket", filename1);
		
		ffs_DeleteDir(filename1);
		ffs_MakeDir(filename1);

		//MakeDirGame();
		preappend_nand_dev_name("/sys", filename1);
		os_sync_after_write(filename1, MAX_FILENAME_SIZE);

		ffs_MakeDir(filename1);
		preappend_nand_dev_name("/title", filename1);
		ffs_MakeDir(filename1);
		preappend_nand_dev_name("/title/00010000", filename1);
		os_sync_after_write(filename1, MAX_FILENAME_SIZE);
		ffs_MakeDir(filename1);
		preappend_nand_dev_name("/title/00010001", filename1);
		os_sync_after_write(filename1, MAX_FILENAME_SIZE);
		ffs_MakeDir(filename1);

		preappend_nand_dev_name("/title/00010004", filename1);
		os_sync_after_write(filename1, MAX_FILENAME_SIZE);
		ffs_MakeDir(filename1);
		
		// create & truncate log file
		if(verbose_level)
			{
			int fd=os_open("sd:/ffs_log.txt" , O_CREAT | O_TRUNC | O_RDWR );
			if(fd>=0)
				{
				os_close(fd);
				}
			}
		}
}


/*

FFS I/O

NOTE: 

ffs_open, ffs_close, ffs_read, ffs_write, ffs_seek: if ret<0 call to the original

ffs_iocl and ioc_tlv uses the param 2 to determine if call to the original or not.

*/

int ffs_open(ipcmessage *msg)
{
	if(flag_emu==0) return -6; // call the original

	nand_emu_device();

	if(verbose_level>1) debug_printf("ffs open %s\n", msg->open.device);

return -6;

}

int ffs_close(ipcmessage *msg)
{

return -6;

}

int ffs_read(ipcmessage *msg)
{

return -6;

}

int ffs_write(ipcmessage *msg)
{	


return -6;

}

int ffs_seek(ipcmessage *msg)
{

return -6;

}

#define OFFSET_NEW_NAME 0x40



int ffs_ioctl(ipcmessage *msg, int *dont_use_ffs_call)
{
int ret=-6;
static struct stat filestat;

	*dont_use_ffs_call=0;

	if(flag_emu==0) return -6; // call the original

	nand_emu_device();

	if(fatHandle<0) return -6;

	if(verbose_level>1) debug_printf("msg ioclt 0x%x 0x%x\n", msg->ioctl.command, msg->fd);

	if(1) 
	{
	u32 cmd = msg->ioctl.command;
	u32 length_io = msg->ioctl.length_io;
	u32 *buffer_io = msg->ioctl.buffer_io;
	u32 *buffer_in = msg->ioctl.buffer_in;
	
	switch(cmd) {

	case FFS_IOCTL_CREATEDIR: {
		fsattr *attr = (fsattr *) buffer_in;
		if(test_string( (char *)  attr->filepath)) {*dont_use_ffs_call=0;return -6;}
		*dont_use_ffs_call=1;
		preappend_nand_dev_name((const char *)  attr->filepath, filename1);

		if(light_on) swi_mload_led_on();
		ret = ffs_MakeDir(filename1);
		if(light_on) swi_mload_led_off();

		if(verbose_level) debug_printf("Create dir %s ret %i\n", filename1, ret);
		
		break;
	}
	case FFS_IOCTL_CREATEFILE: {
		fsattr *attr = (fsattr *) buffer_in;

	
		if(verbose_level) debug_printf("Trying Create file %s \n",  attr->filepath);
		
		if(test_string( (char *)  attr->filepath)) {*dont_use_ffs_call=0;return -6;}
		*dont_use_ffs_call=1;
		preappend_nand_dev_name((const char *)  attr->filepath, filename1);

		if(light_on) swi_mload_led_on();
		ret = ffs_MakeFile(filename1);

		#ifdef READ_FROM_FILE_ATTR
		// CREATE file with ATTR
		if(ret>=0)
			{
			int n=0; int m=0; 
			while(filename1[n]!=0) {if(filename1[n]=='/') m=n+1; n++; }

			FFS_Memcpy(filename2, filename1, m);filename2[m]=0;
			FFS_Strcat(filename2, "#");
			FFS_Strcat(filename2, &filename1[m]);
	
			int fd=os_open(filename2, O_CREAT | O_TRUNC | O_RDWR );
			if(fd>=0)
				{
				attr->owner_id   = 0x1000+file_counter;
				file_counter++;
				attr->group_id   = get_group_id();
				os_sync_after_write(attr, sizeof(fsattr));
				os_write(fd, attr, sizeof(fsattr));
				os_close(fd);
				
				}
			}
		#endif
		if(light_on) swi_mload_led_off();

		if(verbose_level) debug_printf("Create file %s ret %i\n", filename1, ret);
		
		break;
	}
	case FFS_IOCTL_DELETE: {
		
		if(test_string( (char *) buffer_in)) {*dont_use_ffs_call=0;return -6;}
		*dont_use_ffs_call=1;
		preappend_nand_dev_name((const char *) buffer_in, filename1);

		if(light_on) swi_mload_led_on();

		ret = ffs_Delete(filename1);

		#ifdef READ_FROM_FILE_ATTR
		// delete file with ATTR
		if(ret>=0)
				{
				int n=0; int m=0;

			    while(filename1[n]!=0) {if(filename1[n]=='/') m=n+1; n++;}

				FFS_Memcpy(filename2, filename1, m);filename2[m]=0;
				FFS_Strcat(filename2, "#");
				FFS_Strcat(filename2, &filename1[m]);
				ffs_Delete(filename2);
				}
		#endif

		if(light_on) swi_mload_led_off();
        if(verbose_level) debug_printf("Deleting file %s ret %i\n", filename1, ret);
		
		break;
	}
	case FFS_IOCTL_RENAME: {
		u8 *names = (u8 *) buffer_in;

		if(!buffer_in || ((u8 *)buffer_in)[0]<32)  {*dont_use_ffs_call=1;return -101;}

        if(test_string( (char *) names)) {*dont_use_ffs_call=0;return -6;}
		*dont_use_ffs_call=1;
		
		preappend_nand_dev_name((const char *)names, filename1);
		preappend_nand_dev_name((const char *) names+OFFSET_NEW_NAME, filename2);
		
		if(light_on) swi_mload_led_on();

		if(!FFS_Strncmp(filename1, filename2, OFFSET_NEW_NAME))
			{
			
			ret = ffs_Stat(filename1, NULL);
			
			}
		else
			{
			
			// Check if newname exists
			if (ffs_Stat(filename2, &filestat) >=0) {
				if (S_ISDIR(filestat.st_mode))
					ffs_DeleteDir(filename2);
				else
					ffs_Delete(filename2);
			
			}
			

			ret = ffs_Rename(filename1, filename2);

			 if(verbose_level) debug_printf("Renaming file %s %s ret %i\n", filename1, filename2, ret);

			#ifdef READ_FROM_FILE_ATTR
			// rename file with ATTR
			if(ret>=0)
				{
				int n=0; int m=0; 
				char * filename3= (char *) fatDataPtr;

				if (ffs_Stat(filename2, &filestat) >=0) {
					if (S_ISDIR(filestat.st_mode)) n=1;
					} else n=1;
				
				if(n==0)
					{

					FFS_Strcpy(filename3, filename1);

					while(filename3[n]!=0) {if(filename3[n]=='/') m=n+1; n++;}

					FFS_Memcpy(filename1, filename3, m);filename1[m]=0;
					FFS_Strcat(filename1, "#");
					FFS_Strcat(filename1, &filename3[m]);

					n=m=0;
					FFS_Strcpy(filename3, filename2);
					while(filename3[n]!=0) {if(filename3[n]=='/') m=n+1; n++;}

					FFS_Memcpy(filename2, filename3, m);filename2[m]=0;
					FFS_Strcat(filename2, "#");
					FFS_Strcat(filename2, &filename3[m]);
					
					ffs_Delete(filename2);

					if(ffs_Rename(filename1, filename2)>=0)
					if(verbose_level) debug_printf("Renaming file attr %s %s\n", filename1, filename2);
					//swi_mload_led_on();while(1);
					}
				
				}
				#endif

			
			}

	   if(light_on) swi_mload_led_off();

		
		break;
	}
	case FFS_IOCTL_GETSTATS: {
		static struct statvfs vfsstat;
		if(test_string( (char *) buffer_in)) {*dont_use_ffs_call=0;return -6;}
		*dont_use_ffs_call=1;
		

		if( length_io < 0x1C )
				ret = -1017;
		else
			{
			preappend_nand_dev_name("/", filename1);

			if(light_on) swi_mload_led_on();

			ret = ffs_VFSStats(filename1, &vfsstat);

			if(light_on) swi_mload_led_off();

			if(verbose_level) debug_printf("VFSStats %s ret %i\n", filename1, ret);
			}

		
		if (ret >= 0) {
			fsstats *s = (fsstats *) buffer_io;
			FFS_Memset(buffer_io, 0, length_io);

			s->block_size  = vfsstat.f_bsize;
			s->free_blocks = vfsstat.f_bfree;
			s->free_inodes = vfsstat.f_ffree;
			s->used_blocks = vfsstat.f_blocks - vfsstat.f_bfree;

			os_sync_after_write(buffer_io, length_io);
		}

		break;
	}
	case FFS_IOCTL_GETFILESTATS: {

		
		//fstats *s = (fstats *) buffer_io;
		if(verbose_level) debug_printf("FFS GETFILESTATS \n");
		
		ret=-6;
		
		break;
	}
	case FFS_IOCTL_GETATTR: {
		int simulate=1;
		

		if(test_string( (char *) buffer_in)) {*dont_use_ffs_call=0;return -6;}

		*dont_use_ffs_call=1;

        if(verbose_level) debug_printf("GETATTR1 %s\n", buffer_in);

		preappend_nand_dev_name((const char *)buffer_in, filename1);

		if(light_on) swi_mload_led_on();

		ret = ffs_Stat(filename1, &filestat); //
		
		if(light_on) swi_mload_led_off();
		
		if(verbose_level) debug_printf("GETATTR2 %s ret %i\n", filename1, ret);
		
        
		if (ret >= 0) 
			{
			
			fsattr *attr = (fsattr *) buffer_io;
           
			#ifdef READ_FROM_FILE_ATTR
		

				if(!S_ISDIR(filestat.st_mode))
				{
				int n=0; int m=0; 
				while(filename1[n]!=0) {if(filename1[n]=='/') m=n+1; n++; }

				FFS_Memcpy(filename2, filename1, m);filename2[m]=0;
				FFS_Strcat(filename2, "#");
				FFS_Strcat(filename2, &filename1[m]);
		
				int fd=os_open(filename2, O_RDWR );
				if(fd>=0)
					{
					n=os_read(fd, attr, sizeof(fsattr));
					os_close(fd);

					if(n==sizeof(fsattr)) simulate=0;
					}
				}
		
			#endif

			if(simulate)
				{
				attr->owner_id   = 0x1001;
				attr->group_id   = get_group_id();
				FFS_Memcpy(attr->filepath, buffer_in, ISFS_MAXPATH);

				attr->attributes = 0;
				if(!cmp_string((void *) buffer_in, "/title/0001000#/########/data/nocopy"))
					{
					attr->groupperm  = 3;
					attr->otherperm  = 0;
					}
				else
					{
					attr->groupperm  = ISFS_OPEN_RW;
					attr->otherperm  = ISFS_OPEN_RW;
					}
				attr->ownerperm  = ISFS_OPEN_RW;

				os_sync_after_write(buffer_io, length_io);
				}
			
		
			ret=0;
		}
		break;
	}
	case FFS_IOCTL_SETATTR: {
		
		
		fsattr *attr = (fsattr *) buffer_in;
		if(test_string( (char *)  attr->filepath)) {*dont_use_ffs_call=0;return -6;}


		*dont_use_ffs_call=1;
		preappend_nand_dev_name((const char *)attr->filepath, filename1);

		if(light_on) swi_mload_led_on();

		ret = ffs_Stat(filename1, &filestat); // Ignore permission, success if the file exists
		
		#ifdef READ_FROM_FILE_ATTR
		if(ret>=0 && !S_ISDIR(filestat.st_mode))
			{
			int n=0; int m=0; 
			while(filename1[n]!=0) {if(filename1[n]=='/') m=n+1; n++;}

			FFS_Memcpy(filename2, filename1, m);filename2[m]=0;
			FFS_Strcat(filename2, "#");
			FFS_Strcat(filename2, &filename1[m]);
	
			int fd=os_open(filename2, O_CREAT | O_TRUNC | O_RDWR );
			if(fd>=0)
				{
				os_sync_after_write(attr, sizeof(fsattr));
				os_write(fd, attr, sizeof(fsattr));
				os_close(fd);
				}
			}
		#endif

		if(light_on) swi_mload_led_off();

		if(verbose_level) debug_printf("Set ATTR  %s ret %i\n", filename1, ret);
	
		break;
	}
	case FFS_IOCTL_FORMAT: {

		*dont_use_ffs_call=1;
		ret = 0;
		break;
	}

	default:

		if(verbose_level) debug_printf("Unsupported ioclt command 0x%x\n", msg->ioctl.command);
	  
		*dont_use_ffs_call=1;
		ret = -6;
		break;
	}

   
		return ret;
	}

	*dont_use_ffs_call=0;
	return -6;
}





int ffs_ioctlv(ipcmessage *msg, int *dont_use_ffs_call)
{
int ret=-6;


	*dont_use_ffs_call=0;

	if(flag_emu==0) return -6; // call the original
	
	nand_emu_device();

	if(fatHandle<0) return -6;

	
    if(verbose_level>1) debug_printf("msg iocltv 0x%x 0x%x\n", msg->ioctlv.command, msg->fd);

	if(1) 
	{
	ioctlv *vector = msg->ioctlv.vector;
	u32 num_io = msg->ioctlv.num_io;

	switch(msg->ioctlv.command) {

	case FFS_IOCTLV_READDIR: {

		u32 len=0;

		char *dirpath = (char *)vector[0].data;
		u32 *outlen =  (u32 *)vector[1].data;
		void *outbuf=NULL;
		
		if(test_string(dirpath)) {*dont_use_ffs_call=0;return -6;}


		*dont_use_ffs_call=1;

		if (num_io > 1) {
			len = *(u32 *)vector[1].data;
			outbuf = (char *)vector[2].data;
			outlen =  (u32 *)vector[3].data;

			if(verbose_level) debug_printf("Read dir 1 %i %i %s\n", len, *outlen/13, dirpath);
			//len/=13;

			

		} else
			outbuf = NULL;

		//if(test_string2(dirpath)) {*dont_use_ffs_call=0;return -6;}
      
		preappend_nand_dev_name(dirpath, filename1);

		if(light_on) swi_mload_led_on();

		ret = ffs_ReadDir(filename1, outbuf, &len);

		if(light_on) swi_mload_led_off();

		if(verbose_level) debug_printf("Read dir 2 %i %s ret: %i \n", len, filename1, ret);

		if (ret >= 0) 
		{*outlen = len;ret=0;}

		
		break;
	}
	
	case FFS_IOCTLV_GETUSAGE: {
		
		

		char *dirpath = (char *)vector[0].data;
		
		if(test_string(dirpath)) {*dont_use_ffs_call=0;return -6;}
		
		
		preappend_nand_dev_name(dirpath, filename1);

		if(light_on) swi_mload_led_on();

		ret= ffs_GetUsage(filename1, vector[1].data, vector[2].data);
		if(ret>=0)
			{
			swi_mload_led_off();
			os_sync_after_write(vector[1].data, 4);
		    os_sync_after_write(vector[2].data, 4);

			if(verbose_level) debug_printf("GetUsage %i %i %s\n", *((u32*) vector[1].data), *((u32*) vector[2].data), filename1);
			}

		if(light_on) swi_mload_led_off();

		*dont_use_ffs_call=1;
		break;

	}
	

	default:
		if(verbose_level) debug_printf("Unsupported iocltv command 0x%x\n",  msg->ioctlv.command);
		//disable_ffs_nand_flag=1;
		ret = -6;
	}
       
		return ret;
	}

 *dont_use_ffs_call=0;
	return -6;
}

int ffs_FileStats(int fd, void *filestat)
{
	int res;

	fatDataPtr->vector[0].data = fatDataPtr->filestats.data;
	fatDataPtr->vector[0].len = FILESTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_FILESTATS, 0, 1, fatDataPtr->vector);

	if (res >= 0){
		FFS_Memcpy(filestat, fatDataPtr->filestats.data, FILESTAT_DATA_SIZE);
	}
	return res;
}

int ffs_VFSStats(const char *path, struct statvfs *vfsstats)
{
	int res;

	FFS_Strcpy(fatDataPtr->statvfs.filename, path);

	fatDataPtr->vector[0].data = fatDataPtr->statvfs.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->statvfs.stat_vfs;
	fatDataPtr->vector[1].len = VFSSTAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_VFSSTATS, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) 
		FFS_Memcpy(vfsstats,(void *) &fatDataPtr->statvfs.stat_vfs, VFSSTAT_DATA_SIZE);

	return res;
}

int ffs_Stat(const char *filename, struct stat *statdata)
{
	int res;

	FFS_Strcpy(fatDataPtr->stat.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->stat.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->stat.data_stat;
	fatDataPtr->vector[1].len = STAT_DATA_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	res = os_ioctlv(fatHandle, IOCTL_FAT_STAT, 1, 1, fatDataPtr->vector);
	
	if (res >= 0) {
		if (statdata)
			FFS_Memcpy(statdata,(void *) &fatDataPtr->stat.data_stat, STAT_DATA_SIZE);
	}
	return res;
}

int ffs_Rename(const char *oldname, const char *newname)
{
	FFS_Strcpy(fatDataPtr->rename.oldfilename, oldname);
	FFS_Strcpy(fatDataPtr->rename.newfilename, newname);

	fatDataPtr->vector[0].data = fatDataPtr->rename.oldfilename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = fatDataPtr->rename.newfilename;
	fatDataPtr->vector[1].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_RENAME, 2, 0, fatDataPtr->vector);
}

int ffs_DeleteDir(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETEDIR, 1, 0, fatDataPtr->vector);
}

int ffs_Delete(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_DELETE, 1, 0, fatDataPtr->vector);
}

int ffs_ReadDir(const char *dirpath, u32 *outbuf, u32 *outlen)
{
	int ret;
	
	u32 io;

	FFS_Strcpy(fatDataPtr->basic.filename, dirpath);

	fatDataPtr->vector[0].data = &fatDataPtr->readdir.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	fatDataPtr->vector[1].data = (u32 *) &fatDataPtr->readdir.outlen;
	fatDataPtr->vector[1].len = sizeof(int);

	if (outbuf) {
		fatDataPtr->readdir.outlen = *outlen;

		fatDataPtr->vector[2].data = outbuf;
		fatDataPtr->vector[2].len =  13/*MAX_FILENAME_SIZE*/ * (*outlen);

		fatDataPtr->vector[3].data = &fatDataPtr->readdir.outlen;
		fatDataPtr->vector[3].len = sizeof(int);
		io = 2;
	} else
		io = 1;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	ret = os_ioctlv(fatHandle, IOCTL_FAT_READDIR_SHORT, io, io, fatDataPtr->vector);
	if (ret == 0) {
		*outlen = fatDataPtr->readdir.outlen;
	}
	return ret;
}

int ffs_MakeFile(const char *filename)
{
	FFS_Strcpy(fatDataPtr->basic.filename, filename);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKFILE, 1, 0, fatDataPtr->vector);
}


int ffs_MakeDir(const char *dirname)
{
	FFS_Strcpy(fatDataPtr->basic.filename, dirname);

	fatDataPtr->vector[0].data = fatDataPtr->basic.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;

	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	return os_ioctlv(fatHandle, IOCTL_FAT_MKDIR, 1, 0, fatDataPtr->vector);
}


s32 ffs_GetUsage(const char *dirpath, u32 *size, u32 *ionodes)
{
s32 ret;

	FFS_Strcpy(fatDataPtr->getusage.filename, dirpath);

	fatDataPtr->vector[0].data = fatDataPtr->getusage.filename;
	fatDataPtr->vector[0].len = MAX_FILENAME_SIZE;
    fatDataPtr->vector[1].data = &fatDataPtr->getusage.size;
	fatDataPtr->vector[1].len=4;
	fatDataPtr->vector[2].data = &fatDataPtr->getusage.ionodes;
	fatDataPtr->vector[2].len=4;
	os_sync_after_write(fatDataPtr, FAT_DATA_SIZE);

	ret=os_ioctlv(fatHandle, IOCTL_FAT_GETUSAGE, 1, 2, fatDataPtr->vector);
	if(ret==0)
		{
		*size= fatDataPtr->getusage.size;
		*ionodes= fatDataPtr->getusage.ionodes;
		}
	return ret;

}



