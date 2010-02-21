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


int block_ciso=0;

wbfs_t *wbfs_try_open(char *disc, char *partition, int reset);
wbfs_t *wbfs_try_open_partition(char *fn, int reset);

u32 wbfs_add_cfg(wbfs_t *p, read_wiidisc_callback_t read_src_wii_disc, void *callback_data, progress_callback_t spinner, partition_selector_t sel);

void wbfs_integrity_check(wbfs_t* p);

#define GB (1024 * 1024 * 1024.)

int cios_mode=0;
char cios_map[32768];
int cios_map_off[32768];
unsigned cios_size=64*32768;

int read_wii_disc_sector(void *_handle, u32 _offset, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD read;
	u64 offset = _offset;
	
	offset <<= 2L;
	

	if(cios_mode== (int) _handle)
	{
	int off=cios_map_off[(u32)(offset/(u64)cios_size)];
	if(off<0) {memset(buf,0,count);if(off==-2) return 1; return 0;}
//	printf("coffset %i size %i\n",_offset,count);
	large.QuadPart = (offset & ((u64)(cios_size-1)))+(((u64) ((u32)off))* (u64)cios_size)+32768ULL;
	}
	else {large.QuadPart = offset;}

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

void set_cios_mode(wiidisc_t*d, int mode)
{
if(!mode) cios_mode=0;
else
	{
	int n,o=0,last=0;
	cios_mode=0;
	read_wii_disc_sector(d->fp,0, 0x8000,cios_map);
	cios_mode=(int) d->fp;
	cios_size=((u32)((u8)cios_map[4]))+(((u32)((u8)cios_map[5]))<<8)+(((u32)((u8)cios_map[6]))<<16)+(((u32)((u8)cios_map[7]))<<24);

    for(n=0;n<32760;n++)
		{
		if(cios_map[n+8]) {cios_map_off[n]=o;o++;last=n;} else cios_map_off[n]=-1;
		}

	for(n=last+1;n<32760;n++) cios_map_off[n]=-2;
	
	}
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

int pos_list=0;

char string_id[7]="";

void wbfs_display_list(wbfs_t *p)
{
	int count = wbfs_count_discs(p);
    u32 count2 = wbfs_count_usedblocks(p);

	string_id[0]=0;

	 printf(" Games: %i | Total: %.2fGB Used: %.2fGB Free: %.2fGB\n\n",count, (float)p->n_wbfs_sec * p->wbfs_sec_sz / GB, 
			(float)(p->n_wbfs_sec-count2) * p->wbfs_sec_sz / GB, (float)(count2) * p->wbfs_sec_sz / GB);
	

	if (count == 0)
	{
		printf("wbfs is empty\n");
		pos_list=-1;
	}
	else
	{
		int i,m;
		u32 size;
		u8 *b = wbfs_ioalloc(0x100);

		if(pos_list<0) pos_list=0;
		
		if(pos_list>=count) pos_list=count-1;
		i = pos_list-5;
		if(i<0) i=0;

		for (m=0; m <10; m++)
		{
		if(i>=count) printf("\n");
		else
			{
			if (!wbfs_get_disc_info(p, i, b, 0x100, &size))
				{
					printf("    %c %c%c%c%c%c%c %40s %.2fG\n",i==pos_list ? '>': ' ', b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20, size * 4ULL / (GB));
					if(i==pos_list) {memcpy(&string_id[0], b, 6);string_id[6]=0;}
				}
			}
		i++;
		}

		wbfs_iofree(b);
	}
}

void wbfs_applet_list(wbfs_t *p)
{
	int count = wbfs_count_discs(p);
	
	if (count == 0)
	{
		fprintf(stderr,"wbfs is empty\n");
	}
	else
	{
		int i;
		u32 size;
		u8 *b = wbfs_ioalloc(0x100);
		for (i = 0; i < count; i++)
		{
			if (!wbfs_get_disc_info(p, i, b, 0x100, &size))
			{
				fprintf(stderr, "%c%c%c%c%c%c %40s %.2fG\n", b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20, size * 4ULL / (GB));
			}
		}

		wbfs_iofree(b);
	}
}

void wbfs_applet_info(wbfs_t *p)
{
	u32 count = wbfs_count_usedblocks(p);
	
	fprintf(stderr, "wbfs\n");
	fprintf(stderr, "  blocks : %u\n", count);
	fprintf(stderr, "  total  : %.2fG\n", (float)p->n_wbfs_sec * p->wbfs_sec_sz / GB);
	fprintf(stderr, "  used   : %.2fG\n", (float)(p->n_wbfs_sec-count) * p->wbfs_sec_sz / GB);
	fprintf(stderr, "  free   : %.2fG\n", (float)(count) * p->wbfs_sec_sz / GB);
}

void wbfs_applet_makehbc(wbfs_t *p)
{
	int count = wbfs_count_discs(p);
	FILE *xml;
	
	if (count == 0)
	{
		fprintf(stderr,"wbfs is empty\n");
	}
	else
	{
		int i;
		u32 size;
		u8 *b = wbfs_ioalloc(0x100);
		
		for (i = 0; i < count; i++)
		{
			char dirname[7 + 1];
			char dolname[7 + 1 + 8 + 1];
			char pngname[7 + 1 + 8 + 1];
			char xmlname[7 + 1 + 8 + 1];

			wbfs_get_disc_info(p, i, b, 0x100, &size);

			snprintf(dirname, 7, "%c%c%c%c%c%c", b[0], b[1], b[2], b[3], b[4], b[5]);
			snprintf(dolname, 7 + 1 + 8, "%c%c%c%c%c%c\\boot.dol", b[0], b[1], b[2], b[3], b[4], b[5]);
			snprintf(pngname, 7 + 1 + 8, "%c%c%c%c%c%c\\icon.png", b[0], b[1], b[2], b[3], b[4], b[5]);
			snprintf(xmlname, 7 + 1 + 8, "%c%c%c%c%c%c\\meta.xml", b[0], b[1], b[2], b[3], b[4], b[5]);
			
			CreateDirectory(dirname, NULL);
			printf("%s\n", dirname);
			
			CopyFile("boot.dol", dolname, FALSE);
			CopyFile("icon.png", pngname, FALSE);

			xml = fopen(xmlname, "wb");
			fprintf(xml,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
			fprintf(xml,"<app>\n\t<name>%s</name>\n", b + 0x20);
			fprintf(xml,"<short_description>%.2fGB on USB HD</short_description>\n", size * 4ULL / GB);
			fprintf(xml,"<long_description>This launches the yal wbfs game loader by Kwiirk for discid %s</long_description>\n", dirname);
			fprintf(xml,"</app>");
			fclose(xml);
		}

		wbfs_iofree(b);
	}
}

void wbfs_applet_init(wbfs_t *p)
{
	// nothing to do actually..
	// job already done by the reset flag of the wbfs_open_partition
	fprintf(stderr, "wbfs initialized.\n");
}

void wbfs_applet_estimate(wbfs_t *p, char *argv)
{
	HANDLE *handle = CreateFile(argv, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "unable to open disc file\n");
	}
	else
	{
		u32 estimation = wbfs_estimate_disc(p, read_wii_disc_sector, handle, ONLY_GAME_PARTITION);
		fprintf(stderr, "%.2fG\n", (estimation / (GB))* 512.0f);
		CloseHandle(handle);
	}
}

static void _spinner(int x, int y){ spinner(x, y); }



void wbfs_applet_addcfg(wbfs_t *p)
{
	wbfs_disc_t *d;


		d = wbfs_open_disc(p, "__CFG_");
		
		if (d)
		{
			fprintf(stderr, "%s already in disc...\n", "__CFG_");
			wbfs_close_disc(d);
		}
		else
		{
			wbfs_add_cfg(p, read_wii_disc_sector, NULL, _spinner, ONLY_GAME_PARTITION);
        }

}


void wbfs_applet_add(wbfs_t *p, char *argv)
{
	u8 discinfo[7];
	wbfs_disc_t *d;



	HANDLE *handle = CreateFile(argv, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "unable to open disc file\n");
	}
	else
	{
		DWORD read = 0;
		u32 count = wbfs_count_usedblocks(p);
		u32 estimation = wbfs_estimate_disc(p, read_wii_disc_sector, handle, ONLY_GAME_PARTITION);
		LARGE_INTEGER large;
		large.QuadPart = 0;
		
		SetFilePointerEx(handle, large, NULL, FILE_BEGIN);

	    if( ((double)estimation)> ( ((double)count) * ((double) (p->wbfs_sec_sz/512))))
			{
			fprintf(stderr, "no space left on device (disc full)");
			
			CloseHandle(handle);
			return;
			}
		
		ReadFile(handle, discinfo, 6, &read, NULL);
        
		if(discinfo[0]=='C' && discinfo[1]=='I' && discinfo[2]=='S' && discinfo[3]=='O')
			{
			LARGE_INTEGER large;
			large.QuadPart=32768ULL;
			SetFilePointerEx(handle, large, NULL, FILE_BEGIN);
			ReadFile(handle, discinfo, 6, &read, NULL);
			}

		d = wbfs_open_disc(p, discinfo);
		
		if (d)
		{
			discinfo[6] = 0;
			fprintf(stderr, "%s already in disc...\n", discinfo);
			wbfs_close_disc(d);
		}
		else
		{
			wbfs_add_disc(p, read_wii_disc_sector, handle, _spinner, ONLY_GAME_PARTITION, 0);
        }

		CloseHandle(handle);
	}
}

void wbfs_applet_remove(wbfs_t *p, char *argv)
{
	wbfs_rm_disc(p, (u8 *)argv);
}

void wbfs_applet_isoextract(wbfs_t *p, char *argv)
{
	wbfs_disc_t *d;
	d = wbfs_open_disc(p, (u8 *)argv);
	
	if (d)
	{
		HANDLE *handle;
		char isoname[0x100];
		int i,len;
		
		/* get the name of the title to find out the name of the iso */
		strncpy(isoname, (char *)d->header->disc_header_copy + 0x20, 0x100);
		len = strlen(isoname);
		
		// replace silly chars by '_'
		for (i = 0; i < len; i++)
		{
			if (isoname[i] == ' ' || isoname[i] == '/' || isoname[i] == ':' || isoname[i] == '|')
			{
				isoname[i] = '_';
			}
		}
		
		strncpy(isoname + len, ".iso", 0x100 - len);
		
		handle = CreateFile(isoname, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
		
		if (handle == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "unable to open disc file (%s) for writing\n", isoname);
		}
		else
		{
			LARGE_INTEGER large;

			fprintf(stderr, "writing to %s\n", isoname);

			large.QuadPart = (d->p->n_wii_sec_per_disc / 2) * 0x8000ULL;
			SetFilePointerEx(handle, large, NULL, FILE_BEGIN);
			SetEndOfFile(handle);

			wbfs_extract_disc(d,write_wii_disc_sector, handle, _spinner);
			
			CloseHandle(handle);
		}
		
		wbfs_close_disc(d);
	}
	else
	{
		fprintf(stderr, "%s not in disc in disc...\n", argv);
	}
}
void wbfs_applet_png(wbfs_t *p, char *argv, char *png)
{
	wbfs_disc_t *d;
	
	if(!png){ printf("You need a .png file!\n"); return;}
	

	d = wbfs_open_disc(p, (u8 *)argv);
	
	if (d)
	{
		
		wbfs_add_png(d,png);
			
		wbfs_close_disc(d);
	}
	else
	{
		fprintf(stderr, "%s not in disc in disc...\n", argv);
	}
}

void wbfs_applet_remove_cfg(wbfs_t *p, char *argv)
{
	wbfs_disc_t *d;


	d = wbfs_open_disc(p, (u8 *)argv);
	
	if (d)
	{
		
		wbfs_remove_cfg(d);
			
		wbfs_close_disc(d);
	}
	else
	{
		fprintf(stderr, "%s not in disc in disc...\n", argv);
	}
}

void wbfs_applet_extract(wbfs_t *p, char *argv)
{
	wbfs_disc_t *d;

	if(block_ciso) {wbfs_applet_isoextract(p, argv);return;}

	d = wbfs_open_disc(p, (u8 *)argv);
	
	if (d)
	{
		HANDLE *handle;
		char isoname[0x100];
		int i,len;
		
		/* get the name of the title to find out the name of the iso */
		strncpy(isoname, (char *)d->header->disc_header_copy + 0x20, 0x100);
		len = strlen(isoname);
		
		// replace silly chars by '_'
		for (i = 0; i < len; i++)
		{
			if (isoname[i] == ' ' || isoname[i] == '/' || isoname[i] == ':' || isoname[i] == '|')
			{
				isoname[i] = '_';
			}
		}
		
		strncpy(isoname + len, ".ciso", 0x100 - len);
		
		handle = CreateFile(isoname, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW  , 0, NULL);
		
		if (handle == INVALID_HANDLE_VALUE)
		{
		
			fprintf(stderr, "unable to open disc file (%s) for writing\n", isoname);
		}
		else
		{
			LARGE_INTEGER large;

			fprintf(stderr, "writing to %s\n", isoname);

			large.QuadPart = 0;//(d->p->n_wii_sec_per_disc / 2) * 0x8000ULL;
			SetFilePointerEx(handle, large, NULL, FILE_BEGIN);
			SetEndOfFile(handle);

			wbfs_extract_disc2(d,write_wii_disc_sector, handle, _spinner);
			
			CloseHandle(handle);
		}
		
		wbfs_close_disc(d);
	}
	else
	{
		fprintf(stderr, "%s not in disc in disc...\n", argv);
	}
}

struct wbfs_applets
{
	char *command;
	void (*function_with_argument)(wbfs_t *p, char *argv);
	void (*function)(wbfs_t *p);
	void (*function_with_twoarguments)(wbfs_t *p, char *argv,char *argv2 );
}

#define APPLET(x) { #x,wbfs_applet_##x, NULL, NULL }
#define APPLET2(x) { #x, wbfs_applet_##x, NULL, wbfs_applet_##x}
#define APPLET_NOARG(x) { #x, NULL, wbfs_applet_##x, NULL }

wbfs_applets[] =
{
        APPLET_NOARG(list),
        APPLET_NOARG(info),
        APPLET_NOARG(makehbc),
        APPLET_NOARG(init),
        APPLET(add),
        APPLET(remove),
		APPLET(estimate),
        APPLET(extract),
		APPLET(isoextract),
		APPLET2(png),
		//APPLET(create),
};

#undef APPLET
#undef APPLET_NOARG

static int num_applets = sizeof(wbfs_applets) / sizeof(wbfs_applets[0]);

int usage(char **argv)
{
	int i;
	fprintf(stderr, "Usage:\n");
	
	for (i = 0;i < num_applets; i++)
	{
		fprintf(stderr, "  %s <drive letter> %s %s %s\n", argv[0], wbfs_applets[i].command, wbfs_applets[i].function_with_argument ? "<file | id>" : "",
			wbfs_applets[i].function_with_twoarguments ? "file.png" : "");
	}

system("pause");
	return EXIT_FAILURE;
}

int Ask_Yes_no()
{
 char c;

	while(1) {if(!kbhit()) break; getch();}

while(1)
	{
	c = getchar();
	if(c==10 || c==13) continue;

	if (toupper(c) != 'Y')
		{
		printf("Aborted.\n");
		return 0;
		}
	else break;
	}

 return 1;
}

int main(int argc, char *argv[])
{
	int opt;
	int i;
	BOOL executed = FALSE;
	char *partition = NULL;
	char *command = NULL;

	static char device_letter[2]="Z";
	char current_directory[1024];

	current_directory[0]=0;
	getcwd(current_directory,1024);
	
	if(argc>1)
		fprintf(stderr, "wbfs windows port build 'delta'\nModified by Hermes\n\n");
	else
	{
	wbfs_t *p;
			
	while(1) {if(!kbhit()) break; getch();}

    while(1)
	{
		while(1)
		{
		int k;
		system("cls");
		printf("wbfs windows port build 'delta'\nModified by Hermes\n\n");
		printf("Press Device letter:\n\n");

		device_letter[0]=getch();
		device_letter[1]=0;

		if(device_letter[0]==27) return 0;
	   
		if((device_letter[0]>='A' && device_letter[0]<='Z') || (device_letter[0]>='a' && device_letter[0]<='z')) {device_letter[0]&=~32;break;}

		}

  
	p = wbfs_try_open(NULL, device_letter, 0);

	if(p) break; 
	else 
		{
			
	    printf("\n\nFormat as WBFS? (y/n)\n\n");
        if(Ask_Yes_no())
			{
			system("cls");
			printf("!!! Warning ALL data on drive '%s' will be lost irretrievably !!!\n\n", device_letter);
		    printf("Are you sure? (y/n): ");
			
            if(Ask_Yes_no())
				{
				p = wbfs_try_open(NULL, device_letter, 1);
				if(p) {wbfs_close(p);p = wbfs_try_open(NULL, device_letter, 0);}
				if(p) break; 
				}
			}
		printf("\nPress Any Key\n");
		while(1) {if(!kbhit()) break; getch();}
		getch();

		}
	}

	while(1)
	{
	int k;
	system("cls");
	printf("wbfs windows port build 'delta'\nModified by Hermes\n\n");

	printf("Device: %c\n\n",device_letter[0]);

    
	wbfs_display_list(p);

	printf("\nUse ARROWS to Select and press key:\n\n");
    
	printf(" 1-> Add 2-> Add PNG 3-> Extract 4-> ISO Extract 5-> Remove\n 6->Remove CFG 8->Integrity Check 0-> Format\n");

	k=getch();

	if(k==224)
		{
		k=getch();
		if(k==80 && pos_list>=0) pos_list++;
		if(k==72 && pos_list>=0) {pos_list--;if(pos_list<0) pos_list=0;}
		}
	/*if(k=='7')
		{
		wbfs_applet_addcfg(p);
		}*/
    // add game
	if(k=='1') 
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
			wbfs_applet_add(p, szFileName);

			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();
			}
		}

	// add .png
	if(k=='2' && pos_list>=0 && string_id[0]!=0) 
		{
		OPENFILENAME ofn;
		char szFileName[MAX_PATH] = "";

		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn); 
		ofn.hwndOwner =NULL;
		ofn.lpstrFilter = "PNG Files (*.png)\0*.png\0\0\0";
		ofn.lpstrFile = szFileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE ;
		ofn.lpstrDefExt = "png";
		
		if(GetOpenFileName(&ofn)) 
			{
				system("cls");
				wbfs_applet_png(p, string_id, szFileName);

				printf("\nPress Any Key\n");
				while(1) {if(!kbhit()) break; getch();}
				getch();
			}
		}

	// extract .ciso
	if(k=='3' && pos_list>=0 && string_id[0]!=0)
		{
			
			system("cls");
			chdir(current_directory);
			printf("extract %s to .ciso format? (y/n)\n\n",string_id);
			if(Ask_Yes_no())
				wbfs_applet_extract(p, string_id);
			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();

		}
		
	// extract .iso
	if(k=='4' && pos_list>=0  && string_id[0]!=0)
		{
			
			system("cls");
			chdir(current_directory);
			printf("extract %s to .iso format? (y/n)\n\n",string_id);
			if(Ask_Yes_no())
				wbfs_applet_isoextract(p, string_id);
			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();

		}
		
	// remove
	if(k=='5' && pos_list>=0  && string_id[0]!=0)
		{   
			
			system("cls");
			chdir(current_directory);
			printf("!!! Warning it remove %s game !!!\n\n", string_id);
			printf("Are you sure? (y/n):\n");
            if(Ask_Yes_no())
				wbfs_applet_remove(p, string_id);

			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();

		}

	// remove game cfg
	if(k=='6' && pos_list>=0  && string_id[0]!=0)
		{   
			
			system("cls");
			chdir(current_directory);
			printf("!!! Warning it remove game CFG for %s!!!\n\n", string_id);
			printf("Are you sure? (y/n):\n");
            if(Ask_Yes_no())
				wbfs_applet_remove_cfg(p, string_id);

			printf("\nPress Any Key\n");
			while(1) {if(!kbhit()) break; getch();}
			getch();

		}

	if(k=='8' && pos_list>=0)
		{
		printf("Integrity Check:\n\n");
		wbfs_integrity_check(p);
		}

	if(k=='0')
		{
		system("cls");	
	    printf("\n\nFormat as WBFS? (y/n)\n\n");
        if(Ask_Yes_no())
			{
			system("cls");
			printf("!!! Warning ALL data on drive '%s' will be lost irretrievably !!!\n\n", device_letter);
		    printf("Are you sure? (y/n):\n");
			
            if(Ask_Yes_no())
				{
				wbfs_close(p);
				p = wbfs_try_open(NULL, device_letter, 1);
				if(p) {wbfs_close(p);p = wbfs_try_open(NULL, device_letter, 0);}
				if(!p)
					{
					printf("Exiting by ERROR\n");
					while(1) {if(!kbhit()) break; getch();}
					getch();
					return -1;
					}
				}
			}
		printf("\nPress Any Key\n");
		while(1) {if(!kbhit()) break; getch();}
		getch();

		}

	if(k==27) break;
	}
    wbfs_close(p);
	return 0;
	}

	while ((opt = getopt(argc, argv, "d:hf")) != -1)
	{
		switch (opt)
		{
			case 'f':
				wbfs_set_force_mode(1);
				break;
			
			case 'h':
			default: /* '?' */
				return usage(argv);
		}
	}
	
	if (optind + 2 > argc)
	{
		return usage(argv);
	}
	
	partition = argv[optind + 0];
	command = argv[optind + 1];
	
	if (partition == NULL || strlen(partition) != 1)
	{
		fprintf(stderr, "You must supply a valid drive letter.\n");
		return EXIT_FAILURE;
	}
	
	if (strcmp(command, "init") == 0)
	{
		char c;
		fprintf(stderr, "!!! Warning ALL data on drive '%s' will be lost irretrievably !!!\n\n", partition);
		fprintf(stderr, "Are you sure? (y/n): ");

		c = getchar();
		if (toupper(c) != 'Y')
		{
			fprintf(stderr, "Aborted.\n");
			return EXIT_FAILURE;
		}
		
		fprintf(stderr, "\n");
	}

	for (i = 0; i < num_applets; i++)
	{
		struct wbfs_applets *ap = &wbfs_applets[i];
		
		if (strcmp(command, ap->command) == 0)
		{
			wbfs_t *p;
			
			p = wbfs_try_open(NULL, partition, ap->function == wbfs_applet_init);
			if (!p)
			{
				executed = TRUE;
				break;
			}
			
			if (ap->function_with_argument)
			{
				if (optind + 3 > argc)
				{
					break;
				}
				else
				{
					executed = TRUE;
					if (strcmp(command, "png") == 0) ap->function_with_twoarguments(p, argv[optind + 2], argv[optind + 3]);
					else ap->function_with_argument(p, argv[optind + 2]);
				}
			}
			else
			{
				executed = TRUE;
				ap->function(p);
			}
			
			wbfs_close(p);
			break;
		}
	}
	
	if (executed == FALSE)
	{
		return usage(argv);
	}
	
	return EXIT_SUCCESS;
}
