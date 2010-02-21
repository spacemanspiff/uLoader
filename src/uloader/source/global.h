#ifndef _GLOBAL_H_
#define _GLOBAL_H_

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
#include "sys/dir.h"
#include <sys/statvfs.h>
#include "remote.h"
#include "files.h"

#define DONT_USE_IOS249
#define TIME_SLEEP_SCR 5*60

extern int is_fat;
extern int sd_ok;
extern int ud_ok;
extern int get_fat_name_pass;
extern int global_mount; // 1 ->sd_ok SD was mounted | 2-> ud_ok  USB was mounted | 128-> Use NAND Emulation. Used for load_fat_module()

extern int use_icon2;

extern struct discHdr *gameList;
extern u32 gameCnt;

extern u8 *disc_conf;
extern u8 *temp_data;

extern char* bannerTitle;

void make_rumble_off();

// for loader.c

int load_disc(u8 *discid);
void menu_alternativedol(u8 *id);
int menu_format();

struct _multi_ciso
{
u32 lba;
u32 len;
u8 name[64];
};

extern struct _multi_ciso multi_ciso[8];
extern int ciso_sel;

extern int bca_mode;
extern u8 BCA_Data[64];

extern u32 langsel;
extern unsigned char *buff_cheats;
extern int len_cheats;

extern int current_partition;
extern u32 hook_selected;

// uLoader hacks
extern char no_more_snow;
extern char automatic_dvd_mode;

extern int parental_control_on;
extern int force_reload_ios222;
extern int is_fat; // working in FAT .ciso Mode
extern int dvd_only; // no WBFS HDD (FAT .ciso, DVD USB, DVD)

extern float angle_icon; // angle for pointer

extern int partition_cnt[4];

extern u32 sd_clusters;
extern u32 usb_clusters;

//---------------------------------------------------------------------------------
/* Sound effects */
//---------------------------------------------------------------------------------

void snd_fx_tick();
void snd_fx_yes();
void snd_fx_no();
void snd_fx_scroll();
void snd_fx_fx(int percent);


//---------------------------------------------------------------------------------
/* Configuration datas (Favorites) */
//---------------------------------------------------------------------------------

struct _game_log
	{
	u8 id[6];
	u8 bcd_time[6];
	};

typedef struct 
{
	u32 magic;
	u8 id[16][8];
	char parental[4];
	struct _game_log game_log[8];
	u32 hi_score;
	u8 music_mod;
	u8 rumble_off;
	u8 icon;
	u8 pad;
	u32 pad2[4];
} _config_file;

extern _config_file config_file;

/* ------------------------------------------------------------------------------- */

extern int num_fat_games;

extern char fat_games[256][256];
extern struct discHdr fat_disc[256];

void wiimote_rumble(int status);

char * get_fat_name(u8 *id);

s32 global_RenameGame(u8 *discid, char *new_name);

s32 global_SetBCA(u8 *discid, u8 *bca);

s32 global_LoadDolInfo(void *data);
s32 global_SaveDolInfo(void *data);

s32 global_GetProfileDatas(u8 *discid, u8 *buff);
s32 global_SetProfileDatas(u8 *discid, u8 *buff);

void dol_GetProfileDatas(u8 *id, u8 *buff);
void dol_SetProfileDatas(u8 *id, u8 *buff);

int load_cfg(int mode);
void save_cfg(int mode);

int list_fat(char *device);

/* ------------------------------------------------------------------------------- */

void update_bca(u8 *id, u8 *bca_datas);

int get_bca(u8 *id, u8 *bca_datas);

/* ------------------------------------------------------------------------------- */

extern u8 alt_dol_disc_tab[32768];

typedef struct
{
u64 offset;
u32 size;
u8 id[6];
u16 pad1;
u32 flags[2];
u32 pad2[1];

} dol_infodat;

extern dol_infodat AlternativeDol_infodat;

extern void *dol_data;
extern int dol_len;

extern int load_alt_game_disc; // 0->from WBFS 1-> from DVD

int Get_AlternativeDol(u8 *id);

void add_game_log(u8 *id);

/* ------------------------------------------------------------------------------- */
// partitions

struct _partition_type
{
	int type;
	u32 lba, len;
};

extern struct _partition_type partition_type[32];

extern int num_partitions;
extern u32 sector_size;

int get_partitions();

/* ------------------------------------------------------------------------------- */

u8 *search_for_ehcmodule_cfg(u8 *p, int size);
u8 *search_for_uloader_cfg(u8 *p, int size);


#endif
