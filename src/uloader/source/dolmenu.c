// NOTE by Hermes: source from Neogamma, some changes to use from uLoader

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "global.h"

#include "dolmenu.h"


char *p;
char *pend;

char *getnextstring()
{
	char *temp = NULL;
	char *pos;
	char *pos13;
	
	while (true)
	{
		if (p >= pend)
		{
			return NULL;
		}
		pos = strchr(p, 10);
		if (pos == NULL)
		{
			return p;
		}
		if (pos > pend)
		{
			return NULL;
		}
		pos[0] = 0;
		pos13 = strchr(p, 13);
		if (pos13 != NULL && pos13 < pos)
		{
			pos13[0] = 0;
		}

		temp = p;
		p = (char *)((u32)pos+1);
		
		if (temp[0] != '#') break;		// Skip comments which start with #
	}
	return temp;
}

u32 getnextnumber()
{
	char *temp = getnextstring();
	if (temp == NULL || temp[0] == 0 || temp[0] == 10 || temp[0] == 13 || strlen(temp) == 0)
	{
		return 0;
	} else
	if (strlen(temp) > 2 && strncmp(temp, "0x", 2) == 0)
	{
		return strtol((char *)((u32)temp+2), NULL, 16);
	} else
	{
		return strtol(temp, NULL, 10);
	}
}

void copynextstring(char *output)
{
	char *temp = getnextstring();
	if (temp == NULL || temp[0] == 0 || temp[0] == 10 || temp[0] == 13 || strlen(temp) == 0)
	{
		strcpy(output, "?");	
	} else
	{
		strcpy(output, temp);	
	}
}

void parse_dolmenubuffer(u32 index, u32 count, u32 parent)
{
	u32 i;
	for (i = 0;i < count;i++)
	{
		dolmenubuffer[index + i].count = getnextnumber();
		memset(dolmenubuffer[index + i].name, 0, 64);
		copynextstring(dolmenubuffer[index + i].name);
		memset(dolmenubuffer[index + i].dolname, 0, 32);
		copynextstring(dolmenubuffer[index + i].dolname);
		
		dolmenubuffer[index + i].parameter = getnextnumber();
		dolmenubuffer[index + i].parent = parent;
		if (dolmenubuffer[index + i].count != 0)
		{
			parse_dolmenubuffer(index + i + 1, dolmenubuffer[index + i].count, index + i);	
			i+=dolmenubuffer[index + i].count;
		}
	}	
}

s32 createdolmenubuffer(u32 count)
{
	u32 i;
	dolmenubuffer = memalign(32, sizeof(test_t) * count);
	if (dolmenubuffer == NULL)
	{
		return -1;
		//error
	}
	
	for (i=0;i<count;i++)
	{
		dolmenubuffer[i].count = 0;
		dolmenubuffer[i].parameter = 0;
		dolmenubuffer[i].parent = 0;
		memset(dolmenubuffer[i].name, 0, 64);
		memset(dolmenubuffer[i].dolname, 0, 32);
	}
	dolmenubuffer[0].parent = -1;
	return 1;
}

s32 load_dolmenu(char *discid)
{
	dolmenubuffer = NULL;

/*
	print_status("Loading menu file...");
	
	if (prepare_storage_access() < 0)
	{
		wait(2);
		print_status("Storage access failed...");
		return -1;
	}
*/

	char tempbuffer[8];
	static char buf[128];
	FILE *fp = NULL;
	int filesize = 0;

	fp=NULL;
    if(sd_ok)
		{
		memset(tempbuffer, 0, 8);
		memcpy(tempbuffer, discid, 6);
		snprintf(buf, 128, "sd:/codes/%s.wdm", tempbuffer);
		fp = fopen(buf, "rb");
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 4);
			snprintf(buf, 128, "sd:/codes/%s.wdm", tempbuffer);
			fp = fopen(buf, "rb");
			}
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 3);
			snprintf(buf, 128, "sd:/codes/%s.wdm", tempbuffer);
			fp = fopen(buf, "rb");
			}
		}

	if(ud_ok && !fp)
		{
		memset(tempbuffer, 0, 8);
		memcpy(tempbuffer, discid, 6);
		snprintf(buf, 128, "ud:/codes/%s.wdm", tempbuffer);
		fp = fopen(buf, "rb");
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 4);		
			snprintf(buf, 128, "ud:/codes/%s.wdm", tempbuffer);
			fp = fopen(buf, "rb");
			}
		if(!fp)
			{
			memset(tempbuffer, 0, 8);
			memcpy(tempbuffer, discid, 3);		
			snprintf(buf, 128, "ud:/codes/%s.wdm", tempbuffer);
			fp = fopen(buf, "rb");
			}
		}
	
	

	if (!fp)
	{
		
		//wait(2);
		//print_status("No menu file found");
		//resume_disc_loading();
		return -1;
	}


	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	u8 *buffer = malloc(filesize+1);
	if (buffer == NULL)
	{
		//wait(2);
		//print_status("Ouf of memory");
		//resume_disc_loading();
		return -1;
	}

	fread(buffer, 1, filesize, fp);
	fclose(fp);

	buffer[filesize] = 10; // Prevent error if there's no newline at the end of the file

	p = (char *)buffer;
	pend = (char *)((u32)p+filesize);

	u32 count = getnextnumber();

	dolmenubuffer = malloc(sizeof(test_t) * (count + 1));
	if (dolmenubuffer == NULL)
	{
		if(buffer) free(buffer);
		//wait(2);
		//print_status("Ouf of memory");
		//resume_disc_loading();
		return -1;
	}

	memset(dolmenubuffer, 0, sizeof(test_t) * (count + 1));

	dolmenubuffer[0].count = count;
	dolmenubuffer[0].parent = -1;
	dolmenubuffer[0].parameter = 1;
	memset(dolmenubuffer[0].name, 0, 64);
	memcpy(dolmenubuffer[0].name, discid, 6); // copy id in the first entry (it is used to avoid the reload of this file)
	memset(dolmenubuffer[0].dolname, 0, 32);
    
	dolmenubuffer[0].offset=0; // in the first entry it is a flag used to know if is necessary to call alternative dol routine

	parse_dolmenubuffer(1 , count, 0);
	
	free(buffer);

	//resume_disc_loading();

	return 1;
}
