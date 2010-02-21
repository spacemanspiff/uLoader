/* uLoader- Project based in YAL and usbloader by Hermes */

/*   
	Copyright (C) 2009 Hermes
    Copyright (C) 2009 Kwiirk (YAL)
	Copyright (C) 2009 Waninkoko (usbloader)
	
    Yet Another Loader.  The barely minimum usb loader
    
    Based on SoftChip, which should be based on GeckoOS...

    no video, no input, try to load the wbfs passed as argument or return to menu.
    
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

// ALTERNATIVE_VERSION

#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include "defs.h"
#include "wdvd.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>

#include "libpng/pngu/pngu.h"
#include "libwbfs/libwbfs.h"
#include "libwbfs/wiiscrubber.h"
#include "wbfs.h"
#include "usbstorage2.h"

#include <wiiuse/wpad.h>
#include <sdcard/wiisd_io.h>


#include "wdvd.h"
#include "disc.h"

#include <network.h>
#include "remote.h"

#include "global.h"
#include "cheats.h"

#include "sys/dir.h"
#include "fatffs_util.h"
#include "diario.h"
#include "sonido.h"


char uloader_version[5]="4.5A"; // yes, you can change here the current version of the application

extern u8 launch_short;

int global_mount=0;
u32 nand_mode=0;

int frames2=0;

int use_port1=0;

extern float angle_icon; // angle for pointer

int use_icon2=0;

#define MEM_PRINT

int test_mode=0;

int is_fat=0; // working in FAT .ciso Mode
int dvd_only=0; // no WBFS HDD (FAT .ciso, DVD USB, DVD)

#define CIOS 222

#include <stdarg.h>
#include <stdio.h>

#include "screen.h"
#include "fat.h"
#include <math.h>


#include "gfx.h"

#ifndef ALTERNATIVE_VERSION

#include "resources/defpng.h"
#include "resources/button1.h"
#include "resources/button2.h"
#include "resources/button3.h"

#include "resources/icon.h"

#include "resources/sound.h"

#else

#include "resources_alt/defpng.h"
#include "resources_alt/button1.h"
#include "resources_alt/button2.h"
#include "resources_alt/button3.h"

#include "resources_alt/icon.h"

#include "resources_alt/sound.h"

#endif

#include "files.h"

#include "mload_modules.h"

extern int home_menu(struct discHdr *header);

extern unsigned hiscore;
extern int pintor();

static syswd_t scr_poweroff;

int time_sleep=0;


static void scr_poweroff_handler(syswd_t alarm, void * arg)
{	
	if(time_sleep>1) time_sleep--;
	if(time_sleep==1) SetVideoSleep(1);
	
}

#define ticks_to_msecs(ticks)      ((u32)((ticks)/(TB_TIMER_CLOCK)))

u32 gettick();


int is_16_9=0;

//#include "logmodule.h"

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

u8 BCA_Data[64] ATTRIBUTE_ALIGN(32);
int BCA_ret=-1;

int show_bca=0;


int force_reload_ios222=0;
int forcevideo=0;

int sd_ok=0;
int ud_ok=0;


#define USE_MODPLAYER 1

#ifdef USE_MODPLAYER

#include "asndlib.h"

#ifdef ALTERNATIVE_VERSION

#include "oggplayer.h"
#include "resources_alt/bg_music.h"

#else

// MOD Player
#include "gcmodplay.h"

MODPlay mod_track;

#include "resources/lotus3_2.h" 
#endif

#endif

// parental control flag
int parental_control_on=1;

/*LANGUAGE PATCH - FISHEARS*/
u32 langsel = 0;
char languages[11][22] =
{
{" Default  "},
{" Japanese "},
{" English  "},
{"  German  "},
{"  French  "},
{" Spanish  "},
{" Italian  "},
{"  Dutch   "},
{"S. Chinese"},
{"T. Chinese"},
{"  Korean  "}
};

u32 hook_selected=1;

char hook_type_name[8][16]={
	"     VBI     ",
	"    KPAD     ",
	"   Joypad    ",
	"   GXDraw    ",
	"   GXFlush   ",
	"OSSleepThread",
	" AXNextFrame ",
	"   VBI OLD    "
};


//---------------------------------------------------------------------------------
/* Global definitions */
//---------------------------------------------------------------------------------

int force_ios249=0;

unsigned char *buff_cheats=NULL;
int len_cheats=0;

int load_cheats(u8 *discid);

int load_disc(u8 *discid);

// to read the game conf datas
u8 *temp_data= NULL;

// texture of white-noise animation generated here
u32 *game_empty= NULL;

s8 sound_effects[2][2048] ATTRIBUTE_ALIGN(32);

//---------------------------------------------------------------------------------
/* language GUI */
//---------------------------------------------------------------------------------

int idioma=0;


char letrero[2][70][64]=
	{
	{"Return", "Configure", "Delete Favorite", "Add Favorite", "Load Game", "Add to Favorites", "Favorites", "Page", "Ok" ,"Discard",
	" Cheats Codes found !!! ", "Apply Codes ", "Skip Codes ", "Format WBFS", "Selected", "You have one WBFS partition", "Are You Sure You Can Format?",
	" Yes ", " No ", "Formatting...","Formatting Disc Successfull","Formatting Disc Failed",
//22 
	"Return", "Add New Game", "Add PNG Bitmap", "Delete PNG Bitmap",  "Fix Parental Control","Return to Wii Menu", "Rename Game", "Delete Game", "Format Disc", 
	"Alternative .dol","uLoader Update","uLoader Hacks","","",
//36
	"Are you sure you want delete this?", "Press A to accept or B to cancel", 
// 38
"Insert the game DVD disc...", "ERROR! Aborted", "Press B to Abort","Opening DVD disc...", "ERROR: Not a Wii disc!!",
"ERROR: Game already installed!!!", "Installing game, please wait... ", "Done", "Change the password", "Use this password?","Restore Name",
//49
"Delete Alternative .dol", ".dol Search", "Searching for .dol...","Alternative .dol Selected","Alternative .dol Deleted", "Edit CFG #1", "Edit CFG #2",
// 56 
"ehcmodule - Use USB Port 1", "ehcmodule - Use Bulk Reset","ehcmodule - Force GetMaxLun", "ehcmodule - Force SetConfiguration", "ehcmodule - Alternative Timeout", 
"No More Snow, Please!!!", "Automatic DVD/SD Mode", "Short Direct Launch (from Channel)", "???", "",
	},
    // spanish
	{"Retorna", "Configurar", "Borra Favorito", "Añade Favorito", "Carga juego", "Añade a Favoritos", "Favoritos", "Página", "Hecho", "Descartar",
	" Códigos de Trucos encontrados !!! ","Usa Códigos", "Salta Códigos", "Formatear WBFS", "Seleccionado", "Ya tienes una partición WBFS", 
	"¿Estas seguro que quieres formatear?", " Si ", " No ", "Formateando...", "Exito Formateando el Disco", "Error al Formatear el Disco",
//22	
	"Retornar", "Añadir Nuevo Juego", "Añadir Bitmap PNG", "Borrar Bitmap PNG", "Fijar Control Parental", "Retorna al Menu de Wii", "Renombrar Juego", "Borrar Juego", "Formatear Disco",
	".dol Alternativo","Actualización de uLoader","uLoader Hacks","","",

//36
	"¿Estás seguro de que quieres borrar éste?", "Pulsa A para aceptar o B para cancelar",
// 38
"Inserta el DVD del juego ...", "ERROR! Abortado", "Pulsa B para Abortar","Abriendo el disco DVD...", "ERROR: No es un disco de Wii!!",
"ERROR: Juego ya instalado!!!", "Instalando el juego, espera... ", "Terminado", "Cambia la contraseña", "¿Usar esta contraseña?", "Restaurar Nombre",
//49
"Borrar .dol Alternativo", "Buscar .dol", "Buscando ficheros .dol...","Alternativo .dol Seleccionado","Alternativo .dol Borrado", "Editar CFG #1", "Editar CFG #2",
// 56 
"ehcmodule - Usa Puerto USB 1", "ehcmodule - Usa Reset Bulk", "ehcmodule - Fuerza GetMaxLun", "ehcmodule - Fuerza SetConfiguration", "ehcmodule - Timeout Alternativo",
"No Mas Nieve, Por Favor!!!", "Modo DVD/SD Automático", "Lanzamiento Directo Corto (Desde Canal)", "???", "",
	},
	};


_config_file config_file ATTRIBUTE_ALIGN(32);
																     
//---------------------------------------------------------------------------------
/* Reset and Power Off */
//---------------------------------------------------------------------------------

int exit_by_reset=0;

int return_reset=2;

void reset_call() {exit_by_reset=return_reset;}
void power_call() {exit_by_reset=3;}


int cios = CIOS;

//---------------------------------------------------------------------------------
/* from Waninkoko usbloader */
//---------------------------------------------------------------------------------


/* Gamelist variables */
struct discHdr *gameList = NULL;

u32 gameCnt = 0;


s32 __Menu_EntryCmp(const void *a, const void *b)
{
	struct discHdr *hdr1 = (struct discHdr *)a;
	struct discHdr *hdr2 = (struct discHdr *)b;

	/* Compare strings */
	return strcmp(hdr1->title, hdr2->title);
}
//---------------------------------------------------------------------------------
/* Wiimote's routines */
//---------------------------------------------------------------------------------

unsigned temp_pad=0,new_pad=0,old_pad=0;

WPADData * wmote_datas=NULL;

// wiimote position (temp)
int px=-100,py=-100;

// wiimote position emulated for Guitar
int guitar_pos_x,guitar_pos_y;

static int w_index=-1;

int rumble=0;

void wiimote_rumble(int status)
{
	
	
	if(config_file.rumble_off) status=0;
	//if(status==0) rumble=0;
	 
    if(w_index<0) 
		{
        if(status==0)
			{
			WPAD_Rumble(0, status);
			WPAD_Rumble(1, status);
			WPAD_Rumble(2, status);
			WPAD_Rumble(3, status);
			}
		return;
		}
	WPAD_Rumble(w_index, status);
	
}

void make_rumble_off()
{
int n;
rumble=0;

for(n=0;n<3;n++)
	{
	usleep(30*1000);
	wiimote_rumble(0);
	WPAD_Flush(w_index);
	WPAD_ScanPads();
	}

}

unsigned wiimote_read()
{
int n;

int ret=-1;

unsigned type=0;

unsigned butt=0;

	wmote_datas=NULL;

	w_index=-1;

	for(n=0;n<4;n++) // busca el primer wiimote encendido y usa ese
		{
		ret=WPAD_Probe(n, &type);

		if(ret>=0)
			{
			
			butt=WPAD_ButtonsHeld(n);
			
				wmote_datas=WPAD_Data(n);

			w_index=n;

			break;
			}
		}

	if(n==4) butt=0;

		temp_pad=butt;

		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(new_pad)
		{
		time_sleep=TIME_SLEEP_SCR;
		SetVideoSleep(0);
		}

return butt;
}


void wiimote_ir()
{
	px=wmote_datas->ir.x-104*is_16_9;py=wmote_datas->ir.y;
	angle_icon=wmote_datas->orient.roll;

	time_sleep=TIME_SLEEP_SCR;
	SetVideoSleep(0);
	
	SetTexture(NULL);
	DrawIcon(px,py,frames2);
}

void wiimote_guitar()
{
	angle_icon=0.0f;

	if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
		{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH+104*is_16_9-16)) guitar_pos_x=(SCR_WIDTH+104*is_16_9-16);}
	if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
		{guitar_pos_x-=8;if(guitar_pos_x<16-104*is_16_9) guitar_pos_x=16-104*is_16_9;}
		

	if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
		{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
	if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
		{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

	if(guitar_pos_x<-104*is_16_9) guitar_pos_x=104*is_16_9;
	if(guitar_pos_x>(SCR_WIDTH-16+104*is_16_9)) guitar_pos_x=(SCR_WIDTH-16+104*is_16_9);
	if(guitar_pos_y<0) guitar_pos_y=0;
	if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);

	if(px!=guitar_pos_x || py!=guitar_pos_y)
		{
		time_sleep=TIME_SLEEP_SCR;
		SetVideoSleep(0);
		}

	px=guitar_pos_x; py=guitar_pos_y;


	SetTexture(NULL);
	DrawIcon(px,py,frames2);

	if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
	if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

	if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
	if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

	if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
	if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

	if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
	if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

	if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
	if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

}

//---------------------------------------------------------------------------------
/* Sound effects */
//---------------------------------------------------------------------------------

void snd_fx_tick()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 4096*6*2,0, &sound_effects[0][0], 2048/128, 64, 64, NULL);
#endif
}
void snd_fx_yes()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 12000,0, &sound_effects[1][0], 2048/4, 127, 127, NULL);
#endif
}

void snd_fx_no()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(1, VOICE_MONO_8BIT, 4096,0, &sound_effects[0][0], 2048/16, 127, 127, NULL);
#endif
}

void snd_fx_scroll()
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 1024,0, &sound_effects[0][0], 64, 64, 64, NULL);
#endif
}

void snd_fx_fx(int percent)
{
#ifdef USE_MODPLAYER
	ASND_SetVoice(2, VOICE_MONO_8BIT, 5000+percent*250,0, &sound_effects[0][0], 128, 64, 64, NULL);
#endif
}



//---------------------------------------------------------------------------------
/* GUI routines and datas*/
//---------------------------------------------------------------------------------



struct _game_datas
{
	int ind;
	void * png_bmp;
	GXTexObj texture;
	u32 config;
} game_datas[32];



void create_game_png_texture(int n)
{
	

	PNGUPROP imgProp;
	IMGCTX ctx;
	char *texture_buff;

	s32 ret;

	if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
		{game_datas[n].png_bmp=NULL;game_datas[n].config=0;
		 return;
		 }

    game_datas[n].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);

	if(!(temp_data[9]=='P' && temp_data[10]=='N' && temp_data[11]=='G'))
		{game_datas[n].png_bmp=NULL;return;}

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(temp_data+8);
	if (!ctx)
		{game_datas[n].png_bmp=NULL;return;}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK)
		{game_datas[n].png_bmp=NULL;return;}

    texture_buff=memalign(32, imgProp.imgWidth * imgProp.imgHeight *4+2048);
	if(!texture_buff) {game_datas[n].png_bmp=NULL;return;}

	
	PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, texture_buff, 255);
	PNGU_ReleaseImageContext(ctx);

	game_datas[n].png_bmp=texture_buff;
	
    DCFlushRange( texture_buff, imgProp.imgWidth * imgProp.imgHeight *4);

	GX_InitTexObj(&game_datas[n].texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_CLAMP ,  GX_CLAMP , GX_FALSE);
	GX_InitTexObjLOD(&game_datas[n].texture, // objeto de textura
						 GX_LINEAR, // filtro Linear para cerca
						 GX_LINEAR, // filtro Linear para lejos
						 0, 0, 0, 0, 0, GX_ANISO_1);
		
}



void usb_test()
{
static int n_sectors=0,errores=0;
static u32 sector=0, sector_size;

static u32 err_time=0,ok_time=0;
static u32 first_time=0, curr_time=0,start_time=0;
static int err=2;
static u32 addr=0,dat,dat2=0,dat3=0,bytes_readed=0,dat4=0, flag=0;



if(err==2) 
	{
	err=0;
	start_time=first_time=time(NULL);
	}
	

if(addr==0)
	{
			// get EHCI base registers
	mload_getw((void *) 0x0D040000, &addr);

	addr&=0xff;
	addr+=0x0D040000;
	}
			

    if(n_sectors==0)
			n_sectors = USBStorage2_GetCapacity(&sector_size);

	dat=0;
	
mload_getw((void *) (addr+0x44), &dat);
if(dat!=0x1005) dat2=dat;
if(dat2!=0x1805) dat3=dat2;

	PY+=32;
	s_printf("port: %x %x %x\n", dat,dat2,dat3);
	PY-=32;

	if(USBStorage2_ReadSectors(sector, 64*512/sector_size, temp_data)<=0) 
		{sprintf(cabecera2_str,"ERROR! %i", errores);
		errores++;err_time=gettick();if(err==0) curr_time= time(NULL)-first_time;err=1;
		
		} 
	else 
		{
		if(err) 
			{first_time=time(NULL);ok_time=gettick();err=0;}
		sprintf(cabecera2_str,"OK %i %u Last T: %u Time %u", errores, ticks_to_msecs(ok_time-err_time), curr_time, (u32) (time(NULL)-start_time)); 
		bytes_readed+=64*512;
		}

	switch((((u32) (time(NULL)-start_time)) % 10))
		{
		case 0:
			if(flag==0) 
				{dat4=bytes_readed/(10);bytes_readed=0;flag=1;}
			break;
		default:
			flag=0;
			break;
		}

	PY=480-64;
	s_printf("sector: %u / %u\n", sector, n_sectors);

	PY=480-32;
	s_printf("speed: %i bytes/sec\n", dat4);

	
	sector+=64*512/sector_size;
	if(sector>(n_sectors-(64*512/sector_size))) sector=0;
	
	if(err) signal_draw_cabecera2=1;
	draw_cabecera2();
	signal_draw_cabecera2=0;
}


int load_game_routine(u8 *discid, int game_mode);

int load_file_dol(char *name);


#ifdef MEM_PRINT
void save_log()
{
FILE *fp;
int len;
	
	mload_init();
	len=mload_get_log();
//	mload_seek(0x13750000, SEEK_SET);
	if(len>0)
		mload_read(temp_data, len);
	mload_close();

	if(len<=0) return;

	if(sd_ok )
		{
		for(len=0;len<4096;len++)
			{
			if(temp_data[len]==0) break;
			}

		fp=fopen("sd:/log_ehc.txt","wb");

		if(fp!=0)
			{
			fwrite(temp_data,1, len ,fp);
				
			fclose(fp);
			}
		}
}
#endif

int remote_DVD_disc_status=0;
static struct discHdr mydisc_header ATTRIBUTE_ALIGN(32);

int mode_disc=0; // bits 0-1 : 0 -> HDD, 1-> DVD Wii, 2-> USB DVD

int bca_status_read=0;

struct _multi_ciso multi_ciso[8];

int ciso_sel=0;

static u8 disc_buffer[65536] ATTRIBUTE_ALIGN(32);


void remote_DVD_disc()
{
int r;
int ciso=0;

if(remote_DVD_disc_status==3)
	{
	if(mode_disc & 2)
		WDVD_SetUSBMode((u8 *) "_DVD", multi_ciso[ciso_sel].lba);
	else
		WDVD_SetUSBMode(NULL, multi_ciso[ciso_sel].lba);

	bca_status_read=0;BCA_ret=-1;

	remote_DVD_disc_status=1;

	WDVD_Reset();

	r=Disc_Open();
	if(r>=0) r=Disc_IsWii();
	if(r>=0)
		{
		Disc_ReadHeader(&mydisc_header);
		memset(BCA_Data,0,64);
		bca_status_read=0;BCA_ret=-1;
		show_bca=0;
		remote_DVD_disc_status=2;
		}
	}

	
	/*if(mode_disc & 2) WDVD_SetUSBMode((u8 *) "_DVD", 0);
			else
				WDVD_SetUSBMode(NULL, 0);*/

	if(mode_disc & 2) r=Disc_USB_DVD_Wait();
		else r=Disc_Wait();

	if(r==0)
		{
		
		if(remote_DVD_disc_status==0)
			{
			remote_DVD_disc_status=1;

			if(mode_disc & 2) WDVD_SetUSBMode((u8 *) "_DVD", 0);
			else
				WDVD_SetUSBMode(NULL, 0);

			WDVD_Reset();
			
			r=Disc_Open();

			if(r>=0) 
				{
				r=Disc_IsWii();
				ciso_sel=0;multi_ciso[0].len=multi_ciso[0].lba=0;

				// TEST for CISO Disc
				
				if(r==-1 && mode_disc && !(mode_disc & 2))
					{
					r=WDVD_UnencryptedRead(disc_buffer, 2048, 0);
					if(r>=0 && !memcmp((void *) disc_buffer, "CISO",4))
						{
						int m;
						ciso_sel=0;
						for(m=0;m<8;m++)
							{
							multi_ciso[m].len=0;
							multi_ciso[m].lba=(m==0); // add one for DVD Wii
							}
						
						WDVD_SetUSBMode(NULL, multi_ciso[ciso_sel].lba);

						ciso=1;

						r=Disc_Open();
						if(r>=0) r=Disc_IsWii();
						}
					else r=-1;
					}

				// TEST for UDF Disc
				if(r==-1 && (mode_disc /*& 2*/))
					{
					if(mode_disc & 2)
						r = USBStorage2_ReadSectors(16, 1, disc_buffer);
					else
						r=WDVD_UnencryptedRead(disc_buffer, 2048, 16<<11);

					if(r>=0 && !memcmp((void *) disc_buffer, "\0BEA01\1\0",8))
						{
						ciso=1;

						if(mode_disc & 2)
							r = USBStorage2_ReadSectors(256, 32, disc_buffer);
						else
							r=WDVD_UnencryptedRead(disc_buffer, 2048*32, 256<<11);

						if(r>=0 && !memcmp(disc_buffer, "\2\0\2\0",4))
							{
                            int n,m,p, last_lba=0, curr_lba;;
							for(n=1;n<16;n++) if(!memcmp((void *) &disc_buffer[2048*n], "\1\1\2\0",4)) break;
							if(n==16) r=-1;
							else
								{
								n*=2048;
								p=0;

								for(m=0;m<8;m++)
									{
                                    if(!memcmp((void *) &disc_buffer[n+p], "\1\1\2\0",4))
										{
										u8 *punt=&disc_buffer[n+p+0x18];
										
										curr_lba=(punt[0]+(punt[1]<<8)+(punt[2]<<16)+(punt[3]<<24))+1;

										if(last_lba<curr_lba) last_lba=curr_lba;
										
										if(!disc_buffer[n+p+0x13] || disc_buffer[n+p+0x12]==0xa) m--;
										else 
										if(disc_buffer[n+p+0x12] & 0x2)
											{
											// sorry don´t support directories (files must be in root)
											break;
											}

										else 
										if(curr_lba>32)
											{
											// sorry LBA is out of the buffer
											break;
											}
										else
											{
											punt=&disc_buffer[curr_lba*2048+0x40];
											multi_ciso[m].len=(punt[0]+(punt[1]<<8)+(punt[2]<<16)+(punt[3]<<24));
											
											memset((void *)  multi_ciso[m].name,0,64);
							
											if(disc_buffer[n+p+0x26]==16) 
												{
												int s;
												for(s=0;s<63 && s< (disc_buffer[n+p+0x13]/2);s++) multi_ciso[m].name[s]=disc_buffer[n+p+0x28+s*2];
												}
											else
												{
												int s;
												for(s=0;s<63 && s< (disc_buffer[n+p+0x13]-1);s++) multi_ciso[m].name[s]=disc_buffer[n+p+0x27+s];
												}

											multi_ciso[m].name[63]=0;
                                            
											// SKIP if not .ciso file
											if(!(strstr((void *) multi_ciso[m].name, ".ciso") || strstr((void *) multi_ciso[m].name, ".CISO")))
												{
												multi_ciso[m].len=0;
												m--;
												}

											}

										
										p+=(0x26+disc_buffer[n+p+0x13]+3) & ~3;
										}
									else multi_ciso[n].len=0;
									
									}

								if(m<8) // bad UDF format
								  r=-1;
								else
									{
								
									last_lba+=257;

									for(m=0;m<8;m++)
										{
										if(!multi_ciso[m].len) break;
										multi_ciso[m].lba=last_lba+1*(!(mode_disc & 2)); // add one for DVD Wii
										last_lba+=multi_ciso[m].len;
										}
									ciso_sel=0;
									//exit(0);
									if(mode_disc & 2)
										WDVD_SetUSBMode((u8 *) "_DVD", multi_ciso[ciso_sel].lba);
									else
										WDVD_SetUSBMode(NULL, multi_ciso[ciso_sel].lba);

									WDVD_Reset();

									r=Disc_Open();
									if(r>=0) r=Disc_IsWii();
									}
								}
							}
						 else r=-1;
						}
					else r=-1;
					}
				// end  of TEST for UDF Disc
				if(r>=0) 
					{
					Disc_ReadHeader(&mydisc_header);
					memset(BCA_Data,0,64);
					if(ciso) bca_status_read=0; else bca_status_read=1;BCA_ret=-1;
					show_bca=0;
					remote_DVD_disc_status=2;
					}
				else {bca_status_read=0;remote_DVD_disc_status=r;BCA_ret=-1;ciso_sel=0;multi_ciso[0].len=multi_ciso[0].lba=0;}
				}
			else {remote_DVD_disc_status=r;BCA_ret=-1;bca_status_read=0;ciso_sel=0;multi_ciso[0].len=multi_ciso[0].lba=0;}
			}
		}
	else {remote_DVD_disc_status=0;bca_status_read=0;ciso_sel=0;multi_ciso[0].len=multi_ciso[0].lba=0;}

	if(bca_status_read==2)
		{
		memset(BCA_Data,0,64);
		BCA_ret=WDVD_Read_Disc_BCA(BCA_Data);
		if(BCA_ret<0) memset(BCA_Data,0,64);
		bca_status_read=3;
		}


	usleep(50*1024);
}


void update_bca(u8 *id, u8 *bca_datas);

int bca_mode=0;

int get_bca(u8 *id, u8 *bca_datas);

/////////////////////////////////////////////////

// main() variables
u8 discid[7];
int direct_launch=0;

int temp_game_cnt;
struct discHdr *temp_buffer = NULL;
/////////////////////////////////////////////////


int menu_main();
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
int n;
int ret,ret2;
struct timespec tb;
int rel_time;

       
	if (*(u32*)0x80001800)  return_reset=1;
	else return_reset=2;
	/*
	if(argc<1) return_reset=2;

	else 
		if(*((char *) argv[0])=='s') return_reset=2;
	*/

	current_partition=0;

	if(argc>1)
		if(argv[1])
			{
			char *t=argv[1];
			if(t[0]=='#')
				{
				t++;
				direct_launch=1;
				memcpy(discid,t,6);
				t+=6;
				if(t[0]=='-') current_partition=t[1]-48;
				current_partition&=3;
				}
			}


	SYS_SetResetCallback(reset_call); // esto es para que puedas salir al pulsar boton de RESET
	SYS_SetPowerCallback(power_call); // esto para apagar con power
	LWP_SetThreadPriority(LWP_GetSelf(),40);

	SYS_CreateAlarm(&scr_poweroff);
	tb.tv_sec = 1;tb.tv_nsec = 0;
	SYS_SetPeriodicAlarm(scr_poweroff, &tb, &tb, scr_poweroff_handler, NULL);

	discid[6]=0;

	ret2=-1;
   
	ret=IOS_ReloadIOS(cios);
	if(ret!=0)
		{
		cios++;
		ret=IOS_ReloadIOS(cios);
		}

	if(ret==0)
		{
		sleep(1);
		if((*(volatile u32 *)0x80003140 & 0xffff)<4) ret=-7777;
		}
	else
		{
		#ifndef DONT_USE_IOS249
		force_ios249=1;
		cios=249;
		ret=IOS_ReloadIOS(cios);
		#else
		ret=-1;
		#endif
		sleep(1);
		}

	if(CONF_Init()==0)
	{
		is_16_9=CONF_GetAspectRatio()!=0;

		switch(CONF_GetLanguage())
		{
		case CONF_LANG_SPANISH:
					idioma=1;break;
		default:
					idioma=0;break;
		}
		
	}

	InitScreen();  // Inicialización del Video
	time_sleep=TIME_SLEEP_SCR;

	remote_init();

	
	time_t  my_time=(time(NULL));
	struct tm *l_time=localtime(&my_time);

	if(l_time)
		{
		l_time->tm_mon++;
		if(!no_more_snow)
			if((l_time->tm_mday>=12 && l_time->tm_mon==12) || l_time->tm_mon<3 || (l_time->tm_mday<=6 && l_time->tm_mon==3)) flag_snow=1;
		}
	

	//create_png_texture(&text_background, background, 1);
	if(1) // new background
	{
	u32 *t=memalign(32,128*128*4*3);

	create_background_text(670, 128, 128, t);
	CreateTexture(&text_background[0], TILE_SRGBA8 , t, 128, 128, 1);
	create_background_text(663, 128, 128, t+128*128*1);
	CreateTexture(&text_background[1], TILE_SRGBA8 , t+128*128*1, 128, 128, 1);
	create_background_text(663*2, 128, 128, t+128*128*2);
	CreateTexture(&text_background[2], TILE_SRGBA8 , t+128*128*2, 128, 128, 1);
	

	GX_InitTexObj(&text_background2, t, 128, 128, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);	

	GX_InitTexObjLOD(&text_background2, // objeto de textura
					 GX_LINEAR, // filtro Linear para cerca
					 GX_LINEAR, // filtro Linear para lejos
					 0, 0, 0, 0, 0, GX_ANISO_1);
	}
	
	bkcolor=0;
	remote_call(splash_scr_send);

	WPAD_Init();
	WPAD_SetIdleTimeout(60*5); // 5 minutes 

	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
	WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH+is_16_9*208, SCR_HEIGHT);	


	sd_ok=0;
	for(n=0;n<5;n++)
	{
	if(__io_wiisd.startup())
		{
		sd_ok = fatMountSimple("sd", &__io_wiisd);
		if(!sd_ok) __io_wiisd.shutdown();
		}
	if(sd_ok) break;
	usleep(50000);
	}
	
	
	if(sd_ok) global_mount|=64;
	

	if(ret!=0) 
		{
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		splash_scr();
		SelectFontTexture(1); // selecciona la fuente de letra extra

		letter_size(8,32);
				
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);
		DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
		DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);
		if(ret==-7777)
			s_printf("ERROR: You need cIOS222 from uLoader 3.0 v4 to work!!!\n");
		else
			#ifndef DONT_USE_IOS249
			s_printf("ERROR: You need cIOS222 and/or cIOS249 to work!!!\n");
			#else
			s_printf("ERROR: You need cIOS222 and/or cIOS223 to work!!!\n");
			#endif
		Screen_flip();
		sleep(2);
		goto error;
		}

	temp_data=memalign(32,256*1024);
	// texture of white-noise animation generated here
	game_empty=memalign(32,128*64*3*4);


	test_and_patch_for_port1();
	
	load_ehc_module();

	if(external_ehcmodule)
		{remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<20;n++)
			{
			
			splash_scr();
			SelectFontTexture(1); // selecciona la fuente de letra extra

			letter_size(8,32);
					
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa000ff00);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);
			
			s_printf("External ehcmodule.elf loaded\n");
			Screen_flip();
			}
		
		
		}

	if(exit_by_reset) {test_mode=1; SYS_SetResetCallback(reset_call);exit_by_reset=0;}
 

	

        // sound pattern generator
		for(n=0;n<2;n++)
		{
		int m,l;
		switch(n)
			{
			case 0:
				l=64;
				for(m=0;m<2048;m++)
				 {
			     sound_effects[0][m]=((m) & l) ? 127 : -128;
				 if((m & 255)==0) l>>=1; if(l<16) l=16;
				 }
			break;

			case 1:
				l=127;
				for(m=0;m<2048;m++)
				 {
			     sound_effects[1][m]=((m) & 8) ? l : -l;
				 if((m & 7)==0) l--; if(l<0) l=0;
				 }
			break;

			}
		}
		
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		ret2=REMOTE_BUSY;

		num_fat_games=0;
		
		if(sd_ok)
			{
			down_frame=0;
			down_mess="Reading .CISO From SD";
			remote_call(wait_splash_scr);

			list_fat("sd:/ciso/");

			if(sd_ok)
				{
				load_cfg(1);
				}
	
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			}

		int ret_flag=0;
		
		rel_time= time(NULL);

		if(automatic_dvd_mode)
			{
			Disc_Init();
			}

		for(n=0;n<10;n++)
		{
		int r,mod=0;
		while(1)
			{
			WPAD_ScanPads();

			splash_scr();
			SelectFontTexture(1); // selecciona la fuente de letra extra

			letter_size(8,32);
					
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
				
			if(mod==0) 
			    {
				ret_flag=0;
				r=remote_USBStorage2_Init();
				if(r!=REMOTE_BUSY) mod=1;
				
				}
			if(mod==1)
				{
				
				r=remote_ret();
				if(r!=REMOTE_BUSY) {mod=0;ret2=r;ret_flag=1;}

				}
			
            if(ret2==REMOTE_BUSY) ;
			else
			if(ret2==-20001 || ret2==-20002) 
				{n=0;
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);
				
				if(ret2==-20001)
					s_printf("ERROR: USB Device Sector Size must be 512 bytes");
				else
					s_printf("ERROR: DVD Device Sector Size must be 2048 bytes");

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
				
				PY+=80;
				
				exit_by_reset=return_reset; 
				

				}
			else
			if(ret2==-20000) 
				{n=0;
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("ERROR: USB Device is detected as HUB!!!");

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
				
				PY+=80;

				if(!use_port1)
					s_printf("You need plug one device on USB port 0...");
				else
					s_printf("You need plug one device on USB port 1...");

				}
			else
			if(ret2==-100) 
				{
				static int conta=0;
				n=0;
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("ERROR: USB Device Disconnected (try unplug/plug)");

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);

				if(automatic_dvd_mode && (time(NULL)-rel_time)>5)
					{
					
					if(num_fat_games!=0) {dvd_only=1;mode_disc=0;is_fat=1;ret2=0;}
						else
					if(Disc_Wait()==0) 
						{
						dvd_only=1;
						mode_disc=1;
						ret2=0;
						}
					}
				
				PY+=80;
				if(!(conta & 64))
					{
					if(!use_port1)
						s_printf("Maybe you need plug the device on USB port 0...");
					else
						s_printf("Maybe you need plug the device on USB port 1...");
					}
				else
					s_printf("Press < 2 > for DVD Mode");

				conta++;

				if((new_pad & WPAD_BUTTON_2) && ret2!=0) 
					{
					dvd_only=1;
					if(num_fat_games==0) mode_disc=1; else {mode_disc=0;is_fat=1;}
					ret2=0;
					}

				//if(mode_disc) ret2=0; // mode disc

				}
			else
			if(ret2==-101) 
				{n=0;
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("%s","ERROR: USB Device don´t work as USB 2.0 (try unplug/plug)");

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
				
				PY+=80;

				if(!use_port1)
					s_printf("Maybe you need plug the device on USB port 0...");
				else
					s_printf("Maybe you need plug the device on USB port 1...");
				
				}
			else
			if(ret2==-122) 
				{
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("ERROR: Can't Mount Device");

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
				
				PY+=80;
				s_printf("... waiting for Storage or DVD media ...");

				}
			else
			if(ret2<0) 
				{
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("ERROR: Could not initialize USB subsystem! (ret = %d)", ret2);

				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
				
				PY+=80;
				s_printf("Retries: %i",n);

				
				}
			
			if((exit_by_reset || (new_pad & WPAD_BUTTON_HOME))) 
				{
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 0, 0xff0000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 0, 4, 0xa0000000);
				
				letter_size(16,32);PY= SCR_HEIGHT/2+32+80;
				s_printf("Exiting...");
				Screen_flip();
				ret2=-1;n=100;
			    break;
				}

			Screen_flip();
			if(ret2<0 && ret2!=-122 && ret2!=-100 && ret2!=-101 && mod<2 && ret2!=REMOTE_BUSY && ret_flag) {ret_flag=0;mod=2;}
			
			
			if(!ret2 || ret2==1) break;
			

			wiimote_read();

			if(mod>1)
				{
				mod++;if(mod>30) {mod=0;break;}
				}
			
			}
			
			//if(ret_flag==0) n--;
			if(!ret2  || ret2==1) break;
	
		}

		if(ret2==1)
			{
			ret2=0;
			dvd_only=1;
			mode_disc=1024+2; // USB DVD MODE
			if(num_fat_games!=0) {is_fat=1;}
			}

		CreatePalette(&palette_icon, TLUT_RGB5A3, 0, icon_palette, icon_palette_colors); // crea paleta 0
		CreateTexture(&text_icon[0], TILE_CI8, icon_sprite_1, icon_sprite_1_sx, icon_sprite_1_sy, 0);
		CreateTexture(&text_icon[1], TILE_CI8, icon_sprite_2, icon_sprite_2_sx, icon_sprite_2_sy, 0);
		CreateTexture(&text_icon[2], TILE_CI8, icon_sprite_3, icon_sprite_3_sx, icon_sprite_3_sy, 0);
		CreateTexture(&text_icon[3], TILE_CI8, icon_sprite_4, icon_sprite_4_sx, icon_sprite_4_sy, 0);
		CreateTexture(&text_icon[4], TILE_CI8, icon_sprite_5, icon_sprite_5_sx, icon_sprite_5_sy, 0);
        
		
		CreateTexture(&text_icon[5], TILE_CI8, icon_sprite_6, icon_sprite_6_sx, icon_sprite_6_sy, 0);
		CreateTexture(&text_icon[6], TILE_CI8, icon_sprite_7, icon_sprite_7_sx, icon_sprite_7_sy, 0);
		CreateTexture(&text_icon[7], TILE_CI8, icon_sprite_8, icon_sprite_8_sx, icon_sprite_8_sy, 0);
		CreateTexture(&text_icon[8], TILE_CI8, icon_sprite_9, icon_sprite_9_sx, icon_sprite_9_sy, 0);
		

		#ifdef MEM_PRINT
		
		if(test_mode && (mode_disc & 1)==0)
			{
				if(ret2>=0)
				{
				USBStorage2_TestMode(1);

				remote_call(usb_test);
				while(1)
					{
					WPAD_ScanPads();
					wiimote_read();

					if(exit_by_reset || (new_pad & WPAD_BUTTON_HOME)) break;
					usleep(1000*50);
					
					}
				}
				remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

				splash_scr();

				SelectFontTexture(1); // selecciona la fuente de letra extra

				letter_size(8,32);
						
				PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
						
				bkcolor=0;
				autocenter=1;
				SetTexture(NULL);

				s_printf("save log");
				Screen_flip();


				save_log();

				splash_scr();
				
				Screen_flip();


				if(sd_ok)
				{
				fatUnmount("sd");
				__io_wiisd.shutdown();sd_ok=0;
				}
				sleep(2);
				goto error;
		
			} // end test mode
		#endif

		if(ret2<0)  {sleep(2);goto error;}


		remote_call(splash_scr_send);
		
		if(ret2>=0) 
			{
		    ret2=WBFS_Init();
			if(mode_disc && ret2<0) {mode_disc|=1024;ret2=0;} // mode disc
			if(ret2>=0) ret2=Disc_Init();
			}
	
   
		if(ret2>=0 && (mode_disc & 1026)==0)
			{
			__io_usbstorage2.startup();
			ud_ok = fatMountSimple("ud", &__io_usbstorage2);
			if(ud_ok) global_mount|=128;
			}
		else ud_ok=0;

		if(num_fat_games!=0) ret2=Disc_Init();;
        
		if(ud_ok)
			{
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			down_frame=0;
			down_mess="Reading .CISO From USB";
			remote_call(wait_splash_scr);

			list_fat("ud:/ciso/");
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			remote_call(splash_scr_send);
			}

        /*
		// test warning
		if(usb_clusters) usb_clusters=4;
		if(sd_clusters) sd_clusters=4;
		*/

		if((sd_clusters && sd_clusters<32) || (usb_clusters && usb_clusters<32)) 
			{
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			cluster_warning();
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			remote_call(splash_scr_send);
			}
        

		if(!direct_launch) sleep(2); //for(n=0;n<120;n++) {splash_scr();Screen_flip();}

		//
	   
	   
		screen_fx=memalign(32, 128*128*4);

		create_png_texture(&text_button[0], button1, 0);
		create_png_texture(&text_button[1], button2, 0);
		create_png_texture(&text_button[2], button3, 0);
		text_button[3]=text_button[1];

		create_png_texture(& default_game_texture, defpng, 0);
		
		
		for(n=0;n<128*64*3;n++)
			{
			switch((rand()>>3) & 7)
				{
				case 0:
					game_empty[n]=0xfff0f0f0;break;
				case 1:
					game_empty[n]=0xff404040;break;
				case 2:
					game_empty[n]=0xffd0d0d0;break;
				case 3:
					game_empty[n]=0xffc0c0c0;break;
				case 4:
					game_empty[n]=0xffc0a000;break;
				case 5:
					game_empty[n]=0xff404040;break;
				case 6:
					game_empty[n]=0xffd0d0d0;break;
				case 7:
					game_empty[n]=0xffc0c0c0;break;
				}
			}


		CreateTexture(&text_game_empty[0], TILE_SRGBA8, &game_empty[0], 128, 64, 0);
		CreateTexture(&text_game_empty[1], TILE_SRGBA8, &game_empty[128*64], 128, 64, 0);
		CreateTexture(&text_game_empty[2], TILE_SRGBA8, &game_empty[128*64*2], 128, 64, 0);
		text_game_empty[3]=text_game_empty[1];

		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		splash_scr();

		SelectFontTexture(1); // selecciona la fuente de letra extra

		letter_size(8,32);
				
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);

		if (ret2 < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: Could not initialize WBFS subsystem! (ret = %d)", ret2);

			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
			
			PY+=80;

			if(!use_port1)
				s_printf("Maybe you need plug the device on USB port 0...");
			else
				s_printf("Maybe you need plug the device on USB port 1...");

			Screen_flip();
			sleep(2);
			goto error;
			}
		Screen_flip();

		
		
		
		#if 1

		#ifdef USE_MODPLAYER
		ASND_Init();
		ASND_Pause(0);
       
		if(l_time)
			{
			
			if(l_time->tm_mday>=1 && l_time->tm_mday<=6 && l_time->tm_mon==1 && l_time->tm_year>109) happy_new_year(l_time->tm_year);
			
			}
        

	    remote_call(splash_scr_send);
	// inicializa el modplayer en modo loop infinito
		
		#ifndef ALTERNATIVE_VERSION
		MODPlay_Init(&mod_track);
		
		n=-1;
		if(sd_ok)
			{
			FILE *fp;
			fp=fopen("sd:/apps/uloader/music.mod","rb");

			if(fp!=0)
				{
				int size;
				char *p;

				fseek(fp, 0, SEEK_END);
				size = ftell(fp);
				p= malloc(size);
				if(p)
					{
					fseek(fp, 0, SEEK_SET);

					if(fread(p,1, size ,fp)==size)
						{
						n=MODPlay_SetMOD (&mod_track, p);
						}
					else free(p);
					}
				
				fclose(fp);
				}
			}
        if(n<0) n=MODPlay_SetMOD (&mod_track, lotus3_2 );
		if (n < 0 ) // set the MOD song
			{
			MODPlay_Unload (&mod_track);   
			}
		else  
			{

			MODPlay_SetVolume( &mod_track, 16, 16); // fix the volume to 16 (max 64)
			MODPlay_Start (&mod_track); // Play the MOD
			
			}

		#else
		
		// Ogg

		if(sd_ok)
			{
			FILE *fp;
	        int ogg_size;
	        void *ogg_file;

	        fp=fopen("sd:/apps/uloader/music.ogg","rb"); // abre fichero
	
	        if(fp!=0)
				{
		        fseek(fp, 0, SEEK_END); // situa el puntero de lectura al final del fichero
		        ogg_size = ftell(fp); // obtiene la posicion del puntero (al estar al final, obtiene el tamaño del fichero ;) 
        
		        fseek(fp, 0, SEEK_SET); // situa el puntero de lectura al principio del fichero

		        ogg_file=malloc(ogg_size+128); /* asigna memoria suficiente para leer el fichero ogg al completo mas 128 bytes extras de proteccion (seguramente no haga falta, pero por si alguna lectura rebasa un poco) */
	
				if(ogg_file) // si tenemos memoria asignada, procede
					{
                    if(fread(ogg_file,1, ogg_size ,fp)==ogg_size) // lee el fichero al completo, si no falla procede
						{
	                    PlayOgg(mem_open(ogg_file, ogg_size),0,OGG_INFINITE_TIME); // tocala otra vez, Sam
	                    }
					}
         
				fclose(fp); // cierra el fichero
				}
			else PlayOgg(mem_open((void *) bg_music, size_bg_music),0,OGG_INFINITE_TIME);
			}
			else PlayOgg(mem_open((void *) bg_music, size_bg_music),0,OGG_INFINITE_TIME);

		#endif
		
		#endif

		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		splash_scr();

		SelectFontTexture(1); // selecciona la fuente de letra extra

		letter_size(16,32);
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);
		
		Screen_flip();
		
		n=menu_main();
		if(n==1) goto exit_ok;
		if(n==2) goto error_w;
		

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

exit_ok:

	mload_set_ES_ioctlv_vector(NULL);
	mload_close();
	#ifdef USE_MODPLAYER
	
	#ifdef ALTERNATIVE_VERSION
		StopOgg();
	#endif
		ASND_End();		// finaliza el sonido
	#endif
	make_rumble_off();
	wiimote_read();
	WPAD_Shutdown();
	USBStorage2_Umount();
	SYS_RemoveAlarm(scr_poweroff);
	sleep(1);
    
	if(exit_by_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	if(exit_by_reset==3)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);
	return 0;

error_w:

	mload_set_ES_ioctlv_vector(NULL);
	mload_close();
	 #ifdef USE_MODPLAYER
	 #ifdef ALTERNATIVE_VERSION
		StopOgg();
	#endif
		ASND_End();		// finaliza el sonido
	#endif
	

error:
	make_rumble_off();
	wiimote_read();

	WPAD_Shutdown();

	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
	remote_end();
	SYS_RemoveAlarm(scr_poweroff);

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

    sleep(2);
	if(sd_ok)
		{
		fatUnmount("sd");
		__io_wiisd.shutdown();sd_ok=0;
		}
    if(ud_ok)
		{
		fatUnmount("ud");
		__io_usbstorage2.shutdown();ud_ok=0;
		}
	

	if(return_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	if(exit_by_reset==3)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);

return 0;
   // return ret;
}


static u32 lba_start=0;

static int init_ciso_table=1;

int fun_fat_read(void*fp,u32 offset,u32 count,void*iobuf)
{

int ret,l;

static u32 table_lba[2048];
static u8 mem_index[2048];
static u8 buff[32768] ATTRIBUTE_ALIGN(32);
static int ciso_size;

		
	if(!fp)
		{
		void *temp_ptr;
		int size2;
		
		size2=(count+31) & ~31;

			temp_ptr=memalign(32, size2);

			if(!temp_ptr) return 0;


			if(WDVD_UnencryptedRead(temp_ptr, size2, ((u64)offset)<<2)<0) count=0;


			if(count)
				{
				memcpy(iobuf, temp_ptr, count);
				}

			free(temp_ptr);

		return (count==0);
		}
	
		if(init_ciso_table)
			{
			u32 lba_glob=(lba_start<<9)+(16<<9);

			
			fseek(fp,(lba_start<<11), SEEK_SET);
			
			ret=fread(buff,1, 32768 ,fp);
				if(ret<=0) {return 1;}
				else
					if(!(buff[0]=='C' && buff[1]=='I' && buff[2]=='S' && buff[3]=='O')) return 1;
			ciso_size=(((u32)buff[4])+(((u32)buff[5])<<8)+(((u32)buff[6])<<16)+(((u32)buff[7])<<24))/4;
		
			memset(mem_index,0,2048);


			for(l=0;l<16384;l++)
				{

				if((l & 7)==0) table_lba[l>>3]=lba_glob;
				
				if(buff[(8+l)])
					{
					mem_index[l>>3]|=1<<(l & 7);
					lba_glob+=ciso_size;
					}
				}
            
			init_ciso_table=0;
			}

		if(!init_ciso_table)
			{
			u32 temp=((u32) offset)/ciso_size;
	

			u32 read_lba=table_lba[temp>>3];

			for(l=0;l<(temp & 7);l++) if((mem_index[temp>>3]>>l) & 1) read_lba+=ciso_size;

			read_lba<<=2;

			read_lba+=((offset) & (ciso_size-1))<<2;
			

			fseek(fp, read_lba, SEEK_SET);
			//ret=fread(ptr,1, size ,fp_disc);
			return !(fread(iobuf, 1, count, fp)==count);
		
			}

return 1;
}



// adapted from Mark R. (USB Loader mrc v9)
char* FAT_BannerTitle(char *path, SoundInfo *snd ){
	void *banner = NULL;
	int size;
	FILE *fp;

	fp=fopen(path, "r");
	if(!fp)  goto error;
	
	init_ciso_table=1;
	lba_start=0;

	size = wbfs_extract_file2(fp, "opening.bnr", &banner, fun_fat_read);
	
	fclose(fp);

	if (!banner || size <= 0) goto error;

		  
	char* bannerTitle=malloc(84);
	
	int i;
	for(i=0;i<84;i++)
		bannerTitle[i]=((char*)banner)[0xB0+i];

    parse_banner_snd(banner, snd);

	//SAFE_FREE(banner);
	if(banner){
		free(banner);
		banner=NULL;
	}

	return bannerTitle;
error:
	return "\0e\0r\0r\0o\0r\0 \0b\0n\0B";
}

char* disc_BannerTitle(u32 lba, SoundInfo *snd ){
	void *banner = NULL;
	int size;
	FILE *fp=NULL;
	
	init_ciso_table=1;
	lba_start=lba;

	size = wbfs_extract_file2(fp, "opening.bnr", &banner, fun_fat_read);
	

	if (!banner || size <= 0) goto error;

		  
	char* bannerTitle=malloc(84);
	
	int i;
	for(i=0;i<84;i++)
		bannerTitle[i]=((char*)banner)[0xB0+i];

	parse_banner_snd(banner, snd);

	//SAFE_FREE(banner);
	if(banner){
		free(banner);
		banner=NULL;
	}
	

	return bannerTitle;
error:
	
	return "\0e\0r\0r\0o\0r\0 \0b\0n\0B";
}
char* bannerTitle=NULL;

int load_game_routine(u8 *discid, int game_mode)
{
int n, ret,ret2;
int force_ingame_ios=0;

int flag_disc=game_mode & 0xa00000;

	make_rumble_off();

	wiimote_read();

	SoundInfo snd;
	memset(&snd, 0, sizeof(snd));

	thread_in_second_plane=1;

    cabecera2("Reading The Banner...");

	if(flag_disc)
		{
		bannerTitle=disc_BannerTitle(0, &snd);
	    
		}
	else
	if(is_fat)
		{
		char *path;

		get_fat_name_pass=1;
				
		path=get_fat_name(discid);
				
		get_fat_name_pass=0;
		
		
		if(path)
		   bannerTitle=FAT_BannerTitle(path, &snd);
		else  bannerTitle=0;
		 

		}
	else 
		{
		bannerTitle=WBFS_BannerTitle(discid, &snd);
		
		}
        #ifdef USE_MODPLAYER
		#ifdef ALTERNATIVE_VERSION
			StopOgg();
		#else
			MODPlay_Stop(&mod_track); 
		#endif
		//ASND_End();		// finaliza el sonido
		#endif
		if (snd.dsp_data) 
			{
			
			ASND_StopVoice(0);
			ASND_StopVoice(1);

			int fmt = (snd.channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;

			if(snd.loop) ASND_SetInfiniteVoice(1, fmt, snd.rate, 0,	snd.dsp_data, snd.size, 160,160);
		           else  ASND_SetVoice(1, fmt, snd.rate, 0, snd.dsp_data, snd.size, 96,96, NULL);
			}
	
thread_in_second_plane=0;


	//WPAD_Shutdown();

	game_mode &= ~0xa00000;

	add_game_log(discid);

	if(!flag_disc)
		save_cfg(0);
	else 
		save_cfg(1);

	
	#ifndef DONT_USE_IOS249
    if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; 
    else { if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}
    #else
	if((game_datas[game_mode-1].config & 1)) cios=224; 
    else {if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}
   // game_datas[game_mode-1].config &=~1;
    #endif



	forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

	langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

	nand_mode=(game_datas[game_mode-1].config>>8) & 15;

	force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);

	//bca_mode=(game_datas[game_mode-1].config>>29) & 1;

	hook_selected= ((game_datas[game_mode-1].config>>24) & 7)+1;
	//game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);

	if(!((bca_mode & 1) && get_bca(discid, BCA_Data))) bca_mode=0;
	
	Get_AlternativeDol(discid);
	
	if(sd_ok)
		{
		fatUnmount("sd");
		__io_wiisd.shutdown();
		sd_ok=0;
		}
	 __io_wiisd.shutdown();
	 __io_wiisd.shutdown();
		

	if(ud_ok)
		{
		fatUnmount("ud");
		__io_usbstorage2.shutdown();ud_ok=0;
		}
	
    USBStorage2_Watchdog(0); // disable watchdog
	USBStorage2_Deinit();

	// WARNING: this disable ehcmodule for USB 2.0 operations (only to work in DVD Mode without NAND emulation)
	//if(!(flag_disc & 0x200000) && flag_disc && (nand_mode & 2)==0) USBStorage2_EHC_Off();

	WDVD_Close();
	
	/*
	#ifdef USE_MODPLAYER
	#ifdef ALTERNATIVE_VERSION
		StopOgg();
	#else
		MODPlay_Stop(&mod_track); 
	#endif
	//ASND_End();		// finaliza el sonido
	#endif
*/
	if((cios!=222 && force_ios249==0) || force_reload_ios222)
		{
		cabecera2("Loading...");
		
		WPAD_Shutdown();
		usleep(500*1000);

		ret2=IOS_ReloadIOS(cios);
        if((cios==223 || cios==224) && ret2==0)
			{
			if((*(volatile u32 *)0x80003140 & 0xffff)<4) return -7777;
			}
		if(ret2<0)
			{
			cios=222;
			IOS_ReloadIOS(cios);
			}
		sleep(1);
		
		cabecera2( "Loading...");
		
		load_ehc_module();
	
		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH+is_16_9*208, SCR_HEIGHT);	
		
		// WARNING: this disable ehcmodule for USB 2.0 operations (only to work in DVD Mode)
        if(!(flag_disc & 0x200000) && flag_disc && (nand_mode & 2)==0) USBStorage2_EHC_Off();	
		else
			{
			if(!dvd_only)
				{
				for(n=0;n<50;n++)
					{
					cabecera2("Loading...");

						ret2 = USBStorage2_Init(); 
						if(!ret2) break;
						usleep(500*1000);
					}
				
				USBStorage2_Deinit();
				}
			}
		
		
		}

	// enables fake ES_ioctlv (to skip IOS_ReloadIOS(ios))
	if(force_ingame_ios)
		{
		enable_ES_ioctlv_vector();
		}
		
	mload_close();
   
	if(flag_disc & 0x200000) discid[6]=2; // mode DVD USB
	else if(flag_disc) discid[6]=1; // mode DVD

	if(bannerTitle)
			{
			
			if(Diario_ActualizarDiario(bannerTitle,(char*) discid)<0) ;//exit(0);
			free(bannerTitle);
			}

	ret = load_disc(discid);
	ASND_StopVoice(1);
	ASND_End();		// finaliza el sonido

return ret;
}




void snd_explo(int voice, int pos)
{
	#ifdef USE_MODPLAYER
			
	if(ASND_StatusVoice(2+voice)!=SND_WORKING)
		{
		int l,r;

		l=((pos-SCR_WIDTH/2)<100) ? 127 : 64;
		r=((pos-SCR_WIDTH/2)>-100) ? 127 : 64;

		ASND_SetVoice(2+voice, VOICE_MONO_8BIT, 22050,40, sound, size_sound, l, r, NULL);
		}

	#endif
}
///////////////////////////////////////////////////////////////////////////////

int get_wbfs_partition()
{
int n, ret=-1;

	if(!dvd_only)
		{

		// locate WBFS Partitions and number of games from the partitions
		for(n=0;n<4;n++)
			{
			partition_cnt[n]=-1;

			if(!(mode_disc & 1024))
				{
				ret = WBFS_Open2(n);
				if(ret==0) {partition_cnt[n]=0;ret = WBFS_GetCount((u32 *) &partition_cnt[n]);if(ret<0) partition_cnt[n]=0;}
				}
			 
			}
		
		
		// get a valid partition from current partition selected
		if(!(mode_disc & 1024))
		{
		for(n=0;n<4;n++)
			{
			ret = WBFS_Open2((current_partition+n) & 3);
			if(ret==0) {current_partition=(current_partition+n) & 3;break;}
			else partition_cnt[(current_partition+n) & 3]=-1;
			}
		}

		if(!(mode_disc & 1024) && (mode_disc & 1023)==0 && !is_fat && ret<0 && num_fat_games)
			{
			is_fat=1;
			}
	
		if(!(mode_disc & 1024) && (mode_disc & 1023)==0 && !is_fat && ret<0)
			{
			
				for(n=0;n<60*4;n++)
					{
					splash_scr();
					letter_size(16,32);
					PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
							
					bkcolor=0;
					autocenter=1;
					SetTexture(NULL);
					DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
					DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);
				
					s_printf("No WBFS partition found!\n");
					Screen_flip();
					}
           
		    ret= menu_format();
			if(ret==0) 
				{
				// locate WBFS Partitions and number of games from the partitions
				for(n=0;n<4;n++)
					{
					ret = WBFS_Open2(n);
					if(ret==0) {partition_cnt[n]=0;ret = WBFS_GetCount((u32 *) &partition_cnt[n]);if(ret<0) partition_cnt[n]=0;}
					else partition_cnt[n]=-1;
					}
				// get a valid partition from current partition
				for(n=0;n<4;n++)
					{
					ret = WBFS_Open2((current_partition+n) & 3);
					if(ret==0) {current_partition=(current_partition+n) & 3;break;}
					else partition_cnt[(current_partition+n) & 3]=-1;
					}
				if(ret<0) 
					{
					dvd_only=1;
					mode_disc=1+1024;
					}
					//goto error_w;
				}
			else 
				return -0x666;

			Screen_flip();
			SetTexture(&text_background2);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, BACK_COLOR);
			letter_size(16,32);
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
			}

		temp_game_cnt=0;temp_buffer=NULL;
		//ret = WBFS_GetCount(&temp_game_cnt);
		ret= partition_cnt[current_partition];
		temp_game_cnt= ret;
		
		} // !dvd_only
		else
			{
			ret=temp_game_cnt=0;
			temp_game_cnt=0;temp_buffer=NULL;
			current_partition=0;
			for(n=0;n<4;n++)
				{
				partition_cnt[n]=-1;
				}
			}

return ret;
}



///////////////////////////////////////////////////////////////////////////////////



int menu_main()
{
int n;
int ret;
int cnt2, len;

int frames=0;
int load_png=1;
int scroll_mode=0;
int game_mode=0;
int edit_cfg=0;
int cheat_mode=0;
int select_game_bar=0;

int is_favorite=1;
int insert_favorite=0;
int actual_game=0;
int last_game=-1;

int test_favorite=0;

static int old_temp_sel=-1;
f32 f_size=0.0f;
int last_select=-1;

int force_ingame_ios=0;
int game_locked_cfg=0;

int parental_mode=0;
char parental_str[4]={0,0,0,0};

int launch_counter=9;
int partial_counter;

int volume_osd=0;
int rumble_off_osd=0;
int icon_osd=0;
int warning_osd=0;

int bca_saved=0;

int temp_home=-1;

char *warning_message="what?";
int warning_time=0;

int edit2_mode=0;
 
int save_exist=0;

int it_have_fat=is_fat;

struct discHdr *disc_header=NULL;


		if(mode_disc & 3) is_fat=0;
get_games:
		
		frames=0;frames2=0;
		load_png=1;
		scroll_mode=0;
		game_mode=0;
		edit_cfg=0;
		cheat_mode=0;
		select_game_bar=0;

		is_favorite=1;
		insert_favorite=0;
		actual_game=0;
		last_game=-1;
		test_favorite=0;

		if(temp_buffer) free(temp_buffer);
		temp_buffer=NULL;
	    

		splash2_scr();


		///current_partition=0;
		
		ret=get_wbfs_partition();

		if(ret==-0x666) goto exit_ok;

		if(is_fat)
			{
			ret=temp_game_cnt=num_fat_games;
			}

        
		if (ret < 0 || temp_game_cnt==0) {

			if(!(mode_disc & 3))
				{
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("No Game found\n");
				Screen_flip();

			//if(mode_disc) mode_disc|=1024;
				sleep(3);
				}
			//goto error_w;
			}
		else
	       {

			/* Buffer length */
			len = sizeof(struct discHdr) * temp_game_cnt;

			/* Allocate memory */
			temp_buffer = (struct discHdr *)memalign(32, len);
			if (!temp_buffer){
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in memalign()\n");
				Screen_flip();
				goto error_w;
				}

			/* Clear buffer */
			memset(temp_buffer, 0, len);
            if(is_fat)
				{
				/* Get header list from FAT*/
				memcpy(temp_buffer, (void *) fat_disc, temp_game_cnt * sizeof(struct discHdr));
				ret=0;
				}
			else
				/* Get header list */
				ret = WBFS_GetHeaders(temp_buffer, temp_game_cnt, sizeof(struct discHdr));

			if (ret < 0) {
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in WBFS_GetHeaders()\n");
				Screen_flip();
				free(temp_buffer);
				goto error_w;
				}
		
		    gameList = temp_buffer;
			//temp_game_cnt=0;

		    for(n=0;n<temp_game_cnt;n++)
				{
				  if(gameList[n].id[0]=='_') {memcpy(&gameList[n],&gameList[temp_game_cnt-1],sizeof(struct discHdr));temp_game_cnt--;n--;}
				}

			if(!temp_game_cnt)
				{
				if(!(mode_disc & 3))
					{
					DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
					DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

					s_printf("No Game found\n");
					Screen_flip();
					//if(mode_disc) mode_disc|=1024;
					sleep(2);
					}

				free(temp_buffer);
				temp_buffer=NULL;
				}
			
		    /* Sort entries */
		    if(temp_game_cnt && temp_buffer) qsort(temp_buffer, temp_game_cnt, sizeof(struct discHdr), __Menu_EntryCmp);

		    }

		/*
		// quitar 10
		for(n=1;n<10;n++) memcpy(((char *) temp_buffer)+sizeof(struct discHdr) * temp_game_cnt*n,temp_buffer,sizeof(struct discHdr) * temp_game_cnt);
		temp_game_cnt*=10;
		*/
		
		/* Set values */
		gameList = temp_buffer;
		gameCnt  = temp_game_cnt;
		
		{
		u8 temp=config_file.music_mod;
		/*char temp2[4];

		memcpy(temp2, config_file.parental, 4);*/
		
		memset(&config_file,0, sizeof (config_file));

		if(!(mode_disc & 3)) // mode disc
			{load_cfg(0);memset(parental_str ,0, 4);}
		else {if(load_cfg(1)<0) config_file.music_mod=temp; memset(parental_str ,0, 4);/* memcpy(config_file.parental, temp2, 4);*/}

		}

		#ifndef ALTERNATIVE_VERSION
		MODPlay_SetVolume( &mod_track,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16); // fix the volume to 16 (max 64)
		#else
		SetVolumeOgg((config_file.music_mod & 128) ?  ((config_file.music_mod & 15)*16): 255);
		#endif

		
		#endif

	autocenter=0;
	
	if(mode_disc & 1024) direct_launch=0;

    if(direct_launch)
		{
		if(launch_short) launch_counter=2;
		
		for(n=0;n<gameCnt;n++)
			{
			struct discHdr *header = &gameList[n];
			if(!strncmp((void *) discid, (void *) header->id,6))
				{
				game_datas[0].ind=n;
				
				memcpy(discid,header->id,6); discid[6]=0;
				memset(temp_data,0,256*1024);
				global_GetProfileDatas(discid, temp_data);
				create_game_png_texture(0);
				if(is_fat)
					{
					f_size= ((float) header->magic)/ GB_SIZE;
					}
				else
					if(WBFS_GameSize(header->id, &f_size)) f_size=0.0f;

				game_mode=1;
				
				if((mode_disc & 3) ==0 && config_file.parental[0]==0 && config_file.parental[1]==0 && config_file.parental[2]==0 && config_file.parental[3]==0) parental_control_on=0;

				if(parental_control_on)
					{if((game_datas[game_mode-1].config & (1<<30))!=0) {parental_mode=1024+game_mode;direct_launch=0;}}

				break;
				}
			}
		
		if(n==gameCnt) direct_launch=0;
		}

	if((mode_disc & 3) && is_fat==0)
	{
	game_mode=1;
	//if(is_fat)
	//gameList=NULL;
	game_datas[0].ind=0;
	discid[0]=0;
	memset(temp_data,0,256*1024);
	create_game_png_texture(0);
	parental_mode=0;
	disc_header=NULL;

    if(mode_disc & 2)
		WDVD_SetUSBMode((u8 *) "_DVD", 0);
	else
		WDVD_SetUSBMode(NULL, 0);

	WDVD_Reset();

	remote_DVD_disc_status=0;

	remote_call(remote_DVD_disc);
	usleep(1000*50);

	}
	

	guitar_pos_x=SCR_WIDTH/2-32;guitar_pos_y=SCR_HEIGHT/2-32;

	
	old_temp_sel=-1;

	// destroy favorite #15
	config_file.id[15][0]=0;


	partial_counter=50+10*(SCR_HEIGHT<=480);

	while(1)
	{
	int temp_sel=-1;
	int test_gfx_page=0;
	int go_home=0;
	int go_game=0;
						
	if(direct_launch && game_mode==1 && gameList!=NULL)
		{
		partial_counter--;
		if(partial_counter<=0) 
			{
			partial_counter=50+10*(SCR_HEIGHT<=480);
			if(launch_counter>0) {launch_counter--;snd_fx_tick();}
			}
		}

	if(config_file.parental[0]==0 && config_file.parental[1]==0 && config_file.parental[2]==0 && config_file.parental[3]==0) parental_control_on=0;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes
	

	//if((frames2 & 255)==0) ASND_SetVoice(1, VOICE_MONO_8BIT, 4096,0, &sound_effects[0][0], 2048/8, 255, 255, NULL);
//	if((frames2 & 255)==0) ASND_SetVoice(1, VOICE_MONO_8BIT, 15000,0, &sound_effects[1][0], 2048, 255, 255, NULL);

	set_text_screen_fx();
   
	draw_background();
		
    temp_game_cnt=actual_game;
	SelectFontTexture(1); // selecciona la fuente de letra extra

   
    {
	int m;
    int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;


	if(game_mode)
		{
		 
		struct discHdr *header;

		altdol_frames2+=20;
		
		if(remote_DVD_disc_status==2) disc_header=&mydisc_header;
		else
		if(remote_DVD_disc_status<0)
			{
			disc_header=NULL;altdol_frames2=0;
			}
	    else {disc_header=NULL;if(remote_DVD_disc_status==0) altdol_frames2=0;}
		
		
		if(!(mode_disc & 3))
		    header= &gameList[game_datas[game_mode-1].ind];
		else header= disc_header;

		

		SetTexture(NULL);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
		
		if((mode_disc & 3) && is_fat==0)
			SetTexture(&default_game_texture);
		else
		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);

		SetTexture(&text_button[0]);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffffffff);

		
		SetTexture(NULL);
			
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

		if((mode_disc & 3) && is_fat==0)
			SetTexture(&default_game_texture);
		else
		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);


		DrawFillBox(320-127, ylev+8, 254, 338, 0, 0xffffffff);

		if(is_fat && header && header->version)
			{
			letter_size(16,24);
			
			PX=320-127+8;PY=ylev+8+8;color=0xff000000;bkcolor=0xffffffff;
			s_printf("SD");bkcolor=0;
			}
		
		SetTexture(NULL);
		DrawBox(320-127, ylev+8, 254, 338, 0, 2, 0xff303030);
		
		if((mode_disc & 3) && is_fat==0 /*&& remote_DVD_disc_status!=0*/)
			{
			int nn;

			SetTexture(&text_icon[3]);

			#define SLICE_LEN 200

			//DrawFillBox(20, ylev, 148*4, 352, 0, 0xffffffff);
			for(nn=0;nn<8;nn++)
				{
				u32 color;
				int vel=7-(altdol_frames2>>6);
				if(vel<0) vel=0;
				int ang=(((altdol_frames2>>vel) & 127)<<7)-2048+(nn<<11);
				//vel=7;
				//int ang=((frames2) & 31)<<10;
				int xx1=SCR_WIDTH/2+(SLICE_LEN*seno2((ang) & 16383))/16384,yy1=ylev+352/2-(SLICE_LEN*coseno2((ang) & 16383))/16384;
				int xx2=SCR_WIDTH/2+(SLICE_LEN*seno2((ang+4096) & 16383))/16384,yy2=ylev+352/2-(SLICE_LEN*coseno2((ang+4096) & 16383))/16384;
				int xx3=SCR_WIDTH/2+(SLICE_LEN*seno2((ang+4096*2) & 16383))/16384,yy3=ylev+352/2-(SLICE_LEN*coseno2((ang+4096*2) & 16383))/16384;
				int xx4=SCR_WIDTH/2+(SLICE_LEN*seno2((ang+4096*3) & 16383))/16384,yy4=ylev+352/2-(SLICE_LEN*coseno2((ang+4096*3) & 16383))/16384;

				if(remote_DVD_disc_status==0) color=0x57ffffff;
					else
				if(nn==0) color=0xffffffff; else color=0x27ffffff;

				if(mode_disc & 2) color=(color & 0xff000000) | 0x20ff6f;

				SetTexture(&text_icon[3]);
				ConfigureForTexture(10);
				GX_Begin(GX_TRIANGLESTRIP,  GX_VTXFMT0, 5);

				AddTextureVertex(xx1, yy1, 0, color, 1, 1);
				AddTextureVertex(xx2, yy2, 0, color, 1024, 1); 
				AddTextureVertex(xx3, yy3, 0, color, 1024, 1024); 
				AddTextureVertex(xx4, yy4, 0, color, 1, 1024);
				AddTextureVertex(xx1, yy1, 0, color, 1, 1);
				GX_End();

				if(vel!=0) break;

				#undef SLICE_LEN

				}


			SetTexture(NULL);
			}

        
		if(header)
			{
			if(strlen(header->title)<=37) letter_size(16,32);
			else if(strlen(header->title)<=45) letter_size(12,32);
			else letter_size(8,32);
			}
		else letter_size(16,32);

		PX= 0; PY=ylev-32;/* 8+ylev+2*110*/; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		
		autocenter=1;

		if(!header)
			{
			if(remote_DVD_disc_status==0) s_printf("%s",&letrero[idioma][38][0]);
			else if(remote_DVD_disc_status<0) s_printf("DVD Error %i",remote_DVD_disc_status);
				else s_printf("%s",&letrero[idioma][41][0]);
			//s_printf("Scanning DVD...");
			}
		else
			s_printf("%s",header->title);
		
		autocenter=0;

		letter_size(16,32);
		
		bkcolor=0;

		

		if(cheat_mode)
		{

		if(txt_cheats)
			{
			int f=0;
			PX= 26; PY=ylev+16; color= 0xff000000;
			letter_size(16,32);
			autocenter=1;
			bkcolor=0xb0f0f0f0;
			s_printf("%s", &letrero[idioma][10][0]);
			bkcolor=0;
			autocenter=0;
			letter_size(8,32);

			for(n=0;n<5;n++)
				{
				if((actual_cheat+n)>=num_list_cheats) break;
				if(!data_cheats[actual_cheat+n].title) break;
				if(Draw_button2(30+16, ylev+56+56*n, data_cheats[actual_cheat+n].title,data_cheats[actual_cheat+n].apply)) 
					{
					if(select_game_bar!=(500+actual_cheat+n)) scroll_text=-10;
					//if(select_game_bar!=500+actual_cheat+n)  snd_fx_tick();


					select_game_bar=500+actual_cheat+n;
					f=1;
					}
				}
			
			if(f==0) select_game_bar=-1;

			if(num_list_cheats)
					{
					SetTexture(NULL);
					if(actual_cheat>=5)
						{
						if(px>=-is_16_9*80 && px<=80-is_16_9*80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							circle_select(40-is_16_9*80, ylev+220, '-', 1);
							select_game_bar=-1;
							test_gfx_page=-1;

							if(old_temp_sel!=1000 && !load_png) 
								{
								snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							circle_select(40-is_16_9*80, ylev+220, '-', 0);
							}
						}

						if((actual_cheat+5)<num_list_cheats)
						{
						if(px>=SCR_WIDTH-82+is_16_9*80 && px<=SCR_WIDTH-2+is_16_9*80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							circle_select(SCR_WIDTH-42+is_16_9*80, ylev+220, '+', 1);
							select_game_bar=-1;
							test_gfx_page=1;

							if(old_temp_sel!=1000 && !load_png) 
								{
								snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							circle_select(SCR_WIDTH-42+is_16_9*80, ylev+220, '+', 0);
							}
						}

						if(old_temp_sel>=1000 && test_gfx_page==0) old_temp_sel=-1;
						
					}

			
			}
		else
			{
			PX= 26; PY=ylev+108*2-64; color= 0xff000000;
			letter_size(16,64);
			autocenter=1;
			bkcolor=0xb0f0f0f0;
			s_printf("%s", &letrero[idioma][10][0]);
			bkcolor=0;
			autocenter=0;
			letter_size(16,32);
			}
													
		if(select_game_bar>=500 && select_game_bar<500+num_list_cheats && data_cheats[select_game_bar-500].description)
				{
					PX=40;PY=ylev+108*4-64+16;
					DrawRoundFillBox(20, ylev+108*4-64, 148*4, 56, 0, 0xffcfcf00);
					DrawRoundBox(20, ylev+108*4-64, 148*4, 56, 0, 4, 0xffcf0000);
					letter_size(8,32);
				
					draw_box_text(data_cheats[select_game_bar-500].description);
				
				}
		else
			{
			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][11][0])) select_game_bar=1000;
			if(Draw_button(600-32-strlen(&letrero[idioma][12][0])*8, ylev+108*4-64, &letrero[idioma][12][0])) select_game_bar=1001;

			}
       
		}
		else
        if(!edit_cfg)
			{
			select_game_bar=0;
			PX= 26; PY=ylev+12; color= 0xff000000; 
			bkcolor=0x0;//0xb0f0f0f0;

            if((mode_disc & 3))
				{
				if(header)
					dol_GetProfileDatas(header->id, temp_data);
				else
					dol_GetProfileDatas((u8 *) "**12**", temp_data);


				game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
				}
												   

			if((mode_disc & 2) && (game_datas[game_mode-1].config & 1))  // force cIOS 222 for USB DVD
				{
				game_datas[game_mode-1].config&=~1;
				}

			if((game_datas[game_mode-1].config & 1) || force_ios249) 
				#ifndef DONT_USE_IOS249	
				s_printf("cIOS 249"); 
			    #else
				s_printf("cIOS 224");
				#endif
			
			else {if(game_datas[game_mode-1].config & 2)  s_printf("cIOS 223"); else s_printf("cIOS 222");}
			
			forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

			langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

			nand_mode=(game_datas[game_mode-1].config>>8) & 15;

			force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
			game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);

			bca_mode=(game_datas[game_mode-1].config>>29) & 1;

			hook_selected= ((game_datas[game_mode-1].config>>24) & 7)+1;

			if(mode_disc && parental_control_on && game_locked_cfg)
				parental_mode=9999;
			if((mode_disc & 3) && !disc_header) parental_mode=0;

		
			
			PX=608-6*16; 
			if(header)
				{
				s_printf("%c%c%c%c%c%c", header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
			    memcpy(discid,header->id,6); discid[6]=0;
				}
			

			PX= 26; PY=ylev+12+48;
			if(forcevideo==1) s_printf("F. PAL60");
			if(forcevideo==2) s_printf("F. NTSC");
			if(forcevideo==3) s_printf("F. PAL50");
			
            
			PX=608-6*16; if(header && !(mode_disc & 3)) s_printf("%.2fGB", f_size);
		 
			autocenter=0;
			PX= 26; PY=ylev+352-48;
			if(langsel) s_printf("%s", &languages[langsel][0]);
			autocenter=0;

			bkcolor=0x0;

			if(game_locked_cfg)
					{
					SetTexture(&text_icon[2]);
					DrawFillBox(600-24, ylev+352-48, 16, 24, 0, 0xffffffff);
					}

		    //idioma=0;

			if(!parental_mode && header && (mode_disc & 3)!=0 && multi_ciso[0].len)
					{
					
					for(n=0;n<8;n++)
						{
					
						if(Draw_button2(26+(n & 3)*132+24, ylev+146+64*(n>3), "            ", 
							multi_ciso[n].len ? ciso_sel==n : -1)) select_game_bar=30+n;
			
						letter_size(8,24);

						PX= 26+(n & 3) *132+12+24; PY= 12+ylev+146+64*(n>3); color= 0xff000000; 
					
						bkcolor=0xc0f0f000;
						if(multi_ciso[n].len) draw_text((void *) multi_ciso[n].name);
						bkcolor=0;

						}
					}

			x_temp=16;

            if((!(mode_disc & 1024) || it_have_fat) && remote_DVD_disc_status!=1)
				if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;



			if(!parental_mode)
				{

				if(header)
					if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][1][0])) select_game_bar=2;
				

				if(test_favorite && !(mode_disc & 3) && is_fat==0)
					{
					if(is_favorite)
						{if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][2][0])) select_game_bar=4;}
					else
						if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][3][0])) select_game_bar=3;
					}

				
			
				if(header && mode_disc && (mode_disc & 2)==0 && bca_status_read!=0/*&& BCA_ret==0*/)
					{
					if(show_bca==0)
						{
						
						if(Draw_button(x_temp+8, ylev+108*4-64, "Show BCA Data")) select_game_bar=56;
						}
					else
						if(Draw_button(x_temp+8, ylev+108*4-64, "Hide BCA Data")) select_game_bar=56;
					}

				if(dvd_only==1 && it_have_fat==0 && (mode_disc & 1024)/*&& remote_DVD_disc_status==0*/)
					if(Draw_button(x_temp+8, ylev+108*4-64,"Wii Menu"/* &letrero[idioma][27][0]*/)) select_game_bar=6;
				}

            if(header)
				{

				if(!parental_mode)
					{
					if(!mode_disc)
						{if(Draw_button(600-32-strlen(&letrero[idioma][4][0])*8-78, ylev+108*4-64, "A.Dol")) {load_alt_game_disc=0;select_game_bar=55;}}
					else
						{if(Draw_button(600-32-strlen(&letrero[idioma][4][0])*8-78, ylev+108*4-64, "A.Dol")) {load_alt_game_disc=1;select_game_bar=55;}}
					}
                

				if(header && mode_disc && show_bca /*&& BCA_ret==0*/)
					{
					int h,g;
					
					DrawRoundFillBox(40, ylev+352/2-32, 640-80, 64, 0, 0xffcfcf00);
					DrawRoundBox(40, ylev+352/2-32, 640-80, 64, 0, 4, 0xffcf0000);

					letter_size(8,24);
					PX= 52; PY=ylev+352/2-24; 

					if(bca_status_read==3 && BCA_ret==0)
						{

		
						for(g=0;g<4;g++)
							{
							for(h=0;h<8;h++)
								s_printf("%2.2x", BCA_Data[g*8+h]);
							s_printf(" ");
							}
						
						PX= 52; PY+=28;

						for(g=0;g<4;g++)
							{
							for(h=0;h<8;h++)
								s_printf("%2.2x", BCA_Data[32+g*8+h]);
							s_printf(" ");
							}
					
						if(Draw_button(12+SCR_WIDTH/2-(13*12)/2, PY+48, "Save BCA Data")) select_game_bar=57;
						}
					else
						{
						PY+=12;
						autocenter=1;
						if(bca_status_read==3 && BCA_ret<0)
							s_printf("Error Reading BCA");
						else
							s_printf("Reading...");
						autocenter=0;
						}
					
					}
				else show_bca=0;
				
				
				}
            
			if(header && !parental_mode)
				if(Draw_button(600-32-strlen(&letrero[idioma][4][0])*8, ylev+108*4-64, &letrero[idioma][4][0])) {select_game_bar=5;}

			
			}
		else
			{// edit config
			int g;
			select_game_bar=0;
			PX= 36; PY=ylev+8; color= 0xff000000;
			struct discHdr *header;
			

			if(!mode_disc)  header= &gameList[game_datas[game_mode-1].ind];
				else header= disc_header;
			if(!header) 
				{
				edit_cfg=0;
				snd_fx_yes();
							
							
				// si no existe crea uno
				if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
					{
					temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
					temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
					temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
					}
								
				temp_data[4]=game_datas[game_mode-1].config & 255;
				temp_data[5]=(game_datas[game_mode-1].config>>8) & 255;
				temp_data[6]=(game_datas[game_mode-1].config>>16) & 255;
				temp_data[7]=(game_datas[game_mode-1].config>>24) & 255;

				game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
				}

            if(edit_cfg==1)
				{
				warning_time=0;
				edit2_mode=0;

				bca_saved=0; // flag to 0
				save_exist=0;

				letter_size(12,24);
				autocenter=1;
				bkcolor=0xb0f0f0f0;
				s_printf(" Select Language ");
				bkcolor=0;
				autocenter=0;
				
				for(g=0;g<11;g++)
					{
					if((g & 3)==0) m=36+32; else m=x_temp+16;

					if(Draw_button2(m, ylev+36+56*(g/4), &languages[g][0],langsel==g)) select_game_bar=100+g;
					}
				m=x_temp+16;
				if(Draw_button2(m, ylev+36+56*(11/4), &languages[0][0],langsel==0)) select_game_bar=100;

				PY=m=ylev+36+56*(g/4)+56;
				PX=36+192-36;
				letter_size(12,24);
				autocenter=0;
				bkcolor=0xb0f0f0f0;
				s_printf(" Video Mode "); PX+=12*9-4; s_printf(" Parental Ctrl");
				//PX=460-12;s_printf(" Select cIOS ");
				PX=32;PY=m+56+40;s_printf(" cIOS ");
				bkcolor=0;
				m+=28;
				
				if(Draw_button2(36, m, " Auto ",(forcevideo==0))) select_game_bar=200;
				/*if(Draw_button2(x_temp+12, m, " Force PAL ",(forcevideo==1))) select_game_bar=201;
				if(Draw_button2(x_temp+12, m, " Force NTSC ",(forcevideo==2))) select_game_bar=202;*/
				if(Draw_button2(x_temp+12, m, " PAL50 ",(forcevideo==3))) select_game_bar=203;
				if(Draw_button2(x_temp+12, m, " PAL60 ",(forcevideo==1))) select_game_bar=201;
				if(Draw_button2(x_temp+12, m, " NTSC ",(forcevideo==2))) select_game_bar=202;

				if(game_locked_cfg)
					{
					if(Draw_button2(x_temp+28, m, " Game Locked ",game_locked_cfg)) select_game_bar=210;
					}
				else
					{
					if(Draw_button2(x_temp+28, m, " Game Unlock ",game_locked_cfg)) select_game_bar=210;
					
					}

				//if(Draw_button2(472, m, " cIOS 222 ", force_ios249 ? -1 : cios==222)) select_game_bar=300;
				//if(Draw_button2(472, m+56, " cIOS 249 ",cios==249)) select_game_bar=301;
				
				#ifdef DONT_USE_IOS249	
				if(cios==249) cios=222;
				#endif

				if((mode_disc & 2) && cios==249) cios=222; // force cIOS 222 for USB DVD

				if(Draw_button2(36+6*12, m+56, " cIOS 222 ", force_ios249 ? -1 : cios==222)) select_game_bar=300;
				if(Draw_button2(x_temp+12, m+56, " cIOS 223 ", force_ios249 ? -1 : cios==223)) select_game_bar=301;
				#ifndef DONT_USE_IOS249	
				if(Draw_button2(x_temp+12, m+56, " cIOS 249 ",(mode_disc & 2)? -1: cios==249)) select_game_bar=302;
				#else
				//Draw_button2(x_temp+12, m+56, " cIOS 249 ",-1);
				if(Draw_button2(x_temp+12, m+56, " cIOS 224 ",(mode_disc & 2)? -1: cios==224)) select_game_bar=302;
				#endif


				if(Draw_button2(x_temp+20, m+56, " Skip IOS ", force_ios249 ? -1 : force_ingame_ios!=0)) select_game_bar=303;
				}
			else // edit 2
				{
				
				color= 0xff000000;
				if(edit2_mode==0)
					{
					letter_size(12,24);
					autocenter=0;
					bkcolor=0xb0f0f0f0;
					PY+=8;
					s_printf("BCA Code:");
					bkcolor=0;
					

					if(Draw_button2(PX+8, ylev+8, "From Disc", (bca_mode & 1)==0)) select_game_bar=20;
					if(Draw_button2(x_temp+16, ylev+8, "From Database", bca_mode)) select_game_bar=21;
					if(Draw_button2(x_temp+16, ylev+8, "Grab From Database", (mode_disc) ? -1 : bca_saved)) select_game_bar=22;

					letter_size(12,24);
					bkcolor=0xb0f0f0f0;
					PY+=56;
					PX=32;
					s_printf("Hooktype For Cheats: ");
					bkcolor=0;

					if(Draw_button2(PX+8, PY-8, "<<", 0)) select_game_bar=25;
					PX=x_temp+8;

					bkcolor=0xb000f000;
					s_printf(" %s ", &hook_type_name[hook_selected-1][0]);

					bkcolor=0;

					if(Draw_button2(PX+8, PY-8, ">>", 0)) select_game_bar=26;

					// saves

					letter_size(12,24);
					bkcolor=0xb0f0f0f0;
					PY+=56;

					DrawLine(40, PY-16, 600, PY-16, 0, 2, 0xc0404040);


					PX=32;
					s_printf("Saves: ");
					bkcolor=0;

					if((nand_mode & 3)==3) nand_mode&=~3;

					if(Draw_button2(PX+8, PY-8, "NAND", (nand_mode & 3)==0)) select_game_bar= 60;
					PX=x_temp+8;
					if(Draw_button2(PX+8, PY-8, "SD", !sd_ok ? (((nand_mode & 3)==1) ? 128 : -1) :(nand_mode & 3)==1)) if(sd_ok) select_game_bar= 61;
					PX=x_temp+8;
					if(Draw_button2(PX+8, PY-8, "USB", !ud_ok ? (((nand_mode & 3)==2) ? 128 : -1) :(nand_mode & 3)==2)) if(ud_ok) select_game_bar= 62;

					PX=x_temp+100;
					if(Draw_button2(PX+8, PY-8, "NAND Export Save", ((nand_mode & 3)==0) ? 128 :0)) select_game_bar= 63;

					letter_size(12,24);
					bkcolor=0xb0f0f0f0;
					PY+=56;
					PX=32;
					
					color=0xffff0000;

					s_printf("id: /");
					bkcolor=0;
					
					// generate folder from id
					{
					
                   
					if(header)
					{
					for(n=0;n<4;n++)
						{
						s_printf("%2.2x", header->id[n]);
						}
					if(save_exist==0)
						{
						create_FAT_FFS_Directory(header);
						if(test_FAT_game(get_FAT_directory1()) || test_FAT_game(get_FAT_directory2())) save_exist=1; else save_exist=-1;
						}
					}
					color= 0xff000000; 
					}

					letter_size(12,32);
					bkcolor=0xb0f0f0f0;
					PX+=12;
					s_printf("Folder: ");
					bkcolor=0;

					if(Draw_button2(PX+8, PY-8, "<<", 0)) select_game_bar= 64;
					PX=x_temp+8;

					if(save_exist==1 && (nand_mode & 3)!=0) bkcolor=0xb000f000; else bkcolor=0xb00000f0;

					s_printf(" /nand%c ", (nand_mode & 0xc) ? 49+((nand_mode>>2) & 3) : ' ');
					bkcolor=0;

					if(Draw_button2(PX+8, PY-8, ">>", 0)) select_game_bar= 65;

					PX=x_temp+8;
					if(Draw_button2(PX+8, PY-8, "Del Save", (((nand_mode & 1) && sd_ok) || ((nand_mode & 2) && ud_ok))  ? 0 : 128)) select_game_bar= 66;

						
					}
				}

				if(edit2_mode==1)
					{
					warning_time=1;

					if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=67;

					if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=69;
					}
				else
				if(edit2_mode==2)
					{
					warning_time=1;

					if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=68;

					if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=69;
					}
				else
					{

					if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=10;

					if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][54+(edit_cfg==1)][0])) select_game_bar=9;

					if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=11;
					}
			


				/////////////////////////////////////
				if(warning_time)
					{
					SelectFontTexture(1); // selecciona la fuente de letra extra

					letter_size(12,32);
									
					PX=0; PY= SCR_HEIGHT/2-32; color= 0xff000000; 
									
					bkcolor=0;
					autocenter=1;
					SetTexture(NULL);
					if(warning_message && warning_message[0]=='#')
						DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16-32, 540, 64, 0, 0xe000ff00);
					else
						DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16-32, 540, 64, 0, 0xe00000ff);

					DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16-32, 540, 64, 0, 4, 0xa0000000);
						
					if(warning_message)
						{
						s_printf("%s",warning_message+(warning_message[0]=='#'));
						}

					autocenter=0;

					SelectFontTexture(0);
					warning_time--;
					}
				////////////////////////////

			}

		
		step_button++;
		}
	else
		{ // modo panel
		int set_set=0;
		int selected_panel=-1;
		select_game_bar=0;
		

		// change partition draw and selection
		letter_size(16,24);
		bkcolor=0;
		autocenter=0;

		for(n=0;n<4;n++)
			{
			if(partition_cnt[n]<0) continue;
			SetTexture(NULL);

			if(n==current_partition && !is_fat)
				DrawRoundFillBox(22+50*n, ylev-32, 48, 32, 0, 0xa000ff00);
			else
				DrawRoundFillBox(22+50*n, ylev-32, 48, 32, 0, 0xa0ffffff);

			PX=38+50*n; PY=ylev-26; color= 0xff000000;s_printf("%c", 49+n); 

			if(px>=22+50*n && px<22+50*n+48 && py>=ylev-32 && py<ylev)
				{
				DrawRoundBox(22+50*n, ylev-32, 48, 32, 0, 4, 0xfff08000);
				select_game_bar=10000+n;
				}
            else
				DrawRoundBox(22+50*n, ylev-32, 48, 32, 0, 4, 0xa0000000);
		
			}

		SetTexture(NULL);
		
		if(is_fat)
			DrawRoundFillBox(600-128, ylev-32, 64, 32, 0, 0xa000ff00);
		else
			DrawRoundFillBox(600-128, ylev-32, 64, 32, 0, 0xa0ffffff);

		if(num_fat_games>0)
			{
			letter_size(12,24);
			PX=600-48-64-2; PY=ylev-26; color= 0xff000000;s_printf("FAT");
			letter_size(16,24);

			if(px>=600-128 && px<600-128+64 && py>=ylev-32 && py<ylev)
					{
					DrawRoundBox(600-128, ylev-32, 64, 32, 0, 4, 0xfff08000);
					select_game_bar=10015;
					}
				else
					DrawRoundBox(600-128, ylev-32, 64, 32, 0, 4, 0xa0000000);
			}


		DrawRoundFillBox(600-64, ylev-32, 80, 32, 0, 0xa0ffffff);
		PX=600-48; PY=ylev-26; color= 0xff000000;s_printf("DVD"); 

		if(px>=600-64 && px<600-64+80 && py>=ylev-32 && py<ylev)
				{
				DrawRoundBox(600-64, ylev-32, 80, 32, 0, 4, 0xfff08000);
				select_game_bar=10016;
				}
            else
				DrawRoundBox(600-64, ylev-32, 80, 32, 0, 4, 0xa0000000);

		PX= 0; PY=ylev-32; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		letter_size(16,32);
		autocenter=1;
		
		if(gameList==NULL) is_favorite=0;

		if(is_favorite && !insert_favorite)
			{
			for(n=0;n<15;n++)
				if(config_file.id[n][0]!=0) break;
			if(n==15) 
				{is_favorite=0;
				actual_game=0;
				if(last_game>=0) {actual_game=last_game;last_game=-1;}
				}
			}
		if(is_favorite) 
			{
			if(insert_favorite) s_printf("%s", &letrero[idioma][5][0]);
				else s_printf("%s", &letrero[idioma][6][0]);
			}
		else
			s_printf("%s %i",&letrero[idioma][7][0],1+(actual_game/15));

		autocenter=0;
		bkcolor=0;
		//cnt=actual_game;
		
		cnt2=temp_game_cnt;//=actual_game;
		
		if(load_png)
			{
			make_rumble_off();
			}
		for(m=0;m<3;m++)
		for(n=0;n<5;n++)
			{
			game_datas[(m*5)+n].ind=-1;
			if(gameList==NULL) temp_game_cnt=gameCnt+10;
			else
			if(is_favorite) 
				{
				int g;
				
				if(config_file.id[(m*5)+n][0]==0)
					{
					temp_game_cnt=gameCnt+1;
					}
				else
					{
					temp_game_cnt=gameCnt+1;
					for(g=0;g<gameCnt;g++)
						{
						struct discHdr *header = &gameList[g];
						if(strncmp((char *) header->id, (char *) &config_file.id[(m*5)+n][0],6)==0)
							{
							temp_game_cnt=g;break;
							}
						}
					}

				}
        
		    if(is_favorite) cnt2=temp_game_cnt;

			if(scroll_mode && gameList!=NULL)
				{
				
				if(cnt2<gameCnt)
					{
					struct discHdr *header = &gameList[cnt2];
				
					game_datas[(m*5)+n].ind=cnt2;
					
					memcpy(discid,header->id,6); discid[6]=0;

					if(load_png)
						{
						memset(temp_data,0,256*1024);
						global_GetProfileDatas(discid, temp_data);
						create_game_png_texture(16+(m*5)+n);
						}
					cnt2++;
					}
				temp_game_cnt=game_datas[16+(m*5)+n].ind;
				if(temp_game_cnt<0) temp_game_cnt=gameCnt+1;
				}
			
			//if(temp_game_cnt>=gameCnt) temp_game_cnt=0;

			if(temp_game_cnt<gameCnt)
				{
				struct discHdr *header = &gameList[temp_game_cnt];

				if(!scroll_mode) game_datas[16*(scroll_mode!=0)+(m*5)+n].ind=temp_game_cnt;
				
				memcpy(discid,header->id,6); discid[6]=0;

				if(load_png && scroll_mode==0)
					{
					memset(temp_data,0,256*1024);
					global_GetProfileDatas(discid, temp_data);
					create_game_png_texture((m*5)+n);
					}
				letter_size(8,24);
				
				if(!(px>=20+n*120 && px<20+n*120+118 && py>=ylev+m*146 && py<ylev+m*146+144) && parental_mode==0)
					{
					PX= 26+n*120+scroll_mode; PY= 64+36+8+ylev+m*146; color= 0xff000000; 
					
					bkcolor=0xc0f0f000;

					s_printf_z=4;
					if(!game_datas[(m*5)+n].png_bmp) 
									draw_text(header->title);
					s_printf_z=0;
					bkcolor=0;
					}
				else if(scroll_mode==0)
					{
					selected_panel=(m*5)+n;
		
					}

				SetTexture(NULL);
				//DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffafafaf);
				DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0xffafafaf);

				if(game_datas[(m*5)+n].png_bmp) 
					SetTexture(&game_datas[(m*5)+n].texture);
				else SetTexture(&default_game_texture);
				
				
				//DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0xffffffff);

				if(is_fat && header->version)
					{
					letter_size(8,16);
					
					PX=20+n*120+scroll_mode+4;PY=ylev+m*146+4;color=0xff000000;bkcolor=0xffffffff;
					s_printf_z=10;
					s_printf("SD");bkcolor=0;
					s_printf_z=0;
					}

				
				SetTexture(&text_screen_fx);
				//DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0xffffffff);

				// draw lock
				if((game_datas[(m*5)+n].config>>30) & 1)
					{
					SetTexture(&text_icon[2]);
					//DrawFillBox(20+n*150+scroll_mode+124, ylev+m*110+8, 16, 24, 10, 0xffffffff);
					DrawFillBox(20+n*120+scroll_mode+100, ylev+m*146+8, 16, 24, 10, 0xffffffff);
					}

				SetTexture(NULL);

				
				set_set=1;

				temp_game_cnt++;
				}
			else
				{	
				set_set=0;
				//SetTexture(STRIPED_PATTERN);
				if(!scroll_mode) game_datas[16*(scroll_mode!=0)+(m*5)+n].ind=-1;
				SetTexture(&text_game_empty[(frames2>>2) & 3]);
				//DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0xffffffff);

				SetTexture(&text_screen_fx);
				//DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 0xffffffff);
				DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0xffffffff);
				SetTexture(NULL);
				}
			
			SetTexture(NULL);
			if(px>=20+n*120 && px<20+n*120+118 && py>=ylev+m*146 && py<ylev+m*146+144 && parental_mode==0)
				{
				unsigned color2=0xff00ff00;

				if(old_temp_sel!=(m*5)+n && old_temp_sel<1000) 
					{
					snd_fx_tick();
					if(game_datas[16*(scroll_mode!=0)+(m*5)+n].ind>=0 && !load_png)
						{
						if(rumble==0) {wiimote_rumble(1);rumble=10;}
						}
					old_temp_sel=(m*5)+n;
					}
				if(set_set || insert_favorite) temp_sel=(m*5)+n; else temp_sel=-1;
				//DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 6, 0xfff08000/*0xff00a0a0*/);

				if(game_datas[16*(scroll_mode!=0)+(m*5)+n].ind>=0)
					{
					struct discHdr *header = &gameList[game_datas[(m*5)+n].ind];

					if(game_datas[(m*5)+n].png_bmp) 
						SetTexture(&game_datas[(m*5)+n].texture);
					else SetTexture(&default_game_texture);
					
					DrawFillBox(20+n*120+scroll_mode-8, ylev+m*146-12*m, 118+16, 144+24, 0, 0xffffffff);

					SetTexture(&text_screen_fx);
					DrawFillBox(20+n*120+scroll_mode-8, ylev+m*146-12*m, 118+16, 144+24, 0, 0xffffffff);


					// draw lock
					if((game_datas[(m*5)+n].config>>30) & 1)
						{
						SetTexture(&text_icon[2]);
						//DrawFillBox(20+n*150+scroll_mode+124, ylev+m*110+8, 16, 24, 10, 0xffffffff);
						DrawFillBox(20+n*120+scroll_mode+100+4, ylev+m*146+10-12*m, 16, 24, 0, 0xffffffff);
						}

					if(is_fat && header && header->version)
					{
					letter_size(8,16);
					
					PX=20+n*120+scroll_mode-8+4;PY=ylev+m*146-12*m+4;color=0xff000000;bkcolor=0xffffffff;
					s_printf("SD");bkcolor=0;
					}

					SetTexture(NULL);
					//DrawBox(20+n*120+scroll_mode-8-4, ylev+m*146-12*m-4, 118+16+8, 144+24+8, 0, 4,0xfff08000);
					switch((frames2>>2) & 3)
						{
						case 0:
							color2=0xff00ff00;break;
						case 1:
							color2=0xfff08000;break;
						case 2:
							color2=0xff0000ff;break;
						case 3:
							color2=0xff00ffff;break;
						
						}
					DrawBox(20+n*120+scroll_mode-8-4, ylev+m*146-12*m-4, 118+16+8, 144+24+8, 0, 4,color2/*0xfff08000*/);
					}
				else
					{
					SetTexture(NULL);
					DrawBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 4, 0xfff08000);
					}
				}
			else
			    {
				//DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 10, 4, 0xff303030);
			   // DrawFillBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 0x20000000);
			    DrawBox(20+n*120+scroll_mode, ylev+m*146, 118, 144, 10, 2, 0xff303030);
				}

			// scroll mode
				if(scroll_mode<0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);

					if(game_datas[(m*5)+n].ind>=0)
						{
						if(game_datas[16+(m*5)+n].png_bmp) 
							SetTexture(&game_datas[16+(m*5)+n].texture);
						else SetTexture(&default_game_texture);
						}
					//DrawRoundFillBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 0xffffffff);
					DrawFillBox(20+n*120+scroll_mode+620, ylev+m*146, 118, 144, 0, 0xffffffff);

					if(game_datas[(m*5)+n].ind>=0)
						{
						struct discHdr *header = &gameList[game_datas[(m*5)+n].ind];
						PX= 26+n*120+scroll_mode+620; PY= 64+36+8+ylev+m*146; color= 0xff000000; 
					
						bkcolor=0xc0f0f000;
					
						if(!game_datas[16+(m*5)+n].png_bmp) 
									draw_text(header->title);
						bkcolor=0;
						}
					SetTexture(NULL);
					//DrawRoundBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					DrawBox(20+n*120+scroll_mode+620, ylev+m*146, 118, 144, 0, 4, 0xff303030);
					}
				if(scroll_mode>0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);

					if(game_datas[(m*5)+n].ind>=0)
						{
						if(game_datas[16+(m*5)+n].png_bmp) 
							SetTexture(&game_datas[16+(m*5)+n].texture);
						else SetTexture(&default_game_texture);
						}
					
					//DrawRoundFillBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 0xffffffff);
					DrawFillBox(20+n*120+scroll_mode-620, ylev+m*146, 118, 144, 0, 0xffffffff);

					if(game_datas[(m*5)+n].ind>=0)
						{
						struct discHdr *header = &gameList[game_datas[(m*5)+n].ind];
						PX= 26+n*120+scroll_mode-620; PY= 64+36+8+ylev+m*146; color= 0xff000000; 
					
						bkcolor=0xc0f0f000;
					
						if(!game_datas[16+(m*5)+n].png_bmp) 
									draw_text(header->title);
						bkcolor=0;
						}
					SetTexture(NULL);
					//DrawRoundBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					DrawBox(20+n*120+scroll_mode-620, ylev+m*146, 118, 144, 0, 4, 0xff303030);
					}

				if(!scroll_mode && !insert_favorite  && parental_mode==0)
					{
					SetTexture(NULL);
			
						if(px>=-is_16_9*80 && px<=80-is_16_9*80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							circle_select(40-is_16_9*80, ylev+220, '-', 1);
							temp_sel=-1;
							test_gfx_page=-1;

							if(old_temp_sel!=1000) 
								{
								snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
								old_temp_sel=1000;
								}
							}
						else
						if(frames2 & 32)
							{
							circle_select(40-is_16_9*80, ylev+220, '-', 0);
							}
						

					
						if(px>=SCR_WIDTH-82+is_16_9*80 && px<=SCR_WIDTH-2+is_16_9*80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							circle_select(SCR_WIDTH-42+is_16_9*80, ylev+220, '+', 1);
							temp_sel=-1;
							test_gfx_page=1;

							if(old_temp_sel!=1001) 
								{
								snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
								old_temp_sel=1001;
								}
							}
						else
						if(frames2 & 32)
							{
							circle_select(SCR_WIDTH-42+is_16_9*80, ylev+220, '+', 0);

							}

						if(old_temp_sel>=1000 && test_gfx_page==0) old_temp_sel=-1;
						
					}
			}

			if(selected_panel>=0 && scroll_mode==0  && parental_mode==0)
				{
				struct discHdr *header = &gameList[game_datas[selected_panel].ind];
				int py=ylev+((selected_panel>=10) ? 0 : 3*110+55);
				if(SCR_HEIGHT>480) py=SCR_HEIGHT-108/2;
					SetTexture(NULL);
					DrawRoundFillBox(20, py, 150*4, 108/2, 0, 0xcfcfffff);
					DrawRoundBox(20, py, 150*4, 108/2, 0, 6, 0xcff08000);
		
					if(strlen(header->title)<=37) letter_size(16,32);
					else if(strlen(header->title)<=45) letter_size(12,32);
					else letter_size(8,32);		

					PX= 0; PY=py+(54-32)/2; color= 0xff000000; 
							
					bkcolor=0;
					
					autocenter=1;
					s_printf("%s",header->title);
					autocenter=0;
				}
			else
			if(((select_game_bar>=10000 && select_game_bar<10004) || select_game_bar==10015) && parental_mode==0)
				{
					SetTexture(NULL);
					DrawRoundFillBox(20, ylev+ 3*110, 150*4, 108, 0, 0xcfcfffff);
					DrawRoundBox(20, ylev+ 3*110, 150*4, 108, 0, 6, 0xcff08000);
					letter_size(16,32);

					PX= 0; PY=ylev+ (3*110)+(108-32)/2; color= 0xff000000; 
								
					bkcolor=0;
						
					autocenter=1;
					if(select_game_bar==10015)	s_printf("Select FAT Mode");
					else
						s_printf("Select Partition #%i",select_game_bar-9999);
					autocenter=0;
					parental_control_on=1;

				}
			else if(select_game_bar==10016 && parental_mode==0)
				{

				SetTexture(NULL);

					DrawRoundFillBox(20, ylev+ 3*110, 150*4, 108, 0, 0xcfcfffff);
					DrawRoundBox(20, ylev+ 3*110, 150*4, 108, 0, 6, 0xcff08000);
					letter_size(16,32);

					PX= 0; PY=ylev+ (3*110)+(108-32)/2; color= 0xff000000; 
								
					bkcolor=0;
						
					autocenter=1;
					s_printf("Load From DVD");
					autocenter=0;

				parental_control_on=1;

				}

		
		
		} // modo panel

		//	parental_control_on=1;
//		int parental_mode=0;
		/////////////////////////
		if(parental_mode )
			{
			ylev+=32;

			SetTexture(NULL);

			DrawRoundFillBox(20+148, ylev, 148*2, 352, 0, 0xcfafafaf);

			letter_size(16,32);

			PX= 0; PY=ylev+8; color= 0xff000000; 
					
			bkcolor=0;//0xc0f08000;
			
			autocenter=1;
			s_printf("%s","Parental Control");
			autocenter=0;

            if(select_game_bar!=1)	select_game_bar=0;
			

			// change partition draw and selection
			letter_size(16,24);
			bkcolor=0;
			autocenter=0;
			
			for(n=0;n<4;n++)
				{
				int my_x=SCR_WIDTH/2-100+50*n;
				int my_y=ylev+50;

				if(parental_str[n]==0) DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0ffffff);
				else DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa000ff00);

				PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("%c", parental_str[n]==0 ? ' ' : '*');
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);

				}

			for(n=0;n<10;n++)
				{
				int my_x=SCR_WIDTH/2-75+50*((n==0) ? 1 :((n-1) % 3));
				int my_y=ylev+64+50*((n==0) ? 4 :3-((n-1)/3));
				
				DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

				PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("%c", 48+n); 

				if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
					{
					DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
					select_game_bar=20000+n;
					}
				else
					DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
			
				}

			ylev-=32;

			}
		////////////////////////////
		} 
	

	if(direct_launch && game_mode==1 && gameList!=NULL) 
		{
        PX=0;PY=(SCR_HEIGHT-440)+(352-256)/2;
		letter_size(192,256);
		switch(frames2 & 3)
			{
			case 0:
			case 2:
				color=0xff00ffff; break;
			case 1:
				color=0xff0000ff; break;
			case 3:
				color=0xff00ff00; break;	
			}

		bkcolor=0;
		autocenter=1;
		SelectFontTexture(1);
		s_printf("%i", launch_counter);
		SelectFontTexture(1);
		color=0xff000000;
		letter_size(16,24);
		bkcolor=0;
		autocenter=0;

		}

	load_png=0;
	frames2++;
	
	
		{
		if(select_game_bar>=0)
			{
			if(select_game_bar!=last_select)
				{
				snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
				last_select=select_game_bar;
				}
			}
		else last_select=-1;
		}

	if(!scroll_mode)
		{
		frames++;
		if(frames>=8) {frames=0;if(txt_cheats) scroll_text+=2; else scroll_text++;}

		wiimote_read();

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						
						time_sleep=TIME_SLEEP_SCR;
						SetVideoSleep(0);

						px=wmote_datas->ir.x-104*is_16_9;py=wmote_datas->ir.y;
						angle_icon=wmote_datas->orient.roll;

						if(insert_favorite)
							{

							SetTexture(&text_move_chan);

							//DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							DrawFillBox(px-118/2, py-144/2, 118, 144, 0, 0x8060af60);
							
							}
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						angle_icon=0.0f;

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16+104*is_16_9)) guitar_pos_x=(SCR_WIDTH-16+104*is_16_9);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16-104*is_16_9) guitar_pos_x=16-104*is_16_9;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

						if(guitar_pos_x<-104*is_16_9) guitar_pos_x=-104*is_16_9;
						if(guitar_pos_x>(SCR_WIDTH-16+104*is_16_9)) guitar_pos_x=(SCR_WIDTH-16+104*is_16_9);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
						
						if(px!=guitar_pos_x || py!=guitar_pos_y)
							{
							time_sleep=TIME_SLEEP_SCR;
							SetVideoSleep(0);
							}
						
						px=guitar_pos_x; py=guitar_pos_y;

						if(insert_favorite)
							{
							SetTexture(&text_move_chan);
							//DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							DrawFillBox(px-118/2, py-144/2, 118, 144, 0, 0x8060af60);
							}
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						}
					 else
					   {px=-100;py=-100;}
					
					if(new_pad & WPAD_BUTTON_DOWN)
						{
						config_file.rumble_off^=1;
						rumble_off_osd=1;
						if(!mode_disc) save_cfg(0);
						}

					
					if(new_pad & WPAD_BUTTON_UP)
						{
						
						icon_osd=1;
						use_icon2=(use_icon2+1) & 7;
						if(!mode_disc) save_cfg(0);
						}
					

					if((new_pad & WPAD_BUTTON_LEFT) || (volume_osd && (frames2 & 7)==0 && (old_pad & WPAD_BUTTON_LEFT)))
						{
						volume_osd=1;
						if(config_file.music_mod & 128)
							{
							if((config_file.music_mod & 15)>0)
								config_file.music_mod=((config_file.music_mod-1) & 15) | (config_file.music_mod & ~15);
							}
						else config_file.music_mod=128+15;

						#ifndef ALTERNATIVE_VERSION
						MODPlay_SetVolume( &mod_track,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16); // fix the volume to 16 (max 64)
						#else
						SetVolumeOgg((config_file.music_mod & 128) ?  ((config_file.music_mod & 15)*16): 255);
						#endif

						if(!mode_disc) save_cfg(0); //else save_cfg(1);
						}
					if((new_pad & WPAD_BUTTON_RIGHT)  || (volume_osd && (frames2 & 7)==0 && (old_pad & WPAD_BUTTON_RIGHT)))
						{
						volume_osd=1;
						if(config_file.music_mod & 128) 
							{
							if((config_file.music_mod & 15)<15)
								config_file.music_mod=((config_file.music_mod+1) & 15) | (config_file.music_mod & ~15);
							}
						else config_file.music_mod=128+15;

						#ifndef ALTERNATIVE_VERSION
						MODPlay_SetVolume( &mod_track,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16,(config_file.music_mod & 128) ?  (config_file.music_mod & 15): 16); // fix the volume to 16 (max 64)
						#else
						SetVolumeOgg((config_file.music_mod & 128) ?  ((config_file.music_mod & 15)*16): 255);
						#endif

						if(!mode_disc) save_cfg(0); //else save_cfg(1);
						}

					 if(cheat_mode && txt_cheats)
						{
						if(new_pad & WPAD_BUTTON_MINUS)
							{
								actual_cheat-=5;if(actual_cheat<0) actual_cheat=0;
							}
						
						if(new_pad & WPAD_BUTTON_PLUS)
							{
							if((actual_cheat+5)<num_list_cheats) actual_cheat+=5;
							}
						}

					 if(!mode_disc && !insert_favorite && !cheat_mode && gameList!=NULL && parental_mode==0 
						 && game_mode==0) //limit left/right
						{
						
						if(new_pad & WPAD_BUTTON_MINUS)
							{

								scroll_mode=1;
								px=py=-100;
							}
						if(new_pad & WPAD_BUTTON_PLUS)
							{
								scroll_mode=-1;
								px=py=-100;
							
							}
						} // limit left_right

 
                   
					if((new_pad & WPAD_BUTTON_A) && 
						(gameList!=NULL || (mode_disc && game_mode) || (mode_disc==0 && select_game_bar>=10000 && select_game_bar<=10016)))
						{
						if(parental_mode && select_game_bar!=1/*(!mode_disc || (mode_disc && select_game_bar!=1))*/)
							{
							direct_launch=0;
							if(select_game_bar>=20000 && select_game_bar<20010)
								{
								for(n=0;n<4;n++) if(parental_str[n]==0) break;
								if(n<4) {parental_str[n]=select_game_bar+48-20000;snd_fx_yes();}
								if(n>=4) {
									     for(n=0;n<4;n++)
											{
											if(config_file.parental[n]!=(parental_str[n]-48)) break;
											}
										  if(select_game_bar+48-20000=='0') n++;
									      if(n==5)
											{
											parental_control_on=0;
											snd_fx_yes();
											if(parental_mode==9999);
											else
											if(parental_mode>=1024) game_mode=parental_mode-1024;
											else go_home=1;

											parental_mode=0;
											}
										  else
											{
											for(n=0;n<4;n++) parental_str[n]=0;
											parental_control_on++;
											snd_fx_no();
											if(game_mode==0) parental_mode=0;
											if(parental_control_on>=5) {sleep(3);exit_by_reset=3; goto exit_ok;}
											}		  
										  
										  }
								select_game_bar=0;
								}
							}
						else
							{// q
						if(test_gfx_page)
							{ // w
							 if(cheat_mode && txt_cheats)
								{
								if(test_gfx_page<0)
									{
										actual_cheat-=5;if(actual_cheat<0) actual_cheat=0;
										snd_fx_yes();
									}
								if(test_gfx_page>0)
									{
										actual_cheat+=5;if(actual_cheat>=num_list_cheats) actual_cheat=num_list_cheats-5;
										snd_fx_yes();
									}
								}
							else
							if(insert_favorite==0)
								{
								if(gameList!=NULL)
									{
									if(test_gfx_page<0)
										{scroll_mode=1;px=py=-100;snd_fx_yes();}
									if(test_gfx_page>0)
										{scroll_mode=-1;px=py=-100;snd_fx_yes();}
									}
								}
							}// w
						else
						if(game_mode==0)
							{// r
							if((old_pad & WPAD_BUTTON_B) && is_favorite)
								{
								if(temp_sel>=0)
									{
									insert_favorite=game_datas[temp_sel].ind+1;
									snd_fx_yes();

									mem_move_chan=NULL;

									if(game_datas[temp_sel].png_bmp) 
										memcpy(&text_move_chan, &game_datas[temp_sel].texture, sizeof(GXTexObj));
									else
									memcpy(&text_move_chan, &default_game_texture, sizeof(GXTexObj));

									
									}
								}
							else
								{
								// insert game in favorites
								if(insert_favorite)
									{
								
									struct discHdr *header = &gameList[insert_favorite-1];
									int n,m=0;

									if(temp_sel>=0)
										{
										for(n=0;n<15;n++)
											{
											if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
												{
												if(m==0)
													{m=1;memcpy(&config_file.id[n][0],&config_file.id[temp_sel][0],8);config_file.id[n][6]=0;memset(&config_file.id[temp_sel][0],0,8);}
												else {memset(&config_file.id[n][0],0,8);} // elimina repeticiones

												break;
												}
											}
										snd_fx_yes();
										memcpy(&config_file.id[temp_sel][0],header->id,6);
										config_file.id[temp_sel][6]=0;
										insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;
										temp_sel=-1;
										for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
										if(!mode_disc) save_cfg(0); else save_cfg(1);
										load_png=1;
	
										}
									}
								else
									{
									int n;
									struct discHdr *header;
									if(!is_favorite) last_game=actual_game;

									if(select_game_bar==10016)  {is_fat=0;load_cfg(1);parental_control_on=1;snd_fx_yes();if(!(mode_disc & 2)) mode_disc=1;Screen_flip();goto get_games;}
									if(select_game_bar==10015)  {parental_control_on=1;snd_fx_yes();is_fat=1;Screen_flip();goto get_games;}
									

									if(select_game_bar>=10000 && select_game_bar<10004)  
										{is_fat=0;snd_fx_yes();current_partition=select_game_bar-10000;Screen_flip();goto get_games;}

									if(temp_sel>=0)
									     snd_fx_yes();
									else snd_fx_no();

									game_mode=temp_sel+1;

									header = &gameList[game_datas[game_mode-1].ind];
									
									if(is_fat)
										{
										f_size= ((float) header->magic)/ GB_SIZE;
										}
								    else
										if(WBFS_GameSize(header->id, &f_size)) f_size=0.0f;

									if(is_favorite) test_favorite=0; else test_favorite=1;
									
									for(n=0;n<15;n++)
										{
										if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
											{
											if(!is_favorite) test_favorite=0; else test_favorite=1;
											break;
											}
										}
									
									
									
									// game is locked
									if(((game_datas[game_mode-1].config>>30) & 1) && parental_control_on)
										{
											parental_mode=game_mode+1024;
											game_mode=0; // block game_mode, active parental_mode
										}
								

									}
								}
							}
						else
							{
							if(select_game_bar==1) {int n;  // return
													int f_j=0;
													snd_fx_no();

													if(mode_disc && game_mode && !edit_cfg) 
														{
														remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
														mode_disc&=~1;f_j=1;parental_control_on=1;parental_mode=0;
														if(dvd_only) is_fat=it_have_fat;
														}
													if(!(mode_disc & 1) && game_mode && !edit_cfg) parental_mode=0;

													game_mode=0;edit_cfg=0;select_game_bar=0;
													for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
													load_png=1;
													direct_launch=0;
													rumble=0;
													last_select=select_game_bar;
													make_rumble_off();
													if(f_j) {Screen_flip();save_cfg(1);goto get_games;}
													} 

							if(select_game_bar==2) 
												  { // Configuration (load)
							                       struct discHdr *header;

												   if(!mode_disc)
														header= &gameList[game_datas[game_mode-1].ind];
												   else header= disc_header;

												   direct_launch=0;

												   memcpy(discid,header->id,6); discid[6]=0;

												   snd_fx_yes();

												   select_game_bar=0;
												   
												   memset(temp_data,0,256*1024);

												   if(!mode_disc || is_fat)
													  {
													  global_GetProfileDatas(discid, temp_data);
													  }
													 else
													   dol_GetProfileDatas(discid, temp_data);


												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
												   
												   #ifndef DONT_USE_IOS249
												   if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; 
												   else { if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}
												   #else
												   /*if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;
                                                   game_datas[game_mode-1].config &=~1;
												   */
												   if((game_datas[game_mode-1].config & 1)) cios=224; 
												   else { if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}
												   #endif

												   forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

												   langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

												   nand_mode=(game_datas[game_mode-1].config>>8) & 15;

												   force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
												   game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);

												   bca_mode=(game_datas[game_mode-1].config>>29) & 1;
												   hook_selected= ((game_datas[game_mode-1].config>>24) & 7)+1;

												   edit_cfg=1;
												   }

							if(select_game_bar==3) {int n;// add favorite
													direct_launch=0;
													select_game_bar=0;is_favorite=1;insert_favorite=game_datas[game_mode-1].ind+1;

													snd_fx_yes();

													mem_move_chan=game_datas[game_mode-1].png_bmp;

													if(game_datas[game_mode-1].png_bmp) 
														memcpy(&text_move_chan, &game_datas[game_mode-1].texture, sizeof(GXTexObj));
													else
														memcpy(&text_move_chan, &default_game_texture, sizeof(GXTexObj));

													game_datas[game_mode-1].png_bmp=NULL;
													
													game_mode=0;edit_cfg=0;
													
												    for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												    save_cfg(0);
												    load_png=1;
													} 

							if(select_game_bar==4) {// del favorite
												   int n;
												   direct_launch=0;
												   select_game_bar=0;is_favorite=1; 

												   snd_fx_yes();

												   memset(&config_file.id[game_mode-1][0],0,8);
												   game_mode=0;edit_cfg=0;
												   for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   save_cfg(0);
												   load_png=1;
												   }

							if(select_game_bar==55)
													{
													struct discHdr *header;

													if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}

													if(!mode_disc)
														header= &gameList[game_datas[game_mode-1].ind];
												    else header= disc_header;

													direct_launch=0;
												    select_game_bar=0;//is_favorite=1; 

												    snd_fx_yes();

													menu_alternativedol(header->id);
													old_pad|=new_pad;
													new_pad=0;
//													new_pad &=~ WPAD_BUTTON_A | WPAD_BUTTON_B;

													if(mode_disc) {remote_call(remote_DVD_disc);usleep(1000*50);}

													}
							if(select_game_bar==56)
													{
													show_bca^=1;
													if((show_bca & 1) && bca_status_read==1) bca_status_read=2;
								
													}
							if(select_game_bar==57)
													{
													if(show_bca) update_bca(discid, BCA_Data);
													}

							if(select_game_bar>=500 && select_game_bar<500+MAX_LIST_CHEATS)
													{
													snd_fx_yes();
													data_cheats[select_game_bar-500].apply^=1;
													}
							if(select_game_bar>=30 && select_game_bar<38)
													{

													snd_fx_yes();
													ciso_sel=select_game_bar-30;

													remote_DVD_disc_status=3;
													
													}

							if(select_game_bar==5 || select_game_bar==1000 || select_game_bar==1001) 
													{
													struct discHdr *header;

													if(((nand_mode & 1) && !sd_ok) || ((nand_mode & 2) && !ud_ok)) 
														{
														warning_osd=1;
													    snd_fx_no();
														goto ignore_all;
														}
													
													

													if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}

													if(!mode_disc)
														header= &gameList[game_datas[game_mode-1].ind];
												    else header= disc_header;

													direct_launch=0;

													snd_fx_yes();

													memcpy(discid,header->id,6); discid[6]=0;
													
													if(select_game_bar==5)
														if(load_cheats(discid)) cheat_mode=1;

													if(select_game_bar>=1000) cheat_mode=0;
													
													set_cheats_list(discid);
													if(select_game_bar==1000) create_cheats();

													
													
													if(select_game_bar==1001) len_cheats=0; // don't apply cheats
						
													select_game_bar=0;

													if(!cheat_mode)
														{
														select_game_bar=0;
														Screen_flip();
														
														ret=load_game_routine(discid, game_mode | (0x800000 *(mode_disc!=0)) | (0x200000 * (( mode_disc & 2)!=0) ) );
                           
														//////////////////////////////////
														remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
														
													
														draw_background();
			
												
														SelectFontTexture(1); // selecciona la fuente de letra extra

														letter_size(8,32);
																
														PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
																
														bkcolor=0;
														autocenter=1;
														SetTexture(NULL);

														DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 0, 0xa00000ff);
														DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 0, 4, 0xa0000000);
                                                       
													    if(ret==555)
															s_printf("ERROR: fail in FAT Module\n");
														else
														if(ret==101)
															s_printf("You can't use cIOS 249 with multiple partitions!!!\n");
														else
														if(ret==-7777)
															s_printf("ERROR: You need cIOS223 from uLoader 3.0 v4 to work!!!\n");
														else
														if(ret==666)s_printf("ERROR FROM THE LOADER: Disc ID is not equal!\n"); 
														else 
															s_printf("ERROR FROM THE LOADER: %i\n", ret);

														Screen_flip();
														sleep(3);

														//////////////////////////////////
														break;
														}
													ignore_all: ;
												   } // load game
							// josete2k
                            if(select_game_bar==6) {if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}
							                         exit_by_reset=return_reset; // fixed by hermes //SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
													}

							if(select_game_bar==9)  {if(edit_cfg==1) edit_cfg=2; else edit_cfg=1;snd_fx_yes();}
							
							// BCA from disc (offset 0x100)
							if(select_game_bar==20)  {bca_mode&=128;snd_fx_yes();}
							// BCA from database (sd:/bca_database.txt)
							if(select_game_bar==21)  {bca_mode=1;snd_fx_yes();make_rumble_off();if(!get_bca(discid, BCA_Data)) bca_mode=128;}

							// Grab BCA from database (WBFS Only)
							if(select_game_bar==22)  {
													 make_rumble_off();
								                     
													 memset(BCA_Data,0,64);

													 if(get_bca(discid, BCA_Data))
														{  
														if(global_SetBCA(discid, BCA_Data))
															{
															snd_fx_yes();bca_mode=0;bca_saved=1;
															}
														else {snd_fx_no();bca_saved=128;}
														} 
													 else {snd_fx_no();bca_saved=128;bca_mode=128;}

													 }
							// hooktype selection
							if(select_game_bar==25)  {
													 hook_selected--; if(hook_selected<1) hook_selected=8;
													 }
							if(select_game_bar==26)  {
													 hook_selected++; if(hook_selected>8) hook_selected=1;
													 }

							// nand ->60 sd->61 usb ->62 nand export -> 63 <<->64 >> -> 65 delete ->66 (nand_mode)

							// NAND SAVES (emulation off)
							if(select_game_bar==60)  {
													 save_exist=-1;
													 nand_mode&=~3;
													 snd_fx_yes();
													 }
							// SD SAVES
							if(select_game_bar==61)  {
													 save_exist=0;
													 nand_mode= (nand_mode & 0xc) | 1;
													 snd_fx_yes();
													 }
							// USB SAVES
							if(select_game_bar==62)  {
													 save_exist=0;
													 nand_mode=(nand_mode & 0xc) | 2;
													 snd_fx_yes();
													 }
							 // nand export
							if(select_game_bar==63 && (nand_mode & 3))  
													 {
													 struct discHdr *header;
											
														if(!mode_disc)  header= &gameList[game_datas[game_mode-1].ind];
														else header= disc_header;
														
														if(header)
															{
															edit2_mode=2;

															create_FAT_FFS_Directory(header);

															if(test_FAT_game(get_FAT_directory1()) || test_FAT_game(get_FAT_directory2()))
																warning_message="Save Found in FAT. Are You Sure Rewrite It?";
															else
																warning_message="#Save in FAT is Empty. Press YES to Start";
															}

													 warning_time=90;
													 snd_fx_yes();
													 }
							 // NAND FOLDER --                
							if(select_game_bar==64)  {
													 save_exist=0;
													 nand_mode=((nand_mode-4) & 0xc) | (nand_mode & 3);
													 snd_fx_yes();
													 }
							// NAND FOLDER ++						
							if(select_game_bar==65)  {
													 save_exist=0;
													 nand_mode=((nand_mode+4) & 0xc) | (nand_mode & 3);
													 snd_fx_yes();
													 }
							if(select_game_bar==66 && (nand_mode & 3))
													 {
													 struct discHdr *header;

														if(!mode_disc)  header= &gameList[game_datas[game_mode-1].ind];
														else header= disc_header;

														if(header)
															{
															create_FAT_FFS_Directory(header);
															if(test_FAT_game(get_FAT_directory1()) || test_FAT_game(get_FAT_directory2()))
																{
																edit2_mode=1;

																warning_message="Save Found. Are You Sure To Delete It?";
																warning_time=90;
																snd_fx_yes();
																}
															else
																{
																warning_message="#No Saves To Delete";
																warning_time=90;
																snd_fx_no();
																}
															}
													 // delete saves
													 
													 }
							// real delete save
							if(select_game_bar==67)  {
										
													 char * name="nononono";

                                                     edit2_mode=0;
													 save_exist=0;
													 
														
														if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}

														if(nand_mode & 1)
															{down_mess="Deleting Save From SD";}
														if(nand_mode & 2)
															{down_mess="Deleting Save From USB";}
														
														if(test_FAT_game(get_FAT_directory1())) name=get_FAT_directory1();
														else	
															if(test_FAT_game(get_FAT_directory2())) name=get_FAT_directory2();

														down_frame=0;
														remote_call(down_uload_gfx);usleep(1000*50);

													 if(FAT_DeleteDir(name) ||  remove(name))
														{
														warning_message="Error Deleting Save";
														snd_fx_no();
														}
													 else
														{
														warning_message="#Save Deleted";
														snd_fx_yes();
														}
													 warning_time=90;
													 remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
													 down_mess="";
													 down_frame=-1;
													
													 if(mode_disc) {remote_call(remote_DVD_disc);usleep(1000*50);}
													 }

							 // real nand export
							if(select_game_bar==68)  {
													
													 int ret=-123456;

													 edit2_mode=0;
													 save_exist=0;
														
														if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}

														if(nand_mode & 1)
															{	
															down_mess="Exporting Datas From NAND To SD";
															}
														if(nand_mode & 2)
															{
															down_mess="Exporting Datas From NAND To USB";
															}

														down_frame=0;
														remote_call(down_uload_gfx);usleep(1000*50);
	
														ret= FFS_to_FAT_Copy(get_FFS_directory1(), get_FAT_directory1());
														if(ret==-101)
															{
															ret= FFS_to_FAT_Copy(get_FFS_directory2(), get_FAT_directory2());
															}
														
														if(ret==0)
															{
															warning_time=90;
															snd_fx_yes();
															}
														else
														if(ret<0)
															{
															warning_time=90;
															snd_fx_no();
															}

														switch(ret)
															{
															case 0:
																warning_message="#Save Copied";
																break;
															case -123456:
																warning_message="Stupid Monkey Error";
																break;
															case -101:
																warning_message="No Save Found";
																break;
															case -1:
																warning_message="Invalid Path";
																break;
															case -2:
																warning_message="Out Of Memory";
																break;
															case -3:
																warning_message="Error In Read Dir";
																break;
															case -4:
																warning_message="Error Opening FFS File";
																break;
															case -5:
																warning_message="Error Creating FAT File";
																break;
															case -6:
																warning_message="Error Seeking FFS File";
																break;
															case -7:
																warning_message="Error Reading Datas";
																break;
															case -8:
																warning_message="Error Writing Datas";
																break;
															}


													 remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
													 down_mess="";
													 down_frame=-1;
													
													 if(mode_disc) {remote_call(remote_DVD_disc);usleep(1000*50);}
													 }
							// cancel delete						 

							if(select_game_bar==69)  {edit2_mode=0;warning_time=0;snd_fx_no();}

							if(select_game_bar==10 || select_game_bar==11) { // return from config (saving or not)
												   edit_cfg=0;

												   if(select_game_bar==10) snd_fx_yes(); else snd_fx_no();
												   // si no existe crea uno
												   if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
														{
														temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
														temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
														temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
														}
													
													
												   temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
												   if(select_game_bar==11) 
														{
													    temp_data[4]=game_datas[game_mode-1].config & 255;
														temp_data[5]=(game_datas[game_mode-1].config>>8) & 255;
														temp_data[6]=(game_datas[game_mode-1].config>>16) & 255;
														temp_data[7]=(game_datas[game_mode-1].config>>24) & 255;
														}
												   else
														{
														if(cios==249 || cios==224) temp_data[4]|=1;
														if(cios==223) temp_data[4]|=2;
														temp_data[4]|=(forcevideo & 3)<<2;
														temp_data[4]|=(langsel & 15)<<4;
														temp_data[5]|=(nand_mode & 15);
														temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;
														temp_data[7]|=((game_locked_cfg!=0) & 1)<<6;
														temp_data[7]|=(bca_mode & 1)<<5;
														temp_data[7]|= (hook_selected-1) & 7;
											
														}

												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
												   select_game_bar=0;
												   make_rumble_off();
												   if(select_game_bar!=11)
														{
														if(!mode_disc)
															global_SetProfileDatas(discid, temp_data);
														else 
															dol_SetProfileDatas(discid, temp_data);
														}
												   
												   }
							if(select_game_bar>=100 && select_game_bar<=111)
												   {
												   langsel= select_game_bar-100;
												   snd_fx_yes();
												   }
							if(select_game_bar>=200 && select_game_bar<=203)
												   {
												   forcevideo= select_game_bar-200;
												   snd_fx_yes();
												   }

							if(select_game_bar==210)
												   {
												   game_locked_cfg=(game_locked_cfg==0);
												   snd_fx_yes();usleep(250);
												   }
							if(select_game_bar>=300 && select_game_bar<=302)
												   {
												   switch(select_game_bar-300)
													   {
													   case 0:
														   cios=222;break;
													   case 1:
														   cios=223;break;
													   case 2:
														   #ifndef DONT_USE_IOS249
														   cios=249;break;
														   #else
														   cios=224;break;
														   #endif
													   }
												   //cios= (select_game_bar-300) ? 249 : 222;
												   snd_fx_yes();
												   }
							if(select_game_bar==303)
												   {
												   force_ingame_ios=(force_ingame_ios==0);
												   snd_fx_yes();
												   }
												   

							}
							} // q
						
						}

					
					if(new_pad & WPAD_BUTTON_B)
						{
						
						int n;

						direct_launch=0;
						
						if(parental_mode)
							{
							for(n=0;n<4;n++) parental_str[n]=0;
							snd_fx_no();
							if(parental_mode==1) go_home=1;
							parental_mode=0;
							}
						if(cheat_mode)
							{
							}
						else
						if(edit_cfg)
							{// return from config (no saving)

							if(edit_cfg>1) 
								{
								if(edit2_mode) edit2_mode=0;
								else edit_cfg=1;
								snd_fx_no();
								}
							else
								{
								edit_cfg=0;
								snd_fx_yes();
							
							
								// si no existe crea uno
								if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
									{
									temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
									temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
									temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
									}
								
								temp_data[4]=game_datas[game_mode-1].config & 255;
								temp_data[5]=(game_datas[game_mode-1].config>>8) & 255;
								temp_data[6]=(game_datas[game_mode-1].config>>16) & 255;
								temp_data[7]=(game_datas[game_mode-1].config>>24) & 255;

								/*
								
								temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
								if(cios==249 || cios==224) temp_data[4]|=1;
								if(cios==223) temp_data[4]|=2;
								temp_data[4]|=(forcevideo & 3)<<2;
								temp_data[4]|=(langsel & 15)<<4;
								temp_data[5]|=(nand_mode & 15);
								temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;
								temp_data[7]|=((game_locked_cfg!=0) & 1)<<6;
								temp_data[7]|=(bca_mode & 1)<<5;
								temp_data[7]|= (hook_selected-1) & 7;
								*/

								game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
								/*
								if(!mode_disc || is_fat)
									global_SetProfileDatas(discid, temp_data);
								else
									dol_SetProfileDatas(discid, temp_data);*/
								}
							}
						else
							{
							if((!(mode_disc & 1024) || it_have_fat) && remote_DVD_disc_status!=1)
								{
								int f_j=0;

								if(mode_disc && game_mode && !edit_cfg) 
									{
									remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
									//mode_disc=0;f_j=1;parental_control_on=1;parental_mode=0;
									mode_disc&=~1;f_j=1;parental_control_on=1;parental_mode=0;
									if(dvd_only) is_fat=it_have_fat;
									//is_fat=it_have_fat;
									}
								if(!mode_disc && game_mode && !edit_cfg) parental_mode=0;

								

								if(game_mode)
									{
									snd_fx_no();
									for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
														   
									load_png=1;
									}
								else
									if(insert_favorite) snd_fx_no();
								
								if(is_favorite)
									{
									int flag=0,g;

									if(gameList)
									for(n=0;n<15;n++)
										{
											if(config_file.id[n][0]!=0)	
												{
												for(g=0;g<gameCnt;g++)
													{
													struct discHdr *header = &gameList[g];
													if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
														{
														flag=1;break;
														}
													 }
												}
										}
									
									if(!flag) 
										{is_favorite=0;
										for(n=0;n<15;n++) {game_datas[n+16].ind=game_datas[n].ind;game_datas[n+16].png_bmp=NULL;}
										load_png=1;
										}
									}
					
                               
								game_mode=0;edit_cfg=0;
								insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;

								if(f_j) {Screen_flip();save_cfg(1);goto get_games;}
								}
							}
						}
               
					if((new_pad & WPAD_BUTTON_1) && parental_mode==0 && !mode_disc && game_mode==0) // swap favorites/page
						{
						int n;

						scroll_text=-20;

						if(!mode_disc && !insert_favorite)
							{
							if(is_favorite)
								{
								is_favorite=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}
								}
							else
								{
								last_game=actual_game;is_favorite=1;
								}
							for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
							load_png=1;
							}
						
						}
					// launch pintor game
					if((new_pad & WPAD_BUTTON_2) && (game_mode==0 || (game_mode!=0 && mode_disc && !edit_cfg)) && insert_favorite==0 && scroll_mode==0 && parental_mode==0) go_game=1;

					if((new_pad & WPAD_BUTTON_HOME) && (game_mode==0 || (game_mode!=0 && mode_disc && (disc_header || (!disc_header && remote_DVD_disc_status==0)) && !edit_cfg)) && insert_favorite==0 && scroll_mode==0 && parental_mode==0) 
						{//go_home=1;
					    temp_home=-1;
						if(parental_control_on)
										{
										parental_mode=1; if(game_mode==0) temp_home=temp_sel;
										}
									else {go_home=1; if(game_mode==0) temp_home=temp_sel;}
						}
							
			
			} 
			else {px=-100;py=-100;}
		}

	if(direct_launch && game_mode==1 && gameList!=NULL) 
		{

		if(launch_counter<=0)
			{
			struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

			if(((nand_mode & 1) && !sd_ok) || ((nand_mode & 2) && !ud_ok)) 
				{
				warning_osd=1;
				snd_fx_no();
				direct_launch=0;
				goto ignore_all2;
				}
													

			snd_fx_yes();

			memcpy(discid,header->id,6); discid[6]=0;
														
			if(load_cheats(discid)) cheat_mode=1;

			set_cheats_list(discid);
			create_cheats();


			
			Screen_flip();											
			ret=load_game_routine(discid, game_mode);

			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

			draw_background();


			SelectFontTexture(1); // selecciona la fuente de letra extra

			letter_size(8,32);
					
			PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);

			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			if(ret==555)
				s_printf("ERROR: fail in FAT Module\n");
			else
			if(ret==101)
				s_printf("You can't use cIOS 249 with multiple partitions!!!\n");
			else
			if(ret==-7777)
				s_printf("ERROR: You need cIOS223 from uLoader 2.6 v3 to work!!!\n");
			else
			if(ret==666) s_printf("ERROR FROM THE LOADER: Disc ID is not equal!\n"); 
			else 
				s_printf("ERROR FROM THE LOADER: %i\n", ret);

			Screen_flip();
			sleep(3);


			//////////////////////////////////
			break;
			}
		ignore_all2: ;
		//
		}
	else direct_launch=0;

	if(scroll_mode) // scroll mode
		{
		if(scroll_mode==-1 || scroll_mode==1)
			{
			int n,g;
			if(scroll_mode<0)
				{
				if(is_favorite) {is_favorite=0;actual_game=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}} 
				else {actual_game+=15;if(actual_game>=gameCnt) {actual_game=0;is_favorite=1;}}
				if(is_favorite)
					{
					int flag=0;
					for(n=0;n<15;n++)
							if(config_file.id[n][0]!=0)	
								{
								for(g=0;g<gameCnt;g++)
									{
									struct discHdr *header = &gameList[g];
									if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
										{
										flag=1;break;
										}
									 }
								}
					if(!flag) {is_favorite=0;}
					}
				}
			if(scroll_mode>0)
				{
				
				if(!is_favorite)
					{
					actual_game-=15; 
				    if(actual_game<0) 
						{
						int flag=0;
						last_game=-1;is_favorite=1;actual_game=0;
					    for(n=0;n<15;n++)
							if(config_file.id[n][0]!=0)	
								{
								for(g=0;g<gameCnt;g++)
									{
									struct discHdr *header = &gameList[g];
									if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
										{
										flag=1;break;
										}
									 }
								}
						if(!flag) {actual_game=((gameCnt-1)/15)*15;is_favorite=0;}
						}
					}
					else {actual_game=((gameCnt-1)/15)*15;is_favorite=0;}
				}

			//scroll_mode=0;
			scroll_text=-20;

			//for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
			for(n=0;n<15;n++) {game_datas[n+16].ind=game_datas[n].ind;game_datas[n+16].png_bmp=NULL;}
			load_png=1;
			}

		if(scroll_mode<0) scroll_mode-=32;
		if(scroll_mode>0) scroll_mode+=32;

		if((scroll_mode & 64)==0) snd_fx_scroll();//usleep(1000*100);

		if(scroll_mode<-639 || scroll_mode>639)
			{
			scroll_mode=0;
			for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);
			                  /*game_datas[n].png_bmp=NULL;*/
							  game_datas[n+16].ind=-1;
							  game_datas[n].png_bmp=game_datas[n+16].png_bmp;
							  game_datas[n].texture=game_datas[n+16].texture;
							  game_datas[n].config=game_datas[n+16].config;
							  game_datas[n+16].png_bmp=NULL;
							  }
			}
		}

	if(load_png)
			{
		    #ifndef ALTERNATIVE_VERSION
			SelectFontTexture(0); // selecciona la fuente de letra normal

			letter_size(32,64);
			#else

			SelectFontTexture(1); // selecciona la fuente de letra normal

			letter_size(16,32);

			#endif
															
			PX=0; PY= SCR_HEIGHT/2-32; color= 0xcf000000; bkcolor=0;
															
			autocenter=1;
			SetTexture(NULL);

			#ifndef ALTERNATIVE_VERSION
			DrawRoundFillBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 0xcf00ff00);
			DrawRoundBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 4, 0xcf000000);
			#else
            DrawRoundFillBox(SCR_WIDTH-400, SCR_HEIGHT/2-40, 160, 48, 0, 0xcf3399ff);
			DrawRoundBox(SCR_WIDTH-400, SCR_HEIGHT/2-40, 160, 48, 0, 4, 0xcf000000);
			#endif

			s_printf("Loading");
			autocenter=0;
			draw_snow();
			Screen_flip();

			//make_rumble_off();
			
			SelectFontTexture(0);
			if(scroll_mode) {volume_osd=0;rumble_off_osd=0;icon_osd=0;warning_osd=0;}
			}
	else
		{

		if(scroll_mode) {volume_osd=0;rumble_off_osd=0;icon_osd=0;warning_osd=0;}

		if(warning_osd)
			{
			letter_size(10, 32);
				
				warning_osd++;												
				if(warning_osd>60) warning_osd=0;
				
				SetTexture(NULL);
				DrawRoundFillBox((SCR_WIDTH-620)/2, SCR_HEIGHT/2-30, 620 , 40, 0, 0xcfffffff);
				PX=0; PY= SCR_HEIGHT/2-30+4; color= 0xff000000; bkcolor=0x0;
				autocenter=1;
				if(nand_mode & 1)
					s_printf("You Have Selected Emulation of NAND From SD without One!!!");
				else
				if(nand_mode & 2)
					s_printf("You Have Selected Emulation of NAND From USB without One!!!");
				else s_printf("You Have Selected One Shit!!!");
				autocenter=0;
				bkcolor=0;
				DrawRoundBox((SCR_WIDTH-620)/2, SCR_HEIGHT/2-30, 620 , 40, 0, 2, 0xcf000000);
			}
		else
		if(rumble_off_osd)
			{
				rumble_off_osd++;
				if(rumble_off_osd>60) rumble_off_osd=0;
				letter_size(8,16);
																
				
				SetTexture(NULL);
				DrawRoundFillBox((SCR_WIDTH-240)/2, SCR_HEIGHT/2-8, 240 , 16, 0, 0xcfffffff);
				PX=0; PY= SCR_HEIGHT/2-8; color= 0xff000000; bkcolor=0x0;
				autocenter=1;
				if(config_file.rumble_off) s_printf("Rumble Off"); else s_printf("Rumble On");
				autocenter=0;
				bkcolor=0;
				DrawRoundBox((SCR_WIDTH-248)/2, SCR_HEIGHT/2-12, 248, 24, 0, 2, 0xcf000000);
			}
		else
		if(icon_osd)
			{
				icon_osd++;
				if(icon_osd>60) icon_osd=0;
				letter_size(8,16);
																
				
				SetTexture(NULL);
				DrawRoundFillBox((SCR_WIDTH-240)/2, SCR_HEIGHT/2-8, 240 , 16, 0, 0xcfffffff);
				PX=0; PY= SCR_HEIGHT/2-8; color= 0xff000000; bkcolor=0x0;
				autocenter=1;
				s_printf("Icon Changed");
				autocenter=0;
				bkcolor=0;
				DrawRoundBox((SCR_WIDTH-248)/2, SCR_HEIGHT/2-12, 248, 24, 0, 2, 0xcf000000);
			}
		else
		if(volume_osd)
			{
				volume_osd++;
				if(volume_osd>60) volume_osd=0;
				letter_size(8,16);
																
				
				SetTexture(NULL);
				DrawRoundFillBox((SCR_WIDTH-240)/2, SCR_HEIGHT/2-8, 240 , 16, 0, 0xcfffffff);
				DrawRoundFillBox((SCR_WIDTH-240)/2, SCR_HEIGHT/2-8, 240 * (config_file.music_mod & 15)/15, 16, 0, 0xcf0000ff);
				PX=0; PY= SCR_HEIGHT/2-8; color= 0xff000000; bkcolor=0x0;
				autocenter=1;
				s_printf("Volume");
				autocenter=0;
				bkcolor=0;
				DrawRoundBox((SCR_WIDTH-248)/2, SCR_HEIGHT/2-12, 248, 24, 0, 2, 0xcf000000);
			}

		draw_snow();
		Screen_flip();
		}

	if(go_game)
		{
		rumble=0;
	
        hiscore=config_file.hi_score;
		guMtxIdentity(modelView);
		GX_LoadPosMtxImm(modelView,	GX_PNMTX0); // carga la matriz mundial como identidad
		GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz



		if(is_16_9)
			{
			ChangeProjection(0,SCR_HEIGHT<=480 ? -12: 0,848,SCR_HEIGHT+(SCR_HEIGHT<=480 ? 16: 0));
			guMtxIdentity(modelView);

			GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz
			guMtxTrans(modelView, 104.0f, 0.0f, 0.0f);
			GX_LoadPosMtxImm(modelView,	GX_PNMTX0); // carga la matriz mundial como identidad
			
			}
		else
			ChangeProjection(0,SCR_HEIGHT<=480 ? -12: 0,SCR_WIDTH,SCR_HEIGHT+(SCR_HEIGHT<=480 ? 16: 0));
		
		pintor();
		config_file.hi_score=hiscore;
		
		if(!mode_disc) save_cfg(0); else save_cfg(1);

		go_game=0;
		}

	if(go_home)
		{
		struct discHdr *header =NULL;

		if(!mode_disc)
			{
			if(temp_home>=0)
				{//
				int g;

				g=game_datas[temp_home].ind;
				if(g>=0)
					{
					header = &gameList[g];
					}

				}//
			}
		else header=disc_header;

		if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}

		n=home_menu(header);

		if(mode_disc) {remote_call(remote_DVD_disc);usleep(1000*50);}

		if(n==0)
			{
			if(parental_control_on)
				{
				for(n=0;n<4;n++) parental_str[n]=0;
				parental_control_on=1;
				}
			}
		if(n==1) {exit_by_reset=return_reset;break;}
		if(!mode_disc)
			{
			if(n==2) {Screen_flip();goto get_games;}
			if(n==3) {load_png=1;}
			}
		go_home=0;
		}

	if(exit_by_reset)  
		{	
		break;
		}

	}// while
	return 0;
exit_ok:
	return 1;
error_w:
	return 2;

return 0;
}
