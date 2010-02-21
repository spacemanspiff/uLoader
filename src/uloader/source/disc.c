#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>


#include "disc.h"
//#include "subsystem.h"
//#include "video.h"
#include "wdvd.h"

/* Constants */
#define PTABLE_OFFSET	0x40000
#define WII_MAGIC	0x5D1C9EA3

/* Disc pointers */
static u32 *buffer = (u32 *)0x93000000;
static u8  *diskid = (u8  *)0x80000000;




s32 __Disc_FindPartition(u32 *outbuf)
{
	u32 offset = 0, table_offset = 0;

	u32 cnt, nb_partitions;
	s32 ret;

	/* Read partition info */
	ret = WDVD_UnencryptedRead(buffer, 0x20, PTABLE_OFFSET);
	if (ret < 0)
		return ret;

	/* Get data */
	nb_partitions = buffer[0];
	table_offset  = buffer[1] << 2;

	/* Read partition table */
	ret = WDVD_UnencryptedRead(buffer, 0x20, table_offset);
	if (ret < 0)
		return ret;

	/* Find game partition */
	for (cnt = 0; cnt < nb_partitions; cnt++) {
		u32 type = buffer[cnt * 2 + 1];

		/* Game partition */
		if(!type)
			offset = buffer[cnt * 2] << 2;
	}

	/* No game partition found */
	if (!offset)
		return -1;

	/* Set output buffer */
	*outbuf = offset;

	return 0;
}


s32 Disc_Init(void)
{
	/* Init DVD subsystem */
	return WDVD_Init();
}

s32 Disc_Open(void)
{
	s32 ret;

	/* Reset drive */
	ret = WDVD_Reset();
	if (ret < 0)
		return ret;

	/* Read disc ID */
	return WDVD_ReadDiskId(diskid);
}

s32 Disc_Wait(void)
{
	u32 cover = 0;
	s32 ret;

		/* Get cover status */
		ret = WDVD_GetCoverStatus(&cover);
		if (ret < 0)
			return ret;
		if(!(cover & 0x2)) return 1;

	return 0;
}


s32 Disc_ReadHeader(void *outbuf)
{
	/* Read disc header */
	return WDVD_UnencryptedRead(outbuf, sizeof(struct discHdr), 0);
}

s32 Disc_IsWii(void)
{
	struct discHdr *header = (struct discHdr *)buffer;

	s32 ret;

	/* Read disc header */
	ret = Disc_ReadHeader(header);
	if (ret < 0)
		return ret;

	/* Check magic word */
	if (header->magic != WII_MAGIC)
		return -1;

	return 0;
}

