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


#include <gccore.h>
#include <string.h>
#include <malloc.h>
#include "defs.h"
#include "dip.h"
#include <stdlib.h>
#include <unistd.h>

#include "libpng/pngu/pngu.h"
#include "libwbfs/libwbfs.h"
#include "wbfs.h"

#include <wiiuse/wpad.h>



#define CIOS 222

#include <stdarg.h>
#include <stdio.h>

#include "screen.h"
#include <fat.h>
#include <math.h>

#include "defpng.h"
#include "button1.h"
#include "button2.h"
#include "button3.h"
#include "background.h"
#include "icon.h"

#define USE_MODPLAYER 1

#ifdef USE_MODPLAYER

#include "asndlib.h"
// MOD Player
#include "gcmodplay.h"

MODPlay mod_track;

#include "affair.h" // segundo mod

#endif

//---------------------------------------------------------------------------------
/* Gloabal definitions */
//---------------------------------------------------------------------------------

s32 WBFS_SetProfileDatas(u8 *discid, u8 *buff);
s32 WBFS_GetProfileDatas(u8 *discid, void *buff);

void LoadLogo(void);
void DisplayLogo(void);
int load_disc(u8 *discid);

// to read the game conf datas
u8 temp_data[256*1024] ALIGNED(0x20);

// texture of white-noise animation generated here
u32 game_empty[128*64*3] ALIGNED(0x20);


//---------------------------------------------------------------------------------
/* language GUI */
//---------------------------------------------------------------------------------

int idioma=0;

char letrero[2][32][32]=
	{
	{"Return", "Change the IOS", "Delete Favorite", "Add Favorite", "Load Game", "Add to Favorites", "Favorites", "Page", },
	{"Retorna", "Cambia IOS", "Borra Favorito", "Añade Favorito", "Carga juego", "Añade a Favoritos", "Favoritos", "Página", },
	};


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


void Determine_VideoMode(char Region)
{
	// Get vmode and Video_Mode for system settings first
	u32 tvmode = CONF_GetVideo();
	// Attention: This returns &TVNtsc480Prog for all progressive video modes
        vmode = VIDEO_GetPreferredMode(0);
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
}

void Set_VideoMode(void)
{
    // TODO: Some exception handling is needed here
 
    // The video mode (PAL/NTSC/MPAL) is determined by the value of 0x800000cc
    // The combination Video_Mode = NTSC and vmode = [PAL]576i, results in an error
    
    *Video_Mode = _Video_Mode;

    VIDEO_Configure(vmode);
    return;
	VIDEO_SetNextFramebuffer(xfb);
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

/* Disc header structure */
struct discHdr
{
	/* Game ID */
	u8 id[6];

	/* Game version */
	u16 version;

	/* Audio streaming */
	u8 streaming;
	u8 bufsize;

	/* Padding */
	u8 unused1[14];

	/* Magic word */
	u32 magic;

	/* Padding */
	u8 unused2[4];

	/* Game title */
	char title[64];

	/* Encryption/Hashing */
	u8 encryption;
	u8 h3_verify;

	/* Padding */
	u8 unused3[30];
} ATTRIBUTE_PACKED;

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


unsigned wiimote_read()
{
int n;

int ret=-1;

unsigned type=0;

unsigned butt=0;

wmote_datas=NULL;


for(n=0;n<4;n++) // busca el primer wiimote encendido y usa ese
	{
	ret=WPAD_Probe(n, &type);

	if(ret>=0)
		{
		
		butt=WPAD_ButtonsHeld(n);
		
			wmote_datas=WPAD_Data(n);
		
		return butt;
		}
	}

return 0;
}
//---------------------------------------------------------------------------------
/* GUI routines and datas*/
//---------------------------------------------------------------------------------

int scroll_text=0;

GXTlutObj palette_icon;
GXTexObj text_icon[2];

GXTexObj text_button[4], default_game_texture, text_background,text_game_empty[4];

GXTexObj text_move_chan;
void *mem_move_chan= NULL;

struct _game_datas
{
	int ind;
	void * png_bmp;
	GXTexObj texture;
	u32 config;
} game_datas[16];

void draw_text(char *text)
{
int n,m,len;
len=strlen(text);


for(n=0;n<17;n++)
	{
	m=(n+scroll_text) % (len+4);

	if(m<len)
		s_printf("%c",text[m]);
	else
		s_printf(" ");
	}
}

// Draw_button variables

int step_button=0;

int x_temp=0;

int Draw_button(int x,int y,char *cad)
{
int len=strlen(cad);

	SetTexture(&text_button[(step_button>>4) & 3]);
	
	if(px>=x && px<=x+len*8+32 && py>=y && py<y+56) DrawRoundFillBox(x-8, y-8, len*8+32+16, 56+16, 0, 0xffcfcfcf);
		else DrawRoundFillBox(x, y, len*8+32, 56, 0, 0xffcfcfcf);
	SetTexture(NULL);
	PX=x+16; PY= y+12; color= 0xff000000;
	letter_size(8,32);

	s_printf("%s",cad);

	x_temp=x+len*8+32;

	if(px>=x && px<=x+len*8+32 && py>=y && py<y+56)
		{DrawRoundBox(x-8, y-8, len*8+32+16, 56+16, 0, 5, 0xfff08000);return 1;}
	
	DrawRoundBox(x, y, len*8+32, 56, 0, 4, 0xff606060);
	

return 0;
}


void * create_png_texture(GXTexObj *texture, void *png, int repeat)
{
PNGUPROP imgProp;
IMGCTX ctx;
char *texture_buff;
s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(png);
	if (!ctx)
		{return NULL;}

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK)
		{return NULL;}

    texture_buff=memalign(32, imgProp.imgWidth * imgProp.imgHeight *4+2048);
	if(!texture_buff) {return NULL;}

	
	PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, texture_buff, 255);
	PNGU_ReleaseImageContext(ctx);

	DCFlushRange( texture_buff, imgProp.imgWidth * imgProp.imgHeight *4);

	if(repeat) GX_InitTexObj(texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_REPEAT ,  GX_REPEAT , GX_FALSE);
	else GX_InitTexObj(texture,texture_buff, imgProp.imgWidth, imgProp.imgHeight, GX_TF_RGBA8, GX_CLAMP ,  GX_CLAMP , GX_FALSE);

	GX_InitTexObjLOD(texture, // objeto de textura
						 GX_LINEAR, // filtro Linear para cerca
						 GX_LINEAR, // filtro Linear para lejos
						 0, 0, 0, 0, 0, GX_ANISO_1);

return texture_buff;
}


void create_game_png_texture(int n)
{
	

	PNGUPROP imgProp;
	IMGCTX ctx;
	char *texture_buff;

	s32 ret;


	if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
		{game_datas[n].png_bmp=NULL;return;}

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


//---------------------------------------------------------------------------------
/* Configuration datas (Favorites) */
//---------------------------------------------------------------------------------

struct _config_file
{
	u32 magic;
	u8 id[16][8];
	u32 pad[31];
}
config_file;


void load_cfg()
{
FILE *fp;
int n=0;

	config_file.magic=0;

	fp=fopen("fat:/apps/uloader/uloader.cfg","rb"); // lee el fichero de texto
	if(!fp)
		fp=fopen("fat:/uloader/uloader.cfg","rb"); // lee el fichero de texto
	if(!fp)
		fp=fopen("fat:/uloader.cfg","rb"); // lee el fichero de texto

	if(fp!=0)
		{
		n=fread(&config_file,1, sizeof (config_file) ,fp);
		fclose(fp);
		}

	if(n!=sizeof (config_file) || !fp || config_file.magic!=0xcacafea1)
		{
		memset(&config_file,0, sizeof (config_file));
		}
}

void save_cfg()
{
FILE *fp;


	config_file.magic=0xcacafea1;

	fp=fopen("fat:/apps/uloader/uloader.cfg","wb"); // escribe el fichero de texto
	if(!fp)
		fp=fopen("fat:/uloader/uloader.cfg","wb"); // escribe el fichero de texto
	if(!fp)
		fp=fopen("fat:/uloader.cfg","wb"); // escribe el fichero de texto

	if(fp!=0)
		{
		fwrite(&config_file,1, sizeof (config_file) ,fp);
		fclose(fp);
		}

}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
        u8 discid[7];

		int ret;
		u32 cnt, len;
		struct discHdr *buffer = NULL;
		int frames=0,frames2=0;
		int load_png=1;
		int scroll_mode=0;
		int game_mode=0;
		int select_game_bar=0;

		int is_favorite=1;
		int insert_favorite=0;
		int actual_game=0;
		int last_game=-1;

		int test_favorite=0;
		int n;
		int force_ios249=0;

        return_reset=1;

		if(argc<1) return_reset=2;

        SYS_SetResetCallback(reset_call); // esto es para que puedas salir al pulsar boton de RESET
		SYS_SetPowerCallback(power_call); // esto para apagar con power
		
        discid[6]=0;

		cios=222;
        ret=IOS_ReloadIOS(cios);

		if(ret!=0)
			{
			force_ios249=1;
			cios=249;
			ret=IOS_ReloadIOS(cios);
			}

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

		fatInit(8, true);

		
		InitScreen();  // Inicialización del Vídeo
		
		create_png_texture(&text_background, background, 1);
		
		bkcolor=0;
		
   
		for(n=0;n<3;n++)
			{
			autocenter=1;
			SetTexture(&text_background);
			DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
			PX=0; PY= 16; color= 0xff000000; 
			letter_size(32,64);
			SelectFontTexture(1);
			s_printf("%s","uLoader");
			SetTexture(NULL);
			DrawRoundBox((SCR_WIDTH-260)/2, 16, 260, 64, 0, 4, 0xff000000);

			SelectFontTexture(0);
			PY+=80;
			letter_size(16,32);
			s_printf("%s","\251 2009, Hermes (www.elotrolado.net)");
			PY+=40;
			
			s_printf("%s","Based in YAL \251 2009, Kwiirk");
			PY+=40;
			s_printf("%s","and USBLoader \251 2009, Waninkoko");
			autocenter=0;
			PX=20; PY= 32; color= 0xff000000; 
			letter_size(8,16);
			SelectFontTexture(1);
			s_printf("v1.0");
			PX=SCR_WIDTH-20-32;
			s_printf("v1.0");
			autocenter=1;
			if(n!=2) Screen_flip();
			}
	   
		CreatePalette(&palette_icon, TLUT_SRGB5A1, 0, icon_palette, icon_palette_colors); // crea paleta 0
		CreateTexture(&text_icon[0], TILE_CI8, icon_sprite_1, icon_sprite_1_sx, icon_sprite_1_sy, 0);
		CreateTexture(&text_icon[1], TILE_CI8, icon_sprite_2, icon_sprite_2_sx, icon_sprite_2_sy, 0);

		create_png_texture(&text_button[0], button1, 0);
		create_png_texture(&text_button[1], button2, 0);
		create_png_texture(&text_button[2], button3, 0);
		text_button[3]=text_button[1];

		create_png_texture(& default_game_texture, defpng, 0);
		
		
		for(n=0;n<128*64*3;n++)
			{
			switch((rand()>>3) & 3)
				{
				case 0:
					game_empty[n]=0xfff0f0f0;break;
				case 1:
					game_empty[n]=0xff404040;break;
				case 2:
					game_empty[n]=0xffd0d0d0;break;
				case 3:
					game_empty[n]=0xffc0c0c0;break;
				}
			}

		CreateTexture(&text_game_empty[0], TILE_SRGBA8, &game_empty[0], 128, 64, 0);
		CreateTexture(&text_game_empty[1], TILE_SRGBA8, &game_empty[128*64], 128, 64, 0);
		CreateTexture(&text_game_empty[2], TILE_SRGBA8, &game_empty[128*64*2], 128, 64, 0);
		text_game_empty[3]=text_game_empty[1];

		SelectFontTexture(1); // selecciona la fuente de letra extra

		letter_size(8,32);
				
		PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; 
				
		bkcolor=0;
		autocenter=1;
		SetTexture(NULL);

		if(ret!=0) 
			{
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: You need cIOS222 and/or cIOS249 to work!!!\n");
			Screen_flip();
			goto error;
			}
		

		ret = WBFS_Init();
		if (ret < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("ERROR: Could not initialize USB subsystem! (ret = %d)\n", ret);
			Screen_flip();
			goto error;
			}

		letter_size(16,32);

		/* Get list length */
		ret = WBFS_Open();
		if (ret < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("No WBFS partition found!\n");
			Screen_flip();
			goto error;
			}

		ret = WBFS_GetCount(&cnt);
		if (ret < 0 || cnt==0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("No Game found\n");
			Screen_flip();
			goto error;
			}

		/* Buffer length */
		len = sizeof(struct discHdr) * cnt;

		/* Allocate memory */
		buffer = (struct discHdr *)memalign(32, len);
		if (!buffer){
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("Error in memalign()\n");
			Screen_flip();
			goto error;
			}

		/* Clear buffer */
		memset(buffer, 0, len);

		/* Get header list */
		ret = WBFS_GetHeaders(buffer, cnt, sizeof(struct discHdr));
		if (ret < 0) {
			DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
			DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

			s_printf("Error in WBFS_GetHeaders()\n");
			Screen_flip();
			free(buffer);
			goto error;
			}

		autocenter=0;

		/* Sort entries */
		qsort(buffer, cnt, sizeof(struct discHdr), __Menu_EntryCmp);

		/*
		// quitar 10
		for(n=1;n<10;n++) memcpy(((char *) buffer)+sizeof(struct discHdr) * cnt*n,buffer,sizeof(struct discHdr) * cnt);
		cnt*=10;
		*/
		
		/* Set values */
		gameList = buffer;
		gameCnt  = cnt;

		load_cfg();


	WPAD_Init();
	WPAD_SetIdleTimeout(60*5); // 5 minutes 

	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
	WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH, SCR_HEIGHT);

	#ifdef USE_MODPLAYER
	ASND_Init();
	ASND_Pause(0);
	// inicializa el modplayer en modo loop infinito
		MODPlay_Init(&mod_track);


		if (MODPlay_SetMOD (&mod_track, affair ) < 0 ) // set the MOD song
			{
			MODPlay_Unload (&mod_track);   
			}
		else  
			{
		
			MODPlay_SetVolume( &mod_track, 32,32); // fix the volume to 32 (max 64)
			MODPlay_Start (&mod_track); // Play the MOD
			}
	#endif

	guitar_pos_x=SCR_WIDTH/2-32;guitar_pos_y=SCR_HEIGHT/2-32;

	
	while(1)
	{
	int temp_sel=-1;
	int test_gfx_page=0;

	WPAD_ScanPads(); // esto lee todos los wiimotes

	

	SetTexture(&text_background/*NULL*/);
   
	ConfigureForTexture(10);
	GX_Begin(GX_QUADS,  GX_VTXFMT0, 4);
	AddTextureVertex(0, 0, 999, 0xffa0a0a0, 0, (frames2 & 1023));
	AddTextureVertex(SCR_WIDTH, 0, 999, 0xffa0a0a0, 1023, (frames2 & 1023)); 
	AddTextureVertex(SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0, 1023, 1024+(frames2 & 1023)); 
	AddTextureVertex(0, SCR_HEIGHT, 999, 0xffa0a0a0, 0, 1024+(frames2 & 1023)); 
	GX_End();
		
    cnt=actual_game;
	SelectFontTexture(1); // selecciona la fuente de letra extra


    {
	int n,m;
    int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(game_mode)
		{
		struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

		SetTexture(NULL);
		DrawRoundFillBox(20, ylev, 148*4, 108*4-80, 0, 0xffafafaf);

		if(game_datas[game_mode-1].png_bmp) 
			SetTexture(&game_datas[game_mode-1].texture);
		else SetTexture(&default_game_texture);

		DrawRoundFillBox(20, ylev, 148*4, 108*4-80, 0, 0xffffffff);
			
		SetTexture(NULL);
			
		DrawRoundBox(20, ylev, 148*4, 108*4-80, 0, 4, 0xff303030);

		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev-32;/* 8+ylev+2*110*/; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;

		//draw_text(header->title);
		bkcolor=0;

		PX= 26; PY=ylev; color= 0xff000000; 
		if((game_datas[game_mode-1].config & 1) || force_ios249) s_printf("cIOS 249"); else s_printf("cIOS 222");

	
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
		if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][1][0])) select_game_bar=2;
		
		if(test_favorite)
			{
			if(is_favorite)
				{if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][2][0])) select_game_bar=4;}
			else
				if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][3][0])) select_game_bar=3;
			}

		if(Draw_button(600-32-strlen(&letrero[idioma][4][0])*8, ylev+108*4-64, &letrero[idioma][4][0])) select_game_bar=5;
		
		step_button++;
		}
	else
		{ // modo panel
		int set_set=0;
		select_game_bar=0;

		PX= 0; PY=ylev-32; color= 0xff000000; 
				
		bkcolor=0;//0xc0f08000;
		letter_size(16,32);
		autocenter=1;

		if(is_favorite && !insert_favorite)
			{
			for(n=0;n<16;n++)
				if(config_file.id[n][0]!=0) break;
			if(n==16) 
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
			s_printf("%s %i",&letrero[idioma][7][0],1+(actual_game/16));

		autocenter=0;
		bkcolor=0;
  
		for(m=0;m<4;m++)
		for(n=0;n<4;n++)
			{

			if(is_favorite) 
				{
				int g;
				
				if(config_file.id[(m<<2)+n][0]==0)
					{
					cnt=gameCnt+1;
					}
				else
					{
					cnt=gameCnt+1;
					for(g=0;g<gameCnt;g++)
						{
						struct discHdr *header = &gameList[g];
						if(strncmp((char *) header->id, (char *) &config_file.id[(m<<2)+n][0],6)==0)
							{
							cnt=g;break;
							}
						}
					}

				}

			//if(cnt>=gameCnt) cnt=0;

			if(cnt<gameCnt)
				{
				struct discHdr *header = &gameList[cnt];

				game_datas[(m<<2)+n].ind=cnt;

				memcpy(discid,header->id,6); discid[6]=0;

				if(load_png)
					{
					memset(temp_data,0,256*1024);
					WBFS_GetProfileDatas(discid, temp_data);
					create_game_png_texture((m<<2)+n);
					}
				
				SetTexture(NULL);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 0, 0xffafafaf);

				if(game_datas[(m<<2)+n].png_bmp) 
					SetTexture(&game_datas[(m<<2)+n].texture);
				else SetTexture(&default_game_texture);
				
				
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 0, 0xffffffff);
					

				letter_size(8,24);
				
				PX= 26+n*150+scroll_mode; PY= 64-24+30+8+ylev+m*110; color= 0xff000000; 
				
				bkcolor=0xc0f0f000;
				draw_text(header->title);
				bkcolor=0;
				set_set=1;

				cnt++;
				}
			else
				{
				set_set=0;
				//SetTexture(STRIPED_PATTERN);
				SetTexture(&text_game_empty[(frames2>>2) & 3]);
				DrawRoundFillBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 0, 0xffffffff);
				}
			
			SetTexture(NULL);
			if(px>=20+n*150 && px<20+n*150+148 && py>=ylev+m*110 && py<ylev+m*110+108)
				{
				if(set_set || insert_favorite) temp_sel=(m<<2)+n; else temp_sel=-1;
				DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 0, 6, 0xfff08000/*0xff00a0a0*/);
				}
			else
				DrawRoundBox(20+n*150+scroll_mode, ylev+m*110, 148, 108, 0, 4, 0xff303030);

			// scroll mode
				if(scroll_mode<0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);
					DrawRoundFillBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 0xffffffff);
					SetTexture(NULL);
					DrawRoundBox(20+n*150+scroll_mode+620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					}
				if(scroll_mode>0)
					{
					SetTexture(&text_game_empty[(frames2>>2) & 3]);
					
					DrawRoundFillBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 0xffffffff);
					SetTexture(NULL);
					DrawRoundBox(20+n*150+scroll_mode-620, ylev+m*110, 148, 108, 0, 4, 0xff303030);
					}

				if(!scroll_mode && !insert_favorite)
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
			}

		
		
		} // modo panel
		} 

	load_png=0;
	frames2++;

	if(!scroll_mode)
		{
		frames++;
		if(frames>=8) {frames=0;scroll_text++;}

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
							DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							
							}
						SetTexture(NULL);
						DrawSurface(&text_icon[(frames2 & 4)!=0 && ((frames2 & 128)>110 || (frames2 & 32)!=0)],px-31,py-31, 64, 64, 0, 0xffffffff);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{

						if(wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x+8)
							{guitar_pos_x+=8;if(guitar_pos_x>(SCR_WIDTH-16)) guitar_pos_x=(SCR_WIDTH-16);}
						if(wmote_datas->exp.gh3.js.pos.x<=wmote_datas->exp.gh3.js.center.x-8)
							{guitar_pos_x-=8;if(px<16) px=16;}
							

						if(wmote_datas->exp.gh3.js.pos.y>=wmote_datas->exp.gh3.js.center.y+8)
							{guitar_pos_y-=8;if(guitar_pos_y<16) guitar_pos_y=16;}
						if(wmote_datas->exp.gh3.js.pos.y<=wmote_datas->exp.gh3.js.center.y-8)
							{guitar_pos_y+=8;if(guitar_pos_y>(SCR_HEIGHT-16)) guitar_pos_y=(SCR_HEIGHT-16);}
						
						px=guitar_pos_x; py=guitar_pos_y;

						if(insert_favorite)
							{
							SetTexture(&text_move_chan);
							DrawRoundFillBox(px-148/2, py-108/2, 148, 108, 0, 0x8060af60);
							}
						SetTexture(NULL);
						DrawSurface(&text_icon[(frames2 & 4)!=0 && ((frames2 & 128)>110 || (frames2 & 32)!=0)],px-31,py-31, 64, 64, 0, 0xffffffff);
						
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

					 if(!game_mode && !insert_favorite) //limit left/right
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

					if(new_pad & WPAD_BUTTON_A)
						{
						if(test_gfx_page)
							{
							if(insert_favorite==0)
								{
								if(test_gfx_page<0)
									{scroll_mode=1;px=py=-100;}
								if(test_gfx_page>0)
									{scroll_mode=-1;px=py=-100;}
								}
							}
						else
						if(game_mode==0)
							{
							if((old_pad & WPAD_BUTTON_B) && is_favorite)
								{
								if(temp_sel>=0)
									{
									insert_favorite=game_datas[temp_sel].ind+1;

									mem_move_chan=NULL;
									memcpy(&text_move_chan, &game_datas[temp_sel].texture, sizeof(GXTexObj));
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
										for(n=0;n<16;n++)
											{
											if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
												{
												if(m==0)
													{m=1;memcpy(&config_file.id[n][0],&config_file.id[temp_sel][0],8);config_file.id[n][6]=0;memset(&config_file.id[temp_sel][0],0,8);}
												else {memset(&config_file.id[n][0],0,8);} // elimina repeticiones

												break;
												}
											}
										memcpy(&config_file.id[temp_sel][0],header->id,6);
										config_file.id[temp_sel][6]=0;
										insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;
										temp_sel=-1;
										for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
										save_cfg();
										load_png=1;
										}
									}
								else
									{
									int n;
									struct discHdr *header;
									if(!is_favorite) last_game=actual_game;

									game_mode=temp_sel+1;

									header = &gameList[game_datas[game_mode-1].ind];

									if(is_favorite) test_favorite=0; else test_favorite=1;
									
									for(n=0;n<16;n++)
										{
										if(strncmp((char *) header->id, (char *) &config_file.id[n][0],6)==0)
											{
											if(!is_favorite) test_favorite=0; else test_favorite=1;
											break;
											}
										}
								

									}
								}
							}
						else
							{
							if(select_game_bar==1) {int n;  // return
													game_mode=0;select_game_bar=0;
													for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
													load_png=1;
													} 

							if(select_game_bar==2 && !force_ios249) { // change the IOS
												   struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

												   memcpy(discid,header->id,6); discid[6]=0;

												   select_game_bar=0;

												   memset(temp_data,0,256*1024);
												   WBFS_GetProfileDatas(discid, temp_data);
												   // si no existe crea uno
												   if(!(temp_data[0]=='H' && temp_data[1]=='D' && temp_data[2]=='R'))
														{
														temp_data[0]='H'; temp_data[1]='D'; temp_data[2]='R';temp_data[3]=0;
														temp_data[4]=temp_data[5]=temp_data[6]=temp_data[7]=0;
														temp_data[8]=temp_data[9]=temp_data[10]=temp_data[11]=0;
														}

												   temp_data[4]^=1;
												   game_datas[game_mode-1].config=temp_data[4]+(temp_data[5]<<8)+(temp_data[6]<<16)+(temp_data[7]<<24);

												   WBFS_SetProfileDatas(discid, temp_data);
												   }
							if(select_game_bar==3) {int n;
													select_game_bar=0;is_favorite=1;insert_favorite=game_datas[game_mode-1].ind+1;

													mem_move_chan=game_datas[game_mode-1].png_bmp;game_datas[game_mode-1].png_bmp=NULL;
													memcpy(&text_move_chan, &game_datas[game_mode-1].texture, sizeof(GXTexObj));
													
													game_mode=0;
													
												    for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												    save_cfg();
												    load_png=1;
													} // add favorite
							if(select_game_bar==4) {// del favorite
												   int n;
												   select_game_bar=0;is_favorite=1; 
												   memset(&config_file.id[game_mode-1][0],0,8);
												   game_mode=0;
												   for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   save_cfg();
												   load_png=1;
												   } 
							if(select_game_bar==5) {
													struct discHdr *header = &gameList[game_datas[game_mode-1].ind];

													memcpy(discid,header->id,6); discid[6]=0;
													select_game_bar=0;
													Screen_flip();
													WPAD_Shutdown();
													
													save_cfg();
													fatUnmount("fat:");
													#ifdef USE_MODPLAYER
													ASND_End();		// finaliza el sonido
													#endif

													if((game_datas[game_mode-1].config & 1) || force_ios249) cios=249; else cios=222;
													IOS_ReloadIOS(cios);
													ret = load_disc(discid);
													//////////////////////////////////
												
													SetTexture(&text_background);
													DrawRoundFillBox(0, 0, SCR_WIDTH, SCR_HEIGHT, 999, 0xffa0a0a0);
		
											
													SelectFontTexture(1); // selecciona la fuente de letra extra

													letter_size(8,32);
															
													PX=0; PY= SCR_HEIGHT/2; color= 0xff000000; bkcolor=0;
															
													bkcolor=0;
													autocenter=1;
													SetTexture(NULL);

													DrawRoundFillBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 0xa00000ff);
													DrawRoundBox((SCR_WIDTH-540)/2, SCR_HEIGHT/2-16, 540, 64, 999, 4, 0xa0000000);

													s_printf("ERROR FROM THE LOADER: %i\n", ret);
													Screen_flip();


													//////////////////////////////////
													break;
												   } // load game
							}
						
						}

					if(new_pad & WPAD_BUTTON_B)
						{
						
						int n;
						if(game_mode)
							{
							for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
							load_png=1;
							}
						
						game_mode=0;
						insert_favorite=0;if(mem_move_chan) free(mem_move_chan);mem_move_chan=NULL;
						}

					if(new_pad & WPAD_BUTTON_1) // swap favorites/page
						{
						int n;

						if(!game_mode && !insert_favorite)
							{
							if(is_favorite)
								{
								is_favorite=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}
								}
							else
								{
								last_game=actual_game;is_favorite=1;
								}
							for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
												   
							load_png=1;
							}
						
						}
			
			} 
			else {px=-100;py=-100;}
		}
	else // scroll mode
		{
		if(scroll_mode<0) scroll_mode-=32;
		if(scroll_mode>0) scroll_mode+=32;

		if(scroll_mode<-639 || scroll_mode>639)
			{
			int n,g;
			if(scroll_mode<0)
				{
				
				if(is_favorite) {is_favorite=0;actual_game=0;if(last_game>=0) {actual_game=last_game;last_game=-1;}} 
				else {actual_game+=16;if(actual_game>=gameCnt) {actual_game=0;is_favorite=1;}}
				}
			if(scroll_mode>0)
				{
				// juasjuas
				if(!is_favorite)
					{
					actual_game-=16; 
				    if(actual_game<0) 
						{
						int flag=0;
						last_game=-1;is_favorite=1;actual_game=0;
					    for(n=0;n<16;n++)
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
						if(!flag) {actual_game=((gameCnt-1)/16)*16;is_favorite=0;}
						}
					}
				}

			scroll_mode=0;
			scroll_text=0;

			for(n=0;n<16;n++) {if(game_datas[n].png_bmp) free(game_datas[n].png_bmp);game_datas[n].png_bmp=NULL;}
			load_png=1;

			
			}
		}

	if(load_png)
			{
			SelectFontTexture(0); // selecciona la fuente de letra normal

			letter_size(32,64);
															
			PX=0; PY= SCR_HEIGHT/2-32; color= 0xcf000000; bkcolor=0;
															
			autocenter=1;
			SetTexture(NULL);
			DrawRoundFillBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 0xcf00ff00);
			DrawRoundBox((SCR_WIDTH-320)/2, SCR_HEIGHT/2-40, 320, 80, 0, 4, 0xcf000000);

			s_printf("Loading");
			autocenter=0;
			Screen_flip();


			SelectFontTexture(0); // selecciona la fuente de letra extra
			}

	Screen_flip();

	if(exit_by_reset)  
		{
		#ifdef USE_MODPLAYER
		ASND_End();		// finaliza el sonido
		#endif
		break;
		}

	}// while

	WPAD_Shutdown();
	sleep(1);

	if(exit_by_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	if(exit_by_reset==3)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);
error:
    sleep(4);

	if(return_reset==2)
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

    return ret;
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

        //printf("Loading disc %s\n",discid);

        DI_Initialize();
        DI_Enable_WBFS(discid);
        DI_Reset();
        memset(Disc_ID, 0, 0x20);
        DI_Read_DiscID(Disc_ID);

        if (*Disc_ID==0x10001 || *Disc_ID==0x10001)
                return 2;
        
        Determine_VideoMode(*Disc_Region);
        DI_Read_Unencrypted(&Header, sizeof(Header), 0);

        //printf("%s\n",Header.Title);
		

        u32 Offset = 0x00040000; // Offset into disc to partition descriptor
        DI_Read_Unencrypted(&Descriptor, sizeof(Descriptor), Offset);

        Offset = Descriptor.Primary_Offset << 2;

        u32 PartSize = sizeof(Partition_Info);
        u32 BufferLen = Descriptor.Primary_Count * PartSize;
        
        // Length must be multiple of 0x20
        BufferLen += 0x20 - (BufferLen % 0x20);
        u8 *PartBuffer = (u8*)memalign(0x20, BufferLen);

        memset(PartBuffer, 0, BufferLen);
        DI_Read_Unencrypted(PartBuffer, BufferLen, Offset);

        struct Partition_Info *Partitions = (struct Partition_Info*)PartBuffer;
        for ( i = 0; i < Descriptor.Primary_Count; i++)
        {
                if (Partitions[i].Type == 0)
                {
                        memcpy(&Partition_Info, PartBuffer + (i * PartSize), PartSize);
                        break;
                }
        }
        Offset = Partition_Info.Offset << 2;
        free(PartBuffer);
        if (!Offset)
                return 3;
        DI_Set_OffsetBase(Offset);
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
        DI_Read_Unencrypted(Ticket_Buffer, 0x800, Partition_Info.Offset << 2);
        Ticket		= (signed_blob*)(Ticket_Buffer);
        T_Length	= SIGNED_TIK_SIZE(Ticket);

        // Open Partition and get the TMD buffer
        if (DI_Open_Partition(Partition_Info.Offset, 0,0,0, Tmd_Buffer) < 0)
                return 4;
        Tmd = (signed_blob*)(Tmd_Buffer);
        MD_Length = SIGNED_TMD_SIZE(Tmd);
        static struct AppLoaderHeader Loader ALIGNED(32);
        DI_Read(&Loader, sizeof(Loader), 0x00002440);// Offset into the partition to apploader header
        DCFlushRange((void*)(((u32)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);


        // Read apploader from 0x2460
        DI_Read(Apploader, Loader.Size + Loader.Trailer_Size, 0x00002440 + 0x20);
        DCFlushRange((void*)(((int)&Loader) + 0x20),Loader.Size + Loader.Trailer_Size);


        AppLoaderStart	Start	= Loader.Entry_Point;
        AppLoaderEnter	Enter	= 0;
        AppLoaderLoad		Load	= 0;
        AppLoaderExit		Exit	= 0;
        Start(&Enter, &Load, &Exit);

        void*	Address = 0;
        int		Section_Size;
        int		Partition_Offset;
        //printf("Loading game");
        while (Load(&Address, &Section_Size, &Partition_Offset))
        {
                if (!Address) return 5;
                DI_Read(Address, Section_Size, Partition_Offset << 2);
                DCFlushRange(Address, Section_Size);
                //printf(".");
        }
        // Patch in info missing from apploader reads
        *Sys_Magic	= 0x0d15ea5e;
        *Version	= 1;
        *Arena_L	= 0x00000000;
        *Bus_Speed	= 0x0E7BE2C0;
        *CPU_Speed	= 0x2B73A840;

        // Enable online mode in games
        memcpy(Online_Check, Disc_ID, 4);
        
        // Retrieve application entry point
        void* Entry = Exit();

        // Set Video Mode based on Configuration
        Set_VideoMode();

        // Flush application memory range
        DCFlushRange((void*)0x80000000, 0x17fffff);	// TODO: Remove these hardcoded values

        // Cleanup loader information
        DI_Close();

        // Identify as the game
        if (IS_VALID_SIGNATURE(Certs) 	&& IS_VALID_SIGNATURE(Tmd) 	&& IS_VALID_SIGNATURE(Ticket) 
            &&  C_Length > 0 				&& MD_Length > 0 			&& T_Length > 0)
        {
                int ret = ES_Identify(Certs, C_Length, Tmd, MD_Length, Ticket, T_Length, NULL);
                if (ret < 0)
                        return ret;
        }

       // debug_printf("start %p\n",Entry);

        SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

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
