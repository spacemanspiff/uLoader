#include "mload_modules.h"


static u32 ios_36[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022DDAC, // dvd_read_controlling_data
	0x20201010+1, // handle_di_cmd_reentry (thumb)
	0x20200b9c+1, // ios_shared_alloc_aligned (thumb)
	0x20200b70+1, // ios_shared_free (thumb)
	0x20205dc0+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202b4c+1, // ios_doReadHashEncryptedState (thumb)
	0x20203934+1, // ios_printf (thumb)
};

static u32 ios_38[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022cdac, // dvd_read_controlling_data
	0x20200d38+1, // handle_di_cmd_reentry (thumb)
	0x202008c4+1, // ios_shared_alloc_aligned (thumb)
	0x20200898+1, // ios_shared_free (thumb)
	0x20205b80+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202874+1, // ios_doReadHashEncryptedState (thumb)
	0x2020365c+1, // ios_printf (thumb)
};


/*
static u32 ios_60[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022cd60, // dvd_read_controlling_data
	0x20200f04+1, // handle_di_cmd_reentry (thumb)
	0, // ios_shared_alloc_aligned (thumb) // no usado
	0, // ios_shared_free (thumb) // no usado
	0x20205e00+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202944+1, // ios_doReadHashEncryptedState (thumb)
	0x20203750+1, // ios_printf (thumb)
};
*/

u32 patch_datas[8] ATTRIBUTE_ALIGN(32);

data_elf my_data_elf;

void *external_ehcmodule= NULL;
int size_external_ehcmodule=0;

static int my_thread_id=0;

int load_ehc_module()
{
int is_ios=0;

#if 0

FILE *fp;

// WARNING!!!: load external module suspended
if(sd_ok && !external_ehcmodule)
	{

	fp=fopen("sd:/apps/uloader/ehcmodule.elf","rb");

	if(fp!=0)
		{
		fseek(fp, 0, SEEK_END);
		size_external_ehcmodule = ftell(fp);
		external_ehcmodule= memalign(32, size_external_ehcmodule);
		if(!external_ehcmodule) 
			{fclose(fp);}
		else
			{
			fseek(fp, 0, SEEK_SET);

			if(fread(external_ehcmodule,1, size_external_ehcmodule ,fp)!=size_external_ehcmodule)
				{free(external_ehcmodule); external_ehcmodule=NULL;}
		
			fclose(fp);
			}
		}
	}
#endif

/*
	if(mload_init()<0) return -1;
	mload_elf((void *) logmodule, &my_data_elf);
	my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, my_data_elf.prio);
	if(my_thread_id<0) return -1;
	*/
  
	if(!external_ehcmodule)
		{
		if(mload_init()<0) return -1;
		mload_elf((void *) ehcmodule, &my_data_elf);
		my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, my_data_elf.prio);
		if(my_thread_id<0) return -1;
		//if(mload_module(ehcmodule, size_ehcmodule)<0) return -1;
		}
	else
		{
		//if(mload_module(external_ehcmodule, size_external_ehcmodule)<0) return -1;
		if(mload_init()<0) return -1;
		mload_elf((void *) external_ehcmodule, &my_data_elf);
		my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, my_data_elf.prio);
		if(my_thread_id<0) return -1;
		}
	usleep(350*1000);
	

	// Test for IOS

	
	mload_seek(0x20207c84, SEEK_SET);
	mload_read(patch_datas, 4);
	if(patch_datas[0]==0x6e657665) 
		{
		is_ios=38;
		}
	else
		{
		is_ios=36;
		}

	if(is_ios==36)
		{
		// IOS 36
		memcpy(ios_36, dip_plugin, 4);		// copy the entry_point
		memcpy(dip_plugin, ios_36, 4*10);	// copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20209040, SEEK_SET);
		mload_write(ios_36, 4);
		 
		}
	if(is_ios==38)
		{
		// IOS 38

		memcpy(ios_38, dip_plugin, 4);	    // copy the entry_point
		memcpy(dip_plugin, ios_38, 4*10);   // copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20208030, SEEK_SET);
		mload_write(ios_38, 4);

		
		}

	mload_close();

return 0;
}



#define IOCTL_FAT_MOUNTSD	0xF0
#define IOCTL_FAT_UMOUNTSD	0xF1
#define IOCTL_FAT_MOUNTUSB	0xF2
#define IOCTL_FAT_UMOUNTUSB	0xF3



int load_fat_module(u8 *discid)
{
static char fs[] ATTRIBUTE_ALIGN(32) = "fat";
static s32 hid = -1, fd = -1;
static char file_name[256]  ALIGNED(0x20)="sd:";
int n;

s32 ret;

	if(mload_init()<0) return -1;

	mload_elf((void *) fat_module, &my_data_elf);
	my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, my_data_elf.prio);
	if(my_thread_id<0) return -1;

	sd_ok=ud_ok=1;
			
	char *p=get_fat_name(discid);
			
	sd_ok=ud_ok=0;
			
	if(!p) return -1;
			
	// change 'ud:' by 'usb:'
	if(p[0]=='u') {file_name[0]='u';file_name[1]='s';file_name[2]='b';memcpy(file_name+3, (void *)p+2, 253);}			   
	else {memcpy(file_name, (void *) p, 256);}

	// copy filename to dip_plugin filename area
	mload_seek(*((u32 *) (dip_plugin+14*4)), SEEK_SET);	// offset 14 (filename Address - 256 bytes)
	mload_write(file_name, sizeof(file_name)+1);
	mload_close();
		
	usleep(350*1000);

	/* Create heap */
	if (hid < 0) {
		hid = iosCreateHeap(0x100);
		if (hid < 0)
			return -1; 
	}

	/* Open USB device */
	fd = IOS_Open(fs, 0);
	
	if (fd < 0)
		return -1;
  
	n=20; // try 20 times
	while(n>0)
	{
		if(p[0]=='u') ret=IOS_IoctlvFormat(hid, fd, IOCTL_FAT_MOUNTUSB, ":");
		else ret=IOS_IoctlvFormat(hid, fd, IOCTL_FAT_MOUNTSD, ":");
	
	if(ret==0) break;
	usleep(500*1000);
	n--;
	}

    if (fd >= 0) {
		IOS_Close(fd);
		fd = -1;
	}

	if(hid>=0)
		{
		iosDestroyHeap(hid);
		hid=-1;
		}
	
	if(n==0) return -1;

return 0;
}

void enable_ES_ioctlv_vector(void)
{
	patch_datas[0]=*((u32 *) (dip_plugin+16*4));
	mload_set_ES_ioctlv_vector((void *) patch_datas[0]);
}

void Set_DIP_BCA_Datas(u8 *bca_data)
{
	// write in dip_plugin bca data area
	mload_seek(*((u32 *) (dip_plugin+15*4)), SEEK_SET);	// offset 15 (bca_data area)
	mload_write(bca_data, 64);
	mload_close();
}


void test_and_patch_for_port1()
{
// test for port 1
	
u8 * ehc_data=search_for_ehcmodule_cfg((void *) ehcmodule, size_ehcmodule);
	
	if(ehc_data)
		{
		ehc_data+=12;
		use_port1=ehc_data[0];
		
		}
	

	if(use_port1)
	// release port 0 and use port 1
	{
		u32 dat=0;
		u32 addr;

		// get EHCI base registers
		mload_getw((void *) 0x0D040000, &addr);

		addr&=0xff;
		addr+=0x0D040000;
		
		
		mload_getw((void *) (addr+0x44), &dat);
		if((dat & 0x2001)==1) mload_setw((void *) (addr+0x44), 0x2000);
		mload_getw((void *) (addr+0x48), &dat);
		if((dat & 0x2000)==0x2000) mload_setw((void *) (addr+0x48), 0x1001);
	}
}
//////////////////////////////////

