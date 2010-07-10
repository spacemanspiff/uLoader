#include "test.h"

#include <gccore.h>
#include "mload.h"
#include "usbstorage2.h"

#include "gfx.h"
#include "global.h"

void usb_test()
{
	static int n_sectors = 0, errores = 0;
	static u32 sector = 0, sector_size;

	static u32 err_time = 0,ok_time = 0;
	static u32 first_time = 0, curr_time = 0,start_time = 0;
	static int err = 2;
	static u32 addr = 0,dat,dat2 = 0,dat3 = 0,bytes_readed = 0,dat4 = 0, flag = 0;

		if (err == 2) {
			err = 0;
			start_time = first_time = time(NULL);
		}

	if (addr == 0) {
		// get EHCI base registers
		mload_getw((void *) 0x0D040000, &addr);
		addr &= 0xff;
		addr += 0x0D040000;
	}

	if (n_sectors == 0)
		n_sectors = USBStorage2_GetCapacity(&sector_size);
	dat = 0;
	
	mload_getw((void *) (addr+0x44), &dat);
	if (dat != 0x1005) 
		dat2 = dat;
	if (dat2 != 0x1805) 
		dat3 = dat2;
	PY += 32;
	s_printf("port: %x %x %x\n", dat, dat2, dat3);
	PY -= 32;

	if (USBStorage2_ReadSectors(sector, 64*512/sector_size, disc_conf) <= 0) {
		sprintf(cabecera2_str," ERROR! %i", errores);
		errores++;
		err_time = gettick();
		if (err == 0) 
			curr_time = time(NULL) - first_time;
		err=1;
	} else {
		if (err) {
			first_time = time(NULL);
			ok_time = gettick();
			err=0;
		}
		sprintf(cabecera2_str,"OK %i %u Last T: %u Time %u", errores, ticks_to_msecs(ok_time-err_time), curr_time, (u32) (time(NULL)-start_time)); 
		bytes_readed += 64*512;
	}

	switch ((((u32) (time(NULL)-start_time)) % 10)) {
	case 0:
		if (flag == 0) {
			dat4 = bytes_readed / (10);
			bytes_readed = 0;
			flag = 1;
		}
		break;
	default:
		flag = 0;
		break;
	}

	PY = 480 - 64;
	s_printf("sector: %u / %u\n", sector, n_sectors);

	PY = 480 - 32;
	s_printf("speed: %i bytes/sec\n", dat4);

	
	sector += 64*512 / sector_size;
	if (sector > (n_sectors - (64 * 512 / sector_size)))
		sector = 0;
	
	if (err) 
		signal_draw_cabecera2 = 1;
	draw_cabecera2();

	signal_draw_cabecera2 = 0;
}

#ifdef MEM_PRINT
void save_log()
{
	FILE *fp;
	int len;
	if (!sd_ok) 
		return;

	mload_init();
	len = mload_get_log();
//	mload_seek(0x13750000, SEEK_SET);
	if (len>0)
		mload_read(temp_data, len);
	mload_close();

	if (len <= 0)
		return;

	if (sd_ok) {
		for (len = 0;len < 4096; len++) {
			if (temp_data[len] == 0) 
				break;
		}

		fp = fopen("sd:/log_ehc.txt","wb");

		if(fp != 0) {
			fwrite(temp_data, 1, len ,fp);
				
			fclose(fp);
		}
	}
}
#endif

