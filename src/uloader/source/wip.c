// NOTE by Hermes: source from usbloader gx, some changes to use from uLoader

#include <gccore.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "global.h"

// #include "settings/cfg.h"

u32 doltableoffset[64];
u32 doltablelength[64];
u32 doltableentries=0;

void wipreset()
{
	doltableentries = 0;
}

void wipregisteroffset(u32 dst, u32 len)
{
	doltableoffset[doltableentries] = dst;
	doltablelength[doltableentries] = len;
	doltableentries++;
}

//extern int PX,PY;

//void s_printf(char *cad, ...);

void patchu8(u32 offset, u8 value)
{
	u32 i = 0;
	u32 tempoffset = 0;

	while ((doltablelength[i] <= offset-tempoffset) && (i+1 < doltableentries))
	{
		tempoffset+=doltablelength[i];
		i++;
	}
	if (offset-tempoffset < doltablelength[i])
	{
//s_printf("%2.2x", *(u8 *)(offset-tempoffset+doltableoffset[i]));
		*(u8 *)(offset-tempoffset+doltableoffset[i]) = value;
	}
}

void wipparsebuffer(u8 *buffer, u32 length)
// The buffer needs a 0 at the end to properly terminate the string functions
{
	int i;
	u32 pos = 0;
	u32 offset;
	char buf[10];
	
	while (pos < length)
	{
		if ( *(char *)(buffer + pos) != '#' && *(char *)(buffer + pos) != ';' && *(char *)(buffer + pos) != 10 && *(char *)(buffer + pos) != 13 && *(char *)(buffer + pos) != 32 && *(char *)(buffer + pos) != 0 )
		{
			memcpy(buf, (char *)(buffer + pos), 8);
			buf[8] = 0;
			offset = strtol(buf,NULL,16);

			pos += (u32)strchr((char *)(buffer + pos), 32)-(u32)(buffer + pos) + 1;
			pos += (u32)strchr((char *)(buffer + pos), 32)-(u32)(buffer + pos) + 1;
			
			while (*(char *)(buffer + pos) != 10 && *(char *)(buffer + pos) != 13 && *(char *)(buffer + pos) != 0)
			{
				memcpy(buf, (char *)(buffer + pos), 2);
				buf[2] = 0;
			
				patchu8(offset, strtol(buf,NULL,16));
				offset++;
				pos +=2;		
			}	
		//	s_printf("\n");
		}
		if (strchr((char *)(buffer + pos), 10) == NULL)
		{
			break;
		} else
		{
			pos += (u32)strchr((char *)(buffer + pos), 10)-(u32)(buffer + pos) + 1;
		}
	}

	for(i=0;i<doltableentries;i++)
		DCFlushRange((void *) doltableoffset[i], doltablelength[i]);
}

static u8 *wipCode= NULL;
static u32 filesize= 0;

u32 load_wip_code(u8 *discid)
{
	FILE *fp= NULL;
	
	char tempbuffer[8];
	char filepath[150];
	memset(filepath, 0, 150);
	
	if(wipCode) free(wipCode);
	wipCode= NULL;
	filesize=0;

    if(sd_ok)
		{
		memset(tempbuffer, 0, 8);
		memcpy(tempbuffer, discid, 6);
		snprintf(filepath, 128, "sd:/codes/%s.wip", tempbuffer);
		fp = fopen(filepath, "rb");

		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 4);
			snprintf(filepath, 128, "sd:/codes/%s.wip", tempbuffer);
			fp = fopen(filepath, "rb");
			
			}
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 3);
			snprintf(filepath, 128, "sd:/codes/%s.wip", tempbuffer);
			fp = fopen(filepath, "rb");
			}
		}

	if(ud_ok && !fp)
		{
		memset(tempbuffer, 0, 8);
		memcpy(tempbuffer, discid, 6);
		snprintf(filepath, 128, "ud:/codes/%s.wip", tempbuffer);
		fp = fopen(filepath, "rb");
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 4);		
			snprintf(filepath, 128, "ud:/codes/%s.wip", tempbuffer);
			fp = fopen(filepath, "rb");
			}
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 3);		
			snprintf(filepath, 128, "ud:/codes/%s.wip", tempbuffer);
			fp = fopen(filepath, "rb");
			}
		}
    

	if (fp) 
		{
		u32 ret = 0;

		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		
		wipCode = malloc(filesize + 1);
		wipCode[filesize] = 0; // Wip code functions need a 0 termination
		
		fseek(fp, 0, SEEK_SET);
		ret = fread(wipCode, 1, filesize, fp);
		fclose(fp);
		
			if (ret != filesize)
			{
			if(wipCode) free(wipCode);
			wipCode= NULL;
			filesize=0;

			return -1;	
			}
			
		return 0;
		}

	return -2;
}


u32 do_wip_code(void)
{

// Apply wip patch
	
	if(wipCode &&  filesize)
		
		wipparsebuffer(wipCode, filesize + 1);

return 0;
}
