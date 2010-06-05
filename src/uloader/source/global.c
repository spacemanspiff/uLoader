#include "global.h"
#include "cheats.h"
#include "../libfat/source/partition.h"
#include "fatffs_util.h"

extern void* SYS_AllocArena2MemLo(u32 size,u32 align);

//---------------------------------------------------------------------------------
/* FAT/WBFS data routines */
//---------------------------------------------------------------------------------


int num_fat_games=0;

char fat_games[256][256];
struct discHdr fat_disc[256];

static int fat_index = 0;

extern u8 alt_dol_disc_tab[32768];

dol_infodat AlternativeDol_infodat;

int get_fat_name_pass = 0;

char *get_fat_name(u8 *id)
{
	int n;

	for (n=0; n < num_fat_games; n++) {
		if (!strncmp((void *) fat_disc[n].id, (void *) id, 6)) {
			if (!get_fat_name_pass) {
				if (!strncmp((void *) & fat_games[n][0], "sd", 2)) {
					if (!sd_ok)
						return NULL;
				} else {
					if (!ud_ok) 
						return NULL;
				}
			}
			fat_index = n;
			return &fat_games[n][0];
		}
	}

	return NULL;
}

s32 global_RenameGame(u8 *discid, char *new_name)
{
	FILE *fp;
	char *name;

	if (!is_fat)
		WBFS_RenameGame(discid, new_name);

	name = get_fat_name(discid);
	if (!name) 
		return 0;

	if (strstr(name, "#uloader.dat"))
		return 0;

	fp = fopen(name,"r+"); // lee el fichero de texto
	if (fp) {
		fseek(fp, 32768+32, SEEK_SET);
		new_name[63] = 0;
		if (fwrite(new_name, 1, 64 ,fp) != 64)
			goto err;

		memcpy((char *) fat_disc[fat_index].title, 
		       (char *) new_name, 64);

		fclose(fp);
		return 1;
	} else
		return 0;
err:
	fclose(fp);

	return 0;
}

s32 global_SetBCA(u8 *discid, u8 *bca)
{
	FILE *fp;
	char *name;

	if (!is_fat)
		WBFS_SetBCA(discid, bca);

	name = get_fat_name(discid);
	
	if (!name)
		return 0;

	if (strstr(name, "#uloader.dat"))
		return 0;

	fp = fopen(name,"r+"); // lee el fichero de texto
	if(fp) {
		fseek(fp, 32768+256, SEEK_SET);
		if (fwrite(bca, 1, 64 ,fp) != 64)
			goto err;

		fclose(fp);

		return 1;
	} else
		return 0;

err:
	fclose(fp);

	return 0;

}

s32 global_LoadDolInfo(void *data)
{
	if (!is_fat)
		return WBFS_LoadDolInfo(data);

	return 0;
}

s32 global_SaveDolInfo(void *data)
{
	if (!is_fat)
		return WBFS_SaveDolInfo(data);
	
	return 0;
}

s32 global_GetProfileDatas(u8 *discid, u8 *buff)
{
	FILE *fp;
	char *name;

	if (!is_fat)
		return WBFS_GetProfileDatas(discid, buff);

	name = get_fat_name(discid);

	if (!name) 
		return 0;

	fp = fopen(name,"rb");
	if (fp) {
		if (!strstr(name, "#uloader.dat"))
			fseek(fp, 32768+1024, SEEK_SET);

		if (fread(buff, 1, 1024 ,fp) != 1024) {
			memset((void *) buff,0,1024); 
			goto err;

		}
		if (buff[0]=='H' && 
		    buff[1]=='D' && 
		    buff[2]=='R' && 
		    buff[3]!=0) {
			if(!strstr(name, "#uloader.dat"))
				fseek(fp, 32768+2048, SEEK_SET);
			else
				fseek(fp, 1024, SEEK_SET);

			if (((u32) buff[3])<201) {
				if (fread(buff+1024, 1, ((u32) buff[3])*1024 ,fp) != ((u32) buff[3])*1024) {
					memset((void *) buff,0,1024); 
					goto err;
				}
			} else {
				buff[3]=0;
				buff[9]=0;
			} // bad PNG
		}
		fclose(fp);
		return 1;
	} else
		return 0;

err:
	fclose(fp);
	return 0;
}

s32 global_SetProfileDatas(u8 *discid, u8 *buff)
{
	FILE *fp;
	char *name;

	make_rumble_off();

	if (!is_fat) 
		return WBFS_SetProfileDatas(discid, buff);

	name = get_fat_name(discid);
	if (!name)
		return 0;

	fp = fopen(name,"r+"); // lee el fichero de texto
	if(fp) {
		if (!strstr(name, "#uloader.dat"))
			fseek(fp, 32768+1024, SEEK_SET);
		
		if (fwrite(buff, 1, 1024 ,fp) != 1024) 
			goto err;

		if (buff[0]=='H' && 
		    buff[1]=='D' && 
		    buff[2]=='R' && 
		    buff[3]!=0 && 
		    ((u32) buff[3])<201) {
			if(!strstr(name, "#uloader.dat"))
				fseek(fp, 32768+2048, SEEK_SET);
			else
				fseek(fp, 1024, SEEK_SET);

			if (fwrite(buff+1024, 1, ((u32) buff[3])*1024 ,fp) != ((u32) buff[3])*1024)
				goto err;
		}

		fclose(fp);
		return 1;
	} else
		return 0;
err:
	fclose(fp);
	return 0;
}

void dol_GetProfileDatas(u8 *id, u8 *buff)
{
	dol_infodat *dol_infop = (dol_infodat *) alt_dol_disc_tab;
	int indx = 0;

	for(indx=0; indx<1024; indx++) {
		if (!strncmp((void *) dol_infop[indx].id, (void *) id, 6)) {
			buff[0]='H'; 
			buff[1]='D'; 
			buff[2]='R';
			buff[3]=0;
			memcpy(&buff[4], &dol_infop[indx].flags[0], 8);
			return;				
		}
	}
	if (indx >= 1024) {
		buff[0]='H';
		buff[1]='D';
		buff[2]='R';
		buff[3]=0;
		memset(&buff[4], 0, 8);
	}
	
}

void dol_SetProfileDatas(u8 *id, u8 *buff)
{
	dol_infodat *dol_infop = (dol_infodat *) alt_dol_disc_tab;
	int indx = 0;
	int last_indx = 1023;
	make_rumble_off();

	for(indx = 0; indx<1024; indx++) {
		if (dol_infop[indx].id[0] == 0 && 
		    last_indx == 1023)
			last_indx = indx;

		if (!strncmp((void *) dol_infop[indx].id, (void *) id, 6)) {
			memcpy(&dol_infop[indx].flags[0], &buff[4], 8);
			save_cfg(1);
			return;
		}
	}
	if (indx >= 1024) {
		if (last_indx == 1023)
			memcpy((char *) &dol_infop[0], ((char *) &dol_infop[0])+32, 32768-32);
		indx = last_indx;

		memset((void *) &dol_infop[indx],0, 32);
		memcpy((void *) &dol_infop[indx].id, id, 6);
		memcpy(&dol_infop[indx].flags[0], &buff[4], 8);
		save_cfg(1);
	}
}

int load_cfg(int mode)
{
	FILE *fp = NULL;
	int n = 0;

	make_rumble_off();

	config_file.magic = 0;

	memset((void *) &cheat_file, 0, 32768);

	if(is_fat) {
		mode = 1;
	}

	if(mode == 0) {// from WBFS
		// Load CFG from HDD (or create __CFG_ disc)
		if (WBFS_LoadCfg(&config_file,sizeof (config_file), &cheat_file)) {
			if (config_file.magic!=0xcacafea1) {
				memset(&config_file,0, sizeof (config_file));
			} else {
				int n,m;

				// delete not found entries in disc
				for(m=0; m<16; m++) {
					for(n=0; n<gameCnt; n++) {
						struct discHdr *header = &gameList[n];
						if (strncmp((char *) header->id, 
							    (char *) &config_file.id[m][0],6) == 0)
							break;
					}
					if (n == gameCnt) 
						memset(&config_file.id[m][0],0,6);
				}
			}
			
			use_icon2 = config_file.icon & 7;
			return 0;
		}
	} else {
		if (sd_ok) { // lee el fichero de configuracion
			fp = fopen("sd:/apps/uloader/uloader.cfg","rb"); 
		}
		if (ud_ok && !fp) {// lee el fichero de configuracion
			fp=fopen("ud:/uloader.cfg","rb"); 
		}

		if (fp != 0) {
			memset((void *) alt_dol_disc_tab, 0, 32768);
			n = fread(&config_file,1, sizeof (config_file) ,fp);
			if (n!=sizeof (config_file) || 
			    !fp || 
			    (config_file.magic!=0xcacafea1 && config_file.magic!=0xcacafea2)) {
				memset(&config_file, 0, sizeof (config_file));
			}

			if(is_fat) {
				int n,m;

				// delete not found entries in disc
				for (m=0; m<16; m++) {
					for(n=0;n<gameCnt;n++) {
						struct discHdr *header = &gameList[n];
						if (strncmp((char *) header->id, 
							    (char *) &config_file.id[m][0],6) == 0)
							break;
					}
					if (n==gameCnt)
						memset(&config_file.id[m][0],0,6);
				}
			}
			if (config_file.magic == 0xcacafea2) {
				n = fread(&alt_dol_disc_tab,1, 32768 ,fp);
				if (n == 32768)
					n = fread(&cheat_file,1, 32768 ,fp);
			}
				
			fclose(fp);
			use_icon2 = config_file.icon & 7;
			return 0;
		}
	}
	return -1;
}

void save_cfg(int mode)
{
	FILE *fp=NULL;

	make_rumble_off();

	if(is_fat) {
		mode = 1;
	}

	config_file.icon= use_icon2 & 7;

	if (mode == 0) {
		config_file.magic = 0xcacafea1;

		if (WBFS_SaveCfg(&config_file,sizeof(config_file), &cheat_file)) {
			return;
		}
	} else {
		if (!sd_ok && !ud_ok)
			return;

		config_file.magic = 0xcacafea2;
		
		if (sd_ok) {// escribe el fichero de configuracion
			fp = fopen("sd:/apps/uloader/uloader.cfg","wb"); 
		}

		if (ud_ok && !fp) { // escribe el fichero de configuracion
			fp = fopen("ud:/uloader.cfg","wb");
		}
		
		if (fp != 0)
		{
			fwrite(&config_file,1, sizeof (config_file) ,fp);
			fwrite(&alt_dol_disc_tab,1, 32768 ,fp);
			fwrite(&cheat_file,1, 32768 ,fp);
			fclose(fp);
		}
	}

}

u32 sd_clusters  = 0;
u32 usb_clusters = 0;

int list_fat(char *device)
{
	DIR_ITER *fd;

	int n=0;
	int m=0;

	FILE *fp;
	u8 *p;

	int flag=0;
	int is_sd=0;
	int first=1;
	int i;

	static struct stat filestat;
	static char namefile[256*4]; // reserva espacio suficiente para utf-8
	char nand_path[256];

	char filename[256];

	if (!strncmp((void *) device, "sd", 2)) {
		if (!sd_ok)
			return 0;
		is_sd = 1;
        
	} else {
		if (!ud_ok)
			return 0;
		is_sd = 0;
	}

	
	p = malloc(32768);
	fd = diropen(device);

	n = num_fat_games;

	if (fd != NULL) {
		while (n < 256) {
			if (dirnext(fd, namefile, &filestat) != 0)
				break; /*dirreset(fd);*/ 
		
			if (!(filestat.st_mode & S_IFDIR)) {
				if (!(is_ext(namefile,".ciso") ))
					continue;
				
				sprintf(&fat_games[n][0], "%s%s", device, namefile);

				flag = 1;

				//////////////////////////////////////////////////
				
				fp = fopen(&fat_games[n][0], "rb");

				if(fp != 0) {
					if (fread(p,1, 4 ,fp) != 4)
						flag = 0;

					if (flag && 
					    !(p[0]=='C' && p[1]=='I' && p[2]=='S' && p[3]=='O')) // isn't CISO file
						flag=0; 
					
					if(flag) {
						fseek(fp, 0x8000, SEEK_SET);
						if (fread(p,1, 0x60 ,fp) != 0x60)
							flag = 0;
					}

					if (flag) {
						for (m = 0; m < n; m++) {
							if (!strncmp((char *) fat_disc[m].id, // test ID
								     (char *) p, 6))
								break; 
						}
						
						/*m=n;*/

						// ID
						memcpy((char *) fat_disc[m].id, (char *) p, 6); 

                                                // title
						memcpy((char *) fat_disc[m].title, (char *) p+32, 63);
						fat_disc[m].title[63] = 0;
						fat_disc[m].magic = filestat.st_size; // magic contain the file size 
						fat_disc[m].version = is_sd; // version contain flag for file in SD (used for display "SD" in the covers)
						
						if (n==m) 
							n++;
						else
							sprintf(&fat_games[m][0], "%s%s", device, namefile);

						if (first) {
							PARTITION *part;
							if (is_sd) {
								part =_FAT_partition_getPartitionFromPath("sd:");
								if (part)
									sd_clusters = part->sectorsPerCluster;
							} else {
								part =_FAT_partition_getPartitionFromPath("ud:");
								if (part) 
									usb_clusters = part->sectorsPerCluster;
							}
							first = 0;
						}
					}
					
				}
				fclose(fp);
				//////////////////////////////////////////////////
			}
		}
		dirclose(fd);
	}

	num_fat_games = n;

	sprintf(nand_path, "%c%c:/nand/title/00010001/", device[0], device[1]);
	
	fd = diropen(nand_path);

	if(fd != NULL) {
		//n=num_fat_games;
		while (n < 256) {
			if (dirnext(fd, namefile, &filestat) != 0)
				break; /*dirreset(fd);*/ 

			if(first) {
				PARTITION *part;
				if (is_sd) {
					part =_FAT_partition_getPartitionFromPath("sd:");
					if (part) 
						sd_clusters = part->sectorsPerCluster;
				} else {
					part =_FAT_partition_getPartitionFromPath("ud:");
					if (part)
						usb_clusters = part->sectorsPerCluster;
				}
				first = 0;
			}
		
			if ((filestat.st_mode & S_IFDIR)) {
				void *Tmd;
				int Tmd_len;
				
				if (!strncmp(namefile,"48",2) || 
				    !strncmp(namefile,"55",2)) 
					continue;

				sprintf(&fat_games[n][0], "%s%s/content/title.tmd", nand_path, namefile);

				
				//////////////////////////////////////////////////
			
				if (FAT_read_file(&fat_games[n][0], &Tmd, &Tmd_len) >= 0) {
					tmd *p_tmd = (tmd*)SIGNATURE_PAYLOAD((signed_blob *) Tmd);
					tmd_content *Content;

					Content= p_tmd->contents;

					memcpy((void *) fat_disc[n].id, ((char *) &p_tmd->title_id)+4, 4);
					memcpy((void *) &fat_disc[n].id[4], ((char *) &p_tmd->group_id), 2);

					for (m = 0; m < n; m++) {
						if (!strncmp((char *) fat_disc[m].id, 
							     (char *) fat_disc[n].id, 6)) 
							break; // test ID
					}
					strncpy((char *) fat_disc[m].title, (char *) namefile, 64);

					for (i = 0; i < p_tmd->num_contents; i++) {
						char appnamefile[256];
						static char temp_buf[84];

						if (Content[i].index == 0) {
							memcpy((char *) fat_disc[m].unused2, &Content[i].cid, 4);
							sprintf(appnamefile, 
								"%s%s/content/%08x.app", 
								nand_path, 
								namefile,  
								Content[i].cid);
							fp = fopen(appnamefile, "rb");
							if (fp) {
								fseek(fp, 0xF0, SEEK_SET);
								if (fread(temp_buf, 1, 84, fp) == 84) {
									for (i=0; i<42; i++) {
										fat_disc[m].title[i] = temp_buf[1+(i<<1)];
									}
								}
								fclose(fp);
							}
							break;
						}
					}
					
					sprintf(&fat_games[n][0], 
						"%s%s/content/#uloader.dat", nand_path, namefile);
					// create file if not exist
					fp = fopen(&fat_games[n][0], "rb");
					if (!fp) {
						fp = fopen(&fat_games[n][0], "wb");
						if (fp)
							fclose(fp); 
						fp = NULL;

						memset(disc_conf,0,256*1024);
						goto create_header;
					} else {
						int update_info;
						
						memset(disc_conf,0,256*1024);
						fread(disc_conf, 1, 16, fp);
						fclose(fp);
						
					create_header:;
						
						update_info = 0;

						if (!(disc_conf[0]=='H' && 
						      disc_conf[1]=='D' && 
						      disc_conf[2]=='R'))   {
							disc_conf[0]='H'; 
							disc_conf[1]='D'; 
							disc_conf[2]='R';
							disc_conf[3]=0;
							disc_conf[4] = disc_conf[5] =  disc_conf[6] =  disc_conf[7] = 0;
							disc_conf[8] = disc_conf[9] = disc_conf[10] = disc_conf[11] = 0;
							update_info = 1;
						}
#if 1
						if (!(disc_conf[9]=='P' && disc_conf[10]=='N' && disc_conf[11]=='G') &&
						    !(disc_conf[9]=='C' && disc_conf[10]=='1' &&disc_conf[11]=='6'))	{
							void *app;
							int size_app;
							void *tpl_1;	
							
							sprintf(filename, 
								"%s%s/content/%08x.app", 
								nand_path, 
								namefile, 
								*((u32 *)fat_disc[m].unused2));
								
							if (FAT_read_file(filename, &app, &size_app) == 0) {
								void parse_banner_tpl(void *banner, void *tpl_1);
								void * tpl_2_rgba(void *tpl);
								extern u16 tpl_w, tpl_h;

								parse_banner_tpl(app, &tpl_1);
								if(tpl_1) {
									void *text = tpl_2_rgba(tpl_1);
									free(tpl_1);
									if (text) {
										disc_conf[0] = 'H'; 
										disc_conf[1] = 'D'; 
										disc_conf[2] = 'R';
										disc_conf[3] = ((tpl_w * tpl_h *2+16+1023)/1024)-1;
										// fake PNG
										disc_conf[8]  = '\0';
										disc_conf[9]  = 'C';
										disc_conf[10] = '1';
										disc_conf[11] = '6';
										memcpy((void *)&disc_conf[12], &tpl_w, 2);
										memcpy((void *)&disc_conf[14], &tpl_h, 2);
										 
										memcpy((void *)&disc_conf[16], text, tpl_w * tpl_h * 2);
										update_info = 1;
										free(text);
									}
								}
								free(app);
							}

								
							if (update_info) {
								fp = fopen(&fat_games[n][0],"r+"); // lee el fichero de texto
								if (fp) {
									if (fwrite(disc_conf, 1, 1024 ,fp) != 1024)
										goto err;

									if (disc_conf[0]=='H' && 
									    disc_conf[1]=='D' && 
									    disc_conf[2]=='R' &&
									    disc_conf[3]!=0 && 
									    ((u32) disc_conf[3])<201) {
												
										fseek(fp, 1024, SEEK_SET);
										if (fwrite(disc_conf+1024, 1, ((u32) disc_conf[3])*1024 ,fp) != 
										    ((u32) disc_conf[3])*1024) 
											goto err;
									}
								err:
									fclose(fp);
								}
							}
							
							
						}
#endif
						
							
					}
					
					
					//memcpy((char *) fat_disc[m].title, (char *) p+32, 63);fat_disc[m].title[63]=0; // title
					fat_disc[m].magic = filestat.st_size; // magic contain the file size 
					fat_disc[m].version = 2 | is_sd; // version contain flag for file in SD and for VC
					fat_disc[m].unused1[0]=(((u8 *) Tmd)[0x18b]);
					if (n == m) 
						n++;
					//else sprintf(&fat_games[m][0], "%s%s", device, namefile);
					else 
						sprintf(&fat_games[n][0], "%s%s/content/#uloader.dat", nand_path, namefile);

					free(Tmd);	
				}

				//////////////////////////////////////////////////
			}
		}
		dirclose(fd);
	}
	
	free(p);
	
	num_fat_games = n;

	return num_fat_games;
}

/* ------------------------------------------------------------------------------- */

void update_bca(u8 *id, u8 *bca_datas)
{
	u8 *r = NULL;
	u8 *f;
	int len = 0;
	int mode;
	int pos = 0;
	int n, m;
	int finded = 0;

	FILE *fp;

	if (!sd_ok) 
		return;

	fp = fopen("sd:/bca_database.txt", "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		r = malloc(len+16);
		if (r)
			len = fread(r, 1, len, fp);
		fclose(fp);
		if (!r) 
			return;
	}
	
	f = malloc(9+136);
	if (!f) {
		if (r) 
			free(r);
		return;
	}

	fp = fopen("sd:/bca_database.txt", "wb");
	if(!fp) {
		free(f);
		free(r);
		return;
	}

	n = 0;
	mode = 0;

	if (r != NULL)
		while (n < len) {
			if (mode == 0) {
				if (r[n] != '[') {
					n++;
					continue;
				}
				if (r[n] == '[' && r[n+7] == ']' && r[n+8] < 32) {
					mode = 1;
					for (m = 0; m < 6; m++)
						if (r[n+m+1] != id[m]) 
							break;
					if (m == 6) 
						finded = 1;
			
					for (m = 0; m < 8; m++)
						f[m] = r[n+m];
					f[m] = '\n';

					n += 9;
				} else {
					n++;
					continue;
				}
			}

			if (mode==1) {
				int k = 9;

				if (r[n] < 32) {
					n++;
					continue;
				}

				for (m = 0; m < 64; m++) {
					u8 h = bca_datas[m] >> 4;

					if ((m & 7)==0 && m!=0) {
						if (m == 32) 
							f[k++]='\n'; 
						else 
							f[k++]=' ';
						//n++;
						while (r[n] <= 32) {
							n++;
							if (n >= len)
								break;
						}
					}

					if (n >= len) 
						break;
					if (r[n] >= 'a' && r[n] <= 'f')
						r[n] -= 32;
					if ((r[n] >= '0' && r[n] <= '9') || 
					    (r[n] >= 'A' && r[n] <= 'F'))   {
						if (finded==1) 
							f[k++] = (h)+48+7*(h >= 10);
						else 
							f[k++] = r[n];
	
						n++;
					} else 
						break; // error!

					h = bca_datas[m] & 15;
					if (r[n] >= 'a' && r[n] <= 'f') 
						r[n] -= 32;

					if ((r[n]>='0' && r[n]<='9') || 
					    (r[n]>='A' && r[n]<='F'))	  {
						if (finded==1) 
							f[k++]=(h)+48+7*(h>=10);
						else 
							f[k++]=r[n];
						n++;
					} else 
						break; // error!
				}
				if (m == 64) {
					f[k++] = '\n';
					n++;
					while (r[n]<=32) {
						n++;
						if (n >= len) 
							break;
					}
					pos = fwrite(f, 1, 9+136, fp);
					if (pos != (9+136))
						break;
				}
				mode=0; 
				if (finded == 1)
					finded=2;
			}
		}
	
	if (finded == 0) {
		int k = 0;
		f[k++] = '[';
		for (n = 0; n < 6; n++)
			f[k++] = id[n];
		f[k++] = ']';
		f[k++] = '\n';

		for (n = 0;n < 64; n++) {
			u8 h = bca_datas[n] >> 4;

			if ((n & 7)==0 && n!=0)  {
				if (n == 32) 
					f[k++]='\n';
				else 
					f[k++]=' ';
			}

			f[k++] = (h)+48+7*(h>=10);
			h      = bca_datas[n] & 15;
			f[k++] = (h)+48+7*(h>=10);
		
		}
		f[k++] = '\n';
		pos = fwrite(f, 1, 9+136, fp);
	}

	fclose(fp);

	if (r)
		free(r);
	
	free(f);

}

int get_bca(u8 *id, u8 *bca_datas)
{
	u8 *r = NULL;
	int len = 0;
	int mode;
	int n,m; 
	int finded=0;

	FILE *fp;

	memset(bca_datas, 0, 64);

	if (!sd_ok)
		return 0;

	fp = fopen("sd:/bca_database.txt", "rb");
	if(fp) {
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		r = malloc(len+16);
		if (r)
			len = fread(r, 1, len, fp);
		fclose(fp);
		if (!r) 
			return 0;
	}
	
	n = 0;
	mode = 0;

	if (r != NULL)
		while (n<len) {
			if (mode == 0) {
				if (r[n]!='[') {
					n++;
					continue;
				}
				if (r[n]=='[' && r[n+7]==']' && r[n+8]<32) {
					mode = 1;
					for(m = 0; m < 6; m++)
						if (r[n+m+1] != id[m]) 
							break;
					if (m == 6) 
						finded = 1;
			
					n += 9;
				} else {
					n++;
					continue;
				}
			}

			if (mode == 1) {
				if (r[n]<32) {
					n++;
					continue;
				}

				for (m = 0; m < 64; m++) {

					if ((m & 7)==0 && m!=0) {
						while (r[n]<=32) {
							n++;
							if (n >= len)
								break;
						}
					}

					if (n >= len)
						break;
					if (r[n] >= 'a' && r[n] <= 'f')
						r[n] -= 32;

					if ((r[n]>='0' && r[n]<='9') || 
					    (r[n]>='A' && r[n]<='F'))    {
						if (finded==1) 
							bca_datas[m] = (r[n] - 48 - 7 * (r[n] >= 'A')) << 4;
						n++;
					} else 
						break; // error!

					if (r[n] >= 'a' && r[n] <= 'f') 
						r[n] -= 32;

					if ((r[n] >= '0' && r[n] <= '9') || 
					    (r[n] >= 'A' && r[n] <= 'F'))   {
						if (finded==1) 
							bca_datas[m] |= r[n] -48 - 7*(r[n] >= 'A');
						n++;
					} else 
						break; // error!

				}

				if (m == 64) {
					n++;
					while (r[n] <= 32) {
						n++;
						if (n>=len)
							break;
					}
					if (finded == 1) {
						finded=2;
						break;
					}
				}
				mode = 0; 
				if (finded == 1)
					break;
			}
		}
	
	
	if (r)
		free(r);


	if (finded == 2)
		return 1; 
	else {
		memset(bca_datas,0,64);
		return 0;
	}
}
/* ------------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------------- */

int Get_AlternativeDol(u8 *id)
{
	dol_infodat *dol_infop = (dol_infodat *) (temp_data+128*1024);
	int indx = 0;

	dol_len  = 0;
	dol_data = 0;

	memset(temp_data+128*1024, 0, 32768);

	if (!load_alt_game_disc) {
		is_fat = 0;
		global_LoadDolInfo(temp_data + 128*1024);
	} else {
		dol_infop = (dol_infodat *) alt_dol_disc_tab;
	}

	AlternativeDol_infodat.id[0] = 0;

	for (indx = 0;indx < 1024; indx++) {
		if (!strncmp((void *) dol_infop[indx].id, (void *) id, 6) && 
		    dol_infop[indx].size!=0 && dol_infop[indx].offset != 0)    {
			AlternativeDol_infodat = dol_infop[indx];
			dol_len = AlternativeDol_infodat.size;
			str_trace = "Get_AlternativeDol alloc";
			dol_data = (u8 *) SYS_AllocArena2MemLo(dol_len + 32768, 32);
			if (!dol_data) { // cancel
				AlternativeDol_infodat.id[0] = 0;
			}
			str_trace = "";
			return 1;
		}
	}
	return 0;
}


void add_game_log(u8 *id)
{
	int n;
	time_t  my_time = (time(NULL));
	struct tm *l_time = localtime(&my_time);
	for (n = 0; n < 8; n++) {
		if (config_file.game_log[n].id[0] == 0)
			break;
		if (!strncmp((void *) config_file.game_log[n].id, (void *) id,6))
			break;
	}
	if (n == 8)
	{
		for (n = 0; n < 7; n++) 
			config_file.game_log[n] = config_file.game_log[n+1];
	}

	memcpy(config_file.game_log[n].id, id, 6);
	memset(config_file.game_log[n].bcd_time, 0, 6);

	if (l_time) {
		l_time->tm_mon++;
		l_time->tm_year += 1900;

		config_file.game_log[n].bcd_time[0] = ((l_time->tm_mday/10)<<4) | (l_time->tm_mday % 10);
		config_file.game_log[n].bcd_time[1] = ((l_time->tm_mon/10)<<4) | (l_time->tm_mon % 10);
		config_file.game_log[n].bcd_time[2] = ((l_time->tm_year/1000)<<4) | (l_time->tm_year/100 % 10);
		config_file.game_log[n].bcd_time[3] = ((l_time->tm_year/10 % 10)<<4) | (l_time->tm_year % 10);
		config_file.game_log[n].bcd_time[4] = ((l_time->tm_hour/10)<<4) | (l_time->tm_hour % 10);
		config_file.game_log[n].bcd_time[5] = ((l_time->tm_min/10)<<4) | (l_time->tm_min % 10);
	}


}
/* ------------------------------------------------------------------------------- */

struct _partition_type partition_type[32];

int num_partitions=0;

#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))

int get_partitions()
{
	u8 part_table[16*4];
	int ret;
	u8 *ptr;
	int i;
	int n_sectors;

	num_partitions = 0;
	memset(partition_type, 0, sizeof(partition_type));


	n_sectors = USBStorage2_GetCapacity(&sector_size);
	if (n_sectors < 0) 
		return -1;

	n_sectors += 1; 
    
	ret = USBStorage2_ReadSectors(0, 1, temp_data);
	if (ret < 0)
		return -2;
    
	///find wbfs partition
	if (temp_data[0x1fe]!=0x55 || temp_data[0x1ff]!=0xaa ||
	    !strncmp((void *) &temp_data[3],"NTFS",4) ||
	    !strncmp((void *) &temp_data[0x36],"FAT",3) ||
	    !strncmp((void *) &temp_data[0x52],"FAT",3))        {
//	if(temp_data[0x1bc]!=0 || temp_data[0x1bd]!=0 || temp_data[0x1fe]!=0x55 || temp_data[0x1ff]!=0xaa) 
		memset(part_table,0,16*4);
		partition_type[num_partitions].lba = 0;
		partition_type[num_partitions].len = n_sectors;
		if (temp_data[0] == 'W' && temp_data[1] == 'B' &&
		    temp_data[2] == 'F' && temp_data[3] == 'S')    
			partition_type[num_partitions].type|=(2<<8);
		else 
			partition_type[num_partitions].type=(4<<8);
		
		num_partitions++;
	} else {
		memcpy(part_table, temp_data + 0x1be, 16*4);

		ptr = part_table;

		for(i=0; i < 4; i++, ptr += 16) { 
			u32 part_lba = read_le32_unaligned(ptr+0x8);

			partition_type[num_partitions].len  = read_le32_unaligned(ptr+0xC);
			partition_type[num_partitions].type = (0<<8) | ptr[4];
			partition_type[num_partitions].lba  = part_lba;

			if (temp_data[0]=='W' && temp_data[1]=='B' &&
			    temp_data[2]=='F' && temp_data[3]=='S' && i==0) {
				partition_type[num_partitions].len  = n_sectors;
				partition_type[num_partitions].type = (2<<8);
				num_partitions++; 
				break;
			}

			if (ptr[4] == 0) {
				continue;
			}

			if (ptr[4] == 0xf) {
				u32 part_lba2 = part_lba;
				u32 next_lba2 = 0;
				int n;
				
				for (n = 0; n < 8; n++) { // max 8 logic partitions (i think it is sufficient!)
					ret = USBStorage2_ReadSectors(part_lba+next_lba2, 1, temp_data);
					if (ret < 0)
						return -2; 

					part_lba2 = part_lba+next_lba2+read_le32_unaligned(temp_data + 0x1C6);
					next_lba2 = read_le32_unaligned(temp_data + 0x1D6);

					partition_type[num_partitions].len  = read_le32_unaligned(temp_data + 0x1CA);
					partition_type[num_partitions].lba  = part_lba2;
					partition_type[num_partitions].type = (1<<8) | temp_data[0x1C2];

					ret = USBStorage2_ReadSectors(part_lba2, 1, temp_data);
					if (ret < 0)
						return -2;

					if (temp_data[0] == 'W' && temp_data[1] == 'B' && 
					    temp_data[2] == 'F' && temp_data[3] == 'S')
						partition_type[num_partitions].type = (3<<8) | temp_data[0x1C2];
					
					num_partitions++;
					if (next_lba2 == 0)
						break;
				}
			} else {
				ret = USBStorage2_ReadSectors(part_lba, 1, temp_data);
				
				if (ret < 0)
					return -2;

				if (temp_data[0] == 'W' && temp_data[1] == 'B' && 
				    temp_data[2] == 'F' && temp_data[3] == 'S')
					partition_type[num_partitions].type|=(2<<8);
				num_partitions++;
			}
		}
	}
	
	return 0;
}

/* ------------------------------------------------------------------------------- */

extern void *ulo_cfg;

u8 *search_for_ehcmodule_cfg(u8 *p, int size)
{
	int n;

	for (n = 0; n < size; n++) {
		if (!memcmp((void *) &p[n],"EHC_CFG",8) && 
		    p[n+ 8] == 0x12 && p[n+ 9] == 0x34 &&
		    p[n+10] == 0x00 && p[n+11] == 0x01)     {
			return &p[n];
		}
	}
	
	return NULL;
}

u8 *search_for_uloader_cfg(u8 *p, int size)
{
	int n;

	for (n = 0; n < size; n++) {
		if (!memcmp((void *) &p[n],"ULO_CFG",8) && 
		    p[n+ 8] == 0x12 && p[n+ 9] == 0x34 && 
		    p[n+10] == 0x00 && p[n+11] == 0x01)    {
			return &p[n];
		}
	}

	return NULL;
}

u8 *search_for_fatffs_module_cfg(u8 *p, int size)
{
	int n;

	for (n = 0;n < size; n++) {
		if (!memcmp((void *) &p[n],"FFS_CFG",8) && 
		    p[n+ 8] == 0x12 && p[n+ 9] == 0x34 && 
		    p[n+10] == 0x00 && p[n+11] == 0x01)     {
			return &p[n];
		}
	}

	return NULL;
}

/* ------------------------------------------------------------------------------- */
