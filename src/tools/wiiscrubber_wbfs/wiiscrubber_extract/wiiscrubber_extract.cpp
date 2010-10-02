// wiiscrubber_extract.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <stdio.h>
#include "wiidisc.h"
#include "wbfs.h"

#define _CRT_SECURE_NO_DEPRECATE 1

int main(int argc, char* argv[])
{

	if (argc < 2 || argc > 4)
	{
		printf("\nUsage:\n");
		printf("%s <input iso> <output opening.bnr>\n",argv[0]);
		printf("%s <input iso>\n",argv[0]);
		printf("%s <drive letter>-<game code> <output opening.bnr>\n",argv[0]);
		printf("%s <drive letter>-<game code>\n",argv[0]);
		printf("%s <drive letter>-list\n",argv[0]);
		printf("%s <input iso> wad <output path>\n\n",argv[0]);
		return 0;
	}

	if (argc == 2 && strlen(argv[1])==6)
	{
		if (stricmp(argv[1]+1,"-list") == 0)
		{
			wbfs_t *p;
			p = wbfs_try_open(NULL, argv[1], 0);
			if (!p) return -1;

			int count = wbfs_count_discs(p);
			
			if (count == 0)
			{
				fprintf(stderr,"wbfs is empty\n");
				return 0;
			}
			int i;
			u32 size;
			u8 *b = (u8*)wbfs_ioalloc(0x100);
			printf("WBFS List:\n");
			for (i = 0; i < count; i++)
			{
				if (!wbfs_get_disc_info(p, i, b, 0x100, &size))
				{
					printf( "%c%c%c%c%c%c %s\n", b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20);
				}
			}
			return 0;
		}
	}

	image_file *image;
	CWIIDisc wiidisc;
	if (wiidisc.CheckAndLoadKey(false, NULL))
	{
		wiidisc.Reset();
		image = wiidisc.image_init(argv[1]);
		if (image == NULL)
		{
			printf("Unable to load \"%s\"",argv[1]);
			return -1;
		}
		wiidisc.image_parse(image);
	} else
	{
		printf("Unable to load key.bin");
		return -1;
	}

	char dst_file[255];

	if (argc == 4 && stricmp(argv[2],"wad") == 0)
	{
		iso_files *file = wiidisc.first_file;
		int count = 0;
		while (true)
		{
			if (file == NULL) break;
			if (stricmp(file->name+strlen(file->name)-3,"wad") == 0)
			{
				printf("Saving \"%s\"...\n",file->name);
				sprintf(dst_file,"%s\\%s",argv[3],file->name);
				wiidisc.SaveDecryptedFile(dst_file, image, file->part, file->offset, file->size);
				count++;
			}
			file = file->next;
		}
		if (!count)
		{
			printf("Unable to find any files to extract!\n");
			return -1;
		} else
		{
			printf("Extracted %d file(s).\n",count);
		}
		
		printf("done!");
		return 0;
	}

	if (argc == 2)
	{
		strcpy(dst_file,"opening.bnr");
	} else strcpy(dst_file,argv[2]);

	iso_files *file = wiidisc.first_file;
	while (true)
	{
		if (file == NULL) break;
		if (strcmp(file->name,"opening.bnr") == 0) break;
		file = file->next;
	}
	if (file == NULL)
	{
		printf("Unable to find opening.bnr");
		return -1;
	}
	wiidisc.SaveDecryptedFile(dst_file, image, file->part, file->offset, file->size);
	printf("done!");

	return 0;
}

