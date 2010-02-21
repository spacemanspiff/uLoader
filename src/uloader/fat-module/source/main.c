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

int usb_mounted=-1;
int sd_mounted=-1;

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

	/** Mount SD card **/
	case IOCTL_FAT_MOUNTSD: {
		/* Initialize SDIO */
		ret = !__io_wiisd.startup();
		if (ret)
			break;

		/* Mount SD card */
		ret = !fatMountSimple("sd", &__io_wiisd);
		
		if(ret==0) sd_mounted=1;

		break;
	}

	/** Unmount SD card **/
	case IOCTL_FAT_UMOUNTSD: {
		/* Unmount SD card */
		fatUnmount("sd");

		/* Deinitialize SDIO */
		ret = !__io_wiisd.shutdown();
		sd_mounted=-1;

		break;
	}

	/** Mount USB card **/
	case IOCTL_FAT_MOUNTUSB: {
		/* Initialize USB */
		ret = !__io_usbstorage.startup();
		if (ret)
			break;

		/* Mount USB */
		ret = !fatMountSimple("usb", &__io_usbstorage);
		if(ret==0) usb_mounted=1;

		break;
	}

	/** Unmount USB card **/
	case IOCTL_FAT_UMOUNTUSB: {
		/* Unmount USB */
		fatUnmount("usb");

		/* Deinitialize USB */
		ret = !__io_usbstorage.shutdown();
		sd_mounted=-1;

		break;
	}

	default:
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
	buffer = Mem_Alloc(0x20);
	if (!buffer)
		return IPC_ENOMEM;

	/* Create message queue */
	ret = os_message_queue_create(buffer, 8);
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

extern s32 swi_ehcmodule(u32 cmd, u32 param1, u32 param2, u32 param3);

void (*release_wbfs_mem)(void)=NULL;

extern s32 hid;

int main(void)
{
	u32 queuehandle;
	s32 ret;

/********************************************************************************************************************************************************/

	// WARNING!!!: this function force to release WBFS memory used in ehcmodule.elf when you load fat-module and use this memory for
	// fat-module operation (actually 139264 bytes) in exclusive. See swi_ehcmodule() function in ehc_loop.c
 
    release_wbfs_mem= (void *) swi_ehcmodule(1 , 0, 0, 0); // get the function to release memory
    release_wbfs_mem(); // use it in user mode
    
	hid= swi_ehcmodule(0, 0, 0, 0); // get the heaphandle from ehcmodule

/********************************************************************************************************************************************************/

	/* Initialize module */
	ret = __FAT_Initialize(&queuehandle);
	if (ret < 0)
		return ret;

	
	/* Main loop */
	while (1) {
		ipcmessage *message = NULL;

		/* Wait for message */
		os_message_queue_receive(queuehandle, (void *)&message, 0);

		switch (message->command) {
		case IOS_OPEN: {
			char *devpath = message->open.device;
			u32   mode    = message->open.mode;
	
			/* FAT device */
			if (!strcmp(devpath, DEVICE_FAT)) {
				ret = 0;
				break;
			}

			/* Open file */
			ret = FAT_Open(devpath, mode);

			break;
		}

		case IOS_CLOSE: {
			/* Close file */
			ret = FAT_Close(message->fd);

			break;
		}

		case IOS_READ: {
			void *buffer = message->read.data;
			u32   len    = message->read.length;

			/* Read file */
			ret = FAT_Read(message->fd, buffer, len);

			/* Flush cache */
			os_sync_after_write(buffer, len);

			break;
		}

		case IOS_WRITE: {
			void *buffer = message->write.data;
			u32   len    = message->write.length;

			/* Invalidate cache */
			os_sync_before_read(buffer, len);

			/* Write file */
			ret = FAT_Write(message->fd, buffer, len);

			break;
		}

		case IOS_SEEK: {
			u32 where  = message->seek.offset;
			u32 whence = message->seek.origin;

			/* Seek file */
			ret = FAT_Seek(message->fd, where, whence);

			break;
		}

		case IOS_IOCTLV: {
			/* Parse IOCTLV message */
			ret = __FAT_Ioctlv(message);

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
