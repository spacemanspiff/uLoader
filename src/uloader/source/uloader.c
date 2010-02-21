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
#include <stdlib.h>
#include <unistd.h>

#include "libpng/pngu/pngu.h"
#include "libwbfs/libwbfs.h"
#include "libwbfs/wiiscrubber.h"
#include "wbfs.h"
#include "usbstorage2.h"

#include <wiiuse/wpad.h>
#include <sdcard/wiisd_io.h>

#include "patchcode.h"
#include "kenobiwii.h"
#include "wdvd.h"
#include "disc.h"

#include <network.h>
#include "remote.h"


#define MEM_PRINT

int test_mode=0;

#define CIOS 222

#include <stdarg.h>
#include <stdio.h>

#include "screen.h"
#include <fat.h>
#include <math.h>
#include "http.h"

#include "gfx.h"

#ifndef ALTERNATIVE_VERSION

#include "resources/defpng.h"
#include "resources/button1.h"
#include "resources/button2.h"
#include "resources/button3.h"

#include "resources/icon.h"

#else

#include "resources_alt/defpng.h"
#include "resources_alt/button1.h"
#include "resources_alt/button2.h"
#include "resources_alt/button3.h"

#include "resources_alt/icon.h"

#endif

#include "files.h"

#ifndef USE_PORT1

#include "port0/ehcmodule.h"

#else

#include "port1/ehcmodule.h"

#endif
#include "dip_plugin.h"
#include "mload.h"

extern unsigned hiscore;
extern int pintor();


#define ticks_to_msecs(ticks)      ((u32)((ticks)/(TB_TIMER_CLOCK)))

u32 gettick();




//#include "logmodule.h"

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

u8 BCA_Data[64] ATTRIBUTE_ALIGN(32);
int BCA_ret=-1;

int show_bca=0;

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

int force_reload_ios222=0;
u32 patch_datas[8] ATTRIBUTE_ALIGN(32);


data_elf my_data_elf;

int my_thread_id=0;

void *external_ehcmodule= NULL;
int size_external_ehcmodule=0;

int sd_ok=0;
int ud_ok=0;



int load_ehc_module()
{
int is_ios=0;
FILE *fp;


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
/*LANGUAGE PATCH - FISHEARS*/


//---------------------------------------------------------------------------------
/* Global definitions */
//---------------------------------------------------------------------------------

int force_ios249=0;

unsigned char *buff_cheats;
int len_cheats=0;

int load_cheats(u8 *discid);
void LoadLogo(void);
void DisplayLogo(void);
int load_disc(u8 *discid);

// to read the game conf datas
u8 *temp_data= NULL;

// texture of white-noise animation generated here
u32 *game_empty= NULL;

s8 sound_effects[2][2048] ALIGNED(0x20);

//---------------------------------------------------------------------------------
/* language GUI */
//---------------------------------------------------------------------------------

int idioma=0;

char month[2][12][4]=
	{
	{"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"},
	{"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"}
	
	};
char letrero[2][60][64]=
	{
	{"Return", "Configure", "Delete Favorite", "Add Favorite", "Load Game", "Add to Favorites", "Favorites", "Page", "Ok" ,"Discard",
	" Cheats Codes found !!! ", "Apply Codes ", "Skip Codes ", "Format WBFS", "Selected", "You have one WBFS partition", "Are You Sure You Can Format?",
	" Yes ", " No ", "Formatting...","Formatting Disc Successfull","Formatting Disc Failed",
//22 
	"Return", "Add New Game", "Add PNG Bitmap", "Delete PNG Bitmap",  "Fix Parental Control","Return to Wii Menu", "Rename Game", "Delete Game", "Format Disc", 
	"Alternative .dol","","","","",
//36
	"Are you sure you want delete this?", "Press A to accept or B to cancel", 
// 38
"Insert the game DVD disc...", "ERROR! Aborted", "Press B to Abort","Opening DVD disc...", "ERROR: Not a Wii disc!!",
"ERROR: Game already installed!!!", "Installing game, please wait... ", "Done", "Change the password", "Use this password?","Restore Name",
//49
"Delete Alternative .dol", ".dol Search", "Searching for .dol...","Alternative .dol Selected","Alternative .dol Deleted",
	},
    // spanish
	{"Retorna", "Configurar", "Borra Favorito", "Añade Favorito", "Carga juego", "Añade a Favoritos", "Favoritos", "Página", "Hecho", "Descartar",
	" Códigos de Trucos encontrados !!! ","Usa Códigos", "Salta Códigos", "Formatear WBFS", "Seleccionado", "Ya tienes una partición WBFS", 
	"¿Estas seguro que quieres formatear?", " Si ", " No ", "Formateando...", "Exito Formateando el Disco", "Error al Formatear el Disco",
//22	
	"Retornar", "Añadir Nuevo Juego", "Añadir Bitmap PNG", "Borrar Bitmap PNG", "Fijar Control Parental", "Retorna al Menu de Wii", "Renombrar Juego", "Borrar Juego", "Formatear Disco",
	".dol Alternativo","","","","",

//36
	"¿Estás seguro de que quieres borrar éste?", "Pulsa A para aceptar o B para cancelar",
// 38
"Inserta el DVD del juego ...", "ERROR! Abortado", "Pulsa B para Abortar","Abriendo el disco DVD...", "ERROR: No es un disco de Wii!!",
"ERROR: Juego ya instalado!!!", "Instalando el juego, espera... ", "Terminado", "Cambia la contraseña", "¿Usar esta contraseña?", "Restaurar Nombre",
//49
"Borrar .dol Alternativo", "Buscar .dol", "Buscando ficheros .dol...","Alternativo .dol Seleccionado","Alternativo .dol Borrado"
	},
	};

//---------------------------------------------------------------------------------
/* Configuration datas (Favorites) */
//---------------------------------------------------------------------------------

struct _game_log
	{
	u8 id[6];
	u8 bcd_time[6];
	};

struct _config_file
{
	u32 magic;
	u8 id[16][8];
	char parental[4];
	struct _game_log game_log[8];
	u32 hi_score;
	u8 music_mod;
	u8 rumble_off;
	
	u16 pad16;
	u32 pad[4];
}
config_file ATTRIBUTE_ALIGN(32);
																     
//---------------------------------------------------------------------------------
/* Reset and Power Off */
//---------------------------------------------------------------------------------

int exit_by_reset=0;

int return_reset=2;

void reset_call() {exit_by_reset=return_reset;}
void power_call() {exit_by_reset=3;}

//---------------------------------------------------------------------------------
/* from YAL loader */
//---------------------------------------------------------------------------------

GXRModeObj*		vmode;					// System Video Mode
unsigned int	_Video_Mode;				// System Video Mode (NTSC, PAL or MPAL)	
u32 *xfb; 

int forcevideo=0;

void Determine_VideoMode(char Region)
{
	u32 progressive;
	// Get vmode and Video_Mode for system settings first
	u32 tvmode = CONF_GetVideo();
	// Attention: This returns &TVNtsc480Prog for all progressive video modes
        vmode = VIDEO_GetPreferredMode(0);

	switch(forcevideo)
	{
	case 0:
				switch (tvmode) 
				{
					case CONF_VIDEO_PAL:
							if (CONF_GetEuRGB60() > 0) 
									_Video_Mode = PAL60;
							else 
									_Video_Mode = PAL;
							break;
					case CONF_VIDEO_MPAL:
							_Video_Mode = MPAL;
							break;

					case CONF_VIDEO_NTSC:
					default:
							_Video_Mode = NTSC;
				}
				
			#if 0

			// Overwrite vmode and Video_Mode when disc region video mode is selected and Wii region doesn't match disc region
				switch (Region) 
				{
				case PAL_Default:
				case PAL_France:
				case PAL_Germany:
				case Euro_X:
				case Euro_Y:
						if (CONF_GetVideo() != CONF_VIDEO_PAL)
						{
								_Video_Mode = PAL60;

								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog; // This seems to be correct!
								else
										vmode = &TVEurgb60Hz480IntDf;
						}
						break;

				case NTSC_USA:
				case NTSC_Japan:
						if (CONF_GetVideo() != CONF_VIDEO_NTSC)
						{
								_Video_Mode = NTSC;
								if (CONF_GetProgressiveScan() > 0 && VIDEO_HaveComponentCable())
										vmode = &TVNtsc480Prog;
								else
										vmode = &TVNtsc480IntDf;
						}
				default:
						break;
				}
				#endif
				break;

		 case 1:
				/* GAME LAUNCHED WITH 1 - FISHEARS*/
                _Video_Mode = PAL60;
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
                break;
         case 2:
                /* GAME LAUNCHED WITH 2 - FISHEARS*/
                _Video_Mode = NTSC;
				
                progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
                vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
                break;

		case 3:
				// PAL 50
				_Video_Mode = PAL;
				progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
				vmode     = (progressive) ? &TVNtsc480Prog : &TVPal528IntDf;
				break;
		
		}		

/* Set video mode register */
	*Video_Mode = _Video_Mode;

	
}

void Set_VideoMode(void)
{
    // TODO: Some exception handling is needed here
 
    // The video mode (PAL/NTSC/MPAL) is determined by the value of 0x800000cc
    // The combination Video_Mode = NTSC and vmode = [PAL]576i, results in an error
    
    *Video_Mode = _Video_Mode;

    VIDEO_Configure(vmode);
	//VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}

#define CERTS_SIZE	0xA00

static const char certs_fs[] ALIGNED(32) = "/sys/cert.sys";

s32 GetCerts(signed_blob** Certs, u32* Length)
{
	static unsigned char		Cert[CERTS_SIZE] ALIGNED(32);
	memset(Cert, 0, CERTS_SIZE);
	s32				fd, ret;

	fd = IOS_Open(certs_fs, ISFS_OPEN_READ);
	if (fd < 0) return fd;

	ret = IOS_Read(fd, Cert, CERTS_SIZE);
	if (ret < 0)
	{
		if (fd >0) IOS_Close(fd);
		return ret;
	}

	*Certs = (signed_blob*)(Cert);
	*Length = CERTS_SIZE;

	if (fd > 0) IOS_Close(fd);

	return 0;
}



int cios = CIOS;

//---------------------------------------------------------------------------------
/* from Waninkoko usbloader */
//---------------------------------------------------------------------------------


/* Gamelist variables */
static struct discHdr *gameList = NULL;

static u32 gameCnt = 0;


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

static int rumble=0;


void wiimote_rumble(int status)
{
	if(w_index<0) return;
	
	if(config_file.rumble_off) status=0;
	//if(status==0) rumble=0;

	WPAD_Rumble(w_index, status);
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

		return butt;
		}
	}

return 0;
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


struct _cheat_file
{
struct
	{
	u8 id[6];
	u8 sel[25];
	u8 magic;
	}
	cheats[1024];
}
cheat_file ATTRIBUTE_ALIGN(32);

void set_cheats_list(u8 *id);
void get_cheats_list(u8 *id);


void add_game_log(u8 *id)
{
int n;
time_t  my_time=(time(NULL));
struct tm *l_time=localtime(&my_time);
for(n=0;n<8;n++) 
	{
	if(config_file.game_log[n].id[0]==0) break;
	if(!strncmp((void *) config_file.game_log[n].id, (void *) id,6)) break;
	}
if(n==8)
	{
	for(n=0;n<7;n++) config_file.game_log[n]=config_file.game_log[n+1];
	}

memcpy(config_file.game_log[n].id, id, 6);
memset(config_file.game_log[n].bcd_time,0,6);

if(l_time)
	{
	l_time->tm_mon++;
	l_time->tm_year+=1900;

	config_file.game_log[n].bcd_time[0]=((l_time->tm_mday/10)<<4) | (l_time->tm_mday % 10);
	config_file.game_log[n].bcd_time[1]=((l_time->tm_mon/10)<<4) | (l_time->tm_mon % 10);
	config_file.game_log[n].bcd_time[2]=((l_time->tm_year/1000)<<4) | (l_time->tm_year/100 % 10);
	config_file.game_log[n].bcd_time[3]=((l_time->tm_year/10 % 10)<<4) | (l_time->tm_year % 10);
	config_file.game_log[n].bcd_time[4]=((l_time->tm_hour/10)<<4) | (l_time->tm_hour % 10);
	config_file.game_log[n].bcd_time[5]=((l_time->tm_min/10)<<4) | (l_time->tm_min % 10);
	}


}


u8 alt_dol_disc_tab[32768];


int load_cfg(int mode)
{
FILE *fp=0;
int n=0;

rumble=0;
usleep(60*1000);
wiimote_rumble(0);
WPAD_ScanPads();

	config_file.magic=0;

	memset((void *) &cheat_file, 0, 32768);

	if(mode==0) // from WBFS
		{
		// Load CFG from HDD (or create __CFG_ disc)
		if(WBFS_LoadCfg(&config_file,sizeof (config_file), &cheat_file))
			{
			if(config_file.magic!=0xcacafea1)
				{
				memset(&config_file,0, sizeof (config_file));
				}
			else
				{
				int n,m;

				// delete not found entries in disc
				for(m=0;m<16;m++)
					{
					for(n=0;n<gameCnt;n++)
						{
						struct discHdr *header = &gameList[n];
						  if(strncmp((char *) header->id, (char *) &config_file.id[m][0],6)==0) break;
						}
					if(n==gameCnt) memset(&config_file.id[m][0],0,6);
					
					}
				}
			return 0;
			}
		}
	else
		{
		if(sd_ok)
			{
			memset((void *) alt_dol_disc_tab,0,32768);

			fp=fopen("sd:/apps/uloader/uloader.cfg","rb"); // lee el fichero de texto
			/*if(!fp)
				fp=fopen("sd:/uloader.cfg","rb"); // lee el fichero de texto*/

			if(fp!=0)
				{
				n=fread(&config_file,1, sizeof (config_file) ,fp);
				if(n!=sizeof (config_file) || !fp || (config_file.magic!=0xcacafea1 && config_file.magic!=0xcacafea2))
					{
					memset(&config_file,0, sizeof (config_file));
					}
				if(config_file.magic==0xcacafea2)
					{
					n=fread(&alt_dol_disc_tab,1, 32768 ,fp);
					if(n==32768)
						n=fread(&cheat_file,1, 32768 ,fp);
					}
				fclose(fp);
				return 0;
				}
			}
		}
return -1;
	
}

void save_cfg(int mode)
{
FILE *fp;

rumble=0;
usleep(60*1000);
wiimote_rumble(0);
WPAD_ScanPads();



if(mode==0)
	{
	config_file.magic=0xcacafea1;

	if(WBFS_SaveCfg(&config_file,sizeof (config_file), &cheat_file))
		{
		return;
		}
	}
else
	{
	if(!sd_ok) return;

	config_file.magic=0xcacafea2;
	

	fp=fopen("sd:/apps/uloader/uloader.cfg","wb"); // escribe el fichero de texto
	
	if(fp!=0)
		{
		fwrite(&config_file,1, sizeof (config_file) ,fp);
		fwrite(&alt_dol_disc_tab,1, 32768 ,fp);
		fwrite(&cheat_file,1, 32768 ,fp);
		fclose(fp);
		}
	}

}

#define MAX_LIST_CHEATS 25

int txt_cheats=0;

int num_list_cheats=0;
int actual_cheat=0;

void create_cheats();

struct _data_cheats
{
	void *title;
	void *description;

	int apply;
	u8 *values;
	int len_values;

} data_cheats[MAX_LIST_CHEATS];

extern int abort_signal;


struct _partition_type
{
	int type;
	u32 lba, len;
}
partition_type[32];

int num_partitions=0;

u32 sector_size;

#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))

int get_partitions()
{
u8 part_table[16*4];
int ret;
u8 *ptr;
int i;
int n_sectors;

	num_partitions=0;
	memset(partition_type, 0, sizeof(partition_type));


	n_sectors = USBStorage2_GetCapacity(&sector_size);
	if(n_sectors<0) return -1;

	n_sectors+=1; 
    
	ret = USBStorage2_ReadSectors(0, 1, temp_data);
    if(ret<0) return -2;
    
	///find wbfs partition
	if(temp_data[0x1fe]!=0x55 || temp_data[0x1ff]!=0xaa 
			|| !strncmp((void *) &temp_data[3],"NTFS",4) 
			|| !strncmp((void *) &temp_data[0x36],"FAT",3)
			|| !strncmp((void *) &temp_data[0x52],"FAT",3))

//	if(temp_data[0x1bc]!=0 || temp_data[0x1bd]!=0 || temp_data[0x1fe]!=0x55 || temp_data[0x1ff]!=0xaa) 
		{
		memset(part_table,0,16*4);
		partition_type[num_partitions].lba=0;
		partition_type[num_partitions].len=n_sectors;
		if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S')
			partition_type[num_partitions].type|=(2<<8);
		else partition_type[num_partitions].type=(4<<8);

		num_partitions++;
		}
	else 
		{
		memcpy(part_table,temp_data+0x1be,16*4);


		ptr = part_table;

		for(i=0;i<4;i++,ptr+=16)
			{ 
			u32 part_lba = read_le32_unaligned(ptr+0x8);

			partition_type[num_partitions].len=read_le32_unaligned(ptr+0xC);
			partition_type[num_partitions].type=(0<<8) | ptr[4];
			partition_type[num_partitions].lba=part_lba;

			if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S' && i==0)
				{partition_type[num_partitions].len=n_sectors;partition_type[num_partitions].type=(2<<8);num_partitions++; break;}


			if(ptr[4]==0) 
				{
				continue;
				}

			if(ptr[4]==0xf)
				{
				u32 part_lba2=part_lba;
				u32 next_lba2=0;
				int n;
				
				for(n=0;n<8;n++) // max 8 logic partitions (i think it is sufficient!)
					{
						ret = USBStorage2_ReadSectors(part_lba+next_lba2, 1, temp_data);
						if(ret<0)
							return -2; 

						part_lba2=part_lba+next_lba2+read_le32_unaligned(temp_data+0x1C6);
						next_lba2=read_le32_unaligned(temp_data+0x1D6);

						partition_type[num_partitions].len=read_le32_unaligned(temp_data+0x1CA);
						partition_type[num_partitions].lba=part_lba2;
						partition_type[num_partitions].type=(1<<8) | temp_data[0x1C2];

						ret = USBStorage2_ReadSectors(part_lba2, 1, temp_data);
						if(ret<0)
							return -2;

						
						if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S')
							partition_type[num_partitions].type=(3<<8) | temp_data[0x1C2];
					
						num_partitions++;
						if(next_lba2==0) break;
					}
				}  
			  else   
					{
						ret = USBStorage2_ReadSectors(part_lba, 1, temp_data);
				
						if(ret<0)
							return -2;

						if(temp_data[0]=='W' && temp_data[1]=='B' && temp_data[2]=='F' && temp_data[3]=='S')
							partition_type[num_partitions].type|=(2<<8);

						num_partitions++;
					}
			}
		}
return 0;
}

int menu_format()
{
int frames2=0;
int n,m;
int ret;
char type[5][9]={"Primary ", "Extended", "WBFS Pri", "WBFS Ext","Unknown "};

int current_partition=0;

int level=0;
//int it_have_wbfs=0;

int last_select=-1;

re_init:

autocenter=0;
ret=get_partitions();
current_partition=0;
level=0;

if(ret==-1 /*|| num_partitions<=0*/) return -1;



while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	draw_background();
		

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	SelectFontTexture(1); // selecciona la fuente de letra extra
    
	
	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][13][0]);
	bkcolor=0;
	autocenter=0;
	

	color= 0xff000000; 
	letter_size(8,24);
	SetTexture(NULL);

	//it_have_wbfs=0;

	/*for(n=0;n<num_partitions;n++)
		{
		if(type[partition_type[n].type>>8][0]>1) {it_have_wbfs=1;break;}
		}*/

	if(level>=100)
		{
		bkcolor=0;
		
		
		if(level>=1000)
			{
			PX= 0; PY=ylev+(352)/2-16;
			letter_size(16,32);
			autocenter=1;
			if(level>=2048+128)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff00ff00);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, 0xff000000);

				s_printf("%s", &letrero[idioma][20][0]);
				}
			else
			if(level>=2048)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff0000ff);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, 0xff000000);

				s_printf("%s", &letrero[idioma][21][0]);
				}
			else
				s_printf("%s", &letrero[idioma][19][0]);

			autocenter=0;
			}
		else
			{
			PX= 0; PY=ylev+32;
			letter_size(16,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][14][0]);
			letter_size(8,32);
			PX= ((SCR_WIDTH-54*8)>>1)-28; PY=ylev+80+32;
			m=level-100;autocenter=0;

			DrawRoundFillBox(PX-32, PY-12, 60*8+64, 56, 0, 0xcf0000ff);
							
			DrawRoundBox(PX-32, PY-12, 60*8+64, 56, 0, 4, 0xff000000);
			
			color= 0xff000000; 
			/*s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));*/
			if((partition_type[m].type>>8)==4)
					{
					s_printf("Unpartitioned Disc (%s) LBA: %.10u LEN: %.2fGB", &type[partition_type[m].type>>8][0],
						partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				else
					{
					s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
			autocenter=1;
			PX= 0; PY=ylev+180;
			letter_size(12,32);
			s_printf("%s", &letrero[idioma][16][0]);
		
			PX= 0; PY=ylev+180+48;
			/*if(it_have_wbfs)
				s_printf("%s", &letrero[idioma][15][0]);*/
			autocenter=0;

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
			}
		}

    if(level==0)
		{
		for(n=0;n<6;n++)
			{
			PX=((SCR_WIDTH-54*8)>>1)-28; PY=ylev+32+n*50;
			m=(current_partition+n);
			if(m>=num_partitions)
				{
				DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xcf808080);
				DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
				if(num_partitions<1 && n==0)
					{
					s_printf("No Partitions in Disc Detected");
					}
				}
			else
				{
				if(px>=PX-32 && px<=PX+60*8+64 && py>=PY-8 && py<PY+32) 
					{
					if((partition_type[m].type>>8)>1 && (partition_type[m].type>>8)!=4) DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-40, PY-12, 60*8+80, 48, 0, 5, 0xfff08000);select_game_bar=100+m;
					}
				else 
					{
					if((partition_type[m].type>>8)>1)  DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
					}
				if((partition_type[m].type>>8)==4)
					{
					s_printf("Unpartitioned Disc (%s) LBA: %.10u LEN: %.2fGB", &type[partition_type[m].type>>8][0],
						partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				else
					{
					s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				}
			}
		
	
			SetTexture(NULL);
		
			if(current_partition>=6)
				{
				if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=50;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
				}

				if((current_partition+6)<num_partitions)
				{
				if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=51;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
				}

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][0][0])*8, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
		} // level==0

	frames2++;step_button++;

	if(select_game_bar>=0)
		{
		if(select_game_bar!=last_select)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			last_select=select_game_bar;
			}
		}
	else last_select=-1;
	

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(level<1000)
		{

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
						
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
					 else
					   {px=-100;py=-100;}

				

				if(new_pad & WPAD_BUTTON_B)
					{
					snd_fx_no();
					if(level==0) break;
					else level=0;
					}

				if((new_pad &  WPAD_BUTTON_MINUS) && level==0)
					{
					current_partition-=6;if(current_partition<0) current_partition=0;
					}
				
				if((new_pad &  WPAD_BUTTON_PLUS) && level==0)
					{
					current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==1) {snd_fx_yes();break;}
					if(select_game_bar==50) {snd_fx_yes();current_partition-=6;if(current_partition<0) current_partition=0;} //page -6
					if(select_game_bar==51) {snd_fx_yes();current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;} // page +6

					if(select_game_bar>=100 && select_game_bar<150) // select partition to format
						{
						level=select_game_bar;snd_fx_yes();
						}
					
					if(select_game_bar==60) {level+=900;snd_fx_yes();} // Yes
					if(select_game_bar==61) {snd_fx_no();level=0;} // No

                   
					}
				}

		}
	
	draw_snow();
	Screen_flip();
	
	if(level>=1000)
		{
		if(level<2000)
			{
			if(WBFS_Format(partition_type[level-1000].lba, partition_type[level-1000].len)<0) level=2048; // error
			else level=2048+128; // ok
			}

        level++;
		if((level & 127)>120) 
			{	 
			if(level>=2048+128) break;
			level=0;goto re_init;
			}
		}


	if(exit_by_reset) break;
	}

return 0;
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




void add_game()
{

int ret;
int n;
static struct discHdr header ATTRIBUTE_ALIGN(32);
static f32 used,free;

char str_id[7];	
//ASND_Pause(1);

    
	WDVD_SetUSBMode(NULL, 0);

	WBFS_Init();

	{
	int ylev=(SCR_HEIGHT-440);


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;
	
	
	while(1)
		{
		if(exit_by_reset)
			{
			autocenter=0;
			goto out;
			}

		cabecera( &letrero[idioma][23][0]);
	
		PX=0;PY=ylev+352/2-16;
		s_printf("%s",&letrero[idioma][38][0]);
		PX=0;PY+=64;
		s_printf("%s",&letrero[idioma][40][0]);

		if(rumble) 
			{
			if(rumble<=7) wiimote_rumble(0); 
			rumble--;
			}
		else wiimote_rumble(0);
	
		WPAD_ScanPads(); // esto lee todos los wiimotes
		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
		
		draw_snow();
		Screen_flip();

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				PX=0;PY=ylev+352/2-16;

				s_printf("%s",&letrero[idioma][39][0]);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			//sleep(4);
			goto out;
			}
	  
		ret = Disc_Wait();
		if(ret==0) break;
		if (ret < 0) 
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				PX=0;PY=ylev+352/2-16;
				s_printf("ERROR! (ret = %d)", ret);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			goto out;
			}
		}

	/*cabecera( &letrero[idioma][23][0]);
	PX=0;PY=ylev+352/2-16;

	s_printf("%s",&letrero[idioma][41][0]);
	Screen_flip();	*/

	remote_call(draw_add_game_mess);

		/* Open disc */
	ret = Disc_Open();
	if (ret < 0) 
		{
		
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			s_printf("ERROR! (ret = %d)", ret);
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			if(n==0) snd_fx_no();
			autocenter=0;
			}
		//sleep(4);
		goto out;
		}
  
	/* Check disc */
	ret = Disc_IsWii();
	if (ret < 0) 
		{
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera(&letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			s_printf("%s",&letrero[idioma][42][0]);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			autocenter=0;
			}
		//sleep(4);
		goto out;
	
	}


	/* Read header */
	Disc_ReadHeader(&header);

	
	memcpy(str_id,header.id,6); str_id[6]=0;
	
	/* Check if game is already installed */
	ret = WBFS_CheckGame((u8*) str_id);
	if (ret) 
		{
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			
			s_printf("%s",&letrero[idioma][43][0]);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			autocenter=0;
			}
		//sleep(4);
		goto out;

	}

/////////////////////

    WBFS_DiskSpace(&used, &free);
	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

    while(1)
		{
		if(rumble) 
			{
			if(rumble<=7) wiimote_rumble(0); 
			rumble--;
			}
		else wiimote_rumble(0);
	
		WPAD_ScanPads(); // esto lee todos los wiimotes
		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
		
		cabecera( &letrero[idioma][23][0]);

		
		PX=0;PY=ylev+32; color= 0xffff3000; bkcolor=0;
		letter_size(12,32);
		s_printf("HDD: Used: %.2fGB Free: %.2fGB", used,free);

		if(strlen(header.title)<=37) letter_size(16,32);
		else if(strlen(header.title)<=45) letter_size(12,32);
		else letter_size(8,32);		


		PX=0;PY=ylev+352/2-32; color= 0xff000000;
				
		bkcolor=0;
		
		s_printf("%s",header.title);
		
		letter_size(12,32);

		PX=0;PY=ylev+352/2+32; 

		s_printf(&letrero[idioma][37][0]);
		
		draw_snow();
		Screen_flip();

        if(exit_by_reset)
			{
			autocenter=0;
			goto out;
			}

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				PX=0;PY=ylev+352/2-16;
				s_printf("%s",&letrero[idioma][39][0]);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			//sleep(4);
			goto out;
			}
		if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_GREEN | WPAD_BUTTON_A)) {snd_fx_yes();break;}
		}
    

////////////////////
    for(n=0;n<60;n++)
		{
		cabecera( &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		
		s_printf("%s",&letrero[idioma][44][0]);
		if(n==0) snd_fx_yes();
		draw_snow();
		Screen_flip();
		wiimote_rumble(0);
		WPAD_ScanPads();
		}
    //sleep(1);

	USBStorage2_Watchdog(0); // to increase the speed
	WPAD_Shutdown();

	/* Install game */
	ret = WBFS_AddGame(0);

	while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
	USBStorage2_Watchdog(1); // to avoid hdd sleep

	if(exit_by_reset) {exit_by_reset=0;}

	if (ret < 0) {
		

		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);	
		
		if(ret==-666) goto out;
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			PX=0;PY=ylev+352/2-16;
			s_printf("Installation ERROR! (ret = %d)", ret);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			autocenter=0;
			
			
			//sleep(1);
			wiimote_rumble(0);
			WPAD_ScanPads();
			}
			//sleep(3);
		goto out;
	}

    for(n=0;n<240;n++)
		{
		cabecera( &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;

		
		s_printf("%s",&letrero[idioma][45][0]);
		if(n==0) snd_fx_yes();
		draw_snow();
		Screen_flip();
		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);	

		wiimote_rumble(0);
		WPAD_ScanPads();
		}

	}
autocenter=0;

out:
	autocenter=0;
	WBFS_Close();
}


int delete_test(int ind, char *name)
{
int frames2=0;
int old_select=-1;

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][ind][0]);
	bkcolor=0;
	
	PX=0;PY=ylev+352/2-32;
	autocenter=1;letter_size(12,32);
	s_printf("%s", &letrero[idioma][36][0]);

	if(strlen(name)<=37) letter_size(16,32);
	else if(strlen(name)<=45) letter_size(12,32);
	else letter_size(8,32);		

	PX=0;PY=ylev+352/2+32; color= 0xff000000; 
			
	bkcolor=0;
	
	s_printf("%s",name);
	autocenter=0;
	letter_size(16,32);

	
	if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
	if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;
		

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
						
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
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					draw_snow();
					Screen_flip();snd_fx_no();break;
					}

				

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_yes();return 1;} // Yes
					if(select_game_bar==61) {draw_snow();Screen_flip();snd_fx_no();break;} // No
                   
					}
				}

	
	draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

// No!!!

return 0;
}

void set_parental_control()
{
int frames2=0;
int n;

int mode=0;
int old_select=-1;
parental_control_on=1;

char parental_str[4]={0,0,0,0};

char title_str[24];


while(1)
	{
	int select_game_bar=-1;
	int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	PX= 0; PY=8; color= 0xff000000; 
	letter_size(16,32);
	
	bkcolor=0;//0xc0f08000;
			
	autocenter=1;
	s_printf("%s","Parental Control");
	autocenter=0;

    if(mode==1)
			{
			ylev+=32;

			SetTexture(NULL);

			DrawRoundFillBox(20+148, ylev, 148*2, 352, 0, 0xcfafafaf);

			letter_size(16,32);

			

			select_game_bar=-1;
			

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

				PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("%c", parental_str[n]==0 ? ' ' : parental_str[n]);
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
    if(mode==0)
		{
		int m,l,k;
		
		m=64;
		for(n=0;n<8;n++)
			{
			PX=36;PY=m;
            if(config_file.game_log[n].id[0])
				{
				letter_size(12,32);
				color=0xff000000;
			
				s_printf("%c%c%c%c%c%c ",config_file.game_log[n].id[0],config_file.game_log[n].id[1],config_file.game_log[n].id[2],config_file.game_log[n].id[3],config_file.game_log[n].id[4],config_file.game_log[n].id[5]);

			    memset(title_str,32,22); title_str[22]=0;

				if(gameList && gameCnt>0)
					{
					for(l=0;l<gameCnt;l++)
						{
						if(!strncmp( (void *)gameList[l].id, (void *) config_file.game_log[n].id, 6))
							{
							for(k=0;k<22;k++) if(gameList[l].title[k]==0) break; else title_str[k]=gameList[l].title[k]<32 ? ' ': gameList[l].title[k];
							}
						}
					}

                color=0xffff1000; s_printf("%s ",title_str);
				color=0xff000000;

				s_printf("%c%c-%s-%c%c%c%c %c%c:%c%c",
					(config_file.game_log[n].bcd_time[0]>>4)+48, (config_file.game_log[n].bcd_time[0] & 15)+48,
					 &month[idioma][((config_file.game_log[n].bcd_time[1]>>4)*10+(config_file.game_log[n].bcd_time[1] & 15)-1) % 12][0],
					(config_file.game_log[n].bcd_time[2]>>4)+48, (config_file.game_log[n].bcd_time[2] & 15)+48,
					(config_file.game_log[n].bcd_time[3]>>4)+48, (config_file.game_log[n].bcd_time[3] & 15)+48,
					(config_file.game_log[n].bcd_time[4]>>4)+48, (config_file.game_log[n].bcd_time[4] & 15)+48,
					(config_file.game_log[n].bcd_time[5]>>4)+48, (config_file.game_log[n].bcd_time[5] & 15)+48);
				}
			else {autocenter=1;s_printf("< Empty >");autocenter=0;}
			m+=40;
		    }
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=61;
		if(Draw_button(600-32-strlen(&letrero[idioma][46][0])*8, ylev+108*4-64, &letrero[idioma][46][0])) select_game_bar=70;
		}
	
	if(mode==2)
		{

		SetTexture(&text_button[0]);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
		SetTexture(NULL);
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

		letter_size(16,24);

		for(n=0;n<5;n++)
			{
			int my_x=SCR_WIDTH/2-125+50*n;
			int my_y=ylev+50;

			if(n==4) DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0ffffff);
			else DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa000ff00);

			PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("%c", n==4 ? '0' : parental_str[n]);
			DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);

			}

		letter_size(16,32);

		color= 0xff000000; bkcolor=0;
		PX=0;PY=ylev+352/2-32;
		autocenter=1;letter_size(12,32);
		s_printf("%s", &letrero[idioma][47][0]);

		autocenter=0;
		
		letter_size(16,32);

		

		
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
		}
	
	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);

						
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
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					for(n=0;n<4;n++) parental_str[n]=0;
					snd_fx_no();
					select_game_bar=-1;

					if(mode==0)
						{
						draw_snow();Screen_flip();break;
						}
					else mode=0;
					}

				

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_yes(); for(n=0;n<4;n++) config_file.parental[n]=parental_str[n]-48;save_cfg(mode_disc!=0); return;} // Yes
					if(select_game_bar==61) {
											for(n=0;n<4;n++) parental_str[n]=0; 
											snd_fx_no();
											if(mode==0) {draw_snow();Screen_flip();break;} else mode=0;
											} // No

					if(select_game_bar==70) {snd_fx_yes();mode=1;}

					if(select_game_bar>=20000 && select_game_bar<20010)
								{
								for(n=0;n<4;n++) if(parental_str[n]==0) break;
								if(n<4) {parental_str[n]=select_game_bar+48-20000;snd_fx_yes();}
								if(n>=3) {
										 mode=2;
										 }
								select_game_bar=-1;
								}
                   
					}
				}

	
	draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	
	}
}


int rename_game(char *old_title)
{
int frames2=0;
int n,m;

int old_select=-1;

int shift=0;

int cursor=0;
int len_str=0;
int sx;

static char keyboard[8][10]={
	{'1','2','3','4','5','6','7','8','9','0',},
	{'Q','W','E','R','T','Y','U','I','O','P',},
	{'A','S','D','F','G','H','J','K','L','Ñ',},
	{'Z','X','C','V','B','N','M','&',':','_',},

	{'1','2','3','4','5','6','7','8','9','0',},
	{'q','w','e','r','t','y','u','i','o','p',},
	{'a','s','d','f','g','h','j','k','l','ñ',},
	{'z','x','c','v','b','n','m','ç','.','-',},
	//{'','','','','','','','','','',},

};

char title[64];

memcpy(title,old_title,64);

title[63]=0;

len_str=strlen(title);

while(1)
	{
	int select_game_bar=-1;
	int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	PX= 0; PY=8; color= 0xff000000; 
	letter_size(16,32);
	
	bkcolor=0;//0xc0f08000;
			
	autocenter=1;
	s_printf("%s",&letrero[idioma][28][0]);
	autocenter=0;

    SetTexture(NULL);

    //DrawRoundFillBox(20+148, ylev+32, 148*2, 352, 0, 0xcfafafaf);

        sx=8;
		if(len_str<=37) {letter_size(16,32);sx=16;}
		else if(len_str<=45) {letter_size(12,32);sx=12;}
		else letter_size(8,32);		

		color= 0xff000000; 
				
		bkcolor=0x20000000;
		
        PX=(SCR_WIDTH-sx*len_str)/2;PY=56;
		for(n=0;n<len_str;n++)
			{
			PX=(SCR_WIDTH-sx*len_str)/2+n*sx; PY=56;
			if(n==cursor && (frames2 & 8)==0) {bkcolor=0xa000ff00;s_printf(" ");bkcolor=0x20000000;PX-=sx;PY=56;}
			s_printf("%c",title[n]);
			}

		if(n==cursor && (frames2 & 8)==0) {PY=56;bkcolor=0xa000ff00;s_printf(" ");bkcolor=0x20000000;}
		

    letter_size(16,32);

	select_game_bar=-1;

	color= 0xff000000; 

	letter_size(16,24);
	bkcolor=0;
	autocenter=0;

	for(m=0;m<4;m++)
		{
		for(n=0;n<10;n++)
			{
			int my_x=SCR_WIDTH/2-5*50+50*n;
			int my_y=ylev+64+50*m;

			DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

			PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("%c", keyboard[m+shift][n]); 

			if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=1024+keyboard[m+shift][n];
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
			}
		}

		{
		int my_x=SCR_WIDTH/2-5*50;
		int my_y=ylev+64+50*4+16;

		// shift
		DrawRoundFillBox(my_x, my_y, 48+16*1, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("Aa");

		if(px>=my_x && px<my_x+48+16*1 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48+16*1, 48, 0, 4, 0xfff08000);
				select_game_bar=100;
				}
			else
				DrawRoundBox(my_x, my_y, 48+16*1, 48, 0, 4, 0xa0000000);
		
		my_x+=50+16*1;

		// <
		DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("<");

		if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=101;
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
		
		my_x+=50;


		// Space

		DrawRoundFillBox(my_x, my_y, 48*5, 48, 0, 0xa0cfffff);

		PX=my_x+16*5; PY=my_y+6+8; color= 0xff000000;s_printf("Space");

		if(px>=my_x && px<my_x+48*5 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48*5, 48, 0, 4, 0xfff08000);
				select_game_bar=1024+32;
				}
			else
				DrawRoundBox(my_x, my_y, 48*5, 48, 0, 4, 0xa0000000);
		
		my_x+=5*50;

		// >
		DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf(">");

		if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=102;
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
		
		my_x+=50;

		// Del

		DrawRoundFillBox(my_x, my_y, 48+16*2, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= 0xff000000;s_printf("Del");

		if(px>=my_x && px<my_x+48+16*2 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48+16*2, 48, 0, 4, 0xfff08000);
				select_game_bar=104;
				}
			else
				DrawRoundBox(my_x, my_y, 48+16*2, 48, 0, 4, 0xa0000000);
		
		//my_x+=5*50;

		}

		
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=60;

		if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][48][0])) select_game_bar=65;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=61;
	
	
	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);

						
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
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					snd_fx_no();
					select_game_bar=-1;
					
					draw_snow();
					
					Screen_flip();break;
						
					}

				
			    if(new_pad & (WPAD_BUTTON_MINUS | WPAD_BUTTON_LEFT))
					{
						cursor--;if(cursor<0) {cursor=0;snd_fx_no();} else snd_fx_yes();
					}
				
				if(new_pad & (WPAD_BUTTON_PLUS | WPAD_BUTTON_RIGHT))
					{
						cursor++;if(cursor>len_str) {cursor=len_str;snd_fx_no();} else snd_fx_yes();
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {if(len_str<2) {select_game_bar=65;}
						                    else {draw_snow();Screen_flip();snd_fx_yes(); title[63]=0;memcpy(old_title,title,64);return 1;}
					                        }// OK
					if(select_game_bar==61) {
											draw_snow();Screen_flip();
											snd_fx_no();
											return 0;
											} // discard

					if(select_game_bar==65) {
											memcpy(title,old_title,64);title[63]=0;len_str=strlen(title);cursor=0;
											snd_fx_no();
					
											} // No


											





					if(select_game_bar==100) {shift^=4;snd_fx_yes();}

					if(select_game_bar==101) {cursor--;if(cursor<0) {cursor=0;snd_fx_no();} else snd_fx_yes();}
					if(select_game_bar==102) {cursor++;if(cursor>len_str) {cursor=len_str;snd_fx_no();} else snd_fx_yes();}

					if(select_game_bar==104) {if(cursor==0) snd_fx_no();
											  else
												{
												memcpy(&title[cursor-1], &title[cursor], 64-cursor);
												len_str=strlen(title);snd_fx_yes();
												cursor--;
												}
											  }

					if(select_game_bar>=1024)
											  {
											  if(len_str>62) snd_fx_no();
											  else
												  {
												  if(cursor>=len_str) cursor=len_str;
												  for(n=62;n>=cursor;n--) title[n+1]=title[n];
												  title[cursor]=select_game_bar-1024; title[63]=0;
                                                  len_str=strlen(title);snd_fx_yes();
												  cursor++;if(cursor>=len_str) cursor=len_str;
												  }
											  
											  }
                   
					}
				}

	
    draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	
	}
return 0;
}
extern int WBFS_getdols(u8 *id);

typedef struct
{
u64 offset;
u32 size;
u8 id[6];
u16 pad1;
u32 flags[2];
u32 pad2[1];

} dol_infodat;

dol_infodat AlternativeDol_infodat;

extern void *dol_data;
extern int dol_len;

int load_alt_game_disc=0; // 0->from WBFS 1-> from DVD

u8 alt_dol_disc_tab[32768]  __attribute__ ((aligned (32))); // temp dol alternative table for DVD mode

void dol_GetProfileDatas(u8 *id, u8 *buff)
{
dol_infodat *dol_infop= (dol_infodat *) alt_dol_disc_tab;
int indx=0;

for(indx=0;indx<1024;indx++)
		{
		if(!strncmp((void *) dol_infop[indx].id, (void *) id, 6))
			{
			buff[0]='H'; buff[1]='D'; buff[2]='R';buff[3]=0;
			memcpy(&buff[4], &dol_infop[indx].flags[0], 8);
			return;				
			}
		}
if(indx>=1024)
	{
	buff[0]='H'; buff[1]='D'; buff[2]='R';buff[3]=0;
	memset(&buff[4], 0, 8);
	}

}

void dol_SetProfileDatas(u8 *id, u8 *buff)
{
dol_infodat *dol_infop= (dol_infodat *) alt_dol_disc_tab;
int indx=0;
int last_indx=1023;

for(indx=0;indx<1024;indx++)
		{
		if(dol_infop[indx].id[0]==0 && last_indx==1023)  last_indx=indx;
		if(!strncmp((void *) dol_infop[indx].id, (void *) id, 6))
			{
			memcpy(&dol_infop[indx].flags[0], &buff[4], 8);
			save_cfg(1);
			return;
			}
		
		}
if(indx>=1024)
	{
	if(last_indx==1023) memcpy((char *) &dol_infop[0], ((char *) &dol_infop[0])+32, 32768-32);
	indx=last_indx;

	memset((void *) &dol_infop[indx],0,32);
	memcpy((void *) &dol_infop[indx].id, id, 6);
	memcpy(&dol_infop[indx].flags[0], &buff[4], 8);
	save_cfg(1);
	}

}

int Get_AlternativeDol(u8 *id)
{
dol_infodat *dol_infop=(dol_infodat *) temp_data;
int indx=0;

	dol_len= 0;dol_data=0;
	memset(temp_data,0,32768);

if(!load_alt_game_disc)
	WBFS_LoadDolInfo(temp_data);
else
	{
	dol_infop=(dol_infodat *) alt_dol_disc_tab;
	}

	AlternativeDol_infodat.id[0]=0;

	for(indx=0;indx<1024;indx++)
		{
		if(!strncmp((void *) dol_infop[indx].id, (void *) id, 6) && dol_infop[indx].size!=0 && dol_infop[indx].offset!=0)
			{
			AlternativeDol_infodat=dol_infop[indx];
			dol_len=AlternativeDol_infodat.size;
			dol_data= (u8 *) SYS_AllocArena2MemLo(dol_len+32768,32);
            if(!dol_data) AlternativeDol_infodat.id[0]=0; // cancel
			return 1;
			}
		}
return 0;
}



void menu_alternativedol(u8 *id)
{
int frames2=0;
int old_select=-1;

char buff[65];
int mode=0,n,m;
dol_infodat *dol_infop=(dol_infodat *) temp_data;
int indx=0;
int last_indx=1023;

int current_dol=0;
int max_dol=0;

memset(temp_data,0,32768);


if(!load_alt_game_disc)
	WBFS_LoadDolInfo(temp_data);
else
	{
    dol_infop=(dol_infodat *) alt_dol_disc_tab;
	//load_cfg(1);
	//memset((void *) alt_dol_disc_tab,0,32768);
	}


for(indx=0;indx<1024;indx++)
	{
	if(dol_infop[indx].id[0]==0 && last_indx==1023)  last_indx=indx;
	if(!strncmp((void *) dol_infop[indx].id, (void *) id, 6)) break;
	}

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=0;
	s_printf("%s", &letrero[idioma][31][0]);
	bkcolor=0;
	
	
	autocenter=0;
	letter_size(16,32);
	
	if(mode==0)
		{
		PX=0;PY=ylev+352/2-32;
		autocenter=1;letter_size(16,32);
		s_printf("Based in Wiiscrubber Code");
		autocenter=0;
			
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;

		if(indx<1024)
			{
			if(dol_infop[indx].offset!=0 && dol_infop[indx].size!=0)
				if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][49][0])) select_game_bar=61;	
			//PX= 20; PY+=32;
			//s_printf("%llx %i",dol_infop[indx].offset, dol_infop[indx].size);
			}

		if(Draw_button(600-32-strlen(&letrero[idioma][50][0])*8, ylev+108*4-64, &letrero[idioma][50][0])) select_game_bar=62;

		}
	else
	if(mode==1)
		{
		iso_files *file =CWIIDisc_first_file;

		for(n=0;n<current_dol;n++)
			{
			file = file->next;
			if(!file) break;
			}
		

		for(m=0;m<5;m++,n++)
		{
		memset(buff,32,64);buff[64]=0;
		
		if (file != NULL)
			{
			
			int l1=strlen((char *)file->path);
			
			int l2=strlen((char *)file->name);
			int c1=l1,c2=l2;
			//printf("%i) %s ->%s %llx %x\n",file->part, file->path, file->name, file->offset, file->size);
			
            if((l1+l2)>64)
				{
				if(l1>16) 
					{
				    c1=16;
					if(l2<(64-16)) c1=64-l2;
					if(c1>l1) c1=l1;
					}
				c2=64-c1;
				
            
				if(l1<=c1)
					{
					memcpy(buff, (char *) file->path, c1);buff[c1]=0;
					}
				else
					{
					int c=c1/2-3;
					memcpy(buff, (char *)file->path, c);buff[c]=0;
					strcat(buff,"...");
					c=c1-c-3;
					strcat(buff, (char *)&file->path[l1-c]);
					}

				if(l2<=c2)
					{
					strcat(buff, (char *) &file->name);
					}
				else
					{
					int c=c2/2-3;
					int k=strlen(buff);

					memcpy(&buff[k], (char *) file->name, c);buff[k+c]=0;
					strcat(buff,"...");
					c=c2-c-3;
					strcat(buff, (char *)&file->name[l2-c]);
					}
				}
			else
				{
				memcpy(buff+(64-(l1+l2))/2, (char *) file->path, l1);
				memcpy(buff+(64-(l1+l2))/2+l1, (char *) file->name, l2);
				}

			if(Draw_button2(30+16, ylev+32-8+64*m, buff, 0)) select_game_bar=1024+n;
			file = file->next;
			}
	      else
			{
			Draw_button2(30+16, ylev+32-8+64*m, buff, -1);
			
			}
		
		//punt=&letrero[idioma][22+n+index][0];
		//memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));
		}
		
				if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=50;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("-");
					DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
			

			
				if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
					letter_size(32,64);
					PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
					select_game_bar=51;
					}
				else
				if(frames2 & 32)
					{
					DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
					letter_size(32,48);
					PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
					s_printf("+");
					DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
					}
				


		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][0][0])*8, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;
		}
		else
		if(mode==2)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][51][0]);
			autocenter=0;
			mode=128;
			}
		
		else
		if(mode==3)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][52][0]);
			autocenter=0;
			mode=129;
			}
		else
		if(mode==4)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][53][0]);
			autocenter=0;
			mode=129;
			}

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;
		
   if(mode<128)
		{

		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

		if(wmote_datas!=NULL)
				{
				SetTexture(NULL);		// no uses textura

						if(wmote_datas->ir.valid)
							{
							px=wmote_datas->ir.x;py=wmote_datas->ir.y;
							
							SetTexture(NULL);
							DrawIcon(px,py,frames2);
							}
						 else 
						 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
							{

							if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
								{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
							if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
								{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
								

							if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
								{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
							if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
								{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

							if(guitar_pos_x<0) guitar_pos_x=0;
							if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
							if(guitar_pos_y<0) guitar_pos_y=0;
							if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
							
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
						 else
						   {px=-100;py=-100;}

					if(mode==1)
						{
						if(new_pad & WPAD_BUTTON_MINUS)
							{
								current_dol-=5;if(current_dol<0) current_dol=0;snd_fx_yes();
							}
						
						if(new_pad & WPAD_BUTTON_PLUS)
							{
								current_dol+=5;if(current_dol>(max_dol/5)*5) current_dol=(max_dol/5)*5;snd_fx_yes();
							}
						}

					if(new_pad & WPAD_BUTTON_B)
						{
						draw_snow();Screen_flip();snd_fx_no();return;
						}

					

					if(new_pad & WPAD_BUTTON_A) 
						{
						if(select_game_bar==50) {current_dol-=5;if(current_dol<0) current_dol=0;snd_fx_yes();}
						if(select_game_bar==51) {current_dol+=5;if(current_dol>(max_dol/5)*5) current_dol=(max_dol/5)*5;snd_fx_yes();}
					

						if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_no();return;} // exit
						
						if(select_game_bar==61) {
												draw_snow();Screen_flip();snd_fx_yes();
						                        if(indx<1024) 
													{
													if(!load_alt_game_disc)
														{
														memset((void *) &dol_infop[indx],0,32);
														}
													else // for DVD mode
														{
														dol_infop[indx].offset=0;
                                                        dol_infop[indx].size=0;
														}
													}
													
												mode=4;
												 }
						
						if(select_game_bar==62) {draw_snow();Screen_flip();snd_fx_yes();mode=2;}
					   //
						if(select_game_bar>=1024)
							{
							iso_files *file =CWIIDisc_first_file;

							if(indx>=1024)
								{
								if(last_indx==1023)
									memcpy((char *) &dol_infop[0], ((char *) &dol_infop[0])+32, 32768-32);
								indx=last_indx;
								}
							memset((void *) &dol_infop[indx],0,32);
							
							n=1024;

							while(n<select_game_bar) {if(!file) break; file=file->next;n++;}

							if(file)
								{
								snd_fx_yes();
								dol_infop[indx].offset= file->offset;
								dol_infop[indx].size=	 file->size;
								memcpy((void *) &dol_infop[indx].id, id, 6);
								}
							else snd_fx_no(); // error!

							mode=3;
							}
							//

						}
					}
		}

	
    draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	if(mode==128) 
		{
		iso_files *file;

		altdol_frames2=0;
		remote_call(draw_altdolscr);

        if(!load_alt_game_disc)
			WBFS_getdols(id);
		else
			disc_getdols(id);

		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		/*
		// simulate more dols
        file =CWIIDisc_first_file;
		for(n=0;n<7;n++)
			{
			file->next=malloc(sizeof(iso_files));
			*file->next=*file;
			file->next->next=NULL;
			file=file->next;
			}
		*/

		file =CWIIDisc_first_file;
		max_dol=0;
		while(file)
			{
			max_dol++;
			file = file->next;
			if(!file) break;
			}
				
		mode=1;
		}
	if(mode==129) break;

	frames2++;
	}


	// FREE FILES
	{
	iso_files *file =CWIIDisc_first_file;
	while(file)
		{
		iso_files *file2=file->next;
		free(file); file=file2;
		}
	CWIIDisc_first_file=CWIIDisc_last_file=NULL;
	}

if(!load_alt_game_disc)
	WBFS_SaveDolInfo(temp_data);
else
	save_cfg(1);

if(mode==129) sleep(2);
}

void select_file(int flag, struct discHdr *header);

int home_menu(struct discHdr *header)
{
int frames2=0;
int n;
int old_select=-1;
char *punt;
char buff[64];

int index=0;

	while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=8; color= 0xff000000; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}

		letter_size(16,32);
		

    for(n=0;n<6;n++)
		{
		int m=n+index;

		memset(buff,32,56);buff[56]=0;
		if(m>9)
			{
			Draw_button2(30+48, ylev+32+64*n, buff, 0);
			continue;
			}
		
		punt=&letrero[idioma][22+n+index][0];
		memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));

		if(Draw_button2(30+48, ylev+32+64*n, buff,
			((!header && !mode_disc && (m==2 || m==3 || m==6 || m==7 || m==9 || (parental_control_on && m!=0 && m!=5))
			)  || (mode_disc && (m==1 || m==2 || m==3 ||  m==6 || m==7 || m==8 || (parental_control_on && m!=0 && m!=5)))  ) ? -1 : 0)) select_game_bar=m+1;
	
		}

		if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
			{
			DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
			letter_size(32,64);
			PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
			s_printf("-");
			DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
			select_game_bar=50;
			}
		else
		if(frames2 & 32)
			{
			DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
			letter_size(32,48);
			PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
			s_printf("-");
			DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
			}
		

		
		if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
			{
			DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
			letter_size(32,64);
			PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
			s_printf("+");
			DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
			select_game_bar=51;
			}
		else
		if(frames2 & 32)
			{
			DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
			letter_size(32,48);
			PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
			s_printf("+");
			DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
			}

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;


	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
						SetTexture(NULL);
						DrawIcon(px,py,frames2);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
						
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
					 else
					   {px=-100;py=-100;}

				
				if(new_pad & (WPAD_BUTTON_B | WPAD_BUTTON_HOME))
					{
					snd_fx_no();draw_snow();Screen_flip();
					return 0;
					}

                if(new_pad & WPAD_BUTTON_MINUS)
					{
						index-=6;if(index<0) index=6;snd_fx_tick();
					}
				
				if(new_pad & WPAD_BUTTON_PLUS)
					{
						index+=6;if(index>6) index=0;snd_fx_tick();
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==50) {index-=6;if(index<0) index=6;}
					if(select_game_bar==51) {index+=6;if(index>6) index=0;}
					
					// return
					if(select_game_bar==1) {snd_fx_yes();draw_snow();Screen_flip(); return 0;}

					// add game
					if(select_game_bar==2) {snd_fx_yes();draw_snow();Screen_flip(); add_game(); return 2;}
				
					if(header)
						{
						// add png
						if(select_game_bar==3) {snd_fx_yes();draw_snow();Screen_flip(); select_file(0,header); return 3;}

						// delete png
                        if(select_game_bar==4) {snd_fx_yes();draw_snow();Screen_flip(); 
						                       if(delete_test(25, header->title))
												   {
												   memset(temp_data,0,256*1024);WBFS_GetProfileDatas(header->id, temp_data);
												   temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;temp_data[9]=0; // break PNG signature
											       WBFS_SetProfileDatas(header->id, temp_data);
												   } 
											   return 3;
											   }
						// rename
						if(select_game_bar==7) {snd_fx_yes();draw_snow();Screen_flip(); if(rename_game(header->title))  {WBFS_RenameGame(header->id, header->title);WBFS_Close();return 2;} else return 0;}
						// delete game
						if(select_game_bar==8) {snd_fx_yes();draw_snow();Screen_flip(); if(delete_test(29, header->title)) WBFS_RemoveGame(header->id);WBFS_Close();return 2;}

						if(select_game_bar==10) {snd_fx_yes();draw_snow();Screen_flip();load_alt_game_disc=(mode_disc & 3)!=0; menu_alternativedol(header->id);return 0;}
						}
					
					// parental control
					if(select_game_bar==5) {load_alt_game_disc=(mode_disc & 3)!=0;set_parental_control();draw_snow();Screen_flip();return 0;}
					
					// menu wii
					if(select_game_bar==6) {snd_fx_yes();draw_snow();Screen_flip(); return 1;}
					
					

					// format
					if(select_game_bar==9) {snd_fx_yes();WBFS_Close();draw_snow();Screen_flip(); if(menu_format()==0) {WBFS_Open();return 2;} else return 1;}

					
					}
			}
	
	frames2++;step_button++;
	draw_snow();
	Screen_flip();
	if(exit_by_reset)  
		{	
		break;
		}
	}

return 0;
}



int current_partition=0;

int partition_cnt[4]={-1,-1,-1,-1};


void get_covers()
{
int n,m;
int ret=0;

static char url[512];
static char region[3][8]={"pal", "ntsc", "ntscj"};
static char region2[3][8]={"PAL", "NTSC", "NTSC-J"};
static char region3[11][4]={"US", "EN", "FR", "DE", "ES", "IT", "NL", "PT", "AU", "JA", "KO"};

char *flag;
int sizex=0,sizex2=0;

u8 *temp_buf=NULL;
u32 temp_size=0;

int reg_ind=0;

//if(!sd_ok) return;

//mkdir("sd:/covers",S_IREAD | S_IWRITE);
if(!gameCnt) return;

	switch(CONF_GetLanguage())
			{
		    case CONF_LANG_JAPANESE:
				reg_ind=9;break;
			case CONF_LANG_ENGLISH:
				reg_ind=1;break;
			case CONF_LANG_GERMAN:
				reg_ind=3;break;
			case CONF_LANG_FRENCH:
				reg_ind=2;break;
			case CONF_LANG_SPANISH:
				reg_ind=4;break;
			case CONF_LANG_ITALIAN:
				reg_ind=5;break;
			case CONF_LANG_DUTCH:
				reg_ind=6;break;
			case CONF_LANG_KOREAN:
				reg_ind=10;break;
			default:
				reg_ind=1;break;
			}

flag=malloc(gameCnt);
if(!flag) return;

memset(flag,0,gameCnt);

if(gameCnt==1) sizex=600;
else sizex=600/(gameCnt);

if(sizex<=2) sizex2=1;
else {if(sizex>4) sizex2=sizex-2; else sizex2=sizex-1;}

//draw_snow();
Screen_flip();

for(n=0;n<=gameCnt;n++)
	{
	struct discHdr *header = &gameList[n];

	int ylev=(SCR_HEIGHT-440);


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	draw_background();

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", "Downloading...");
	bkcolor=0;

	if(header && n<gameCnt)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=112; color= 0xff000000; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}

	letter_size(16,32);
	SetTexture(NULL);
	DrawFillBox(10, SCR_HEIGHT/2-16 , 620, 32, 2, 0xff000000);
    for(m=0;m<gameCnt;m++)
		{
		switch(flag[m])
			{
			case 1:
			case 3:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0xff00cf00);break;
			case 2:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0xff0000ff);break;
			default:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0x30800000);break;
			}
		}

   PX= 0; PY=SCR_HEIGHT/2+120; color= 0xff000000; 
				
	bkcolor=0;
		
	autocenter=1;
    if(n>=gameCnt) s_printf("Done."); else s_printf("Press B to abort");
		
		autocenter=0;
	Screen_flip();

	if(n>=gameCnt) break;

	//////////////////////
	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{

					if(wmote_datas->ir.valid)
						{
		
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						}
					 else
					   {px=-100;py=-100;}

				

				if(new_pad & WPAD_BUTTON_B)
					{
					break;
					}
			}
	/////////////////////

	//if(!strncmp((void *) discid, (void *) header->id,6))
   
   /*
	sprintf(url, "http://www.theotherzone.com/wii/resize/%s/160/224/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
	*/

/*	sprintf(url, "http://www.wiiboxart.com/wii/resize/%s/160/224/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);*/


   
	memset(temp_data,0,256*1024);
	WBFS_GetProfileDatas(header->id, temp_data);
		
	if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R') || !(temp_data[9]=='P' && temp_data[10]=='N' && temp_data[11]=='G'))
		{
		ret=-1;
		for(m=0;m<nfiles;m++)
			{
			if(!files[m].is_directory)
				if(!strncmp((char *) header->id,get_name_from_UTF8(&files[m].name[0]), 6))
					{
					FILE *fp;

					fp=fopen(&files[m].name[0],"rb"); // lee el fichero de texto
					if(fp)
						{
						fseek(fp, 0, SEEK_END);

						temp_size = ftell(fp);

						temp_buf= (u8 *) malloc(temp_size);

						if(!temp_buf) {fclose(fp);break;}
						else
							{
							fseek(fp, 0, SEEK_SET);
							if(fread(temp_buf,1, temp_size ,fp)==temp_size) ret=0;
							
							if(ret<0) {free(temp_buf);temp_buf=NULL;}

							fclose(fp);break;
							}
						}
					}
			}


		if(header->id[3]=='E') reg_ind=0;
		if(header->id[3]=='J') reg_ind=9;

		//reg_ind=1;
	

		sprintf(url, "http://wiitdb.com/wiitdb/artwork/cover/%s/%c%c%c%c%c%c.png",
		&region3[reg_ind][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
        if(reg_ind!=1)
			{
			sprintf(url, "http://wiitdb.com/wiitdb/artwork/cover/%s/%c%c%c%c%c%c.png",
			&region3[1][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
			
			if(ret!=0)
				{
				force_reload_ios222=1;
				ret=download_file(url, &temp_buf, &temp_size);

				if(ret==0 && temp_buf!=NULL) 
					{
					if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
					}
				}
			}
		
		sprintf(url, "http://www.wiiboxart.com/%s/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
		
		sprintf(url, "http://www.muntrue.nl/covers/%s/160/225/boxart/%c%c%c%c%c%c.png", 
		&region2[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
		

		if(ret==0)
			{
			#if 0
			sprintf(url, "sd:/caratulas/%c%c%c%c%c%c.png", header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
            if(sd_ok && temp_size!=0)
				{
				FILE *fp;
				//flag[n]=1;
				fp=fopen(url,"wb"); // escribe el fichero

				if(fp!=0)
					{
					fwrite(temp_buf,1, temp_size ,fp);
					fclose(fp);
					}
				}
			#endif
			flag[n]=2;
			if(temp_size>4 && temp_size<(200*1024-8))
				{
				void * texture_png=create_png_texture(&png_texture, temp_buf, 0);
				if(texture_png)
					{
					free(texture_png);
					temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=((temp_size+8+1023)/1024)-1;
									
					memcpy(&temp_data[8], temp_buf, temp_size);

					WBFS_SetProfileDatas(header->id, temp_data);
					flag[n]=1;
					}
					
				
				}
			if(temp_buf) free(temp_buf); temp_buf=NULL;

			
			}
		  else flag[n]=2;
		}
	else flag[n]=3;

	
	
	}

http_deinit();
sleep(1);
}


int load_game_routine(u8 *discid, int game_mode);


extern u32 load_dol();
int load_file_dol(char *name);



void splash_scr_send()
{
	splash_scr();
	Screen_flip();
}

void splash2_scr()
{
		draw_background();
		letter_size(16,32);
		PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);
}

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

void remote_DVD_disc()
{
int r;

	if(mode_disc & 2) r=Disc_USB_DVD_Wait();
		else r=Disc_Wait();

	if(r==0)
		{
		if(remote_DVD_disc_status==0)
			{
			remote_DVD_disc_status=1;
			
			r=Disc_Open();

			if(r>=0) 
				{
				r=Disc_IsWii();
				if(r>=0) 
					{
					Disc_ReadHeader(&mydisc_header);
					memset(BCA_Data,0,64);
					BCA_ret=WDVD_Read_Disc_BCA(BCA_Data);
					show_bca=0;
					remote_DVD_disc_status=2;
					}
				else {remote_DVD_disc_status=r;BCA_ret=-1;}
				}
			else {remote_DVD_disc_status=r;BCA_ret=-1;}
			}
		}
	else {remote_DVD_disc_status=0;}


	usleep(50*1024);
}

void update_bca(u8 *id, u8 *bca_datas);

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
        u8 discid[7];

		int ret,ret2;
		int cnt,cnt2, len;
		struct discHdr *buffer = NULL;
		int frames=0,frames2=0;
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
		int n;
		
		static int old_temp_sel=-1;
		f32 f_size=0.0f;
		int last_select=-1;

		int force_ingame_ios=0;
		int game_locked_cfg=0;

		int direct_launch=0;

		int parental_mode=0;
		char parental_str[4]={0,0,0,0};

		int launch_counter=9;
		int partial_counter;

		int volume_osd=0;

		

		int dvd_only=0;

		
		struct discHdr *disc_header=NULL;

       
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
			force_ios249=1;
			cios=249;
			ret=IOS_ReloadIOS(cios);
			sleep(1);
			}

		InitScreen();  // Inicialización del Vídeo
		remote_init();

		{
		time_t  my_time=(time(NULL));
		struct tm *l_time=localtime(&my_time);

		if(l_time)
			{
			l_time->tm_mon++;
			
			if((l_time->tm_mday>=12 && l_time->tm_mon==12) || (l_time->tm_mday<=6 && l_time->tm_mon==3)) flag_snow=1;
			}
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

		
		//for(n=0;n<60;n++) {splash_scr();Screen_flip();}
			
	    __io_wiisd.startup();
		sd_ok = fatMountSimple("sd", &__io_wiisd);
		

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
				s_printf("ERROR: You need cIOS222 and/or cIOS249 to work!!!\n");
			Screen_flip();
			goto error_0;
			}

		temp_data=memalign(32,256*1024);
        // texture of white-noise animation generated here
        game_empty=memalign(32,128*64*3*4);
		
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
	 
	////////////////////////////////////
            
			#if 0
			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
			// direct access register using mload v3 functions
	
			splash_scr();
			SelectFontTexture(1); // selecciona la fuente de letra extra

			letter_size(8,32);
					
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);
			{
			u32 dat=0;
			u32 addr;

			// get EHCI base registers
			mload_getw((void *) 0x0D040000, &addr);

			addr&=0xff;
			addr+=0x0D040000;
			
			
			mload_getw((void *) (addr+0x44), &dat);
			//mload_getw((void *) (0x0D040000+8), &dat);

		
			
		
		   #if 0
			mload_setw((void *) (addr+0x44), 0x1803);
			usleep(100*1000);
			mload_setw((void *) (addr+0x44), 0x1903); 
			usleep(100*1000);
			
			// write port_status[0]
			mload_setw((void *) (addr+0x44), 0x1801 ); 
			usleep(100*1000);
			#endif
			 
			// read port_status[0]
			//mload_getw((void *) (addr+0x44), &dat);

			
			

		    s_printf("port_status[0]: %x\n",dat);
			}
			Screen_flip();
			goto error_0;
			#endif
			

////////////////////////////////////


		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);	

		

		//__io_usbstorage.startup();

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

		int ret_flag=0;

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
				#ifndef USE_PORT1
				s_printf("You need plug one device on USB port 0...");
				#else
				s_printf("You need plug one device on USB port 1...");
				#endif

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
				
				PY+=80;
				if(!(conta & 64))
					#ifndef USE_PORT1
					s_printf("Maybe you need plug the device on USB port 0...");
					#else
					s_printf("Maybe you need plug the device on USB port 1...");
					#endif
				else
					s_printf("Press < 2 > for DVD Mode");

				conta++;

				if((new_pad & WPAD_BUTTON_2)) 
					{
					dvd_only=1;
					mode_disc=1;
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

				#ifndef USE_PORT1
				s_printf("Maybe you need plug the device on USB port 0...");
				#else
				s_printf("Maybe you need plug the device on USB port 1...");
				#endif

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
			

			temp_pad= wiimote_read(); 
			new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

			if(mod>1)
				{
				mod++;if(mod>30) {mod=0;break;}
				}
			/*if(ret2==-1)  
				{
				if(mod!=0) {mod=0;break;}
				else mod=1;
				}*/
			
			}
			
			//if(ret_flag==0) n--;
			if(!ret2  || ret2==1) break;
	
		}

		if(ret2==1)
			{
			ret2=0;
			dvd_only=1;
			mode_disc=1024+2; // USB DVD MODE
			}

		CreatePalette(&palette_icon, TLUT_RGB5A3, 0, icon_palette, icon_palette_colors); // crea paleta 0
		CreateTexture(&text_icon[0], TILE_CI8, icon_sprite_1, icon_sprite_1_sx, icon_sprite_1_sy, 0);
		CreateTexture(&text_icon[1], TILE_CI8, icon_sprite_2, icon_sprite_2_sx, icon_sprite_2_sy, 0);
		CreateTexture(&text_icon[2], TILE_CI8, icon_sprite_3, icon_sprite_3_sx, icon_sprite_3_sy, 0);
		CreateTexture(&text_icon[3], TILE_CI8, icon_sprite_4, icon_sprite_4_sx, icon_sprite_4_sy, 0);
		CreateTexture(&text_icon[4], TILE_CI8, icon_sprite_5, icon_sprite_5_sx, icon_sprite_5_sy, 0);

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
					temp_pad= wiimote_read(); 
					new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
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
				goto error;
		
			} // end test mode
		#endif

		if(ret2<0)  goto error;


		remote_call(splash_scr_send);
		

		if(CONF_Init()==0)
		{
			switch(CONF_GetLanguage())
			{
			case CONF_LANG_SPANISH:
						idioma=1;break;
			default:
						idioma=0;break;
			}
			
		}
		
		
		
		
		
       
	   #if 1
		if(ret2>=0) 
			{
		    ret2=WBFS_Init();
			if(mode_disc && ret2<0) {mode_disc|=1024;ret2=0;} // mode disc
			if(ret2>=0) ret2=Disc_Init();
			}
		
		#endif
   
		if(ret2>=0 && (mode_disc & 1024)==0)
			{
			__io_usbstorage2.startup();
			ud_ok = fatMountSimple("ud", &__io_usbstorage2);
			}
		else ud_ok=0;



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

		/*if(ret!=0) 
			{
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: You need cIOS222 and/or cIOS249 to work!!!\n");
			Screen_flip();
			goto error;
			}
		*/
        
		//ret = WBFS_Init();
		if (ret2 < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: Could not initialize WBFS subsystem! (ret = %d)", ret2);

			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32+80, 540, 64, 999, 4, 0xa0000000);
			
			PY+=80;

			#ifndef USE_PORT1
			s_printf("Maybe you need plug the device on USB port 0...");
			#else
			s_printf("Maybe you need plug the device on USB port 1...");
			#endif



			Screen_flip();
			goto error;
			}
		Screen_flip();

		remote_call(splash_scr_send);
		
		
		#if 1
		/* Get list length */

		

		#ifdef USE_MODPLAYER
		ASND_Init();
		ASND_Pause(0);
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

		if(buffer) free(buffer);
		buffer=NULL;
	    

		splash2_scr();


		///current_partition=0;
		
		
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
        for(n=0;n<4;n++)
			{
			ret = WBFS_Open2((current_partition+n) & 3);
			if(ret==0) {current_partition=(current_partition+n) & 3;break;}
			else partition_cnt[(current_partition+n) & 3]=-1;
			}
	
		if(!(mode_disc & 1024) && (mode_disc & 1023)==0)
		if (ret < 0) 
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
				goto exit_ok;

			Screen_flip();
			SetTexture(&text_background2);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, BACK_COLOR);
			letter_size(16,32);
			PX=0; PY= SCR_HEIGHT/2+32; color= 0xff000000; 
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);
			}

		cnt=0;buffer=NULL;
		//ret = WBFS_GetCount(&cnt);
		ret= partition_cnt[current_partition];
		cnt= ret;
		

		if (ret < 0 || cnt==0) {

			if(!mode_disc)
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
			len = sizeof(struct discHdr) * cnt;

			/* Allocate memory */
			buffer = (struct discHdr *)memalign(32, len);
			if (!buffer){
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in memalign()\n");
				Screen_flip();
				goto error_w;
				}

			/* Clear buffer */
			memset(buffer, 0, len);

			/* Get header list */
			ret = WBFS_GetHeaders(buffer, cnt, sizeof(struct discHdr));
			if (ret < 0) {
				DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
				DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

				s_printf("Error in WBFS_GetHeaders()\n");
				Screen_flip();
				free(buffer);
				goto error_w;
				}
		
		    gameList = buffer;
			//cnt=0;
		    for(n=0;n<cnt;n++)
				{
				  if(gameList[n].id[0]=='_') {memcpy(&gameList[n],&gameList[cnt-1],sizeof(struct discHdr));cnt--;n--;}
				}

			if(!cnt)
				{
				if(!mode_disc)
					{
					DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 0xa00000ff);
					DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16+32, 540, 64, 999, 4, 0xa0000000);

					s_printf("No Game found\n");
					Screen_flip();
					//if(mode_disc) mode_disc|=1024;
					sleep(2);
					}

				free(buffer);
				buffer=NULL;
				}
			
		    /* Sort entries */
		    if(cnt && buffer) qsort(buffer, cnt, sizeof(struct discHdr), __Menu_EntryCmp);

		    }

		/*
		// quitar 10
		for(n=1;n<10;n++) memcpy(((char *) buffer)+sizeof(struct discHdr) * cnt*n,buffer,sizeof(struct discHdr) * cnt);
		cnt*=10;
		*/
		
		/* Set values */
		gameList = buffer;
		gameCnt  = cnt;
		
		{
		u8 temp=config_file.music_mod;
		/*char temp2[4];

		memcpy(temp2, config_file.parental, 4);*/
		
		memset(&config_file,0, sizeof (config_file));
		if(!mode_disc) // mode disc
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
		
		for(n=0;n<gameCnt;n++)
			{
			struct discHdr *header = &gameList[n];
			if(!strncmp((void *) discid, (void *) header->id,6))
				{
				game_datas[0].ind=n;
				
				memcpy(discid,header->id,6); discid[6]=0;
				memset(temp_data,0,256*1024);
				WBFS_GetProfileDatas(discid, temp_data);
				create_game_png_texture(0);
				if(WBFS_GameSize(header->id, &f_size)) f_size=0.0f;
				game_mode=1;
				
				if(mode_disc==0 && config_file.parental[0]==0 && config_file.parental[1]==0 && config_file.parental[2]==0 && config_file.parental[3]==0) parental_control_on=0;

				if(parental_control_on)
					{if((game_datas[game_mode-1].config & (1<<30))!=0) {parental_mode=1024+game_mode;direct_launch=0;}}

				break;
				}
			}
		
		if(n==gameCnt) direct_launch=0;
		}

	if(mode_disc)
	{
	game_mode=1;gameList=NULL;
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
		
    cnt=actual_game;
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
		
		
		if(!mode_disc)
		    header= &gameList[game_datas[game_mode-1].ind];
		else header= disc_header;

		

		SetTexture(NULL);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
		
		if(mode_disc)
			SetTexture(&default_game_texture);
		else
		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);

		SetTexture(&text_button[0]);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffffffff);

		
			
		SetTexture(NULL);
			
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

		if(mode_disc)
			SetTexture(&default_game_texture);
		else
		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);


		DrawFillBox(320-127, ylev+8, 254, 338, 0, 0xffffffff);
		
		SetTexture(NULL);
		DrawBox(320-127, ylev+8, 254, 338, 0, 2, 0xff303030);
		
		if(mode_disc /*&& remote_DVD_disc_status!=0*/)
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
						if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
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
							DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
							}
						}

						if((actual_cheat+5)<num_list_cheats)
						{
						if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
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
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);
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

            if(mode_disc)
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

			if((game_datas[game_mode-1].config & 1) || force_ios249) s_printf("cIOS 249"); 
			
			else {if(game_datas[game_mode-1].config & 2)  s_printf("cIOS 223"); else s_printf("cIOS 222");}
			
			forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

			langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

			force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
			game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);

			if(mode_disc && parental_control_on && game_locked_cfg)
				parental_mode=9999;
			if(mode_disc && !disc_header) parental_mode=0;

		
			
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
			

			PX=608-6*16; if(header && !mode_disc) s_printf("%.2fGB", f_size);
		 
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
			x_temp=16;

            if(!(mode_disc & 1024) && remote_DVD_disc_status!=1)
				if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;

			if(!parental_mode)
				{
				if(header)
					if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][1][0])) select_game_bar=2;
				

				if(test_favorite && !mode_disc)
					{
					if(is_favorite)
						{if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][2][0])) select_game_bar=4;}
					else
						if(Draw_button(x_temp+8, ylev+108*4-64, &letrero[idioma][3][0])) select_game_bar=3;
					}

				if(header && mode_disc && BCA_ret==0)
					{
					if(show_bca==0)
						{
						
						if(Draw_button(x_temp+8, ylev+108*4-64, "Show BCA Data")) select_game_bar=56;
						}
					else
						if(Draw_button(x_temp+8, ylev+108*4-64, "Hide BCA Data")) select_game_bar=56;
					}

				if(dvd_only==1 /*&& remote_DVD_disc_status==0*/)
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
                

				if(header && mode_disc && show_bca && BCA_ret==0)
					{
					int h,g;
					
					DrawRoundFillBox(40, ylev+352/2-32, 640-80, 64, 0, 0xffcfcf00);
					DrawRoundBox(40, ylev+352/2-32, 640-80, 64, 0, 4, 0xffcf0000);

					letter_size(8,24);
					PX= 52; PY=ylev+352/2-24; 
		
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

            if((mode_disc & 2) && cios==249) cios=222; // force cIOS 222 for USB DVD

			if(Draw_button2(36+6*12, m+56, " cIOS 222 ", force_ios249 ? -1 : cios==222)) select_game_bar=300;
			if(Draw_button2(x_temp+12, m+56, " cIOS 223 ", force_ios249 ? -1 : cios==223)) select_game_bar=301;
			if(Draw_button2(x_temp+12, m+56, " cIOS 249 ",(mode_disc & 2)? -1: cios==249)) select_game_bar=302;


			if(Draw_button2(x_temp+20, m+56, " Skip IOS ", force_ios249 ? -1 : force_ingame_ios!=0)) select_game_bar=303;

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=10;
			if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=11;

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

			if(n==current_partition)
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
		
		cnt2=cnt;//=actual_game;

		for(m=0;m<3;m++)
		for(n=0;n<5;n++)
			{
			game_datas[(m*5)+n].ind=-1;
			if(gameList==NULL) cnt=gameCnt+10;
			else
			if(is_favorite) 
				{
				int g;
				
				if(config_file.id[(m*5)+n][0]==0)
					{
					cnt=gameCnt+1;
					}
				else
					{
					cnt=gameCnt+1;
					for(g=0;g<gameCnt;g++)
						{
						struct discHdr *header = &gameList[g];
						if(strncmp((char *) header->id, (char *) &config_file.id[(m*5)+n][0],6)==0)
							{
							cnt=g;break;
							}
						}
					}

				}
        
		    if(is_favorite) cnt2=cnt;

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
						WBFS_GetProfileDatas(discid, temp_data);
						create_game_png_texture(16+(m*5)+n);
						}
					cnt2++;
					}
				cnt=game_datas[16+(m*5)+n].ind;
				if(cnt<0) cnt=gameCnt+1;
				}
			
			//if(cnt>=gameCnt) cnt=0;

			if(cnt<gameCnt)
				{
				struct discHdr *header = &gameList[cnt];

				if(!scroll_mode) game_datas[16*(scroll_mode!=0)+(m*5)+n].ind=cnt;
				
				memcpy(discid,header->id,6); discid[6]=0;

				if(load_png && scroll_mode==0)
					{
					memset(temp_data,0,256*1024);
					WBFS_GetProfileDatas(discid, temp_data);
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

				cnt++;
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
			
						if(px>=0 && px<=80 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(40, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= 40-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
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
							DrawFillEllipse(40, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= 40-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("-");
							DrawEllipse(40, ylev+220, 40, 40, 0, 6, 0xc0000000);
							}
						

					
						if(px>=SCR_WIDTH-82 && px<=SCR_WIDTH-2 && py>=ylev+220-40 && py<=ylev+220+40)
							{
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 0xc0f0f0f0);
							letter_size(32,64);
							PX= SCR_WIDTH-42-16; PY= ylev+220-32; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 50, 50, 0, 6, 0xc0f0f000);
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
							DrawFillEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 0xc0f0f0f0);
							letter_size(32,48);
							PX= SCR_WIDTH-42-16; PY= ylev+220-24; color= 0xff000000; bkcolor=0;
							s_printf("+");
							DrawEllipse(SCR_WIDTH-42, ylev+220, 40, 40, 0, 6, 0xc0000000);

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
			if(select_game_bar>=10000 && select_game_bar<10004 && parental_mode==0)
				{
					SetTexture(NULL);
					DrawRoundFillBox(20, ylev+ 3*110, 150*4, 108, 0, 0xcfcfffff);
					DrawRoundBox(20, ylev+ 3*110, 150*4, 108, 0, 6, 0xcff08000);
					letter_size(16,32);

					PX= 0; PY=ylev+ (3*110)+(108-32)/2; color= 0xff000000; 
								
					bkcolor=0;
						
					autocenter=1;
					s_printf("Select Partition #%i",select_game_bar-9999);
					autocenter=0;
					parental_control_on=1;

				}
			else if(select_game_bar==10016 && parental_mode==0)
				{
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

		temp_pad= wiimote_read(); 
		new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						px=wmote_datas->ir.x;py=wmote_datas->ir.y;
						
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

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(guitar_pos_x<16) guitar_pos_x=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}

						if(guitar_pos_x<0) guitar_pos_x=0;
						if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);
						if(guitar_pos_y<0) guitar_pos_y=0;
						if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);
						
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

									if(select_game_bar==10016)  {parental_control_on=1;snd_fx_yes();mode_disc=1;Screen_flip();goto get_games;}

									if(select_game_bar>=10000 && select_game_bar<10004)  {snd_fx_yes();current_partition=select_game_bar-10000;Screen_flip();goto get_games;}

									if(temp_sel>=0)
									     snd_fx_yes();
									else snd_fx_no();

									game_mode=temp_sel+1;

									header = &gameList[game_datas[game_mode-1].ind];
									
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
														mode_disc=0;f_j=1;parental_control_on=1;parental_mode=0;
														}
													if(!mode_disc && game_mode && !edit_cfg) parental_mode=0;

													game_mode=0;edit_cfg=0;select_game_bar=0;
													for(n=0;n<15;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
													load_png=1;
													direct_launch=0;
													rumble=0;
													last_select=select_game_bar;
													usleep(60*1000);
													wiimote_rumble(0);
												    WPAD_ScanPads();
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

												   if(!mode_disc)
													  {
													  
													  WBFS_GetProfileDatas(discid, temp_data);
													  }
													 else
													   dol_GetProfileDatas(discid, temp_data);


												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
												   
												   if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; 
												   else { if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}

												   forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

												   langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

												   force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
												   game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);

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
												    select_game_bar=0;is_favorite=1; 

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
													
													
								/*
													if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}
													snd_fx_yes();

													if(mode_disc) {remote_call(remote_DVD_disc);usleep(1000*50);}*/
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
							if(select_game_bar==5 || select_game_bar==1000 || select_game_bar==1001) 
													{
													struct discHdr *header;

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
														

														ret=load_game_routine(discid, game_mode | (0x800000 *(mode_disc!=0)) | (0x200000 * (( mode_disc & 2)!=0) ) );

														//////////////////////////////////
														remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
													
														SetTexture(&text_background2);
														DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, BACK_COLOR);
			
												
														SelectFontTexture(1); // selecciona la fuente de letra extra

														letter_size(8,32);
																
														PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
																
														bkcolor=0;
														autocenter=1;
														SetTexture(NULL);

														DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
														DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);
														if(ret==-7777)
															s_printf("ERROR: You need cIOS223 from uLoader 3.0 v4 to work!!!\n");
														else
														if(ret==666)s_printf("ERROR FROM THE LOADER: Disc ID is not equal!\n"); 
														else 
															s_printf("ERROR FROM THE LOADER: %i\n", ret);

														Screen_flip();


														//////////////////////////////////
														break;
														}
												   } // load game
							// josete2k
                            if(select_game_bar==6) {if(mode_disc) {remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);}
							                         exit_by_reset=return_reset; // fixed by hermes //SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
													}

							if(select_game_bar==10 || select_game_bar==11) { // return from config (saving)
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
														if(cios==249) temp_data[4]|=1;
														if(cios==223) temp_data[4]|=2;
														temp_data[4]|=(forcevideo & 3)<<2;
														temp_data[4]|=(langsel & 15)<<4;
														temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;
														temp_data[7]|=((game_locked_cfg!=0) & 1)<<6;
											
														}

												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
												   select_game_bar=0;
												   rumble=0;
												   usleep(60*1000);
												   wiimote_rumble(0);
												   WPAD_ScanPads();
												   if(!mode_disc)
														WBFS_SetProfileDatas(discid, temp_data);
												   else 
													   dol_SetProfileDatas(discid, temp_data);
												   
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
														   cios=249;break;
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
							{// return from config (saving)
							edit_cfg=0;
							snd_fx_yes();
							
							
								// si no existe crea uno
								if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
									{
									temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
									temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
									temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
									}
								
								
								temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
								if(cios==249) temp_data[4]|=1;
								if(cios==223) temp_data[4]|=2;
								temp_data[4]|=(forcevideo & 3)<<2;
								temp_data[4]|=(langsel & 15)<<4;
								temp_data[7]|=((force_ingame_ios!=0) & 1)<<7;
								temp_data[7]|=((game_locked_cfg!=0) & 1)<<6;

								game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);
								if(!mode_disc)
									WBFS_SetProfileDatas(discid, temp_data);
								else
									dol_SetProfileDatas(discid, temp_data);
							
							}
						else
							{
							if(!(mode_disc & 1024) && remote_DVD_disc_status!=1)
								{
								int f_j=0;

								if(mode_disc && game_mode && !edit_cfg) 
									{
									remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
									mode_disc=0;f_j=1;parental_control_on=1;parental_mode=0;
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
					if((new_pad & WPAD_BUTTON_2) && game_mode==0 && insert_favorite==0 && scroll_mode==0 && parental_mode==0) go_game=1;

					if((new_pad & WPAD_BUTTON_HOME) && (game_mode==0 || (game_mode!=0 && mode_disc && disc_header && !edit_cfg)) && insert_favorite==0 && scroll_mode==0 && parental_mode==0) 
						{//go_home=1;
						if(parental_control_on)
										{
										parental_mode=1;
										}
									else go_home=1;
						}
							
			
			} 
			else {px=-100;py=-100;}
		}

	if(direct_launch && game_mode==1 && gameList!=NULL) 
		{

		if(launch_counter<=0)
			{
			struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

			snd_fx_yes();

			memcpy(discid,header->id,6); discid[6]=0;
														
			if(load_cheats(discid)) cheat_mode=1;

			set_cheats_list(discid);
			create_cheats();

			
														
			ret=load_game_routine(discid, game_mode);

			remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

			SetTexture(&text_background2);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, BACK_COLOR);


			SelectFontTexture(1); // selecciona la fuente de letra extra

			letter_size(8,32);
					
			PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
					
			bkcolor=0;
			autocenter=1;
			SetTexture(NULL);

			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			if(ret==-7777)
				s_printf("ERROR: You need cIOS223 from uLoader 2.6 v3 to work!!!\n");
			else
			if(ret==666) s_printf("ERROR FROM THE LOADER: Disc ID is not equal!\n"); 
			else 
				s_printf("ERROR FROM THE LOADER: %i\n", ret);

			Screen_flip();


			//////////////////////////////////
			break;
			}
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

			rumble=0;
			usleep(60*1000);
			wiimote_rumble(0);
			WPAD_ScanPads();
			
			SelectFontTexture(0);
			if(scroll_mode) volume_osd=0;
			}
	else
		{

		if(scroll_mode) volume_osd=0;
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
			if(temp_sel>=0)
				{//
				int g;

				g=game_datas[temp_sel].ind;
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
	WPAD_Shutdown();
	USBStorage2_Umount();
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
	WPAD_Shutdown();
error_0:
	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
	remote_end();

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
	//USBStorage2_Umount();

	if(return_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	if(exit_by_reset==3)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);


    return ret;
}

int load_game_routine(u8 *discid, int game_mode)
{
int n, ret,ret2;
int force_ingame_ios=0;

int flag_disc=game_mode & 0xa00000;

	Screen_flip();
	wiimote_rumble(0); 
	WPAD_ScanPads();
	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;
	//WPAD_Shutdown();

	game_mode &= ~0xa00000;

	add_game_log(discid);

	if(!flag_disc)
		save_cfg(0);
	else 
		save_cfg(1);

	if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; 
		else { if(game_datas[game_mode-1].config & 2) cios=223; else cios=222;}


	forcevideo=(game_datas[game_mode-1].config>>2) & 3;//if(forcevideo==3) forcevideo=0;

	langsel=(game_datas[game_mode-1].config>>4) & 15;if(langsel>10) langsel=0;

	force_ingame_ios=1*((game_datas[game_mode-1].config>>31)!=0);
	//game_locked_cfg=1*((game_datas[game_mode-1].config & (1<<30))!=0);
	
	// load alternative DOL
//	sprintf((char *) temp_data,"sd:/games/%s.dol",(char *) discid);
//	load_file_dol((char *) temp_data);

	//if(!flag_disc)
		Get_AlternativeDol(discid);
	/*else 
		{
		if(load_alt_game_disc)
			{
			sprintf((char *) temp_data,"sd:/games/%s.dol",(char *) discid);
			load_file_dol((char *) temp_data);
			}
		}
	*/
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
    USBStorage2_Watchdog(0); // disable watchdog
	USBStorage2_Deinit();
	WDVD_Close();
	
	#ifdef USE_MODPLAYER
	#ifdef ALTERNATIVE_VERSION
		StopOgg();
	#endif
	ASND_End();		// finaliza el sonido
	#endif
	if((cios!=222 && force_ios249==0) || force_reload_ios222)
		{
		cabecera2("Loading...");
		
		WPAD_Shutdown();
		usleep(500*1000);

		ret2=IOS_ReloadIOS(cios);
        if(cios==223 && ret2==0)
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
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);	
		

		for(n=0;n<25;n++)
			{
			cabecera2("Loading...");

				ret2 = USBStorage2_Init(); 
				if(!ret2) break;
				usleep(500*1000);
			}
		
		USBStorage2_Deinit();
		
		
		}

	// enables fake ES_ioctlv (to skip IOS_ReloadIOS(ios))
	if(force_ingame_ios)
		{
		patch_datas[0]=*((u32 *) (dip_plugin+16*4));
		mload_set_ES_ioctlv_vector((void *) patch_datas[0]);
		}
		
	mload_close();
	
	if(flag_disc & 0x200000) discid[6]=2; // mode DVD USB
	else if(flag_disc) discid[6]=1; // mode DVD

	ret = load_disc(discid);

return ret;
}

#include <ogc/lwp_watchdog.h>


// from coverflow loader

bool compare_videomodes(GXRModeObj* mode1, GXRModeObj* mode2)
{
	
	if (mode1->viTVMode != mode2->viTVMode || mode1->fbWidth != mode2->fbWidth ||	mode1->efbHeight != mode2->efbHeight || mode1->xfbHeight != mode2->xfbHeight ||
	mode1->viXOrigin != mode2->viXOrigin || mode1->viYOrigin != mode2->viYOrigin || mode1->viWidth != mode2->viWidth || mode1->viHeight != mode2->viHeight ||
	mode1->xfbMode != mode2->xfbMode || mode1->field_rendering != mode2->field_rendering || mode1->aa != mode2->aa || mode1->sample_pattern[0][0] != mode2->sample_pattern[0][0] ||
	mode1->sample_pattern[1][0] != mode2->sample_pattern[1][0] ||	mode1->sample_pattern[2][0] != mode2->sample_pattern[2][0] ||
	mode1->sample_pattern[3][0] != mode2->sample_pattern[3][0] ||	mode1->sample_pattern[4][0] != mode2->sample_pattern[4][0] ||
	mode1->sample_pattern[5][0] != mode2->sample_pattern[5][0] ||	mode1->sample_pattern[6][0] != mode2->sample_pattern[6][0] ||
	mode1->sample_pattern[7][0] != mode2->sample_pattern[7][0] ||	mode1->sample_pattern[8][0] != mode2->sample_pattern[8][0] ||
	mode1->sample_pattern[9][0] != mode2->sample_pattern[9][0] ||	mode1->sample_pattern[10][0] != mode2->sample_pattern[10][0] ||
	mode1->sample_pattern[11][0] != mode2->sample_pattern[11][0] || mode1->sample_pattern[0][1] != mode2->sample_pattern[0][1] ||
	mode1->sample_pattern[1][1] != mode2->sample_pattern[1][1] ||	mode1->sample_pattern[2][1] != mode2->sample_pattern[2][1] ||
	mode1->sample_pattern[3][1] != mode2->sample_pattern[3][1] ||	mode1->sample_pattern[4][1] != mode2->sample_pattern[4][1] ||
	mode1->sample_pattern[5][1] != mode2->sample_pattern[5][1] ||	mode1->sample_pattern[6][1] != mode2->sample_pattern[6][1] ||
	mode1->sample_pattern[7][1] != mode2->sample_pattern[7][1] ||	mode1->sample_pattern[8][1] != mode2->sample_pattern[8][1] ||
	mode1->sample_pattern[9][1] != mode2->sample_pattern[9][1] ||	mode1->sample_pattern[10][1] != mode2->sample_pattern[10][1] ||
	mode1->sample_pattern[11][1] != mode2->sample_pattern[11][1] || mode1->vfilter[0] != mode2->vfilter[0] ||
	mode1->vfilter[1] != mode2->vfilter[1] ||	mode1->vfilter[2] != mode2->vfilter[2] || mode1->vfilter[3] != mode2->vfilter[3] ||	mode1->vfilter[4] != mode2->vfilter[4] ||
	mode1->vfilter[5] != mode2->vfilter[5] ||	mode1->vfilter[6] != mode2->vfilter[6] )
	{
		return false;
	} else
	{
		
		return true;
	}
	
}



void patch_videomode(GXRModeObj* mode1, GXRModeObj* mode2)
{
	mode1->viTVMode = mode2->viTVMode;
	mode1->fbWidth = mode2->fbWidth;
	mode1->efbHeight = mode2->efbHeight;
	mode1->xfbHeight = mode2->xfbHeight;
	mode1->viXOrigin = mode2->viXOrigin;
	mode1->viYOrigin = mode2->viYOrigin;
	mode1->viWidth = mode2->viWidth;
	mode1->viHeight = mode2->viHeight;
	mode1->xfbMode = mode2->xfbMode;
	mode1->field_rendering = mode2->field_rendering;
	mode1->aa = mode2->aa;
	mode1->sample_pattern[0][0] = mode2->sample_pattern[0][0];
	mode1->sample_pattern[1][0] = mode2->sample_pattern[1][0];
	mode1->sample_pattern[2][0] = mode2->sample_pattern[2][0];
	mode1->sample_pattern[3][0] = mode2->sample_pattern[3][0];
	mode1->sample_pattern[4][0] = mode2->sample_pattern[4][0];
	mode1->sample_pattern[5][0] = mode2->sample_pattern[5][0];
	mode1->sample_pattern[6][0] = mode2->sample_pattern[6][0];
	mode1->sample_pattern[7][0] = mode2->sample_pattern[7][0];
	mode1->sample_pattern[8][0] = mode2->sample_pattern[8][0];
	mode1->sample_pattern[9][0] = mode2->sample_pattern[9][0];
	mode1->sample_pattern[10][0] = mode2->sample_pattern[10][0];
	mode1->sample_pattern[11][0] = mode2->sample_pattern[11][0];
	mode1->sample_pattern[0][1] = mode2->sample_pattern[0][1];
	mode1->sample_pattern[1][1] = mode2->sample_pattern[1][1];
	mode1->sample_pattern[2][1] = mode2->sample_pattern[2][1];
	mode1->sample_pattern[3][1] = mode2->sample_pattern[3][1];
	mode1->sample_pattern[4][1] = mode2->sample_pattern[4][1];
	mode1->sample_pattern[5][1] = mode2->sample_pattern[5][1];
	mode1->sample_pattern[6][1] = mode2->sample_pattern[6][1];
	mode1->sample_pattern[7][1] = mode2->sample_pattern[7][1];
	mode1->sample_pattern[8][1] = mode2->sample_pattern[8][1];
	mode1->sample_pattern[9][1] = mode2->sample_pattern[9][1];
	mode1->sample_pattern[10][1] = mode2->sample_pattern[10][1];
	mode1->sample_pattern[11][1] = mode2->sample_pattern[11][1];
	mode1->vfilter[0] = mode2->vfilter[0];
	mode1->vfilter[1] = mode2->vfilter[1];
	mode1->vfilter[2] = mode2->vfilter[2];
	mode1->vfilter[3] = mode2->vfilter[3];
	mode1->vfilter[4] = mode2->vfilter[4];
	mode1->vfilter[5] = mode2->vfilter[5];
	mode1->vfilter[6] = mode2->vfilter[6];
}

GXRModeObj* vmodes[] = {
	&TVNtsc240Ds,
	&TVNtsc240DsAa,
	&TVNtsc240Int,
	&TVNtsc240IntAa,
	&TVNtsc480IntDf,
	&TVNtsc480IntAa,
	&TVNtsc480Prog,
	&TVMpal480IntDf,
	&TVPal264Ds,
	&TVPal264DsAa,
	&TVPal264Int,
	&TVPal264IntAa,
	&TVPal524IntAa,
	&TVPal528Int,
	&TVPal528IntDf,
	&TVPal574IntDfScale,
	&TVEurgb60Hz240Ds,
	&TVEurgb60Hz240DsAa,
	&TVEurgb60Hz240Int,
	&TVEurgb60Hz240IntAa,
	&TVEurgb60Hz480Int,
	&TVEurgb60Hz480IntDf,
	&TVEurgb60Hz480IntAa,
	&TVEurgb60Hz480Prog,
	&TVEurgb60Hz480ProgSoft,
	&TVEurgb60Hz480ProgAa
};

GXRModeObj* PAL2NTSC[]={
	&TVMpal480IntDf,		&TVNtsc480IntDf,
	&TVPal264Ds,			&TVNtsc240Ds,
	&TVPal264DsAa,			&TVNtsc240DsAa,
	&TVPal264Int,			&TVNtsc240Int,
	&TVPal264IntAa,			&TVNtsc240IntAa,
	&TVPal524IntAa,			&TVNtsc480IntAa,
	&TVPal528Int,			&TVNtsc480IntAa,
	&TVPal528IntDf,			&TVNtsc480IntDf,
	&TVPal574IntDfScale,	&TVNtsc480IntDf,
	&TVEurgb60Hz240Ds,		&TVNtsc240Ds,
	&TVEurgb60Hz240DsAa,	&TVNtsc240DsAa,
	&TVEurgb60Hz240Int,		&TVNtsc240Int,
	&TVEurgb60Hz240IntAa,	&TVNtsc240IntAa,
	&TVEurgb60Hz480Int,		&TVNtsc480IntAa,
	&TVEurgb60Hz480IntDf,	&TVNtsc480IntDf,
	&TVEurgb60Hz480IntAa,	&TVNtsc480IntAa,
	&TVEurgb60Hz480Prog,	&TVNtsc480Prog,
	&TVEurgb60Hz480ProgSoft,&TVNtsc480Prog,
	&TVEurgb60Hz480ProgAa,  &TVNtsc480Prog,
	0,0
};

GXRModeObj* NTSC2PAL[]={
	&TVNtsc240Ds,			&TVPal264Ds,
	&TVNtsc240DsAa,			&TVPal264DsAa,
	&TVNtsc240Int,			&TVPal264Int,
	&TVNtsc240IntAa,		&TVPal264IntAa,
	&TVNtsc480IntDf,		&TVPal528IntDf,
	&TVNtsc480IntAa,		&TVPal524IntAa,
	&TVNtsc480Prog,			&TVPal528IntDf,
	0,0
};

GXRModeObj* NTSC2PAL60[]={
	&TVNtsc240Ds,			&TVEurgb60Hz240Ds,
	&TVNtsc240DsAa,			&TVEurgb60Hz240DsAa,
	&TVNtsc240Int,			&TVEurgb60Hz240Int,
	&TVNtsc240IntAa,		&TVEurgb60Hz240IntAa,
	&TVNtsc480IntDf,		&TVEurgb60Hz480IntDf,
	&TVNtsc480IntAa,		&TVEurgb60Hz480IntAa,
	&TVNtsc480Prog,			&TVEurgb60Hz480Prog,
	0,0
};

bool Search_and_patch_Video_Modes(void *Address, u32 Size )
{
	u8 *Addr = (u8 *)Address;
	bool found = 0;
	u32 i;
	GXRModeObj** Table= NULL;

	switch(_Video_Mode)
			{
			case PAL:
			case MPAL:
				Table = NTSC2PAL;break;

			case PAL60:
				Table = NTSC2PAL60;break;

            default:
				Table = PAL2NTSC;
				break;
			}

	while(Size >= sizeof(GXRModeObj))
	{



		for(i = 0; Table[i]; i+=2)
		{


			if(compare_videomodes(Table[i], (GXRModeObj*)Addr))

			{
				found = 1;
				patch_videomode((GXRModeObj*)Addr, Table[i+1]);
				Addr += (sizeof(GXRModeObj)-4);
				Size -= (sizeof(GXRModeObj)-4);
				break;
			}
		}

		Addr += 4;
		Size -= 4;
	}


	return found;
}

#if 0
void mem_patch(void *buffer, u32 len, u8 *s, int s_size, int p_skip, u8 *p, int p_size)
{
int n;

   /* Patch data*/

for(n=0;n<(len-s_size);n+=4)
   {
   if(!memcmp(buffer+n, (void *) s, s_size))
      {
      memcpy(buffer+n+p_skip, (void *) p,  p_size);
      }
   }
}
#endif

int compare_hex_str(u8 *buff, const u8 *hex_str)
{
u8 dat=0;

while(*hex_str!=0)
	{
	
	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}

	if(*hex_str>='A' && *hex_str<='F') dat=(10+*hex_str-'A')<<4;
	else if(*hex_str>='a' && *hex_str<='f') dat=(10+*hex_str-'a')<<4;
	else if(*hex_str>='0' && *hex_str<='9') dat=(*hex_str-'0')<<4;
	else if(*hex_str=='x' || *hex_str<='X') dat= *buff & 0xf0;
	else {return 0;}

	hex_str++;
	if(hex_str==0) return 0;

	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}
	
	if(*hex_str>='A' && *hex_str<='F') dat|=(10+*hex_str-'A');
	else if(*hex_str>='a' && *hex_str<='f') dat|=(10+*hex_str-'a');
	else if(*hex_str>='0' && *hex_str<='9') dat|=(*hex_str-'0');
	else if(*hex_str=='x' || *hex_str<='X') dat|= *buff & 0xf;
	else {return 0;}	
	
	if(dat!=*buff) return 0;

	buff++; hex_str++;
	}

return 1;
}

int set_hex_str(u8 *buff, const u8 *hex_str)
{
u8 dat=0;

while(*hex_str!=0)
	{
	
	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}

	if(*hex_str>='A' && *hex_str<='F') dat=(10+*hex_str-'A')<<4;
	else if(*hex_str>='a' && *hex_str<='f') dat=(10+*hex_str-'a')<<4;
	else if(*hex_str>='0' && *hex_str<='9') dat=(*hex_str-'0')<<4;
	else if(*hex_str=='x' || *hex_str<='X') dat= *buff & 0xf0;
	else {return 0;}

	hex_str++;
	if(hex_str==0) return 0;

	//while(*hex_str<=' ') {hex_str++;if(hex_str==0) return 0;}
	
	if(*hex_str>='A' && *hex_str<='F') dat|=(10+*hex_str-'A');
	else if(*hex_str>='a' && *hex_str<='f') dat|=(10+*hex_str-'a');
	else if(*hex_str>='0' && *hex_str<='9') dat|=(*hex_str-'0');
	else if(*hex_str=='x' || *hex_str<='X') dat|= *buff & 0xf;
	else {return 0;}	
	
	*buff++=dat;
	hex_str++;
	}

return 1;
}


void patch_hex_str(u8 *buffer, u32 len, u32 step, const u8 *search, int skip_patch, const u8 *patch, int counter )
{
 u32 cnt;

for (cnt = 0; cnt < len; cnt+=step) 
	{
	u8 *ptr = buffer + cnt;

		/* Replace code if found */
		if (compare_hex_str(ptr, search))
			{
		    set_hex_str(ptr+skip_patch, patch);
			cnt=(((cnt+(strlen((char *) search)>>1))/step)*step)-step;
			
			counter--;
			if(counter==0) return;
		
			}
			
	}
}


#if 0
void __Patch_DiscSeek(void *buffer, u32 len)
{
   const   u8 SearchPattern[] =  "38A000DA7CA401AE7C671B787FCAF3783929XXXX800DBXXC388000DA80ADB3XX"; 
   const   u8 PatchData[] =      "38A000717CA401AE7C671B787FCAF3783929XXXX800DB3XX3880007180ADB3XX";
   
   
   patch_hex_str((u8 *) buffer, (len - (strlen((char *) SearchPattern)>>1)),4 , SearchPattern, 0, PatchData, 0);

  
}
#endif

// Based in Waninkoko patch

void __Patch_CoverRegister(void *buffer, u32 len)
{
   const u8 oldcode[] = "5460F7FF4082000C546007FF4182000C";
   const u8 newcode[] = /*"5460F7FF4082000C546007FF"*/ "4800000C";

   patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 12, newcode, 0);

}

void __Patch_Error001(void *buffer, u32 len)
{
	const u8 oldcode[] = "4082000C386000014800024438610018";
	const u8 newcode[] = "40820004";

	patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 0, newcode, 0);
}


#if 0
void __Patch_NSMBW(void *buffer, u32 len)
{

const u8 oldcode[] = "9421FFD07C0802A690010034396100304812XXXX7C7B1B787C9C23787CBD2B78";
const u8 newcode[] = "4E800020";

	patch_hex_str((u8 *) buffer, (len - (strlen((char *) oldcode)>>1)),4 , oldcode, 0, newcode, 0);
}
#endif

#if 0

int load_file_dol(char *name)
{

int ret=0;
FILE *fp=NULL;
int n,m, size;

AlternativeDol_infodat.id[0]=0;
dol_len= 0;

	if(sd_ok)
		fp=fopen(name,"rb"); // lee .dol desde sd

    if(!fp && ud_ok)
		{
		name[0]='u';
		fp=fopen(name,"rb"); // lee .dol desde usb
		}

	if(fp)
		{
		fseek(fp, 0, SEEK_END);
		size=dol_len = ftell(fp);
		dol_data= (u8 *) SYS_AllocArena2MemLo(dol_len+32768,32);//0x92000000;//malloc(dol_len);
		if(!dol_data) {fclose(fp);return -1;}

		fseek(fp, 0, SEEK_SET);

		n=0;

		while(size>0)
			{
			m=size;
			if(m>65536) m=65536;
		
			cabecera2( "Loading Alternative .dol");		

			ret=fread(dol_data+n, 1, m, fp);
            if(ret!=m) {fclose(fp);dol_data= NULL;dol_len= 0;return -1;}
			n+=ret;
			size-=ret;
		
			}
		
		fclose(fp);
		
		}

	
return 0;
}
#endif 

void patch_dol(void *Address, int Section_Size, int mode)
{
	DCFlushRange(Address, Section_Size);
	//if(mode)
	__Patch_Error001((void *) Address, Section_Size);

	__Patch_CoverRegister(Address, Section_Size);

	//__Patch_DiscSeek(Address, Section_Size);

	//__Patch_NSMBW((void *) Address, Section_Size);
	
	
	/*HOOKS STUFF - FISHEARS*/
	dogamehooks(Address, Section_Size);

	/*LANGUAGE PATCH - FISHEARS*/
	langpatcher(Address, Section_Size);

	if(!forcevideo || forcevideo==1)
		Search_and_patch_Video_Modes(Address, Section_Size);

	vidolpatcher(Address, Section_Size);


	/*HOOKS STUFF - FISHEARS*/

	DCFlushRange(Address, Section_Size);
}

static void __noprint(const char *fmt, ...)
{
}

int load_disc(u8 *discid)
{
        static struct DiscHeader Header ALIGNED(0x20);
        static struct Partition_Descriptor Descriptor ALIGNED(0x20);
        static struct Partition_Info Partition_Info ALIGNED(0x20);
        int i;
		

        memset(&Header, 0, sizeof(Header));
        memset(&Descriptor, 0, sizeof(Descriptor));
        memset(&Partition_Info, 0, sizeof(Partition_Info));

		cabecera2( "Loading...");

		WDVD_Init();

	
        if(discid[6]==2) // DVD USB
			{
			if(WDVD_SetUSBMode((u8 *) "_DVD__",0)!=0) return 1;
			}
		else
		if(discid[6]!=0)
			{
			if(WDVD_SetUSBMode(NULL,0)!=0) return 1;
			}
		else
			{
           if(WDVD_SetUSBMode(discid, current_partition)!=0) return 1;
			}
		

        WDVD_Reset();

		
		//return -456;
		
        memset(Disc_ID, 0, 0x20);
        WDVD_ReadDiskId(Disc_ID);
		

        if (*Disc_ID==0x10001 || *Disc_ID==0x10001)
                return 2;

		
      
        Determine_VideoMode(*Disc_Region);
	   
		WDVD_UnencryptedRead(&Header, sizeof(Header), 0);

	
		/* BCA Data can be present at 0x100 offset in the .ISO (normally is a padding area filled with zeroes, but uLoader use it for BCA datas) */

		for(i=0xa2;i<0xe2;i++) if(Header.Padding[i]!=0) break; // test for bca data
        
		if(i==0xe2) // if filled with zeroes set for NSMB bca datas
			{
			memset(BCA_Data,0,64);
			BCA_Data[0x33]=1;
			
			// write in dip_plugin bca data area
			mload_seek(*((u32 *) (dip_plugin+15*4)), SEEK_SET);	// offset 15 (bca_data area)
			mload_write(BCA_Data, 64);
			mload_close();
			}
		else
			{
			memcpy(BCA_Data, &Header.Padding[0xa2],64);

			// write in dip_plugin bca data area
            mload_seek(*((u32 *) (dip_plugin+15*4)), SEEK_SET);	// offset 15 (bca_data area)
			mload_write(BCA_Data, 64);
			mload_close();
			}

		if(discid[6]==0)
			{
			if(strncmp((char *) &Header, (char *) discid, 6)) 
				return 666; // if headerid != discid (on hdd) error
			}
		
		else  memcpy((char *) discid, (char *) &Header, 6);

		cabecera2( "Loading...");

        u64 Offset = 0x00040000; // Offset into disc to partition descriptor
        WDVD_UnencryptedRead(&Descriptor, sizeof(Descriptor), Offset);

        Offset = ((u64) Descriptor.Primary_Offset) << 2;

		

        u32 PartSize = sizeof(Partition_Info);
        u32 BufferLen = Descriptor.Primary_Count * PartSize;
        
        // Length must be multiple of 0x20
        BufferLen += 0x20 - (BufferLen % 0x20);
        u8 *PartBuffer = (u8*)memalign(0x20, BufferLen);

        memset(PartBuffer, 0, BufferLen);
        WDVD_UnencryptedRead(PartBuffer, BufferLen, Offset);

		cabecera2("Loading...");

        struct Partition_Info *Partitions = (struct Partition_Info*)PartBuffer;
        for ( i = 0; i < Descriptor.Primary_Count; i++)
        {
                if (Partitions[i].Type == 0)
                {
                        memcpy(&Partition_Info, PartBuffer + (i * PartSize), PartSize);
                        break;
                }
        }
       
		Offset = ((u64) Partition_Info.Offset) << 2;

        free(PartBuffer);
        if (!Offset)
                return 3;

		

        WDVD_Seek(Offset);


        Offset = 0;
          
        signed_blob* Certs		= 0;
        signed_blob* Ticket		= 0;
        signed_blob* Tmd		= 0;
        
        unsigned int C_Length	= 0;
        unsigned int T_Length	= 0;
        unsigned int MD_Length	= 0;
        
        static u8	Ticket_Buffer[0x800] ALIGNED(32);
        static u8	Tmd_Buffer[0x49e4] ALIGNED(32);
        
        GetCerts(&Certs, &C_Length);
        WDVD_UnencryptedRead(Ticket_Buffer, 0x800, ((u64) Partition_Info.Offset) << 2);
        Ticket		= (signed_blob*)(Ticket_Buffer);
        T_Length	= SIGNED_TIK_SIZE(Ticket);

		cabecera2( "Loading...");


		 // Patch in info missing from apploader reads
        *Sys_Magic	= 0x0d15ea5e;
        *Version	= 1;
        *Arena_L	= 0x00000000;
		*BI2		= 0x817E5480;
        *Bus_Speed	= 0x0E7BE2C0;
        *CPU_Speed	= 0x2B73A840;

		/* Setup low memory */
		*(vu32 *)0x80000060 = 0x38A00040;
		*(vu32 *)0x800000E4 = 0x80431A80;
		*(vu32 *)0x800000EC = 0x81800000;

		*(vu32 *)0x800000F0 = 0x01800000;       // Simulated Memory Size

		if ((strncmp((char *) discid, "R3XE6U", 6)==0) || (strncmp((char *) discid, "R3XP6V", 6)==0))
			{
				*((vu32*) 0x80003184)	= 0x80000000;    // Game ID Address
			}

		DCFlushRange((void*)0x80000000, 0x3f00);
		

        // Open Partition and get the TMD buffer
       
		if (WDVD_OpenPartition((u64) Partition_Info.Offset, 0,0,0, Tmd_Buffer) != 0)
                return 4;
        Tmd = (signed_blob*)(Tmd_Buffer);
        MD_Length = SIGNED_TMD_SIZE(Tmd);
        static struct AppLoaderHeader Loader ALIGNED(32);

        WDVD_Read(&Loader, sizeof(Loader), 0x00002440);// Offset into the partition to apploader header
        DCFlushRange((void*)(((u32)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);

		cabecera2( "Loading...");


        // Read apploader from 0x2460
        WDVD_Read(Apploader, Loader.Size + Loader.Trailer_Size, 0x00002440 + 0x20);
        DCFlushRange((void*)(((int)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);

		cabecera2( "Loading...");
	

        AppLoaderStart	Start	= Loader.Entry_Point;
        AppLoaderEnter	Enter	= 0;
        AppLoaderLoad		Load	= 0;
        AppLoaderExit		Exit	= 0;
        Start(&Enter, &Load, &Exit);

		Enter(__noprint);

        void*	Address = 0;
        int		Section_Size;
        int		Partition_Offset;

		switch(langsel)
                {
                        case 0:
                                configbytes[0] = 0xCD;
                        break;

                        case 1:
                                configbytes[0] = 0x00;
                        break;

                        case 2:
                                configbytes[0] = 0x01;
                        break;

                        case 3:
                                configbytes[0] = 0x02;
                        break;

                        case 4:
                                configbytes[0] = 0x03;
                        break;

                        case 5:
                                configbytes[0] = 0x04;
                        break;

                        case 6:
                                configbytes[0] = 0x05;
                        break;

                        case 7:
                                configbytes[0] = 0x06;
                        break;

                        case 8:
                                configbytes[0] = 0x07;
                        break;

                        case 9:
                                configbytes[0] = 0x08;
                        break;

                        case 10:
                                configbytes[0] = 0x09;
                        break;
                }

	
   
		hooktype = 0;
		
		if(len_cheats)
			{ 
			/*HOOKS STUFF - FISHEARS*/
			memset((void*)0x80001800,0,kenobiwii_size);
			memcpy((void*)0x80001800,kenobiwii,kenobiwii_size);
			memcpy((void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
			DCFlushRange((void*)0x80001800,kenobiwii_size);
			memcpy((void*)0x800027E8, buff_cheats, len_cheats);
            *(vu8*)0x80001807 = 0x01;
			hooktype = 1;
			}

		
        //printf("Loading game");
		//if(!dol_data)
		
        while (Load(&Address, &Section_Size, &Partition_Offset))
        {
	
                if (!Address) return 5;
				cabecera2("Loading...");
                WDVD_Read(Address, Section_Size, ((u64) Partition_Offset) << 2);

				patch_dol(Address, Section_Size,0);

        }
		
	

		if(!strncmp((void *) AlternativeDol_infodat.id, (void *) discid, 6))
			{
			
			cabecera2("Loading Alternative .dol");
			
		
			WDVD_Read(dol_data, dol_len, AlternativeDol_infodat.offset);
		

			}


		WPAD_Shutdown();

		// Cleanup loader information
        WDVD_Close();

		

		#if 1
        // Identify as the game
        if (IS_VALID_SIGNATURE(Certs) 	&& IS_VALID_SIGNATURE(Tmd) 	&& IS_VALID_SIGNATURE(Ticket) 
            &&  C_Length > 0 				&& MD_Length > 0 			&& T_Length > 0)
        {
                int ret = ES_Identify(Certs, C_Length, Tmd, MD_Length, Ticket, T_Length, NULL);
                if (ret < 0)
                        return ret;
        }
		#endif

		

		// Retrieve application entry point
        void* Entry;

		

			if(dol_data)
			{
			
			
				cabecera2( "Loading Alternative .dol");
						
				
				Entry=(void *) load_dol();
			}
		else
			Entry= Exit();

		if(!Entry) return -999;

		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		remote_end();

        // Enable online mode in games
        memcpy(Online_Check, Disc_ID, 4);

		//*(u32 *)0x80003140 = *(u32 *)0x80003188; // removes 002-Error (by WiiPower: http://gbatemp.net/index.php?showtopic=158885&hl=)
        *(u32 *)0x80003188 = *(u32 *)0x80003140;

	

        // Set Video Mode based on Configuration
		if (vmode)
			Set_VideoMode();
		

 
	    usleep(100*1000);

       // debug_printf("start %p\n",Entry);
	   settime(secs_to_ticks(time(NULL) - 946684800));

       //

	   VIDEO_SetBlack(TRUE);
	   VIDEO_Flush();
	   VIDEO_WaitVSync();

	
	   // fix for PeppaPig
	   memcpy((char *) temp_data, (void*)0x800000F4,4);
	
       SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

       // fix for PeppaPig
	   memcpy((void*)0x800000F4,(char *) temp_data, 4);

	   // Flush application memory range
       DCFlushRange((void*)0x80000000, 0x17fffff);	// TODO: Remove these hardcoded value
	

        __asm__ __volatile__
                (
                        "mtlr %0;"			// Move the entry point into link register
                        "blr"				// Branch to address in link register
                        :					// No output registers
                        :	"r" (Entry)		// Input register
                                //:					// difference between C and cpp mode??
                        );
	return 0;
}


void update_bca(u8 *id, u8 *bca_datas)
{
u8 *r=NULL;
u8 *f;
int len=0,mode;
int pos=0,n,m,finded=0;

FILE *fp;

if(!sd_ok) return;

    fp = fopen("sd:/bca_database.txt", "rb");
	if(fp)
		{
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		r=malloc(len+16);
        if(r)
			len = fread(r, 1, len, fp);
        fclose(fp);
		if(!r) return;
		}
	
	f=malloc(9+136);
	if(!f) {if(r) free(r);return;}

	fp = fopen("sd:/bca_database.txt", "wb");
	if(!fp) 
		{
	    free(f);free(r);return;
		}

	n=0;

    mode=0;

	if(r!=NULL)
	while(n<len)
	{
	if(mode==0)
		{
		if(r[n]!='[') {n++;continue;}
		if(r[n]=='[' && r[n+7]==']' && r[n+8]<32)
			{
			mode=1;
			for(m=0;m<6;m++)
				if(r[n+m+1]!=id[m]) break;
			if(m==6) finded=1;
			
			for(m=0;m<8;m++)
				f[m]=r[n+m];
			f[m]='\n';

			n+=9;

			}
		else
			{
			n++;continue;
			}
		}

	if(mode==1)
		{
		int k=9;

		if(r[n]<32) {n++;continue;}

		for(m=0;m<64;m++)
			{
			u8 h=bca_datas[m]>>4;

			if((m & 7)==0 && m!=0) 
			{
		    if(m==32) f[k++]='\n'; else f[k++]=' ';
			//n++;
			while(r[n]<=32) {n++;if(n>=len) break;}
			}

			if(n>=len) break;
			if((r[n]>='0' && r[n]<='9') || (r[n]>='A' && r[n]<='F'))
				{
				if(finded==1) f[k++]=(h)+48+7*(h>=10);
					else f[k++]=r[n];
	
				n++;
			    }
			else break; // error!

            h=bca_datas[m] & 15;
			if((r[n]>='0' && r[n]<='9') || (r[n]>='A' && r[n]<='F'))
				{
				if(finded==1) f[k++]=(h)+48+7*(h>=10);
					else f[k++]=r[n];
				n++;
			    }
			else break; // error!

			}
		if(m==64) 
			{
			f[k++]='\n';n++;while(r[n]<=32) {n++;if(n>=len) break;}
			pos = fwrite(f, 1, 9+136, fp);
			if(pos!=(9+136)) break;
			}
		mode=0; if(finded==1) finded=2;
		}
	}
    
	if(finded==0)
		{
		int k=0;
		f[k++]='[';
		for(n=0;n<6;n++)
			f[k++]=id[n];
		f[k++]=']';
		f[k++]='\n';

		for(n=0;n<64;n++)
			{
			u8 h=bca_datas[n]>>4;

			if((n & 7)==0 && n!=0) 
				{
				if(n==32) f[k++]='\n'; else f[k++]=' ';
				}

			f[k++]=(h)+48+7*(h>=10);
			h=bca_datas[n] & 15;
			f[k++]=(h)+48+7*(h>=10);
		
			}
		f[k++]='\n';
        pos = fwrite(f, 1, 9+136, fp);
		}
	

	fclose(fp);

	if(r) free(r);
	
	free(f);

}

char buff_rcheat[256];

int gettype_line(u8 *b, u8 *v, int *nv)
{

int n;

while(*b==32 || *b==10) b++;

if(*b==0) return 0; // linea separadora

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[n>>1]<<=4; v[n>>1]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return 1; // cadena
	}

b+=8;

*nv=1;
while(*b==32 || *b==10) b++;
if(*b==0) return 2; // numero

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[4+(n>>1)]<<=4; v[4+(n>>1)]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return -1; // error en numero
	}

*nv=2;
return 2; // numero
}

void set_cheats_list(u8 *id)
{
int n,m;

int free=-1;

	if(!txt_cheats) return;

	for(n=0;n<1024;n++)
		{
		if(cheat_file.cheats[n].magic!=0xae && free<0) free=n;

		if(cheat_file.cheats[n].magic==0xae && !strncmp((char *) cheat_file.cheats[n].id, (char *) id, 6))
			{
			for(m=0;m<25;m++) cheat_file.cheats[n].sel[m]=(data_cheats[m].apply!=0);
			return;
			}
		}

	if(free<0)
		{
		free=1023;
		memcpy((char *) &cheat_file, ((char *) &cheat_file)+32, 32768-32);
		}

	memcpy((char *) cheat_file.cheats[free].id, (char *) id, 6);
	cheat_file.cheats[free].magic=0xae;
	for(m=0;m<25;m++) cheat_file.cheats[free].sel[m]=(data_cheats[m].apply!=0);
}

void get_cheats_list(u8 *id)
{
int n, m;

	if(!txt_cheats) return;

	for(n=0;n<1024;n++)
		{
		if(cheat_file.cheats[n].magic==0xae && !strncmp((char *) cheat_file.cheats[n].id, (char *) id, 6))
			{
			for(m=0;m<25;m++) data_cheats[m].apply= (cheat_file.cheats[n].sel[m]==1);
			return;
			}
		}

}


void create_cheats()
{
int n,m,f;

u8 data_head[8] = {
	0x00, 0xD0, 0xC0, 0xDE, 0x00, 0xD0, 0xC0, 0xDE
};
u8 data_end[8] = {
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

if(!txt_cheats) return;

	len_cheats=0;
	buff_cheats =  malloc (16384);
	if(buff_cheats == 0){
		return;
	}

	f=0;
	memcpy(buff_cheats,data_head,8);
	m=0;
	for(n=0;n<MAX_LIST_CHEATS;n++)
		{
		if(data_cheats[n].title && data_cheats[n].apply)
			{
			if(f==0) m+=8;f=1;
			memcpy(buff_cheats+m,data_cheats[n].values,data_cheats[n].len_values);
			m+=data_cheats[n].len_values;
			}
		}
	if(f)
		{
		memcpy(buff_cheats+m,data_end,8);m+=8;
		len_cheats=m;
		}


}

int load_cheats(u8 *discid)
{
char file_cheats[]="sd:/codes/000000.txt";

u8 data_readed[8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

u8 sub_head[32]="";
u8 temp_sub_head[32]="";

FILE *fp;
int ret;
int n,m;
int mode=0;

if(!sd_ok && !ud_ok) return 0;

actual_cheat=0;
num_list_cheats=0;
memcpy(&file_cheats[10],(char *) discid, 6);
len_cheats=0;

txt_cheats=1;
memset(data_cheats,0,sizeof(struct _data_cheats)*MAX_LIST_CHEATS);
fp=NULL;
if(sd_ok)
	{
	fp = fopen(file_cheats, "rb");
	if (!fp) 
		{
		txt_cheats=0;
		file_cheats[17]='g';file_cheats[18]='c';file_cheats[19]='t';
		fp = fopen(file_cheats, "rb");
		}
	}
if(!fp && ud_ok)
	{
	txt_cheats=1;
	file_cheats[0]='u';file_cheats[17]='t';file_cheats[18]='x';file_cheats[19]='t';
	fp = fopen(file_cheats, "rb");
	if (!fp) 
		{
		txt_cheats=0;
		file_cheats[17]='g';file_cheats[18]='c';file_cheats[19]='t';
		fp = fopen(file_cheats, "rb");
		}
	}


	if (!fp) {
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len_cheats = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	buff_cheats =  malloc (16384);
	if(buff_cheats == 0){
		fclose(fp);
		return 0;
	}

	if(txt_cheats)
		{
		
		n=m=0;
		mode=0;

		while(1)
			{
			int force_exit=0;
		
			if(fgets(buff_rcheat,256,fp)) 
				{
				int g=0,ret, ndatas;
				while(buff_rcheat[g]!=0 && buff_rcheat[g]!=10 && buff_rcheat[g]!=13) g++;
				buff_rcheat[g]=0;

				ret=gettype_line((u8 *) buff_rcheat, data_readed, &ndatas);

				switch(mode)
					{
					case 0: // get ID
						if(ret==1)
							{
							if(strncmp((char *) discid, (char *) buff_rcheat,6)!=0) force_exit=1; // error! código no coincide
							mode++;
							}
						break;

					case 1: // get name
						if(ret==0)
							mode++;
						break;

					case 2: // get entry name
						if(ret!=0)
							{
							int sub;
							if(ret!=1) force_exit=1;

							memcpy(temp_sub_head, buff_rcheat, (g>31) ? 31 : g);temp_sub_head[(g>31) ? 31 : g]=0;

							sub=strlen((char *) sub_head);

							if(g>(63-sub)) 
								{
								if(g>39) 
									g=39;
								if((63-g)<sub) sub=63-g;
								}

							memset(buff_cheats+m,32,63); // rellena de espacios
							buff_cheats[m+63]=0;
							
							
                            memcpy(buff_cheats+m+(63-(g+sub))/2,sub_head,sub);// centra nombre
							if(sub>0) buff_cheats[m+(63-(g+sub))/2+sub-1]='>';
							memcpy(buff_cheats+m+(63-(g+sub))/2+sub,buff_rcheat,g);// centra nombre
							g=63;

							data_cheats[n].title=buff_cheats+m;
							m+=g+1;
							data_cheats[n].values=buff_cheats+m;
							data_cheats[n].len_values=0;
							data_cheats[n].description=NULL;
							mode++;
							}
						break;
					case 3: // get entry codes
					case 4:
					    if(ret==0 || ret==1) 
							{
						    if(mode==4) 
								{	
								if(ret==1)
									{
									memcpy(buff_cheats+m,buff_rcheat,g+1);
									data_cheats[n].description=buff_cheats+m;
									m+=g+1;
									}
								n++; 
								}
							else 
								{
								int sub;
								if(data_cheats[n].title)
								  {memcpy(sub_head,temp_sub_head,31);sub_head[31]=0;
								   sub=strlen((char *) sub_head); if(sub<31) {sub_head[sub]='>';sub_head[sub+1]=0;}
								   data_cheats[n].title=NULL;}
							    }
							mode=2;if(n>=MAX_LIST_CHEATS) force_exit=1;
							}
						else
						if(ret==2)
							{
							memcpy(buff_cheats+m,data_readed,ndatas*4);
							data_cheats[n].len_values+=ndatas*4;
							m+=ndatas*4;
							mode=4;
							}
						else
						if(ret<0) {data_cheats[n].title=NULL;data_cheats[n].len_values=0;force_exit=1;}
						break;
					
					}
				}
			 else {
				  if(mode==4) n++; else data_cheats[n].title=NULL;n++;
				  break;
				  }
			
			if(force_exit) break;
			}
		fclose(fp);

		for(n=0;n<MAX_LIST_CHEATS;n++)
			if(data_cheats[n].title!=NULL) num_list_cheats=n+1; else break;

		get_cheats_list(discid);
		
		}
	else
		{
		ret = fread(buff_cheats, 1, len_cheats, fp);
		fclose(fp);

		if(ret != len_cheats){
			len_cheats=0;
			free(buff_cheats);
			return 0;
			}
		}

return 1;
}


void select_file(int flag, struct discHdr *header)
{
int n,m,signal2=2,f=0,len,max_entry;
static  int posfile=0;
static int old_flag=-1;
int flash=0;
int signal=1;
int frames2=0;

u8 *mem_png=NULL;
int len_png=0;
void * texture_png=NULL;
int mode=0;

int flag_auto=1;

have_device=(sd_ok!=0) | 2*(ud_ok!=0);

if(ud_ok && !sd_ok) path_file[0]='u';

read_list_file((void *) path_file, flag);

if(flag!=old_flag)
	{
	posfile=0;
	old_flag=flag;
	}

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	//int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	
   /* DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

*/
	SetTexture(NULL);
	DrawRoundFillBox(8,40, SCR_WIDTH-16, 336+8-32-40, 0, 0x6fffff00);			
	DrawRoundBox(8, 40, SCR_WIDTH-16, 336+8-32-40, 0, 2, 0xff30ffff);

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev-32; color= 0xff000000; 
				
		bkcolor=0;
		
		autocenter=1;
//		memcpy(id,header->id,6);id[6]=0;
		s_printf("%s",header->title);
		autocenter=0;

		// get automatic .png
		if(flag_auto)
			{
			for(n=0;n<nfiles;n++)
			{
			if(!files[n].is_directory)
				if(!strncmp((char *) header->id,get_name_from_UTF8(&files[n].name[0]), 6)) {signal=1;posfile=n;flag_auto=0;break;}
			}
			}
		
		}


	SetTexture(NULL);

	PX= 0; PY=ylev-32; color= 0xff000000; 
				
	bkcolor=0;
	letter_size(16,32);


	autocenter=0;

	max_entry=(320-32)/30;

	flash++;

	m=0;
	
	sizeletter=4;


	for(n=(posfile-max_entry/2)*(nfiles>=max_entry);n<posfile+max_entry;n++)
			{
			if(n<0) continue;
			if(n>=nfiles) break;
			if(m>=max_entry) break;
			
            
			PX=16;PY=48+2+m*30;
			color=0xff000000;
			
			/*if(px>=PX-8 && px<PX+8+16*38 && py>=PY-10 && py<PY+26) 
				select_game_bar=1024+n;//+2048*(kk==-1);*/
			
			if(n== posfile)
				{
				len=38;


				//if(flash & 8) 
				DrawFillBox(PX-8,PY-2-8, len*16+16, 36, 0, 0x6f00ff00);
				
				DrawBox(PX-8,PY-2-8, len*16+16, 36, 0, 2, 0xff30ffff);

				}

			if(files[n].is_directory)
				{
				if(files[n].name[0]=='.' && files[n].name[1]=='[')
					{
					color=0xffff6f00;
					s_printf("%s",&files[n].name[1]);
					}
				else
					{
					color=0xffff6f00;
				
					s_printf("<%s>",get_name_short_fromUTF8(&files[n].name[0]));
					}
				}
			else 
				{
				color=0xff000000;
	            if(files[n].is_selected) color=0xff000000;

				s_printf("%s", (char *) get_name_short_fromUTF8(&files[n].name[0]));
				}
			
			bkcolor=0;color=0xffffffff;
			m++;
			}
	if(mode==0) 
		{
		letter_size(12,32);
		color=0xff000000;
		PX=16;PY=480-160;
		s_printf(" Press PLUS to");
		PX=16;PY+=40;
		s_printf("Download Covers");
		PX=16;PY+=40;
		s_printf(" From Internet");
		PX=16;PY+=40;
		s_printf("or this folder");

		PX=396;PY=480-160;
		s_printf("  Pulsa MAS Para");
		PX=396;PY+=40;
		s_printf("Descargar Caratulas");
		PX=396;PY+=40;
		s_printf("  Desde Internet");
		PX=396;PY+=40;
		s_printf("%s","o éste directorio");
		}
	if(texture_png)
		{
		SetTexture(NULL);

		/*DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffafafaf);
		SetTexture(&png_texture);
		DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffffffff);
		SetTexture(NULL);
		DrawRoundBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 4, 0xff303030);
		*/

		DrawFillBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 0xffafafaf);
		SetTexture(&png_texture);
		DrawFillBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 0xffffffff);
		SetTexture(NULL);
		DrawBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 2, 0xff303030);
		

		if(mode>0) {mode+=signal2;if(mode==0) signal2=2;}
		if(mode>100) mode=100;
		
		if(mode!=0 && mode !=100) snd_fx_fx(mode);

		if(mode==100)
			{
			bkcolor=0;
			PX= 0; PY=480-40; color= 0xff000000; 
			letter_size(12,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][37][0]);
			autocenter=0;
			}
		
		}
	else
		{
		SetTexture(NULL);

		//DrawRoundFillBox(SCR_WIDTH/2-100, 480-128, 200, 128, 0, 0xffafafaf);
		//DrawRoundBox(SCR_WIDTH/2-100, 480-128, 200, 128, 0, 4, 0xff303030);

		DrawFillBox(SCR_WIDTH/2-64, 480-170, 128, 170, 0, 0xffafafaf);
		DrawBox(SCR_WIDTH/2-64, 480-170, 128, 170, 0, 2, 0xff303030);
		}

	SetTexture(NULL);

	temp_pad= wiimote_read(); 
	new_pad=temp_pad & (~old_pad);old_pad=temp_pad;

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						
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

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) new_pad|=WPAD_BUTTON_UP;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) old_pad|=WPAD_BUTTON_UP;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) new_pad|=WPAD_BUTTON_DOWN;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) old_pad|=WPAD_BUTTON_DOWN;

						}
					 else
					   {px=-100;py=-100;}

				
				if(mode==0 || mode==100)
				{

				if(new_pad & WPAD_BUTTON_B)
					{
					if(mode) signal2=-2;
					else break;
					}
				}


				if(nfiles>0) if(new_pad & WPAD_BUTTON_A) 
				{
				
					if(!files[posfile].is_directory) 
						{
						
						if(is_ext(files[posfile].name,".png") && (mode==0 || mode==100))
							{
							if(texture_png!=NULL && mem_png!=NULL)
								{
								
								if(mode==0) mode=1;
								else
									{
									u8 discid[7];

									memcpy(discid,header->id,6); discid[6]=0;

									memset(temp_data,0,256*1024);
									WBFS_GetProfileDatas(discid, temp_data);
									temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=((len_png+8+1023)/1024)-1;
									
									memcpy(&temp_data[8], mem_png, len_png);

									WBFS_SetProfileDatas(discid, temp_data);

									Screen_flip();

									if(mem_png!=NULL) free(mem_png);mem_png=NULL;
									if(texture_png!=NULL) free(texture_png);texture_png=NULL;
									break;

									}
								}
							else mode=0;
								
							}
						}
							
					  else if(mode==0)
						{
						  signal=1;
		                snd_fx_yes();
						 // cambio de device
						 if(files[posfile].name[0]=='.' && files[posfile].name[1]=='[')
							{
							
							if(files[posfile].name[2]=='U') 
								{
								
								current_device=1;
								sprintf(path_file, "ud:/");
								read_list_file(NULL, flag);
								posfile=0;flag_auto=1;
							
								}
							else
							if(files[posfile].name[2]=='S') 
								{

								current_device=0;
							    sprintf(path_file, "sd:/");
								read_list_file(NULL, flag);
								posfile=0;flag_auto=1;
								
								}
							}
						 else
							{read_list_file((void *) &files[posfile].name[0], flag);posfile=0;flag_auto=1;}
						 
						 
						}
				}

			
				if(mode==0)
				{
				if(!(old_pad  & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN))) f=0;
				
				if(old_pad  & WPAD_BUTTON_UP)
					{
					if(f==0) f=2;
					else if(f & 1) f=2;
					else {f+=2;if(f>40) {f=38;new_pad|=WPAD_BUTTON_UP;}}
					}
				if(old_pad  & WPAD_BUTTON_DOWN)
					{
					if(f==0) f=1;
					else if(!(f & 1)) f=1;
					else {f+=2;if(f>41) {f=39;new_pad|=WPAD_BUTTON_DOWN;}}
					}
				if(new_pad & (WPAD_BUTTON_HOME | WPAD_BUTTON_1 | WPAD_BUTTON_2)) break;
				if(new_pad & WPAD_BUTTON_PLUS)
					{
					get_covers();
					if(mem_png!=NULL) free(mem_png);mem_png=NULL;
					if(texture_png!=NULL) free(texture_png);texture_png=NULL;
					snd_fx_yes();return;
					}

				if((new_pad & WPAD_BUTTON_UP)) {snd_fx_tick();signal=1;posfile--;if(posfile<0) posfile=nfiles-1;}
				if((new_pad & WPAD_BUTTON_DOWN)){snd_fx_tick();signal=1;posfile++;if(posfile>=nfiles) posfile=0;} 
               
				if(signal)
				{
				FILE *fp=NULL;
				signal=0;

				if(mem_png!=NULL) free(mem_png);mem_png=NULL;
				if(texture_png!=NULL) free(texture_png);texture_png=NULL;

				if(!files[posfile].is_directory) 
					{
					
					fp=fopen(files[posfile].name,"r");
					if(fp!=0)
						{
						fseek(fp,0,SEEK_END);
						len_png=ftell(fp);
						fseek(fp,0,SEEK_SET);
						if(len_png<(200*1024-8))
							{
							mem_png=malloc(len_png+128);
							if(mem_png) 
								{n=fread(mem_png,1,len_png,fp);
								if(n<0) {len_png=0;free(mem_png);mem_png=0;} else len_png=n;
								}
							} else len_png=0;
						fclose(fp);
						} else len_png=0;

					if(mem_png)
						texture_png=create_png_texture(&png_texture, mem_png, 0);
					}
				}
				} // mode==0

			}

	

	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

snd_fx_yes();
Screen_flip();
if(mem_png!=NULL) free(mem_png);
if(texture_png!=NULL) free(texture_png);texture_png=NULL;
}
