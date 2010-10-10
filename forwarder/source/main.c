/* uLoader- forwarder by Hermes */

/*   
	Copyright (C) 2009 Hermes
   
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


#include <stdio.h>
#include <string.h>

#include <gccore.h>
#include <ogcsys.h>
#include <fat.h>

#include <sdcard/wiisd_io.h>

char param0[256]="sd:/apps/uloader/boot.dol";

char param1[32]="#GAMEID\0\0\0\0\0CFGUSB0000000000";

/* Note:
"#RHOP8P"  -> launch game in the first WBFS partition
"#RHOP8P-0"  -> launch game using partition 0 (first WBFS partition)
"#RHOP8P-1" -> launch game using partition 1 (second WBFS partition)
*/

char my_param[512+32]  __attribute__ ((aligned (32)));

typedef struct _dolheader {
	u32 section_pos[18];
	u32 section_start[18];
	u32 section_size[18];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;

void *dol_data=(u8 *)0x92000000;

u32 load_dol(void * dol_data) 
{
int i;
int flag=1;
dolheader *dol_header;

	dol_header = (dolheader *) dol_data;
	for (i = 0; i < 18; i++) 
		{
		if((!dol_header->section_size[i]) || (dol_header->section_start[i] < 0x100)) continue;

		if(i<7)
			{
			ICInvalidateRange ((void *) dol_header->section_start[i], dol_header->section_size[i]);
			}

		memcpy ((void *) dol_header->section_start[i], dol_data+dol_header->section_pos[i], dol_header->section_size[i]);
		if(i>=7)
			{
			DCFlushRangeNoSync ((void *) dol_header->section_start[i], dol_header->section_size[i]);
			}
		else
		if(flag)
				{
				u32 *p= (u32 *) dol_header->section_start[i];
				char *arg= (void *) my_param;
				u32 *argp;
				int n;

			
				if(p[1]==0x5f617267) // magic
					{
					flag=0;
				    
					p[2]=0x5f617267; // magic
					p[3]=(u32) arg;
					p[4]=1;
					p[5]=1;
					p[6]=(u32) (arg+512);
					p[7]=(u32) (arg+512);
					
					
					argp=(u32 *) p[6];

					n=strlen(param0);
					memcpy(arg, param0, n+1);
					p[4]+=n+1;
					
					argp[0]=(u32) &arg[n];
					argp++;
					p[7]+=4;
					arg=&arg[n+1];

				    // add argv[1]
					if(param1[0]=='#' 
						&& (param1[1]!='G' || param1[2]!='A' || param1[3]!='M' || param1[4]!='E' || param1[5]!='I' || param1[6]!='D'))
						{
						p[5]+=1;
						n=strlen(param1);
						memcpy(arg, param1, n+1);
						p[4]+=n+1;

						argp[0]=(u32)  &arg[n];
						argp++;
						p[7]+=4;
						arg=&arg[n+1];
						}

					arg[0]=0;arg[1]=0;
				
					DCFlushRange ((void *) &my_param,sizeof(my_param));
					DCFlushRange ((void *) dol_header->section_start[i], dol_header->section_size[i]);
					}
				}
		}


	memset ((void *) dol_header->bss_start, 0, dol_header->bss_size);
	DCFlushRange((void *) dol_header->bss_start, dol_header->bss_size);

return dol_header->entry_point;

}

GXRModeObj *rmode;		// Graphics Mode Object
u32 *xfb = NULL;		// Framebuffer  

int main()
{
u32 level=0;
int sd_ok,ret=0;
FILE *fp=NULL;

int dol_len=0;

void (*entry)()=NULL;

	ret=IOS_ReloadIOS(222);
		
	if(ret!=0)
		{
		ret=IOS_ReloadIOS(249);
		}
	
	if(ret!=0)
		{
		ret=IOS_ReloadIOS(202);
		}

	sleep(1);

	ret=0;

	__io_wiisd.startup();
	sd_ok = fatMountSimple("sd", &__io_wiisd);

	if(sd_ok)
		{
		fp=fopen(param0,"rb"); // lee el fichero de texto
		if(fp)
			{
			fseek(fp, 0, SEEK_END);
			dol_len = ftell(fp);
			
			fseek(fp, 0, SEEK_SET);
		
			ret=fread(dol_data, 1, dol_len, fp);
			fclose(fp);
			}
		
		}

	if(sd_ok)
		{
		fatUnmount("sd");
		__io_wiisd.shutdown();
		}

	if(sd_ok && fp && ret==dol_len) entry=(void(*)())load_dol(dol_data);
	else
		{
		VIDEO_Init();                                        //Inicialización del Vídeo.
										  
		rmode = VIDEO_GetPreferredMode(NULL);                //mediante esta función rmode recibe el valor de tu modo de vídeo.
		xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));  //inicialización del buffer.
		console_init(xfb,20,20,rmode->fbWidth,rmode->        //inicialización de la consola.
		xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);  
		VIDEO_Configure(rmode);                              //configuración del vídeo.
		VIDEO_SetNextFramebuffer(xfb);                       //Configura donde guardar el siguiente buffer .
		VIDEO_SetBlack(FALSE);                               //Hace visible el display  .                     
		VIDEO_Flush();
		VIDEO_WaitVSync();                                   
		if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
		
		if(!sd_ok)
			printf("\n\n\n  Fail mounting the SD\n");
		else
		if(!fp)
			printf("\n\n\n  Cannot open: %s\n", param0);
		else
		if(ret!=dol_len)
			printf("\n\n\n  Fail reading from the SD\n");

		sleep(5);
		goto out;
		}


SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

	__IOS_ShutdownSubsystems ();
	level=IRQ_Disable();
	__exception_closeall();
	

	if(entry) entry();
	IRQ_Restore(level);
	
out:	
	WII_Initialize();
	WII_ReturnToMenu();

return 0;
}

