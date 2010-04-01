/*   
	Custom IOS Module (FAT & FFS)

	Copyright (C) 2008 neimod.
	Copyright (C) 2009 WiiGator.
	Copyright (C) 2009 Waninkoko.
	Copyright (C) 2010 Hermes.

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
#include "ffs_emu.h"
#include "swi_mload.h"

#ifdef TINY_FAT
#error "Cannot use TINY_FAT Here"
#endif

int flag_emu=0;
int flag_emu_mode=0; // 0-> default -> 1 DLC redirected to device:/nand -> 2-> Full emulation 128-> redirect diary to the trash
int light_on=1;
int verbose_level=0;
int diary_mode=0;

int usb_mounted=-1;
int sd_mounted=-1;

s32 index_dev=0;

const char dev_names[8][12]=
{
"sd:/nand",
"usb:/nand",

"sd:/nand2",
"usb:/nand2",

"sd:/nand3",
"usb:/nand3",

"sd:/nand4",
"usb:/nand4",
};


extern s32 hid;

extern int internal_debug_printf;
void debug_printf(char *format, ...);

extern s32 swi_ehcmodule(u32 cmd, u32 param1, u32 param2, u32 param3);

void (*release_wbfs_mem)(void)=NULL;

///////////////////////////

// to work in supervisor mode

u32 read_access_perm(void);
void write_access_perm(u32 flags);

void direct_os_sync_before_read(void* ptr, int size);
void direct_os_sync_after_write(void* ptr, int size);
void ic_invalidate(void);

u32 syscall_base;

// FFS variables and functions (jump table)

u32 old_ffs_tab_jump=0;
u32 old_ffs_reentry=0x2000219C;

extern int addr_ffs_unk(void *p);
extern int addr_ffs_open(void *p);
extern int addr_ffs_close(void *p);
extern int addr_ffs_read(void *p);
extern int addr_ffs_write(void *p);
extern int addr_ffs_seek(void *p);
extern int addr_ffs_ioctl(void *p);
extern int addr_ffs_ioctlv(void *p);

static u32 ffs_tab_jump[8]=
{
	(u32) addr_ffs_unk,
	(u32) addr_ffs_open,
	(u32) addr_ffs_close,
	(u32) addr_ffs_read,
	(u32) addr_ffs_write,
	(u32) addr_ffs_seek,
	(u32) addr_ffs_ioctl,
	(u32) addr_ffs_ioctlv,
};



extern int cmp_string( char *s,const char *c);
extern int test_string(char *s);

static char sys_name[256] ATTRIBUTE_ALIGN(32)="sd:/nand/" ;

int syscall_open_mode=0;

int force_to_nand=0;

// syscall open redirected here

char * syscall_open_func( char *name, int mode)
{
int n;

	syscall_open_mode=mode;

	if(!flag_emu) return name;


	if(name[0]=='#') // for example #/title/xxxx : can be used to skip this function to open files
		{
		name++;
		for(n=0;name[n];n++) sys_name[n+32]=name[n];
		sys_name[n+32]=0;
		name=(char *) sys_name+32;
		
		}
	else
	if(!cmp_string(name,"/dev/stm/immediate"))
		{
		static char stm[] ATTRIBUTE_ALIGN(32)=DEVICE_FAT"/dev/immediate";
		name++;
		
		for(n=0;stm[n];n++) sys_name[n+32]=stm[n];
		sys_name[n+32]=0;
		
		name=(char *) sys_name+32;
		}
	else
	if(sd_mounted>0 && !cmp_string(name,"/dev/sdio"))
		{
		static char sdio[] ATTRIBUTE_ALIGN(32)=DEVICE_FAT"/dev/sdio";
		name++;
		
		for(n=0;sdio[n];n++) sys_name[n+32]=sdio[n];
		sys_name[n+32]=0;
		
		name=(char *) sys_name+32;
		}
	else

	if(!test_string(name)) // redirect some routes
		{
		int m;
		int ind=index_dev;
		
		if(flag_emu_mode & 1)
			if(!cmp_string(name, "/title/00010005") || !cmp_string(name, "/ticket/00010005")) ind&=1; // use sd:/nand or usb:/nand for DLC

		for(m=0;dev_names[ind][m];m++) sys_name[m]=dev_names[ind][m];
		sys_name[m]='/';m++;

		if(name[0]=='/') name++;
		for(n=0;name[n];n++) sys_name[n+m]=name[n];
		sys_name[n+m]=0;

		name=(char *) sys_name;
		syscall_open_mode=2;
		}


return name;
}


// syscall open
extern int in_syscall_open(char *, int);
u32 out_syscall_open_thumb=0;
// syscall open patch
static u32 vector[3]={0x477846C0, 0xE51FF004, 0}; //bx pc, ldr pc,=addr
static u32 ffs_data_restore_sysopen[4];

// old dev/es jump
static u32 old_es_jump;
static u32 data_restore_es[2]= {0xB570B088, 0x68851C01};
// dev/es in routine
extern int in_ES_ioctlv(void *);

int patch_ffs(u32 ios, u32 none)
{
u32 perm;

	perm=read_access_perm();
	write_access_perm(0xffffffff);

	ic_invalidate();

	vector[2]=((u32) in_syscall_open);

	direct_os_sync_after_write((void *) vector, 12);
	
	
	if(ios==38)
		{
   
		// sys_open patch
		
		ffs_data_restore_sysopen[0]=*((u32 *) 0xFFFF2D40);
		ffs_data_restore_sysopen[1]=*((u32 *) 0xFFFF2D44);
		ffs_data_restore_sysopen[2]=*((u32 *) 0xFFFF2D48);

	    out_syscall_open_thumb=0xFFFF2D4C; // it uses mov pc

		
        *((u32 *) 0xFFFF2D40)=vector[0];
		*((u32 *) 0xFFFF2D44)=vector[1];
		*((u32 *) 0xFFFF2D48)=vector[2];
		direct_os_sync_after_write((void *) 0xFFFF2D40, 12);
	
        // FFS patch
		
		old_ffs_reentry=0x2000219C;
		old_ffs_tab_jump= * ((u32 *) 0x200021D0);

		//enable emulation vector
		* ((u32 *) 0x200021D0)= (u32) ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x200021D0, 4);

		}
    else
    if(ios==37)
		{

		// sys_open patch
		
		ffs_data_restore_sysopen[0]=*((u32 *) 0xFFFF2E50);
		ffs_data_restore_sysopen[1]=*((u32 *) 0xFFFF2E54);
		ffs_data_restore_sysopen[2]=*((u32 *) 0xFFFF2E58);

	    out_syscall_open_thumb=0xFFFF2E5C; // it uses mov pc

        *((u32 *) 0xFFFF2E50)=vector[0];
		*((u32 *) 0xFFFF2E54)=vector[1];
		*((u32 *) 0xFFFF2E58)=vector[2];
		direct_os_sync_after_write((void *) 0xFFFF2E50, 12);
	
        // FFS patch
		//direct_os_sync_before_read((void *) 0x20005F38, 4);
		old_ffs_reentry=0x20005F0A;
		old_ffs_tab_jump= * ((u32 *) 0x20005F38);

		* ((u32 *) 0x20005F38)= (u32) ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x20005F38, 4);
		}
    else
	if(ios==57 || ios==60)
		{

		// sys_open patch
		ffs_data_restore_sysopen[0]=*((u32 *) 0xFFFF3020);
		ffs_data_restore_sysopen[1]=*((u32 *) 0xFFFF3024);
		ffs_data_restore_sysopen[2]=*((u32 *) 0xFFFF3028);

	    out_syscall_open_thumb=0xFFFF302C; // it uses mov pc

        *((u32 *) 0xFFFF3020)=vector[0];
		*((u32 *) 0xFFFF3024)=vector[1];
		*((u32 *) 0xFFFF3028)=vector[2];
		direct_os_sync_after_write((void *) 0xFFFF3020, 12);
	
        // FFS patch
		old_ffs_reentry=0x2000618A;
		old_ffs_tab_jump= * ((u32 *) 0x200061B8);

		* ((u32 *) 0x200061B8)= (u32) ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x200061B8, 4);
		}

	// dev/es ioctlv
    
    
	old_es_jump= *((u32 *) 0x201000D0);
	*((u32 *) 0x201000D0)= ((u32) in_ES_ioctlv) | 1;

	
	direct_os_sync_after_write((void *) 0x201000D0, 4);

	direct_os_sync_after_write((void *) ffs_data_restore_sysopen, 12);
	
	write_access_perm(perm);

return 0;
}

int restore_ffs(u32 ios, u32 none)
{
u32 perm;

flag_emu=0;

	perm=read_access_perm();
	write_access_perm(0xffffffff);
	ic_invalidate();

	if(ios==38)
		{
	     	    
		// sys_open patch
		
        *((u32 *) 0xFFFF2D40)=ffs_data_restore_sysopen[0];
		*((u32 *) 0xFFFF2D44)=ffs_data_restore_sysopen[1];
		*((u32 *) 0xFFFF2D48)=ffs_data_restore_sysopen[2];
		
		direct_os_sync_after_write((void *) 0xFFFF2D40, 12);
	
	
        // FFS patch
		
		* ((u32 *) 0x200021D0)= (u32) old_ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x200021D0, 4);
		
		}
	else
	if(ios==37)
		{

		// sys_open patch
		
		*((u32 *) 0xFFFF2E50)=ffs_data_restore_sysopen[0];
		*((u32 *) 0xFFFF2E54)=ffs_data_restore_sysopen[1];
		*((u32 *) 0xFFFF2E58)=ffs_data_restore_sysopen[2];
		
		direct_os_sync_after_write((void *) 0xFFFF2E50, 12);
	
        // FFS patch
		
		* ((u32 *) 0x20005F38)= (u32) old_ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x20005F38, 4);
		}
	else
	if(ios==57 || ios==60)
		{

		// sys_open patch
	
        *((u32 *) 0xFFFF3020)=ffs_data_restore_sysopen[0];
		*((u32 *) 0xFFFF3024)=ffs_data_restore_sysopen[1];
		*((u32 *) 0xFFFF3028)=ffs_data_restore_sysopen[2];
        
		direct_os_sync_after_write((void *) 0xFFFF3020, 12);
	
        // FFS patch
		
		* ((u32 *) 0x200061B8)= (u32) old_ffs_tab_jump;
		direct_os_sync_after_write((void *) 0x200061B8, 4);

		}

	// dev/es ioctlv

	 *((u32 *) 0x201000CC)= data_restore_es[0];
	 *((u32 *) 0x201000D0)= data_restore_es[1];

	  direct_os_sync_after_write((void *) 0x201000cc, 8);

	  *((volatile u32 *)0x0d8000c0) |=0x20; // LED ON
	  
	
	write_access_perm(perm);

return 0;
}

////////////////////////////

s32 __FAT_Ioctlv(ipcmessage *message)
{
	ioctlv *vector = message->ioctlv.vector;
	u32     len_in = message->ioctlv.num_in;
	u32     len_io = message->ioctlv.num_io;

	u32 cnt;
	s32 ret = IPC_EINVAL;

	/* Invalidate cache */
	os_sync_before_read(vector, sizeof(ioctlv) * (len_in + len_io));

	for (cnt = 0; cnt < (len_in + len_io); cnt++)
		os_sync_before_read(vector[cnt].data, vector[cnt].len);

	/* Parse IOCTLV command */
	switch (message->ioctlv.command) {
	/** Open file **/
	case IOCTL_FAT_OPEN: {
		char *filepath = (char *)vector[0].data;
		u32   mode     = *(u32 *)vector[1].data;

		/* Open file */
		ret = FAT_Open(filepath, mode);

		break;
	}

	/** Close file **/
	case IOCTL_FAT_CLOSE: {
		u32 fd = *(u32 *)vector[0].data;

		/* Close file */
		ret = FAT_Close(fd);

		break;
	}

	/** Read file **/
	case IOCTL_FAT_READ: {
		u32   fd = *(u32 *)vector[0].data;

		void *buffer = vector[1].data;
		u32   len    = vector[1].len;

		/* Read file */
		ret = FAT_Read(fd, buffer, len);

		break;
	}

	/** Write file **/
	case IOCTL_FAT_WRITE: {
		u32   fd = *(u32 *)vector[0].data;

		void *buffer = vector[1].data;
		u32   len    = vector[1].len;

		/* Write file */
		ret = FAT_Write(fd, buffer, len);

		break;
	}

	/** Seek file **/
	case IOCTL_FAT_SEEK: {
		u32 fd = *(u32 *)vector[0].data;

		u32 where  = *(u32 *)vector[1].data;
		u32 whence = *(u32 *)vector[2].data;

		/* Seek file */
		ret = FAT_Seek(fd, where, whence);

		break;
	}
    #ifndef TINY_FAT
	/** Create directory **/
	case IOCTL_FAT_MKDIR: {
		char *dirpath = (char *)vector[0].data;

		/* Create directory */
		ret = FAT_CreateDir(dirpath);

		break;
	}
	#endif

	/** Create file **/
	case IOCTL_FAT_MKFILE: {
		char *filepath = (char *)vector[0].data;

		/* Create file */
		ret = FAT_CreateFile(filepath);

		break;
	}

	/** Read directory **/
	case IOCTL_FAT_READDIR: {
		char *dirpath = (char *)vector[0].data;
		char *outbuf  = NULL;
		u32  *outlen  = NULL;

		u32 maxlen = 0;

		/* Input values */
		if (len_io > 1) {
			maxlen = *(u32 *)vector[1].data;
			outbuf = (char *)vector[2].data;
			outlen =  (u32 *)vector[3].data;
		} else
			outlen =  (u32 *)vector[1].data;
		
		
		/* Read directory */
		ret = FAT_ReadDir(dirpath, outbuf, outlen, maxlen);
		
		break;
	}

   /** Read directory short (for FFS) **/
	case IOCTL_FAT_READDIR_SHORT: {
		char *dirpath = (char *)vector[0].data;
		char *outbuf  = NULL;
		u32  *outlen  = NULL;

		u32 maxlen = 0;

		/* Input values */
		if (len_io > 1) {
			maxlen = *(u32 *)vector[1].data;
			outbuf = (char *)vector[2].data;
			outlen =  (u32 *)vector[3].data;
		} else
			outlen =  (u32 *)vector[1].data;
		
		
		/* Read directory */
		ret = FAT_ReadDir_short(dirpath, outbuf, outlen, maxlen);
		
		break;
	}

   #ifndef TINY_FAT
	/** Delete object **/
	case IOCTL_FAT_DELETE: {
		char *path = (char *)vector[0].data;

		/* Delete file/directory */
		ret = FAT_Delete(path);

		break;
	}

	/** Delete directory **/
	case IOCTL_FAT_DELETEDIR: {
		char *dirpath = (char *)vector[0].data;

		/* Delete directory */
		ret = FAT_DeleteDir(dirpath);

		break;
	}

	/** Rename object **/
	case IOCTL_FAT_RENAME: {
		char *oldname = (char *)vector[0].data;
		char *newname = (char *)vector[1].data;

		/* Rename file/directory */
		ret = FAT_Rename(oldname, newname);

		break;
	}

	/** Get stats **/
	case IOCTL_FAT_STAT: {
		char *path  = (char *)vector[0].data;
		void *stats = vector[1].data;

		/* Get stats */
		ret = FAT_Stat(path, stats);

		break;
	}

	/** Get filesystem stats */
	case IOCTL_FAT_VFSSTATS: {
		char *path  = (char *)vector[0].data;
		void *stats = vector[1].data;

		/* Get filesystem stats */
		ret = FAT_GetVfsStats(path, stats);

		break;
	}
   #endif

	/** Get file stats **/
	case IOCTL_FAT_FILESTATS: {
		fstats *stats = (fstats *)vector[0].data;
		u32     fd    = message->fd;

		/* FD specified */
		if (len_in) {
			fd    = *(u32 *)vector[0].data;
			stats = (fstats *)vector[1].data;
		}

		/* Get file stats */
		ret = FAT_GetFileStats(fd, stats);

		break;
	}

	/** Read directory **/
	case IOCTL_FAT_GETUSAGE: {
		char *dirpath = (char *)vector[0].data;
		u32  *blocks  = (u32 *)vector[1].data;
		u32  *ionodes  = (u32 *)vector[2].data;

		u64 size=0;
		*ionodes=0;

		/* Read directory */
		ret = FAT_GetUsage(dirpath, &size, ionodes);
		if(ret==-ENOTDIR) ret=0;
		
		*blocks=(u32) (size/0x4000);

		 // limit number of blocks used
		if(*blocks>0x2000) *blocks=0x1000+(*blocks & 0xfff);
		

		break;
	}

	/** NAND EMU **/
	case IOCTL_FFS_MODE: {
		
		u32 * mode= vector[0].data;
		
		index_dev=(*mode) & 7;
		flag_emu=(*mode & 128)!=0; // on/off
		flag_emu_mode=(*mode>>8) & 3;

		light_on=(*mode & 8)!=0;
		
        verbose_level=(*mode & 48)>>4; //1-> for log 3-> All logs

		diary_mode=(*mode & 64)!=0; // 0-> diary from the NAND 1-> diary from device (used to block the diary for now)

		if(diary_mode && usb_mounted<0 && sd_mounted<0) flag_emu_mode|=128; // only capture the diary
		
        ret=0;
		break;
	}

	/** Mount SD card **/
	case IOCTL_FAT_MOUNTSD: {
		ret=1;
		/* Initialize SDIO */
		if(sd_mounted<0)
			{
			ret = !__io_wiisd.startup();
			if (ret) 
					break;
			else sd_mounted=0;
			}
	
		if(sd_mounted==0)
			{
			/* Mount SD card */
			ret = !fatMountSimple("sd", &__io_wiisd);
		
			if(ret==0) 
				{
				sd_mounted=1;
				ret = FAT_Open("sd:", 0);  // don't ask: strange problem with my SDHC...
				if(ret>=0) FAT_Close(ret);
				ret=0;
				}
			else {__io_wiisd.shutdown();sd_mounted=-1;}
			}

		if(sd_mounted>0) ret=0;

		break;
	}

	/** Unmount SD card **/
	case IOCTL_FAT_UMOUNTSD: {
		ret=1;
		if(sd_mounted>0)
			{
			/* Unmount SD card */
			fatUnmount("sd");
			sd_mounted=0;
			}
		if(sd_mounted==0)
			{
			/* Deinitialize SDIO */
			ret = !__io_wiisd.shutdown();
			sd_mounted=-1;
			}
        if(sd_mounted<0) ret=0;
		break;
	}

	/** Mount USB card **/
	case IOCTL_FAT_MOUNTUSB: {
		ret=1;
		/* Initialize USB */
		if(usb_mounted<0)
			{
			ret = !__io_usbstorage.startup();
			if (ret)
				break;
			else usb_mounted=0;
			}
		if(usb_mounted==0)
			{
			/* Mount USB */
			ret = !fatMountSimple("usb", &__io_usbstorage);
			if(ret==0) usb_mounted=1;
			}
		if(usb_mounted>0) ret=0;

		break;
	}

	/** Unmount USB card **/
	case IOCTL_FAT_UMOUNTUSB: {
		ret=1;
		if(usb_mounted>0)
			{
			/* Unmount USB */
			fatUnmount("usb");
			usb_mounted=0;
			}
		if(usb_mounted==0)
			{
			/* Deinitialize USB */
			ret = !__io_usbstorage.shutdown();
			usb_mounted=-1;
			}
		if(usb_mounted<0) ret=0;
		break;
	}

	default:
		while(1) {swi_mload_led_on();Timer_Sleep(5000*1000);swi_mload_led_off();Timer_Sleep(1000*1000);}
		break;
	}

	/* Flush cache */
	for (cnt = 0; cnt < (len_in + len_io); cnt++)
		os_sync_after_write(vector[cnt].data, vector[cnt].len);

	return ret;
}

s32 __FAT_Initialize(u32 *queuehandle)
{
	void *buffer = NULL;
	s32   ret;

	/* Initialize memory heap */
	Mem_Init();

	/* Initialize timer subsystem */
	Timer_Init();

	/* Allocate queue buffer */
	buffer = Mem_Alloc(0x80);
	if (!buffer)
		return IPC_ENOMEM;

	/* Create message queue */
	ret = os_message_queue_create(buffer, 32);
	if (ret < 0)
		return ret;

	/* Register devices */
	os_device_register(DEVICE_FAT,  ret);
	os_device_register(DEVICE_SDIO, ret);
	os_device_register(DEVICE_USB,  ret);
	



	/* Copy queue handler */
	*queuehandle = ret;

	return 0;
}


// fd used to emulate NAND files 
static int ffs_fat_fd[32]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


static int ffs_get_first_fd(void)
{
int n;

	for(n=0;n<32;n++) if(ffs_fat_fd[n]==-1) return n;

return -6;
}

static inline int get_fat_fd(int fd)
{
int n;

	for(n=0;n<32;n++) if(ffs_fat_fd[n]==fd) return n;

return -6;
}

int stm_fd=-1;
int stm_event=-1;

static u32 __stm_out[0x08] ATTRIBUTE_ALIGN(32) = {0,0,0,0,0,0,0,0};


void release_event_handler(void)
{
	
	if(stm_fd<0) stm_fd=os_open("/dev/stm/immediate", 0);
	if(stm_event<0 && stm_fd>=0) stm_event=os_open("/dev/stm/eventhook", 0);
   
	if(stm_fd>=0 && stm_event>=0)		
	{
	os_ioctl(stm_fd, 0x3002, NULL , 0, __stm_out,0x20);
	memset(__stm_out, 0, 32);
	os_close(stm_event);stm_event=-1;
	}
	
}


void ffs_shutdown(void)
{
int n;

	flag_emu=0;

	for(n=0;n<32;n++) {if(ffs_fat_fd[n]>=0) FAT_Close(ffs_fat_fd[n]); ffs_fat_fd[n]=-1;}

	if(sd_mounted>0)
		{
		/* Unmount SD card */
		fatUnmount("sd");
		sd_mounted=0;
		}
	if(sd_mounted==0)
		{
		/* Deinitialize SDIO */
		__io_wiisd.shutdown();
		sd_mounted=-1;
		}

	if(usb_mounted>0)
		{
		/* Unmount USB */
		fatUnmount("usb");
		usb_mounted=0;
		}
	if(usb_mounted==0)
		{
		/* Deinitialize USB */
		__io_usbstorage.shutdown();
		usb_mounted=-1;
		}

	for(n=0;n<2;n++) {swi_mload_led_on();Timer_Sleep(300*1000);swi_mload_led_off();Timer_Sleep(300*1000);}

	swi_mload_call_func((void *) restore_ffs, NULL, NULL);
	Timer_Sleep(300*1000);
	swi_mload_led_off();
}



#define THREAD_STACK 1024

static u8 thread_stack[THREAD_STACK] __attribute__ ((aligned (32)));



static int blink_handle=-1;
static int timer_id;
static volatile int blinker_off=1;

void led_on(void)
{
	blinker_off&=~7;

}

void led_off(void)
{
	blinker_off|=2;

}

int thread_blink(void)
{

u32 message;


u32 queue_mem[8];
blink_handle = os_message_queue_create( queue_mem, 8);

timer_id=os_create_timer(1000*5, 1000*1000*10, blink_handle, 0);


os_thread_set_priority(os_get_thread_id(), 0x79);

blinker_off=4;

while(1)
	{
   	
 
	int ret=os_message_queue_receive(blink_handle, (void*)&message, 0);
	if(ret) continue;
	os_stop_timer(timer_id);

    if((blinker_off & 7)==3) {blinker_off|=4;swi_mload_led_off();}
    if((blinker_off & 4)==4) {os_restart_timer(timer_id, 1000*1000);continue;}

	blinker_off^=128;
   
	if(blinker_off & 128) {swi_mload_led_on(); os_restart_timer(timer_id, 1000*5);blinker_off|=1;}
	else  {swi_mload_led_off();os_restart_timer(timer_id, 1000*1000);}
	
	}


return 0;
}

static char path_name[256];

void my_memcpy(char *d, const char *s, int size)
{
int n;
for(n=0;n<size;n++)
	  {
	  *d++=*s++;
	  }
}

int main(void)
{
	u32 queuehandle;
	s32 ret;
	

	verbose_level=0;

/********************************************************************************************************************************************************/

// led blink thread

	int blink_thread_id=os_thread_create( (void *) thread_blink, NULL, &thread_stack[1024], 1024, 0x79, 0);

	if(blink_thread_id>=0) os_thread_continue(blink_thread_id);

	os_thread_set_priority(os_get_thread_id(), 0x62);
	


/********************************************************************************************************************************************************/
    
// connect supervisor service

	syscall_base=swi_mload_get_syscall_base();
	os_sync_after_write((void *) &syscall_base, 4);
	
	flag_emu=0;
	swi_mload_call_func((void *) patch_ffs, (void *) swi_mload_get_ios_base(), NULL);


/********************************************************************************************************************************************************/
	
	release_event_handler();

	/* Initialize module */
	ret = __FAT_Initialize(&queuehandle);
	if (ret < 0)
		return ret;

	
 // NOTE: some games don´t use dev/stm/eventhook and i use interrupt event to umount devices and flush datas

	os_software_IRQ(0xb);
	os_software_IRQ(0x11);
    
	//os_unregister_event_handler(0xb);
	os_unregister_event_handler(0x11);

   // os_register_event_handler(0xb, queuehandle, 0x6661234); // register power off
    os_register_event_handler(0x11, queuehandle, 0x6661235);   // register reset

	

	/* Main loop */
	while (1) {
		char *p, *p2, use_light=0;
		ipcmessage *message = NULL;
		volatile int res;

		/* Wait for message */
		res=os_message_queue_receive(queuehandle, (void *)&message, 0);
		if(res) continue;
		

		ret=-101;


		if(((u32) message)==0x6661234) // power off
			{
		
			ffs_shutdown();
		
			os_ioctl(stm_fd, 0x2003,  __stm_out,0x20, __stm_out,0x20);
			Timer_Sleep(10000*1000);
			continue;
			}

		if(((u32) message)==0x6661235) // reset
			{
			ffs_shutdown();
			os_ioctl(stm_fd, 0x2001, NULL , 0, __stm_out,0x20);
			Timer_Sleep(10000*1000);
			continue;
			}

		if(((u32) message)==0) continue;

		switch (message->command) {
		case IOS_OPEN: {
			char *devpath = message->open.device;
			u32   mode    = message->open.mode;

			
			memcpy(path_name, (char *) message->open.device, strlen((char *) message->open.device)+1);
			devpath=path_name;
		
			/* FAT device */
			if (!strcmp(devpath, DEVICE_FAT)) {
				ret = 0;
				break;
			}

			
			
            // redirected SD
			if(!strcmp(devpath, DEVICE_FAT"/dev/sdio"))  {ret= IPC_EACCESS	;break;} // SD in game without interferences
			
			// re directed /dev/stm/immediate
			if(!strcmp(devpath, DEVICE_FAT"/dev/immediate"))
			{
				
				if(stm_fd<0) stm_fd=os_open("#/dev/stm/immediate", mode);
				
				if(stm_fd<0) ret=stm_fd; else ret= 0x888; // dev id for immediate
				
				break;
			}

			// fast command
			if(!strcmp(devpath, DEVICE_FAT":off")) {flag_emu=0;ret= -1234;break;}
			if(!strcmp(devpath, DEVICE_FAT":on"))  {flag_emu=1;ret= -1234;break;}
			if(!strcmp(devpath, DEVICE_FAT":shutdown"))  
				{
				ffs_shutdown();ret= -1234;
				break;
				}
			
			use_light=0;
            p=p2=NULL;

			/* Open file */
			
			if(!strncmp(devpath,"sd:/nand",8)) 
			{

				if(sd_mounted<0) {ret=-102;break;}
				use_light=1; // for lighting when it read or write
				
				//mode=2; // force flag for read/write only 

			}

			if(!strncmp(devpath, "usb:/nand", 9))
			{

				if(usb_mounted<0) {ret=-102;break;}
				use_light=1; // for lighting when it read or write
				
				//mode=2; // force flag for read/write only 

			}

			ret = FAT_Open(devpath, mode);
			
			// for lighting when it read or write
			if(ret>=0 && use_light)
				{
				int n;
					
					n=ffs_get_first_fd();
					if(n>=0) ffs_fat_fd[n]=ret;
				}
			if(use_light)
				{
				internal_debug_printf=1;
				if(verbose_level) debug_printf("FAT open ret %i %s\n",ret, devpath);
				internal_debug_printf=0;
				}
			break;
		}

		case IOS_CLOSE: {
			int n;

			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{
				ret=0; //os_close(stm_fd);stm_fd=-1;
				break;
			}

			n=get_fat_fd(message->fd);
            if(n>=0) ffs_fat_fd[n]=-1;

			/* Close file */
			ret = FAT_Close(message->fd);
			

			break;
		}

		case IOS_READ: {
			void *buffer = message->read.data;
			u32   len    = message->read.length;
            int n;
			
			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{
				ret=-101;
				break;
			}

			/* Read file */
			os_sync_before_read(buffer, len);

			n=get_fat_fd(message->fd);
			
			//if(n>=0 && light_on ) swi_mload_led_on();

			if(n>=0 && (light_on & 2)) led_on();

			ret = FAT_Read(message->fd, buffer, len);

			if(n>=0 && (light_on & 2)) led_off();


			if(ret<0)
			{
				internal_debug_printf=1;
				if(verbose_level) debug_printf("Err FAT Read ret %i len i \n",ret, len );
				internal_debug_printf=0;
			}
			memcpy(buffer, buffer, len);
			/* Flush cache */
			os_sync_after_write(buffer, len);


			break;
		}

		case IOS_WRITE: {
			void *buffer = message->write.data;
			u32   len    = message->write.length;
			int n;

			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{
				ret=-101;
				break;
			}

			/* Invalidate cache */
			os_sync_before_read(buffer, len);
			
			n=get_fat_fd(message->fd);
			
			if(n>=0 && light_on  ) led_on(); //swi_mload_led_on();

			/* Write file */
			ret = FAT_Write(message->fd, buffer, len);

			if(n>=0 && light_on) led_off();//swi_mload_led_off();


			break;
		}

		case IOS_SEEK: {
			u32 where  = message->seek.offset;
			u32 whence = message->seek.origin;

			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{
				ret=-101;
				break;
			}

			/* Seek file */
			ret = FAT_Seek(message->fd, where, whence);
			if(ret<0)
			{
				internal_debug_printf=1;
				if(verbose_level) debug_printf("Err FAT Seek ret %i %u %u \n",ret, where, whence );
				internal_debug_printf=0;
			}


			break;
		}

		case IOS_IOCTLV: {

			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{
				ret=-101;
				break;
			}

            if(get_fat_fd(message->fd)>=0) 
				{
				ret=-101;
				internal_debug_printf=1;
				if(verbose_level) debug_printf("eh! you cannot call ioctlv %x from here!\n", message->ioctlv.command);
				internal_debug_printf=0;
			    while(1) {swi_mload_led_on();Timer_Sleep(5000*1000);swi_mload_led_off();Timer_Sleep(1000*1000);}
				}
			else
				/* Parse IOCTLV message */
				ret = __FAT_Ioctlv(message);

			break;
		}

       case IOS_IOCTL:
			{
	      
			u32 cmd = message->ioctl.command;

			if(message->fd==0x888) // /dev/stm/immediate descriptor
			{

				if(cmd==0x2001 || cmd==0x2002 || cmd==0x2003 || cmd==0x2004) // reset
					{
					ffs_shutdown();
					//os_ioctl(stm_fd, 0x2001+2*(cmd==2003 ), NULL , 0, __stm_out,0x20);
					//Timer_Sleep(10000*1000);
					}
			   
				
				ret=os_ioctl(stm_fd, cmd, message->ioctl.buffer_in,  message->ioctl.length_in,  message->ioctl.buffer_io,  message->ioctl.length_io);
				
				break;
			}
			
			if(cmd==FFS_IOCTL_GETFILESTATS)
				{
				static fstats my_stats ATTRIBUTE_ALIGN(32);
				fstats *stats = (fstats *) message->ioctl.buffer_io;
				u32     fd    = message->fd;
				
				if(get_fat_fd(message->fd)>=0) 
					{
					/* Get file stats */
					ret = FAT_GetFileStats(fd, &my_stats);
					if(stats && ret==0)
						{
						memcpy((void *) stats, (void *) &my_stats, message->ioctl.length_io/*sizeof(fstats)*/);
						os_sync_after_write(stats, message->ioctl.length_io);
						}

					internal_debug_printf=1;
					if(verbose_level) debug_printf("FAT GetFileStats ret %i %u %u \n",ret, my_stats.file_length, my_stats.file_pos );
					internal_debug_printf=0;
					}
				else ret=-106;
			
			    }
			 else
				{
				ret=-101;
				internal_debug_printf=1;
				if(verbose_level) debug_printf("eh! you cannot call ioctl 0x from here!\n", message->ioctl.command);
				internal_debug_printf=0;
			    while(1) {swi_mload_led_on();Timer_Sleep(5000*1000);swi_mload_led_off();Timer_Sleep(1000*1000);}
				}
			break;
			}
		default:
			
			
			/* Unknown command */
			ret = IPC_EINVAL;
		}

		/* Acknowledge message */
		os_message_queue_ack((void *)message, ret);

	
	}
   
	return 0;
}
