// NOTE by Hermes: source from Neogamma 

#include <gccore.h>

typedef struct _test
{
	u32 count;
	char name[64];
	char dolname[32];
	u32 parameter;
	u32 parent;
	u64 offset;
	u32 size;
} test_t;

extern test_t *dolmenubuffer;

s32 createdolmenubuffer(u32 count);
s32 load_dolmenu(char *discid);
