#include <stdio.h>
#include <malloc.h>
#include <ogcsys.h>

#include "usbstorage2.h"
#include "utils.h"

#include "wdvd.h"

#include "libwbfs/libwbfs.h"

/* Constants */
#define MAX_NB_SECTORS	64

/* Variables */
static wbfs_t *hdd = NULL;
static u32 nb_sectors, sector_size;


void display_spinner(int mode, int percent, char *str);

void __WBFS_Spinner(s32 x, s32 max)
{
	static time_t start;
	static u32 expected;
	static u32 old_d,dd;
    char string[128];
	u32 wii_sec_per_wbfs_sect = 1<<(hdd->wbfs_sec_sz_s-hdd->wii_sec_sz_s);
	f32 percent, size;
	u32 d, h, m, s;
	s32 max2= max*wii_sec_per_wbfs_sect;

	/* First time */
	if (!x) {
		start    = time(0);
		expected = 300;
		old_d=start;
	}

	/* Elapsed time */
	dd=time(0);
	d =dd  - start;

	if (x != max2) {
		/* Expected time */
		if (d)
			expected = (expected * 3 + d * max2 / x) / 4;

		/* Remaining time */
		d = (expected > d) ? (expected - d) : 0;
	}

	/* Calculate time values */
	h =  d / 3600;
	m = (d / 60) % 60;
	s =  d % 60;

	/* Calculate percentage/size */
	percent = (x * 100.0) / (max2);
	size    = (hdd->wbfs_sec_sz / GB_SIZE) * max;

//	return;);

	/* Show progress */
	if (x != max2 ) {

	    if((dd-old_d)>0)
			{
			old_d=dd;
			snprintf(string, sizeof(string), "%.2fGB ETA: %d:%02d:%02d", size, h, m, s);
			display_spinner(0,(int)percent,string);
			}
	} else
		{
		snprintf(string, sizeof(string), "%.2fGB copied in %d:%02d:%02d", size, h, m, s);
		display_spinner(1,(int)percent,string);
		}
}


s32 __WBFS_ReadDVD(void *fp, u32 lba, u32 len, void *iobuf)
{
	void *buffer = NULL;

	u64 offset;
	u32 mod, size;
	s32 ret;

	/* Calculate offset */
	offset = ((u64)lba) << 2;

	/* Calcualte sizes */
	mod  = len % 32;
	size = len - mod;

	/* Read aligned data */
	if (size) {
		ret = WDVD_UnencryptedRead(iobuf, size, offset);
		if (ret < 0)
			goto out;
	}

	/* Read non-aligned data */
	if (mod) {
		/* Allocate memory */
		buffer = memalign(32, 0x20);
		if (!buffer)
			return -1;

		/* Read data */
		ret = WDVD_UnencryptedRead(buffer, 0x20, offset + size);
		if (ret < 0)
			goto out;

		/* Copy data */
		memcpy(iobuf + size, buffer, mod);
	}

	/* Success */
	ret = 0;

out:
	/* Free memory */
	if (buffer)
		free(buffer);

	return ret;
}

s32 __WBFS_ReadUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do reads */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * sector_size);
		u32   sectors = (count - cnt);

		/* Read sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB read */
		ret = USBStorage2_ReadSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}

s32 __WBFS_WriteUSB(void *fp, u32 lba, u32 count, void *iobuf)
{
	u32 cnt = 0;
	s32 ret;

	/* Do writes */
	while (cnt < count) {
		void *ptr     = ((u8 *)iobuf) + (cnt * sector_size);
		u32   sectors = (count - cnt);

		/* Write sectors is too big */
		if (sectors > MAX_NB_SECTORS)
			sectors = MAX_NB_SECTORS;

		/* USB write */
		ret = USBStorage2_WriteSectors(lba + cnt, sectors, ptr);
		if (ret < 0)
			return ret;

		/* Increment counter */
		cnt += sectors;
	}

	return 0;
}


s32 WBFS_Init(void)
{
	s32 ret;

	/* Initialize USB storage */
	ret = USBStorage2_Init();
	if (ret < 0)
		return ret;

	/* Get USB capacity */
	nb_sectors = USBStorage2_GetCapacity(&sector_size);
	if (!nb_sectors)
		return -1;

	return 0;
}

s32 WBFS_Open(void)
{
	/* Close hard disk */
	if (hdd)
		wbfs_close(hdd);

	/* Open hard disk */
	hdd = wbfs_open_hd(__WBFS_ReadUSB, __WBFS_WriteUSB, NULL, sector_size, nb_sectors, 0);
	if (!hdd)
		return -1;

	return 0;
}

s32 WBFS_Open2(int index) // index 0-3
{
	u32 lba;
	/* Close hard disk */
	if (hdd)
		wbfs_close(hdd);hdd=0;

	lba=wbfs_get_partition_LBA(index);

	if(lba==0xFFFFFFFF)
		{
		if((index & 3)==0) return WBFS_Open();
		return -1;
		}
	/* Open hard disk */
	hdd = wbfs_open_partition(__WBFS_ReadUSB, __WBFS_WriteUSB, NULL, sector_size, 0, lba, 0);
	if (!hdd)
		return -1;

	return 0;
}

s32 WBFS_Close(void)
{
	if (hdd)
		wbfs_close(hdd);
	hdd=0;
return 0;
}

int CWIIDisc_getdols(wbfs_disc_t *d);

s32 WBFS_getdols(u8 *id)
{
int ret=0;

wbfs_disc_t *disc = NULL;
if (!hdd)
		return -1;



	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, id);
	if (disc) {
        
		if(CWIIDisc_getdols(disc)<0) ret=-1;
		/* Close disc */
		wbfs_close_disc(disc);

		return ret;
	}
return -1;
}

s32 disc_getdols(u8 *id)
{
int ret=0;
wbfs_disc_t *disc = NULL;

	if(CWIIDisc_getdols(disc)<0) ret=-1;

return ret;

}

s32 WBFS_Format(u32 lba, u32 size)
{
	wbfs_t *partition = NULL;

	/* Reset partition */
	partition = wbfs_open_partition(__WBFS_ReadUSB, __WBFS_WriteUSB, NULL, sector_size, size, lba, 1);
	if (!partition)
		return -1;

	/* Free memory */
	wbfs_close(partition);

	return 0;
}

s32 WBFS_GetCount(u32 *count)
{
	/* No USB device open */
	if (!hdd)
		return -1;

	/* Get list length */
	*count = wbfs_count_discs(hdd);

	return 0;
}

s32 WBFS_GetHeaders(void *outbuf, u32 cnt, u32 len)
{
	u32 idx, size;
	s32 ret;

	/* No USB device open */
	if (!hdd)
		return -1;

	for (idx = 0; idx < cnt; idx++) {
		u8 *ptr = ((u8 *)outbuf) + (idx * len);

		/* Get header */
		ret = wbfs_get_disc_info(hdd, idx, ptr, len, &size);
		if (ret < 0)
			return ret;
	}

	return 0;
}

s32 WBFS_CheckGame(u8 *discid)
{
	wbfs_disc_t *disc = NULL;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}
// added by Hermes
s32 WBFS_GetProfileDatas(u8 *discid, u8 *buff)
{
	wbfs_disc_t *disc = NULL;
if (!hdd)
		return 0;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		
		wbfs_disc_read(disc,(1024>>2), buff, 1024);
		
		if(buff[0]=='H' && buff[1]=='D' && buff[2]=='R' && buff[3]!=0)
			
			wbfs_disc_read(disc,(2048>>2), buff+1024, ((u32) buff[3])*1024);
		
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}

// added by Hermes
s32 WBFS_SetProfileDatas(u8 *discid, u8 *buff)
{
	wbfs_disc_t *disc = NULL;
if (!hdd)
		return 0;
	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, discid);
	if (disc) {
		
		wbfs_disc_write(disc,(1024>>2), buff, 1024);
		
		if(buff[0]=='H' && buff[1]=='D' && buff[2]=='R' && buff[3]!=0)
			
			wbfs_disc_write(disc,(2048>>2), buff+1024, ((u32) buff[3])*1024);
		/* Close disc */
		wbfs_close_disc(disc);

		return 1;
	}

	return 0;
}

s32 WBFS_LoadCfg(void *data, s32 size, void *data2)
{
wbfs_disc_t *disc = NULL;
if (!hdd)
		return 0;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
	if (!disc)
		{
		wbfs_add_cfg(hdd, NULL, NULL, __WBFS_Spinner, ONLY_GAME_PARTITION);
		memset(data2,0,32768);
		return 1;
		}
	else 
		{
        wbfs_disc_read(disc,(256>>2), data, size); // from 0x100 to 0x1ff is cfg datas (of the 2MB)
		wbfs_disc_read(disc,(1024>>2), data2, 32768);
		wbfs_close_disc(disc);
		
		return 1;
		}

return 0;
}

s32 WBFS_SaveCfg(void *data, s32 size, void *data2)
{
wbfs_disc_t *disc = NULL;
if(!hdd) WBFS_Open();
if (!hdd)
		return 0;
	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
	if (!disc)
		{
		wbfs_add_cfg(hdd, NULL, NULL, __WBFS_Spinner, ONLY_GAME_PARTITION);
	   
		disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
		}
   
	if(disc)
		{
        wbfs_disc_write(disc,(256>>2), data, size); // from 0x100 to 0x1ff is cfg datas (of the 2MB)
		wbfs_disc_write(disc,(1024>>2), data2, 32768);
		wbfs_close_disc(disc);
		
		return 1;
		}

return 0;
}


s32 WBFS_LoadDolInfo(void *data)
{
wbfs_disc_t *disc = NULL;

memset(data,0,32768);

if (!hdd)
		return 0;

	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
	if (!disc)
		{
		wbfs_add_cfg(hdd, NULL, NULL, __WBFS_Spinner, ONLY_GAME_PARTITION);
	
		return 1;
		}
	else 
		{
		wbfs_disc_read(disc,((1024+32768)>>2), data, 32768);
		wbfs_close_disc(disc);
		
		return 1;
		}

return 0;
}

s32 WBFS_SaveDolInfo(void *data)
{
wbfs_disc_t *disc = NULL;
if(!hdd) WBFS_Open();
if (!hdd)
		return 0;
	/* Try to open game disc */
	disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
	if (!disc)
		{
		wbfs_add_cfg(hdd, NULL, NULL, __WBFS_Spinner, ONLY_GAME_PARTITION);
	   
		disc = wbfs_open_disc(hdd, (u8 *) "__CFG_");
		}
   
	if(disc)
		{
		wbfs_disc_write(disc,((1024+32768)>>2), data, 32768);
		wbfs_close_disc(disc);
		
		return 1;
		}

return 0;
}

s32 WBFS_AddGame(int mode)
{
	s32 ret;
	double a,b;

	/* No USB device open */
	if (!hdd)
		return -1;
	u32 count = wbfs_count_usedblocks(hdd);

	u32 estimation = wbfs_estimate_disc(hdd,  __WBFS_ReadDVD,__WBFS_Spinner, (mode==0)  ? ONLY_GAME_PARTITION : ALL_PARTITIONS);

	a=(double)estimation;
	b=( ((double)count) * ((double)(hdd->wbfs_sec_sz/512)));

	if( a> b)
			{
			my_perror("no space left on device (disc full)");
			return -666;
			}


	/* Add game to USB device */
	ret = wbfs_add_disc(hdd, __WBFS_ReadDVD, NULL, __WBFS_Spinner,(mode==0) ? ONLY_GAME_PARTITION : ALL_PARTITIONS, 0);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_RemoveGame(u8 *discid)
{
	s32 ret;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Remove game from USB device */
	ret = wbfs_rm_disc(hdd, discid);
	if (ret < 0)
		return ret;

	return 0;
}

s32 WBFS_GameSize(u8 *discid, f32 *size)
{
	wbfs_disc_t *disc = NULL;

	u32 sectors;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Open disc */
	disc = wbfs_open_disc(hdd, discid);
	if (!disc)
		return -2;

	/* Get game size in sectors */
	sectors = wbfs_sector_used(hdd, disc->header);

	/* Close disc */
	wbfs_close_disc(disc);

	/* Copy value */
	*size = (hdd->wbfs_sec_sz / GB_SIZE) * sectors;

	return 0;
}

s32 WBFS_DiskSpace(f32 *used, f32 *free)
{
	f32 ssize;
	u32 cnt;

	/* No USB device open */
	if (!hdd)
		return -1;

	/* Count used blocks */
	cnt = wbfs_count_usedblocks(hdd);

	/* Sector size in GB */
	ssize = hdd->wbfs_sec_sz / GB_SIZE;

	/* Copy values */
	*free = ssize * cnt;
	*used = ssize * (hdd->n_wbfs_sec - cnt);

	return 0;
}



s32 WBFS_RenameGame(u8 *discid, char *new_name)
{

int ret=0;

if (!hdd)
		return 0;
	/* Try to open game disc */
	ret= wbfs_ren_disc(hdd, (u8 *) discid, new_name);

	
return ret;
}


s32 WBFS_SetBCA(u8 *discid, u8 *bca)
{
u8 *buffer;

wbfs_disc_t *disc = NULL;
if (!hdd)
		return 0;
	
	buffer=malloc(0x8000);

if(!buffer) return 0;


/* Try to open game disc */
	disc = wbfs_open_disc(hdd, (u8 *) discid);
	if (!disc)
		{
		free(buffer);
	
		return 0;
		}
	else 
		{
		wbfs_disc_read(disc, (0)>>2, buffer, 512);
		
		memcpy( (void *) (buffer+0x100), (void *)  bca, 64);

		wbfs_disc_write(disc,(0)>>2, buffer, 512);
		wbfs_close_disc(disc);
		
		}
	
	free(buffer);
	
return 1;
}
