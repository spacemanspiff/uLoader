// Copyright 2009 Kwiirk
// Modified by Hermes
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <windows.h>
#include "commdlg.h"
#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include "xgetopt.h"
#include <sys/stat.h>

#include <conio.h>

#ifdef WIN32
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "tools.h"
#include "libwbfs.h"

#ifdef WIN32
#define snprintf _snprintf
#endif


#define ERR(x) do {wbfs_error(x);goto error;}while(0)
#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(p->hd_sec_sz-1)))

#define GB (1024 * 1024 * 1024.)



int read_wii_disc_sector(void *_handle, u32 _offset, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD read;
	u64 offset = _offset;
	
	offset <<= 2L;
	

    large.QuadPart = offset;

	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		wbfs_error("error seeking in disc file");
		return 1;
	}
	
	read = 0;
	if (ReadFile(handle, buf, count, &read, NULL) == FALSE)
	{
		wbfs_error("error reading wii disc sector");
		return 1;
	}

	if (read != count)
	{
		wbfs_error("error reading wii disc sector (size mismatch)");
		return 1;
	}

	return 0;
}



int write_wii_disc_sector(void *_handle, u32 lba, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD written;
	u64 offset = lba;
	
	offset *= 0x8000;
	large.QuadPart = offset;
	
	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		fprintf(stderr,"\n\n%lld %p\n", offset, handle);
		wbfs_error("error seeking in wii disc sector (write)");
		return 1;
	}
	
	written = 0;
	if (WriteFile(handle, buf, count * 0x8000, &written, NULL) == FALSE)
	{
		wbfs_error("error writing wii disc sector");
		return 1;
	}

	if (written != count * 0x8000)
	{
		wbfs_error("error writing wii disc sector (size mismatch)");
		return 1;
	}
	
	return 0;
}




static void _spinner(int x, int y){ spinner(x, y); }

static u8 size_to_shift(u32 size)
{
        u8 ret = 0;
        while(size)
        {
                ret++;
                size>>=1;
        }
        return ret-1;
}
#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))



// write access


static int block_used(u8 *used,u32 i,u32 wblk_sz)
{
        u32 k;
        i*=wblk_sz;
        for(k=0;k<wblk_sz;k++)
                if(i+k<143432*2 && used[i+k])
                        return 1;
        return 0;
}


u32 wbfs_iso_to_ciso
	(
		void *write_hand,
		read_wiidisc_callback_t read_src_wii_disc,
		void *callback_data,
		progress_callback_t spinner,
		partition_selector_t sel
	)
{
	int i;//, discn;
	u32 tot, cur;
	u32 offset;
	u32 wii_sec_per_wbfs_sect = 1 << (7);
	int n_wbfs_sec_per_disc = (143432*2) >> 7;
	u32 wbfs_sec_sz=1 << (7+size_to_shift(0x8000));

	wiidisc_t *d = 0;
	u8 *used = 0;

	u8* copy_buffer = 0;

	used = wbfs_malloc(143432*2);
	
	if (!used)
	{
		ERR("unable to alloc memory");
	}
	
	
		d = wd_open_disc(read_src_wii_disc, callback_data);
		if(!d)
		{
			ERR("unable to open wii disc");
		}
		wd_build_disc_usage(d, sel, used);
		wd_close_disc(d);
		d = 0;
	
	

	copy_buffer = wbfs_ioalloc(wbfs_sec_sz);
	if (!copy_buffer)
	{
		ERR("alloc memory");
	}

	wbfs_memset(copy_buffer,0, 32768);

	copy_buffer[0]='C';
	copy_buffer[1]='I';
	copy_buffer[2]='S';
	copy_buffer[3]='O';
		
	i=wbfs_sec_sz;

	copy_buffer[4]=i & 255;
	copy_buffer[5]=(i>>8) & 255;
	copy_buffer[6]=(i>>16) & 255;
	copy_buffer[7]=(i>>24) & 255;

	if(n_wbfs_sec_per_disc>32760) ERR("ISO exceeds CISO index table");

	for (i = 0; i < n_wbfs_sec_per_disc; i++)
		{
			if (block_used(used, i, wii_sec_per_wbfs_sect))
			{
			copy_buffer[8+i]=1;
			}
		}

	write_wii_disc_sector((void *) write_hand, 0, 1, copy_buffer);
	
	tot = 0;
	cur = 0;

	offset=1;
	

	if (spinner)
	{
		// count total number to write for spinner
		for (i = 0; i < n_wbfs_sec_per_disc; i++)
		{
			if ( block_used(used, i, wii_sec_per_wbfs_sect))
			{
				tot++;
				spinner(0, tot);
			}
		}
	}


	
	for (i = 0; i < n_wbfs_sec_per_disc; i++)
	{
		//u16 bl = 0;
		if (block_used(used, i, wii_sec_per_wbfs_sect))
		{

			read_src_wii_disc(callback_data, i * (wbfs_sec_sz >> 2), wbfs_sec_sz, copy_buffer);
			
			// fix the partition table.
			if (i == (0x40000 >> (7+size_to_shift(0x8000)/*wbfs_sec_sz_s*/)))
			{
				wd_fix_partition_table(d, sel, copy_buffer + (0x40000 & (wbfs_sec_sz - 1)));
			}

			write_wii_disc_sector((void *) write_hand, offset, wbfs_sec_sz/32768, copy_buffer);
			
			offset+=wbfs_sec_sz/32768;
			
			if (spinner)
			{
				cur++;
				spinner(cur, tot);
			}
		}

	}
	

error:

	if(d)
		wd_close_disc(d);
	if(used)
		wbfs_free(used);

	if(copy_buffer)
		wbfs_iofree(copy_buffer);
	
	// init with all free blocks
	return 0;
}


u32 wbfs_ciso_to_iso
	(
		void *write_hand,
		read_wiidisc_callback_t read_src_wii_disc,
		void *callback_data,
		progress_callback_t spinner
	)
{
	int i;
	u32 tot, cur;

	u32 wbfs_sec_sz=1 << (7+size_to_shift(0x8000));
	u32 write_blocks;

	u32 offset;



	u8* copy_buffer = 0;
	u8* map_buffer =0;

	map_buffer = wbfs_ioalloc(32768);

	if (!map_buffer)
	{
		ERR("alloc memory");
	}
	
	read_src_wii_disc(callback_data, 0, 32768, map_buffer);

	offset=(32768>>2);

	if(!(map_buffer[0]=='C' && map_buffer[1]=='I' && map_buffer[2]=='S' && map_buffer[3]=='O')) ERR("ist'n .CISO format!!");

	wbfs_sec_sz=((u32)((u8)map_buffer[4]))+(((u32)((u8)map_buffer[5]))<<8)+(((u32)((u8)map_buffer[6]))<<16)+(((u32)((u8)map_buffer[7]))<<24);
	
	write_blocks=wbfs_sec_sz/32768;
	

	copy_buffer = wbfs_ioalloc(wbfs_sec_sz);
	if (!copy_buffer)
	{
		ERR("alloc memory");
	}

	
	
	tot = 0;
	cur = 0;
	

	if (spinner)
	{
		// count total number to write for spinner
		for (i = 0; i < 32760; i++)
		{
			if (map_buffer[i+8])
			{
				tot++;
				spinner(0, tot);
			}
		}
	}
	
	
	for (i = 0; i < 32760; i++)
	{
		//u16 bl = 0;
		if (map_buffer[i+8])
		{

			read_src_wii_disc(callback_data, offset, wbfs_sec_sz, copy_buffer);
			offset+=(wbfs_sec_sz >> 2);
		

			write_wii_disc_sector((void *) write_hand, i * write_blocks, write_blocks, copy_buffer);
			
			if (spinner)
			{
				cur++;
				spinner(cur, tot);
			}
		}

	}

 

error:

	if(copy_buffer)
		wbfs_iofree(copy_buffer);
	if(map_buffer)
		wbfs_iofree(map_buffer);
	
	// init with all free blocks
	return 0;
}


void wbfs_isotociso(char *argv)
{
	int n,m;
	u8 discinfo[7];
	char fileName[1024]="";

	n=0;m=-1;

	while(argv[n]!=0) 
		{
		if(argv[n]=='.') m=n;
		n++;
		}

	if(m==-1)
		{
		fprintf(stderr, "disc file must be .iso or .ciso\n"); return;
		}

	if(!strncmp(&argv[m],".iso",4) || !strncmp(&argv[m],".ISO",4))
		{
		memcpy(fileName,argv,m);
		memcpy(&fileName[m],".ciso",6);
		}
	else
	if(!strncmp(&argv[m],".ciso",5) || !strncmp(&argv[m],".CISO",5))
		{
		memcpy(fileName,argv,m);
		memcpy(&fileName[m],".iso",5);
		}
	else
		{
		fprintf(stderr, "disc file must be .iso or .ciso\n"); return;
		}


	HANDLE *handle = CreateFile(argv, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "unable to open disc file\n");return;
	}
	else
	{
		DWORD read = 0;
		
		LARGE_INTEGER large;
		large.QuadPart = 0;
		
		SetFilePointerEx(handle, large, NULL, FILE_BEGIN);

		LARGE_INTEGER large2;

	  
		
		ReadFile(handle, discinfo, 6, &read, NULL);
        
		if(discinfo[0]=='C' && discinfo[1]=='I' && discinfo[2]=='S' && discinfo[3]=='O')
			{
			
			// force ISO

			memcpy(fileName,argv,m);
			memcpy(&fileName[m],".iso",5);
			

			HANDLE *handle2 = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);

			if (handle2 == INVALID_HANDLE_VALUE)
				{
				fprintf(stderr, "unable to create .iso file (maybe it exists)\n");
				CloseHandle(handle);return;
				}

			printf("Making .CISO to .ISO ...\n\n");

			large2.QuadPart = ((u64) (143432*2 / 2)) * 0x8000ULL;
			SetFilePointerEx(handle2, large2, NULL, FILE_BEGIN);
			SetEndOfFile(handle2);

			wbfs_ciso_to_iso((void *) handle2, read_wii_disc_sector, handle, _spinner);
			
			CloseHandle(handle2);
			}
		else
			{

			HANDLE *handle2 = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);

			if (handle2 == INVALID_HANDLE_VALUE)
				{
				fprintf(stderr, "unable to create .ciso file (maybe it exists)\n");
				CloseHandle(handle);return;
				}

			printf("Making .ISO to .CISO ...\n\n");
		
			wbfs_iso_to_ciso((void *) handle2, read_wii_disc_sector, handle, _spinner, ONLY_GAME_PARTITION);
			
			CloseHandle(handle2);
			}
        

		CloseHandle(handle);
	}
}



int main(int argc, char *argv[])
{

	char current_directory[1024];

	current_directory[0]=0;
	getcwd(current_directory,1024);
	
	if(argc>1)
		{
		fprintf(stderr, "isotociso (c) 2010, Hermes\n\n");
		}
	
     if(argc<=1)
		{
		OPENFILENAME ofn;
		char szFileName[MAX_PATH] = "";

		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn); 
		ofn.hwndOwner =NULL;
		ofn.lpstrFilter = "ISO Files (*.iso; *.ciso)\0*.iso;*.ciso\0\0\0";
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE ;
		ofn.lpstrDefExt = "iso";
		
		if(GetOpenFileName(&ofn)) 
			{
			system("cls");
			wbfs_isotociso(szFileName);

			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();
			}
		}
	else
		{
		wbfs_isotociso(argv[1]);

		/*printf("\nPress Any Key\n");
		while(1) {if(!kbhit()) break; getch();}
		getch();*/
		}

	chdir(current_directory);
	
	return EXIT_SUCCESS;
}
