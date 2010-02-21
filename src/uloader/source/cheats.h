#ifndef _CHEATS_H_
#define _CHEATS_H_

#include "global.h"

#ifdef ALTERNATIVE_VERSION
	#include "oggplayer.h"
#endif

#define MAX_LIST_CHEATS 25

extern int txt_cheats;

extern int num_list_cheats;
extern int actual_cheat;

struct _data_cheats
{
	void *title;
	void *description;

	int apply;
	u8 *values;
	int len_values;

};

typedef struct
{
struct
	{
	u8 id[6];
	u8 sel[25];
	u8 magic;
	}
	cheats[1024];
} _cheat_file;

extern int len_cheats;
extern unsigned char *buff_cheats;

extern struct _data_cheats data_cheats[MAX_LIST_CHEATS];
extern _cheat_file cheat_file;

void set_cheats_list(u8 *id);
void get_cheats_list(u8 *id);

void create_cheats();

int load_cheats(u8 *discid);


#endif

