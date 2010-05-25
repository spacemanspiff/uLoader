/*   
	From Custom IOS Module (FAT)
	
	Copyright (C) 2009 Waninkoko.
	Copyright (C) 2010 Hermes.

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


#include "fatffs_util.h"
#include "gfx.h"
#include <sys/dir.h>
#include <sys/stat.h>

#include "lz77.h"


extern int time_sleep; // screen sleep

/* 'WAD Header' structure */
typedef struct {
	/* Header length */
	u32 header_len;

	/* WAD type */
	u16 type;

	u16 padding;

	/* Data length */
	u32 certs_len;
	u32 crl_len;
	u32 tik_len;
	u32 tmd_len;
	u32 data_len;
	u32 footer_len;
} ATTRIBUTE_PACKED wadHeader;



extern u32 nand_mode;

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

static void lower_caps(char * s)
{
while(*s)
	{
	if(*s>='A' && *s<='Z') (*s) +=32; s++;
	}
}


s32 FAT_DeleteDir(const char *dirpath)
{
	DIR_ITER *dir;

	s32 ret;

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Read entries */
	for (;;) {
		char   filename[256], newpath[256];
		struct stat filestat;

		/* Read entry */
		if (dirnext(dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;

		lower_caps(filename);

		/* Generate entry path */
		strcpy(newpath, dirpath);
		strcat(newpath, "/");
		strcat(newpath, filename);

		/* Delete directory contents */
		if (filestat.st_mode & S_IFDIR)
			FAT_DeleteDir(newpath);

		/* Delete object */
		ret = remove(newpath);

		/* Error */
		if (ret != 0)
			break;
	}

	/* Close directory */
	dirclose(dir);

	return 0;
}

static int global_error=0;

//char temp_read_buffer[16384] ATTRIBUTE_ALIGN(32);

static int fat_copy_progress=0, fat_copy_total=0;

s32 _FFS_to_FAT_Copy(const char *ffsdirpath, const char *fatdirpath)
{
int n;
u32 blocks, ionodes;
int pos=0;
char *list;
s32 ret;

u32 ionodes_temp;


	if(ISFS_GetUsage(ffsdirpath, &blocks, &ionodes)) {global_error=-1;return -1;}
	
	list= memalign(32, ionodes*13);

	if(!list) {global_error=-2;return -2;}

	if(ISFS_ReadDir(ffsdirpath, list , &ionodes)) {free(list);global_error=-3;return -3;}

	if(ionodes) mkdir(fatdirpath, S_IRWXO | S_IRWXG | S_IRWXU);


	/* Read entries */
	for (n=0;n<ionodes;n++) {
		char  * filename;
		char newffspath[256], newfatpath[256];

		fat_copy_progress++;
		/* Read entry */
		filename=&list[pos];
		pos+=strlen(&list[pos])+1;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;

		/* Generate entry path */
		strcpy(newffspath, ffsdirpath);
		
		if(ffsdirpath[1]!=0 || ffsdirpath[1]!='/')
			strcat(newffspath, "/");
		strcat(newffspath, filename);

		strcpy(newfatpath, fatdirpath);
		strcat(newfatpath, "/");
		strcat(newfatpath, filename);

		ret=ISFS_ReadDir(newffspath, NULL, &ionodes_temp);
		if(ret==0) // it is a directory
			{
			
			_FFS_to_FAT_Copy(newffspath, newfatpath);

			if(global_error) {free(list);return global_error;}
			}

		else // copy the file
			{
			int fd;
			FILE *fp;

			fd=ISFS_Open(newffspath, ISFS_OPEN_READ);

			if(fd<0) 
				{
				global_error=-4;
				free(list);return global_error;
				}
			else
				{
				int len;

				sprintf((char *) temp_data+1024,"Copying %s %i%%", filename, fat_copy_progress*100/fat_copy_total);
				
				down_mess=(char *) temp_data+1024;

				fp=fopen(newfatpath,"w");
				if(!fd) {ISFS_Close(fd);global_error=-5;free(list);return global_error;}

				len= ISFS_Seek(fd, 0, 2);
				//if(len<0) {ISFS_Close(fd);global_error=-6;free(list);return global_error;}
		        ISFS_Seek(fd, 0, 0);
				
				while(len>0)
					{
					ret=len; if(len>0x10000) ret=0x10000;
					if(ISFS_Read(fd, (char *) temp_data+0x20000, ret)!=ret)
						{
						global_error=-7;
						break;
						}
					if(fwrite((char *) temp_data+0x20000, 1, ret, fp)!=ret)
						{
						global_error=-8;
						break;
						}
					len-=ret;
					}
				
				fclose(fp);
				ISFS_Close(fd);

				if(global_error) {free(list);return global_error;}
				}
			}	

	}
   
	free(list);
	return 0;
}


s32 FFS_to_FAT_Copy(const char *ffsdirpath, const char *fatdirpath)
{
u32 blocks, ionodes;
int ret;

char create_dir[256];

	ISFS_Initialize();

	ret=ISFS_GetUsage(ffsdirpath, &blocks, &ionodes);

	fat_copy_total=ionodes;
	fat_copy_progress=0;

	if(ret==0) 
		{
		int n=0;
	   
	 
		// creating the path directory

		strcpy(create_dir, fatdirpath);

		while(create_dir[n]!=0 && create_dir[n]!='/') n++;

		if(create_dir[n]=='/') n++;

		while(create_dir[n]!=0)
			{
			if(create_dir[n]=='/')
				{
				create_dir[n]=0;
				mkdir(create_dir, S_IRWXO | S_IRWXG | S_IRWXU);
				create_dir[n]='/';
				}
			n++;
			}
		global_error=0;

		

		// copy files
		_FFS_to_FAT_Copy(ffsdirpath, fatdirpath);

		ret=global_error=0;
		}
	else ret=-101;

		ISFS_Deinitialize();
		down_mess="";

return ret;
	
}

s32 FFS_to_FAT_File_Copy(const char *ffsdirpath, const char *fatdirpath)
{
int ret;
int n,m;
int fd;
FILE *fp;

char create_dir[256];

	ISFS_Initialize();

	strcpy(create_dir, fatdirpath);

	n=m=0;
	while(create_dir[n]!=0) {if(create_dir[n]=='/') m=n; n++;}

	down_mess=create_dir;

	
	if(create_dir[m]!=0)
		{
		sprintf((char *) temp_data+1024,"Copying %s File", &create_dir[m+1]);
		down_mess=(char *) temp_data+1024;
		}

	create_dir[m]=0;
	

	n=0;
	while(create_dir[n]!=0 && create_dir[n]!='/') n++;

	if(create_dir[n]=='/') n++;

	while(create_dir[n]!=0)
		{
		if(create_dir[n]=='/')
			{
			create_dir[n]=0;
			mkdir(create_dir, S_IRWXO | S_IRWXG | S_IRWXU);
			create_dir[n]='/';
			}
		n++;
		}
	mkdir(create_dir, S_IRWXO | S_IRWXG | S_IRWXU);

	fd=ISFS_Open(ffsdirpath, ISFS_OPEN_READ);

	if(fd<0) 
		{
		ret=-4;
		goto global_error;
		}
	else
		{
		int len;
		fp=fopen(fatdirpath,"w");
		if(!fd) {ISFS_Close(fd);ret=-5;goto global_error;}

		len= ISFS_Seek(fd, 0, 2);

		ISFS_Seek(fd, 0, 0);
		
		while(len>0)
			{
			ret=len; if(len>0x10000) ret=0x10000;
			if(ISFS_Read(fd, (char *) temp_data+0x20000, ret)!=ret)
				{
				ret=-7;
				break;
				}
			if(fwrite((char *) temp_data+0x20000, 1, ret, fp)!=ret)
				{
				ret=-8;
				break;
				}
			len-=ret;
			}
		ret=0;
		fclose(fp);
		ISFS_Close(fd);
		}

global_error:

	down_mess="";
	ISFS_Deinitialize();


return ret;
}

static char temp_cad[512];

void create_FAT_FFS_Directory(struct discHdr *header)
{

char device[2][4]={
	"sd:",
	"ud:"
};


if(!header) return;

	sprintf((char *) temp_cad,"%s/nand%c", &device[(nand_mode & 2)!=0][0], (nand_mode & 0xc) ? 49+((nand_mode>>2) & 3) : '\0');

	sprintf((char *) temp_cad+32,"%2.2x%2.2x%2.2x%2.2x", header->id[0], header->id[1], header->id[2], header->id[3]);

	
	sprintf((char *) temp_cad+64,"%s/title/00010000/%s", temp_cad, temp_cad+32);
	sprintf((char *) temp_cad+128,"%s/title/00010004/%s", temp_cad, temp_cad+32);
	sprintf((char *) temp_cad+256,"/title/00010000/%s", temp_cad+32);
	sprintf((char *) temp_cad+384,"/title/00010004/%s", temp_cad+32);
	

}

int test_FAT_game(char * directory)
{
DIR_ITER * dir=NULL;

	dir= diropen(directory);
	
	if(dir) {dirclose(dir);return 1;}

return 0;
}


char *get_FAT_directory1(void)
{
	return temp_cad+64;
}

char *get_FAT_directory2(void)
{
	return temp_cad+128;
}

char *get_FFS_directory1(void)
{
	return temp_cad+256;
}

char *get_FFS_directory2(void)
{
	return temp_cad+384;
}



void * FAT_get_dol(const char *dirpath)
{
    void *buffer= NULL;
	int len;
	FILE *fp;
    char *save_trace=str_trace;
	u8 *output=NULL;
	u32 output_size;

	u8 dol[6] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    u8 check[2048];



	s32 ret;

	str_trace="FAT_get_dol() open";

		fp=fopen(dirpath, "r");

		if(fp)
			{
			int lz77=0;
			
            str_trace="FAT_get_dol() read";
			ret=fread((void *) check, 1, 32, fp);

			if(ret<32)
				{
				fclose(fp); goto error;
				}
			
			if(check[0]==0x10 || check[0]==0x11) 
				{
				str_trace="FAT_get_dol() decompressLZ77";
				ret = decompressLZ77content(check, 32, &output, &output_size, 0);
				if(ret<0) memset(check, 0 , 8);
				else
					{
				    memcpy(check, output, 8);
					free(output); lz77=1;
					}
				}
			

			if(memcmp((void *) check, (void *) dol, 6)) {fclose(fp); goto error;}

			str_trace="FAT_get_dol() fseek";

			fseek(fp, 0, SEEK_END);

			len=ftell(fp);

			fseek(fp, 0, SEEK_SET);

			str_trace="FAT_get_dol() alloc";

			sprintf((void *) check,"FAT_get_dol() alloc %i", len);

			str_trace=(void *) check;
            if(lz77) buffer= memalign(32,len+32);
			else
				buffer=  (u8 *) SYS_AllocArena2MemLo(len+32768,32);

			

			if(!buffer) {fclose(fp); goto error;}
			
			str_trace="FAT_get_dol() read_buffer";
			ret=fread(buffer, 1, len, fp);

			fclose(fp);

			if(lz77)
				{
				str_trace="FAT_get_dol() decompressLZ77 2";
				ret = decompressLZ77content(buffer, len, &output, &output_size, 1);
				free(buffer);
				if(ret<0) {buffer=NULL;goto error;}
				buffer=output;
				}
			}

    
error:
	str_trace= save_trace;
	return buffer;
}

int FAT_read_file(const char *filepath, void **buffer, int *len)
{
FILE *fp;
int ret=-1;

	*buffer=NULL;

	fp=fopen(filepath, "r");
	if(!fp) return -1;

	fseek(fp, 0, SEEK_END);

	*len=ftell(fp);

	if(*len<=0) goto error;

//	if(*len==0) {ret=0;goto error;}


	fseek(fp, 0, SEEK_SET);

	*buffer= memalign(32, *len+32);

	if(!*buffer) {ret=-2;goto error;}

	ret=fread(*buffer, 1, *len, fp);

	if(ret!=*len) {free(*buffer);ret=-3;*buffer=NULL;}
	else ret=0;

error:

	fclose(fp);

return ret;
}

int FAT_write_file(const char *filepath, void *buffer, int len)
{
FILE *fp;
int ret=-1;


	fp=fopen(filepath, "w");
	if(!fp) goto error;

	ret=fwrite(buffer, 1, len, fp);

	if(ret!=len) {ret=-2;}
	else ret=0;

	fclose(fp);

error:

return ret;
}


int FAT_copy_file(const char *filepath_ori, const char *filepath_dest)
{
	FILE *fp, *fp2;
	int ret=-1;
	int len;
	int p;
	char *buffer;

	if(!strcmp(filepath_ori, filepath_dest)) return 0;
	
	buffer= malloc(0x40000);
	if(!buffer) return -2;

	fp=fopen(filepath_ori, "r");
	if(!fp) {free(buffer);return -1;}

	fseek(fp, 0, SEEK_END);

	len=ftell(fp);

	if(len<=0) goto error;

	fseek(fp, 0, SEEK_SET);

	fp2=fopen(filepath_dest, "w");
	if(!fp2) {ret=-3;goto error;}

	p=0;

	while(p<len)
	{
		ret=len-p; if(ret>0x40000) ret=0x40000;

		ret=fread(buffer, 1, ret, fp);

		if(ret<0) goto error_w;
		if(ret==0) break;

		if(fwrite(buffer, 1, ret, fp2)!=ret) goto error_w;

		p+=ret;
	}

	ret=0;
	fclose(fp2);

error:

	free(buffer);
	fclose(fp);

return ret;

error_w:

	free(buffer);
	fclose(fp);
	fclose(fp2);
	remove(filepath_dest);

return -4;
}

u32 title_ios;


static int cert_len;
static void *cert=NULL;

static int title_tmd_len;
static void *title_tmd=NULL;

static int title_tik_len;
static void *title_tik=NULL;

static u64 title_id;

static int banner_len=0;
void * title_banner=NULL;

const char dev_names[2][12]=
{
"sd:/nand",
"ud:/nand",
};


typedef struct
{
	u32 title_id;
	u32 cindex;
	u8 ios;
	u8 minor_ios;
	u16 n_shared;
	u8 hash[20];
} shared_entry;

static shared_entry *shared_list=NULL;
static int shared_entries=0;

shared_entry * search_hash(shared_entry *db, int db_len, u8 *hash)
{
int n;

	for(n=0;n<db_len/sizeof(shared_entry);n++)
	{
		if(!memcmp(db[n].hash, hash, 20)) return &db[n];
	}

return NULL;
}

static s32 scan_for_shared2(const char *dirpath)
{
	DIR_ITER *dir;

	//s32 ret;

	void *title_tmd;
	int title_tmd_len;

	tmd *Tmd;
	tmd_content *Content;

	u8 title_ios;

	u32 title_id;

	int n;

	int m;

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Read entries */
	for (;;) {
		char   filename[256], newpath[256];
		struct stat filestat;

		/* Read entry */
		if (dirnext(dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;
		if(!(filestat.st_mode & S_IFDIR)) continue;

		lower_caps(filename);

		/* Generate entry path */
		strcpy(newpath, dirpath);
		strcat(newpath, "/");
		strcat(newpath, filename);
		strcat(newpath, "/content/#title.tmd");

		n=FAT_read_file(newpath, &title_tmd, &title_tmd_len);
		if(n<0)
			{
			/* Generate entry path */
			strcpy(newpath, dirpath);
			strcat(newpath, "/");
			strcat(newpath, filename);
			strcat(newpath, "/content/title.tmd");
            n=FAT_read_file(newpath, &title_tmd, &title_tmd_len);
			}
		
	
		if(n==0)
		{
			Tmd=(tmd*)SIGNATURE_PAYLOAD((signed_blob *)title_tmd);


			Content = Tmd->contents;
			title_id= *(((u32 *) (void *) &Tmd->title_id)+1); 

			title_ios =(((u8 *)title_tmd)[0x18b]);

			for(n=0;n<Tmd->num_contents;n++)
			{
				if(Content[n].index==0) continue;
				if(!(Content[n].type & 0x8000)) continue;
				
				// shared content: search

				for(m=0;m<shared_entries;m++)
				{
					if(!memcmp(shared_list[m].hash, Content[n].hash, 20)) break;
				}

				if(m==shared_entries) // new entry in database
					{
					memcpy(shared_list[m].hash, Content[n].hash, 20);
					shared_list[m].title_id= title_id;
					shared_list[m].cindex= Content[n].cid;
					shared_list[m].ios=  title_ios;
					shared_list[m].minor_ios=  title_ios;
					shared_list[m].n_shared=1;
					shared_entries++;
					}
				else
					{	
					// if new>old update database

					if((u32) title_ios > (u32)shared_list[m].ios)
						{
						FILE *fp;
						sprintf(newpath,"%s/%s/content/%08x.app", dirpath, filename, Content[n].cid);

						// if content exist update
						fp=fopen(newpath, "r");
						if(fp)
							{
							fclose(fp);
							shared_list[m].title_id= title_id;
							shared_list[m].cindex= Content[n].cid;
							shared_list[m].ios=  title_ios;
							shared_list[m].n_shared=0;
							}
						}
					
					if((u32) title_ios < (u32)shared_list[m].minor_ios)
							shared_list[m].minor_ios=  title_ios;

					shared_list[m].n_shared++;
					}

			}
		}

		
		
	}

	/* Close directory */
	dirclose(dir);

	return 0;
}

void scan_for_shared(int is_sd)
{
char dir_path[256];
char dir_path2[256];

	 // to be sure shared folder exist
	sprintf(dir_path, "%s/shared", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	sprintf(dir_path, "%s/shared/00010001", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	shared_list=(void *) malloc(sizeof(shared_list) * 2048);
	if(!shared_list) return ;
	memset(shared_list, 0, sizeof(shared_list) *2048);

	shared_entries=0;
	sprintf(dir_path, "%s/title/00010001", &dev_names[is_sd==0][0]);
	
	scan_for_shared2(dir_path);
	

	FILE *fp, *fp2;
	#if 0
	fp=fopen("sd:/shared.txt", "w");

	if(!fp) return;

	int n,m;

	for(m=0;m<shared_entries;m++)
		{	
		fprintf(fp, "Title: %08x Title %08x.app IOS: %i MINOR IOS %i n_shared %i Hash: ", shared_list[m].title_id, shared_list[m].cindex, shared_list[m].ios, shared_list[m].minor_ios, shared_list[m].n_shared);
	
		for(n=0;n<20;n++) fprintf(fp, "%02x ", shared_list[m].hash[n]);

		fprintf(fp, "\n");
		}

	fclose(fp);
	#endif
	sprintf(dir_path, "%s/shared/#shared.dtb", &dev_names[is_sd==0][0]);
	
	if(shared_entries)
	{
	int n;
	void *data=NULL;
	int len_data;

	n=1;
    if(FAT_read_file(dir_path, &data, &len_data)==0)
		{
		if(len_data==(sizeof(shared_entry)*shared_entries) && len_data!=0 && !memcmp(data, shared_list, len_data)) n=0;
		free(data);data=NULL;
		}

	if(n)
		fp=fopen(dir_path, "w"); // update the database list

	else  fp=NULL; // only test/update the shared content

	if(fp)
		{
		
		for(n=0;n<shared_entries;n++)
			{
			sprintf(dir_path, "%s/shared/00010001/%08x%08x.app", &dev_names[is_sd==0][0], shared_list[n].title_id, shared_list[n].cindex);
			fp2=fopen(dir_path,"r");
			if(fp2) {fclose(fp2);} // if exist don't write the content
			else
				{
				
				sprintf(dir_path, "%s/title/00010001/%08x/content/%08x.app", &dev_names[is_sd==0][0], shared_list[n].title_id, shared_list[n].cindex);
				
				sprintf(dir_path2, "%s/shared/00010001/%08x%08x.app", &dev_names[is_sd==0][0], shared_list[n].title_id, shared_list[n].cindex);
				
				if(FAT_copy_file(dir_path, dir_path2)<0) {continue;}
				
				}

			if(fp) fwrite(&shared_list[n], sizeof(shared_entry), 1, fp);
			}
		if(fp) fclose(fp);
		}
	}

if(shared_list)
	free(shared_list);

}

typedef struct {
	u8 ios;
	char content_folder[255];
} title_path;



int Force_Update(u64 title, int is_sd)
{

title_path titles[64];

char dirpath[256];
char filepath[256];

DIR_ITER *dir;

void *title_tmd;
int title_tmd_len;

tmd *Tmd;
tmd_content *Content;

void *title2_tmd;
int title2_tmd_len;

tmd *Tmd2;
tmd_content *Content2;

u8 title_ios, title_ios2;

int n,m;

int ret=0;

FILE *fp;

int shared=1;


	if((u32) (title>>32)!=0x00010001) return -1;

	sprintf(filepath, "%s/title/00010001/%08x/content/title.tmd", &dev_names[is_sd==0][0],(u32) title);

	if(FAT_read_file(filepath, &title_tmd, &title_tmd_len)<0) return -2;

	Tmd= (tmd*)SIGNATURE_PAYLOAD((signed_blob *)title_tmd);
	Content = Tmd->contents;
	title_ios =(((u8 *)title_tmd)[0x18b]);

	for(n=0;n<Tmd->num_contents;n++)
		{
		if(Content[n].index==0) continue;
		if(Content[n].type & 0xFF00) {shared=1;break;}
			
		}

	if(shared)
		{
		sprintf(filepath, "%s/title/00010001/%08x/content/#title.tmd", &dev_names[is_sd==0][0],(u32) title);
		
		// clone title.tmd
		fp=fopen(filepath, "r");
		if(!fp)
			{
			if(FAT_write_file(filepath, title_tmd, title_tmd_len)<0) {free(title_tmd);return -2;}
			}
		else fclose(fp);

		for(n=0;n<Tmd->num_contents;n++)
			{
			if(Content[n].index==0) continue;
			if(Content[n].type & 0xFF00) Content[n].type&=0x00ff;	
			}
		sprintf(filepath, "%s/title/00010001/%08x/content/title.tmd", &dev_names[is_sd==0][0],(u32) title);
		if(FAT_write_file(filepath, title_tmd, title_tmd_len)<0) {free(title_tmd);return -2;}
		}


	memset(titles, 0, sizeof(titles));

	sprintf(dirpath, "%s/title/00010001", &dev_names[is_sd==0][0]);

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -3;

	/* Read entries */
	for (;;) {
		char   filename[256], newpath[256];
		struct stat filestat;

		/* Read entry */
		if (dirnext(dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;
	
		if(!(filestat.st_mode & S_IFDIR)) continue;

		lower_caps(filename);

		sprintf(filepath, "%s/title/00010001/%08x/content/title.tmd", &dev_names[is_sd==0][0],(u32) title);
		sprintf(newpath, "%s/title/00010001/%s/content/title.tmd", &dev_names[is_sd==0][0], filename);

		if(!strcmp(filepath, newpath)) continue;

        // check for the newest IOS using hash

		if(FAT_read_file(newpath, &title2_tmd, &title2_tmd_len)<0) continue;
       
		Tmd2= (tmd*)SIGNATURE_PAYLOAD((signed_blob *)title2_tmd);
		Content2 = Tmd2->contents;
		title_ios2 =(((u8 *)title2_tmd)[0x18b]);
		
		
		for(n=0;n<Tmd->num_contents;n++)
			{
			if(Content[n].index==0) continue;

			sprintf(filepath, "Scan: %08x.app", Content[n].cid);
			down_mess=filepath;
            
			for(m=0;m<Tmd2->num_contents;m++)
				{
				if(Content2[m].index==0) continue;

				if(!memcmp(Content[n].hash, Content2[m].hash, 20))
					{
					if(((u32)titles[n].ios) < (u32) title_ios2)
						{
						sprintf(newpath, "%s/title/00010001/%s/content/%08x.app", &dev_names[is_sd==0][0], filename, Content2[m].cid);
						fp=fopen(newpath, "rb");
						if(fp)
							{
							fclose(fp);
                            titles[n].ios=title_ios2;
							strcpy(titles[n].content_folder, newpath);
							
							}
						}
					break;
					}
				}
			}

	    down_mess="";

		if(title2_tmd) free(title2_tmd); title2_tmd=NULL;

	}

/* Close directory */
	dirclose(dir);

	// update contents

	for(n=0;n<Tmd->num_contents;n++)
		{
		if(Content[n].index==0) continue;

		if(titles[n].ios)
			{
        
			down_mess="";

            sprintf(filepath, "%s/title/00010001/%08x/content/%08x.app", &dev_names[is_sd==0][0],(u32) title, Content[n].cid);
			
			if(!strcmp(titles[n].content_folder, filepath)) continue; // is the same content
			
			if(titles[n].ios==title_ios)
				{
				fp=fopen(filepath, "r");
				if(fp) {fclose(fp);continue;}
				}

			sprintf(dirpath, "Updating: %08x.app", Content[n].cid);
			down_mess=dirpath;

		    if(FAT_copy_file(titles[n].content_folder, filepath)<0) {ret=-4;break;}
			else ret=1;
			}
		
		}

	free(title_tmd);

	down_mess="";
	
	return ret;
}

static u16 group_id;

u32 ERR_FATFSS_APP=0;


int FAT_get_title(u64 id, void **dol, u8 *str_id, int dont_use_boot)
{
	char dir_path[256],dir_path2[256];

	tmd *Tmd=NULL;
	tmd_content *Content;

	FILE *fp=NULL;

	int patch_tmd=0;

	int n;

	int ret=-1;

	int update_db=0;

	title_id=id;

	*dol=NULL;

	str_trace="FAT_get_title() section 1";
	//nand_mode=1;

	// to be sure shared folder exist

	sprintf(dir_path2, "%s/shared", &dev_names[(nand_mode & 1)==0][0]);
	mkdir(dir_path2, S_IRWXO | S_IRWXG | S_IRWXU);

	sprintf(dir_path2, "%s/shared/%08x", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32));
	mkdir(dir_path2, S_IRWXO | S_IRWXG | S_IRWXU);

	// reading title.tmd

	str_trace="FAT_get_title() section 2";

	sprintf(dir_path, "%s/title/%08x/%08x/content/title.tmd", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
	
    if(FAT_read_file(dir_path, &title_tmd, &title_tmd_len)<0) //return -1; // no exist tmd !!!
		{
		sprintf(dir_path, "%s/title/%08x/%08x/content/#title.tmd", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
		if(FAT_read_file(dir_path, &title_tmd, &title_tmd_len)<0) return -10001;
		}
	else
		{
		sprintf(dir_path, "%s/title/%08x/%08x/content/#title.tmd", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);

		fp=fopen(dir_path, "rb");
		if(!fp) // clone title TMD
			{
			if(FAT_write_file(dir_path, title_tmd, title_tmd_len)<0) return -10004;
			}
		else fclose(fp);
		}

	str_trace="FAT_get_title() section 3";

	Tmd=(tmd*)SIGNATURE_PAYLOAD((signed_blob *)title_tmd);

	if(!Tmd) exit(0);

    u16 boot_index = Tmd->boot_index;
	Content = Tmd->contents;
	group_id= Tmd->group_id;

	title_ios=(u32) (Tmd->sys_version & 0xff);

	str_trace="FAT_get_title() section 3a";

	// obtain the complete game ID
	
	{
	static char str_id2[16];

	sprintf((void *) str_id2, "%04x%02x", (u32) id, group_id);

	str_trace="FAT_get_title() section 3b";

	memcpy(str_id2, str_id, 6);
	}

	// read shared list
	void *shared_db= NULL;
	int shared_db_len=0;


	sprintf(dir_path, "%s/shared/#shared.dtb", &dev_names[(nand_mode & 1)==0][0]);
	
	if(FAT_read_file(dir_path, &shared_db, &shared_db_len)<0) 
		{
		shared_db=NULL;
		shared_db_len=0;
		}

	// test and patch for not shared content
	
	str_trace="FAT_get_title() section 4";

	for(n=0;n<Tmd->num_contents;n++)
		{
	    shared_entry * db_entry=NULL;
        int is_shared=0;

		// patch to not use NAND shared content

		if(Content[n].type & 0xFF00) {Content[n].type&=0x00ff;patch_tmd=1;is_shared=1;}
		if(Content[n].index==0) continue;

         if(is_shared)
			{ //1
			// test if exist shared content. if not exist copy it
			
			update_db=1;

			if(!shared_db) 
				{
				cabecera2("Searching Shared Content...");
				scan_for_shared((nand_mode & 1)==1);
			    sprintf(dir_path, "%s/shared/#shared.dtb", &dev_names[(nand_mode & 1)==0][0]);
	
				if(FAT_read_file(dir_path, &shared_db, &shared_db_len)<0) 
					{
					shared_db=NULL;
					shared_db_len=0;
					}
			    }
			if(shared_db) db_entry= search_hash(shared_db, shared_db_len, Content[n].hash);

			sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);

			if(!db_entry)
				{
				// open .app files

				fp=fopen(dir_path, "rb");
				if(!fp) {ERR_FATFSS_APP=Content[n].cid;return -10003;} // incomplete game. Maybe you have the content in the NAND... or not.
				fclose(fp);
				
				// use this content and update database
				}
			else
				{
				// 2
		
				fp=fopen(dir_path, "rb");
				// update content
				if(!fp || ((u32) title_ios<((u32) db_entry->ios)))
					{

					if(fp)
						{
						fclose(fp);fp=NULL;
						}

					cabecera2("Updating Shared Content...");

					sprintf(dir_path, "%s/shared/00010001/%08x%08x.app", &dev_names[(nand_mode & 1)==0][0], db_entry->title_id, db_entry->cindex);
					sprintf(dir_path2, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);
					
					if(FAT_copy_file(dir_path, dir_path2)<0)
						{
						update_db=1;
						}
			
					}
			
				if(fp) fclose(fp);
				}// 2
			

			} // 1

		else // is not shared
			{
			shared_entry * db_entry=NULL;
			sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);

			fp=fopen(dir_path, "rb");
			if(!fp)
				{
				cabecera2("Broken Content. Searching...");
				
				if(!shared_db) 
					{
					scan_for_shared((nand_mode & 1)==1);
					//exit(0);
					sprintf(dir_path2, "%s/shared/#shared.dtb", &dev_names[(nand_mode & 1)==0][0]);
		
					if(FAT_read_file(dir_path2, &shared_db, &shared_db_len)<0) 
						{
						shared_db=NULL;
						shared_db_len=0;
						}
					}
				if(shared_db) db_entry= search_hash(shared_db, shared_db_len, Content[n].hash);
				
			    if(db_entry)
					{
					

					sprintf(dir_path, "%s/shared/00010001/%08x%08x.app", &dev_names[(nand_mode & 1)==0][0], db_entry->title_id, db_entry->cindex);
					sprintf(dir_path2, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);
					
					fp=NULL;
					
					if(FAT_copy_file(dir_path, dir_path2)==0)
						{
						cabecera2("Equivalent Content Found");fp=fopen(dir_path,"rb");sleep(1);
						}
					}
				}
			

			if(!fp) {ERR_FATFSS_APP=Content[n].cid;return -10003;} // incomplete game. Maybe you have the content in the NAND... or not.
			fclose(fp);
			Content[n].type&=0xff; // set private
			}
		}

	str_trace="FAT_get_title() section 5";

	if(patch_tmd)
		{
		sprintf(dir_path, "%s/title/%08x/%08x/content/title.tmd", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);

		if(FAT_write_file(dir_path, title_tmd, title_tmd_len)<0) {ret=-10005;goto error;} // error saving modifing tmd
		}
	
	// update shared db and games
	if(update_db) 
		{
		str_trace="FAT_get_title() scan shared";
		scan_for_shared((nand_mode & 1)==1);
		}

	str_trace="FAT_get_title() section 6";
	// create data folder if not exist
	sprintf(dir_path, "%s/title/%08x/%08x/data", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

    banner_title=NULL;

	str_trace="FAT_get_title() section 7";
	for(n=0;n<Tmd->num_contents;n++)
		{
		if(Content[n].index==0) 
			{
			
			sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);
			if(FAT_read_file(dir_path, (void *) &title_banner, &banner_len)<0) title_banner=NULL;
		
			break;
			}
		}

	str_trace="FAT_get_title() section 8";

	if(dont_use_boot )
	{
	
	// scan for dols
	for(n=0;n<Tmd->num_contents;n++)
		{
		if(n==boot_index) continue;
		if(Content[n].index==0) continue;

		sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[n].cid);
		
		*dol=FAT_get_dol(dir_path);
		if(*dol) break;
	
		}
	}

	str_trace="FAT_get_title() section 9";

	if(!dont_use_boot || !*dol) 
		{
		sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id, Content[boot_index].cid);
		
		*dol=FAT_get_dol(dir_path);
		}

	if(!*dol) {ret=-10006; goto error;}
	

	#if 0
	sprintf(dir_path, "%s/ticket/%08x/%08x.tik", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) (id));

	if(FAT_read_file(dir_path, &title_tik, &title_tik_len)<0) ; // if error use internal ticket
	#else
	title_tik=NULL;
	#endif

	
	str_trace="FAT_get_title() section 10";
/******************************************************************************************************************************************************/
	// snapshot copy swap 
	{
	void *snap;
	int snap_len;
	int snap_found=0;

    sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/snapshot.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
    if(FAT_read_file(dir_path, &snap, &snap_len)>=0)
		{
		char zeroes[16]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        snap_found=1;
		// test for a good shapshot
		if(memcmp(snap, zeroes, 16))
			{
			sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/#snapshot.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
			FAT_write_file(dir_path, snap, snap_len);
			}
		else goto recover_snap;
		}
	else
		{
		recover_snap:

		sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/#snapshot.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
		if(FAT_read_file(dir_path, &snap, &snap_len)>=0)
			{
			snap_found=1;
			sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/snapshot.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
			FAT_write_file(dir_path, snap, snap_len);
			}
		}
	
	if(!snap_found)
		{
		sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/qsdata.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
		if(FAT_read_file(dir_path, &snap, &snap_len)>=0)
			{
			char zeroes[16]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			snap_found=1;
			// test for a good shapshot
			if(memcmp(snap, zeroes, 16))
				{
				sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/#qsdata.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
				FAT_write_file(dir_path, snap, snap_len);
				}
			else goto recover_snap2;
			}
		else
			{
			recover_snap2:

			sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/dummy", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
			if(FAT_read_file(dir_path, &snap, &snap_len)>=0)
				{
				snap_found=1;
				sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/qsdata.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
				FAT_write_file(dir_path, snap, snap_len);
				}
			else
				{
				sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/#qsdata.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
				
				if(FAT_read_file(dir_path, &snap, &snap_len)>=0)
					{
					snap_found=1;
					sprintf(dir_path, "%s/title/%08x/%08x/data/nocopy/qsdata.bin", &dev_names[(nand_mode & 1)==0][0], (u32) (id>>32), (u32) id);
					FAT_write_file(dir_path, snap, snap_len);
					}
				}
			}
		}
	
	}
	
	if(shared_db) free(shared_db);

	str_trace="";
	return 0;

error:

	//if(*dol) {free(*dol);*dol=NULL;} // dol is in MEM2
	str_trace="FAT_get_title() free shared_db";
	if(shared_db) free(shared_db); shared_db=NULL;
	
	str_trace="FAT_get_title() free title_tmd";
	if(title_tmd) free(title_tmd); title_tmd=NULL;

	//if(title_tik) free(title_tik);

	str_trace="FAT_get_title() free cert";
	if(cert) free(cert); cert=NULL;
   
	*dol=NULL;

	str_trace="";

return ret;
}

static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "#/sys/cert.sys";
static char tik_fs[] ATTRIBUTE_ALIGN(32) = "#/ticket/00000001/00000024.tik"; // i can use '#' because it works under emulation 

int FAT_Identify(void)
{
int fd;
u32 current_ios;
tmd *Tmd;
tik *ticket;
	
	DCInvalidateRange((void *) 0x80003140, 4);
	current_ios= ((*((u32 *)0x80003140))>>16) & 255;

	ISFS_Initialize();

	fd=ISFS_Open(certs_fs, ISFS_OPEN_READ);

	
	if(fd<0)  goto error;

	cert_len= ISFS_Seek(fd, 0, 2);
	
	ISFS_Seek(fd, 0, 0);

	cert=memalign(32,cert_len+32);
	
	if(!cert) goto error;

	if(ISFS_Read(fd, cert, cert_len) != cert_len)  goto error;
	
	ISFS_Close(fd);

	if(!title_tik)
	{
    
	// use ticket from the current IOS
    
	tik_fs[24]=(current_ios>>4)>=10 ? 87 + (current_ios>>4) : 48 + (current_ios>>4);
	tik_fs[25]=(current_ios & 0xf)>=10 ? 87 + (current_ios & 0xf) : 48 + (current_ios & 0xf);

	DCFlushRange(tik_fs, sizeof(tik_fs));

	fd=ISFS_Open(tik_fs, ISFS_OPEN_READ);
	
	if(fd<0)  goto error;

	title_tik_len= ISFS_Seek(fd, 0, 2);
	
	ISFS_Seek(fd, 0, 0);

	title_tik=memalign(32,title_tik_len+32);
	
	if(!title_tik) goto error;

	if(ISFS_Read(fd, title_tik, title_tik_len) <0)  goto error;
	
	ISFS_Close(fd);

   
	ticket = (tik*)SIGNATURE_PAYLOAD((signed_blob *) title_tik);
    
	// modify ticket with the titleid
	
	memcpy(&ticket->titleid, &title_id, 8);
	memset(&ticket->reserved[0], 0, 2);//group_id;
/*
		{
		u8 * tik=(u8 *) title_tik;
//		*((u32 *) &tik[0x1DC])=(u32) (title_id>>32);
//		*((u32 *) &tik[0x1E0])=(u32) title_id;
//		*((u16 *) &tik[0x1E6])=(u16) 0;//group_id;
		tik[0x221]=0;
		tik[0x263]=0;

		}
*/
	
	}

	fd=-1;

	//ISFS_Deinitialize();

	if(!title_tmd) return -1;

	Tmd=(tmd*)SIGNATURE_PAYLOAD((signed_blob *)title_tmd);
	

   ((u32 *) ((void *) &Tmd->sys_version))[1]=(u32 ) current_ios; // fake tmd IOS;

   DCFlushRange(cert, cert_len);
   DCFlushRange(title_tmd, title_tmd_len);
   DCFlushRange(title_tik, title_tik_len);

	return ES_Identify((void *)cert, cert_len, (void *) title_tmd, title_tmd_len, (void *) title_tik, title_tik_len, NULL);

error:
	//ISFS_Deinitialize();
	return -1;
}


int Install_from_wad(char *filename, int is_sd);

static char mess[128];

void FAT_Install(int is_sd)
{
char dir_path[256];

int one=1;
DIR_ITER *dir;


    // test for device
	
	dir = diropen(&dev_names[is_sd==0][0]);
	if (!dir)
		return;
	dirclose(dir);

   // to be sure shared folder exist
	sprintf(dir_path, "%s/shared", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	sprintf(dir_path, "%s/shared/00010001", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);


	sprintf(dir_path, "%s/install", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	
	/* Open directory */
	dir = diropen(dir_path);
	if (!dir)
		return;

	
	/* Read entries */
	for (;;) {
		char   filename[256], newpath[256];
		struct stat filestat;

		time_sleep=15*60;

		/* Read entry */
		if (dirnext(dir, filename, &filestat))
			break;

		/* Non valid entry */
		if (filename[0]=='.')
			continue;

		if(!(filestat.st_mode & S_IFDIR)) 
		    {
			lower_caps(filename);
		    // test for wads
			if(!is_ext(filename,".wad")) continue;

			if(one) 
				{
				if(is_sd) down_mess="Installing SD Content"; 
				else down_mess="Installing USB Content"; 
				one=0;
				sleep(1);
				}

			strcpy(newpath, dir_path);
			strcat(newpath,"/");
			strcat(newpath, filename);

			usleep(500*1000);
			sprintf(mess,"Installing %s title", get_name_short_fromUTF8(filename));
		    down_mess=mess;
			sleep(3);
			if(Install_from_wad(newpath,is_sd)<0)
				{
				sprintf(mess,"Error Installing %s", get_name_short_fromUTF8(filename));
				down_mess=mess;
				sleep(3);
				}
			else
				{
				strcpy(newpath, dir_path);
				strcat(newpath,"/");
				strcat(newpath, filename);

				// delete install datas
				remove(newpath);
				}

			continue;
			}


	}
time_sleep=5*60;
/* Close directory */
	dirclose(dir);

	down_mess="";
}


void aes_set_key(u8 *key);
void aes_decrypt2(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len, int end);
extern void _decrypt_title_key(u8 *tik, u8 *title_key);

static u8 title_iv[16];


int Install_from_wad(char *filename, int is_sd)
{
FILE *fp=NULL, *fp2=NULL;


wadHeader *header=NULL;
void * mem=NULL;
void *decrypt=NULL;
u8 * tmd_data=NULL;

u8 title_key[16];

tmd *Tmd;
tmd_content *Content;

char *tik=NULL;

u32 *title_id=0;

short n;

int offset;

int error=0;

int wad_len=0;

int delete_content=0;

char dir_path[256];

		
    down_frame=0;
	fp=fopen(filename, "r");
	if(!fp) return -1;
    
	
	header=malloc(sizeof(wadHeader)); if(!header) {error=-2;goto error;}

	if(fread(header, 1, sizeof(wadHeader), fp)!=sizeof(wadHeader)){error=-3;goto error;}
 
    offset= round_up(header->header_len, 64)+ round_up(header->certs_len, 64)+ round_up(header->crl_len, 64);
    

	tik=malloc(round_up(header->tik_len, 64));
	if(!tik) {error=-2;goto error;}

	fseek(fp, offset, SEEK_SET);

    if(fread(tik, 1, header->tik_len, fp)!=header->tik_len){free(tik);tik=NULL;error=-3;goto error;}
	

	offset+= round_up(header->tik_len, 64);

    tmd_data= memalign(32, header->tmd_len); if(!tmd_data) {error=-2;goto error;}

	fseek(fp, 0, SEEK_END);
	wad_len=ftell(fp);

	fseek(fp, offset, SEEK_SET);

    if(fread(tmd_data, 1, header->tmd_len, fp)!=header->tmd_len){error=-3;goto error;}
	

	offset += round_up(header->tmd_len, 64);

    Tmd=(tmd*)SIGNATURE_PAYLOAD((signed_blob *) tmd_data);

	Content = Tmd->contents;

	title_id= (u32 *) (void *) &Tmd->title_id;
    
	// create folder destination

	sprintf(dir_path, "%s/title", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);
	sprintf(dir_path, "%s/title/%08x", &dev_names[is_sd==0][0], title_id[0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);
	sprintf(dir_path, "%s/title/%08x/%08x", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);
	sprintf(dir_path, "%s/title/%08x/%08x/content", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);
	sprintf(dir_path, "%s/title/%08x/%08x/data", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	if(1)
	{

	sprintf(dir_path, "%s/ticket", &dev_names[is_sd==0][0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);
	sprintf(dir_path, "%s/ticket/%08x", &dev_names[is_sd==0][0], title_id[0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	sprintf(dir_path, "%s/ticket/%08x/%08x.tik", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
	if(FAT_write_file(dir_path, tik, header->tik_len)<0) {free(tik);tik=NULL;error=-9;goto error;}
	}
	
    delete_content=1;
	// get title_key for decript content
	_decrypt_title_key((void *) tik, title_key);
	aes_set_key(title_key);

	// create shared folder
	sprintf(dir_path, "%s/shared/%08x", &dev_names[is_sd==0][0], title_id[0]);
	mkdir(dir_path, S_IRWXO | S_IRWXG | S_IRWXU);

	free(tik);tik=NULL;

	decrypt=memalign(32, 256*1024+32);
	if(!decrypt)  {error=-2;goto error;}

	mem=memalign(32, 256*1024+32);
	if(!mem) {error=-2;goto error;}
   
    time_sleep=15*60;

	for(n=0;n<Tmd->num_contents;n++)
		{
		int len = Content[n].size; //round_up(Content[n].size, 64);
		int is_shared=0;
		int readed=0;
		int size;
		int decryp_state;
        
		sprintf(mess,"Copying Title /%08x/%08x/ Content #%i", title_id[0], title_id[1], n);
		down_mess=mess;

		sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[is_sd==0][0], title_id[0], title_id[1], Content[n].cid);
	
	
		fp2=fopen(dir_path, "wb");
		if(!fp2) {error=-4;goto error;}


		// fix private content
		if(Content[n].type & 0xC000) 
			{
			is_shared=1;
			}
   
			memset(title_iv, 0, 16);
			memcpy(title_iv, &n, 2);

			decryp_state=0; // start

			fseek(fp, offset, SEEK_SET);
			//size=ftell(fp);
            //if(size>=0 && size<wad_len) 
			while(readed<len)
			{
			int res=0;
			
			size = (len - readed);if (size > 256*1024) size = 256*1024;
			
			memset(mem ,0, size);
			res=fread(mem, 1, size, fp);
			if(res<0){error=-6;goto error;}
			if(res==0) 
				  break;

 
			if((readed+size)>=len || size<256*1024) decryp_state=1; // end
			
			aes_decrypt2(title_iv, mem, decrypt, size, decryp_state);

			if(fwrite(decrypt, 1, size, fp2)!=size) {error=-4;goto error;}
			readed += size;
			}
			
		fclose(fp2);
		fp2=NULL;
		offset+=round_up(Content[n].size, 64);

		if(readed==0)
			{
			if(!is_shared) 
				{
				// special for non title 0x00010001
				if(title_id[0]!=0x00010001)
					{
					// remove content
					
                    sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[is_sd==0][0], title_id[0], title_id[1], Content[n].cid);
					remove(dir_path);

					break;
					}
			     error=-8;goto error;
				}
			else
				{
				sprintf(mess,"Warning: Shared content %08x don't exist",  Content[n].cid);
				down_mess=mess;
				sprintf(dir_path, "%s/title/%08x/%08x/content/%08x.app", &dev_names[is_sd==0][0], title_id[0], title_id[1], Content[n].cid);
				remove(dir_path);
				sleep(1);
				}
			}
		else			
			{
			
			}


		}
    
	//fclose(fp);
	//fp=fp2=NULL;

	// save the original tmd
	sprintf(dir_path, "%s/title/%08x/%08x/content/title.tmd", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
	if(FAT_write_file(dir_path, tmd_data, header->tmd_len)<0) {error=-7;goto error;}
	
error:

	if(mem) free(mem);
	if(decrypt) free(decrypt);
	
	if(tmd_data) free(tmd_data);
	if(header) free(header);

	if(fp) fclose(fp);
	if(fp2) fclose(fp2);
	
    time_sleep=5*60;
	
	  if(error)
		{ 
		down_frame=-1;
		switch(-error)
			{
			case 2:
				 sprintf(mess,"Out of Memory");
			break;

			case 3:
				 sprintf(mess,"Error Reading WAD");
			break;

			case 4:
				 sprintf(mess,"Error Creating Content");
			break;

			case 5:
				 sprintf(mess,"Error Creating Shared Content");
			break;

			case 6:
				 sprintf(mess,"Error Reading Content");
			break;

			case 7:
				 sprintf(mess,"Error Creating TMD");
			break;

			case 8:
				 sprintf(mess,"Error truncated WAD");
			break;

			case 9:
				 sprintf(mess,"Error Creating TIK");
			break;

			default:
				  sprintf(mess,"Undefined Error");
			break;
			}

		down_mess=mess;
	
		
		if(title_id && error && delete_content)
			{
			// deletes the content
			sprintf(dir_path, "%s/title/%08x/%08x", &dev_names[is_sd==0][0], title_id[0], title_id[1]);
			FAT_DeleteDir(dir_path);
			remove(dir_path);
			}
		sleep(5);
		}

	down_mess="Update Shared Info";
    if(!error && title_id && title_id[0]==0x00010001)
		scan_for_shared(is_sd);
	down_mess="";

	return error;
}

// end of this shit xD

