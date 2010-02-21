#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <ogcsys.h>
#include <gccore.h>
#include "ogc/lwp_threads.h"
#include <wiiuse/wpad.h>
#include <fat.h>
#include <fcntl.h>
#include "sys/dir.h"
#include <sys/statvfs.h>
#include "unistd.h"
#include <ogc/conf.h>


#include "files.h"

char path_file[258]="sd:/";
int nfiles=0;
int ndirs=0;

struct _files files[MAX_ENTRY], swap_file;


int is_ext(char *a,char *b) // compara extensiones
{
int n,m;

	m=0;n=0;
	while(a[n]!=0) {if(a[n]=='.') m=n; n++;}

	n=0;
	while(b[n]!=0) {
		           unsigned char t1,t2;
                   t1=a[m];t2=b[n];
				   if(t1>='A' && t1<='Z') t1+=32;
				   if(t2>='A' && t2<='Z') t2+=32;
				   if(t1!=t2) return 0; m++; n++;
				   }

	if(a[m]==0) return 1;

return 0;
}

char * get_name_short(char *name) // devuelve el nombre del fichero recortado a 36 caracteres
{
#define MAX_STR 36
#define BRK_STR 17

static unsigned char name2[MAX_STR+4];

int n,m,l;


m=0;n=-1;
while(name[m]!=0) {if(name[m]=='/') n=m;m++;} // elimina la parte de directorio
n++;

l=n;
while(name[l]!=0) l++;

l-=n;
if(l>=MAX_STR)
	{
	memcpy(name2,&name[n],BRK_STR);
	name2[BRK_STR]='.';
	name2[BRK_STR+1]='.';
	n+=(l-MAX_STR)+(BRK_STR+2);

	for(m=BRK_STR+2;m<MAX_STR;m++)
		{
		name2[m]=name[n];
		if(name[n]==0) break;
		n++;
		}
	}
else
	{
	for(m=0;m<MAX_STR;m++)
		{
		name2[m]=name[n];
		if(name[n]==0) break;
		n++;
		}
	}
	name2[m]=0;

#undef MAX_STR
#undef BRK_STR

return ((char *)name2);
}

char * get_name_short_fromUTF8(char *name) // devuelve el nombre del fichero recortado a 36 caracteres
{
#define MAX_STR 36
#define BRK_STR 17

static unsigned char name2[MAX_STR+4];

int n,m,l,s;


m=0;n=-1;
while(name[m]!=0) {if(name[m]=='/') n=m;m++;} // elimina la parte de directorio
n++;

l=n;
while(name[l]!=0) l++;

l-=n;
if(l>=MAX_STR)
	{
	s=n;
    for(m=0;m<BRK_STR;m++)
		{
		if((name[s] & 0xc0)==0xc0 && (name[s+1] & 0xc0)==0x80)
			{
			name2[m]= (((name[s] & 3)<<6) | (name[s+1] & 63));
			s++;
			}
		else name2[m]=name[s];
		s++;
		}
	
	//memcpy(name2,&name[n],BRK_STR);
	name2[m]='.';
	name2[m+1]='.';
	m+=2;
	n+=(l-MAX_STR)+(m);

	for(;m<MAX_STR;m++)
		{
		if((name[n] & 0xc0)==0xc0 && (name[n+1] & 0xc0)==0x80)
			{
			name2[m]= (((name[n] & 3)<<6) | (name[n+1] & 63));
			n++;
			}
		else name2[m]=name[n];

		if(name[n]==0) break;
		n++;
		}
	}
else
	{
	for(m=0;m<MAX_STR;m++)
		{
		if((name[n] & 0xc0)==0xc0 && (name[n+1] & 0xc0)==0x80)
			{
			name2[m]= (((name[n] & 3)<<6) | (name[n+1] & 63));
			n++;
			}
		else name2[m]=name[n];
		if(name[n]==0) break;
		n++;
		}
	}
	name2[m]=0;

#undef MAX_STR
#undef BRK_STR

return ((char *)name2);
}

void char_to_utf8(char *s, char *d)
{
u8 *c= (u8 *) s;

while(*c!=0)
	{
	if((*c & 0xc0)==0xc0 && (*(c+1) & 0xc0)==0x80) // esto es UTF-8!
		{
		*d++=*c++;
		*d++=*c;
		}
	else
	if(*c>=0x80) // convierte a UTF-8
		{
		*d++= 192 | (*c>>6);
		*d++= 128 | (*c & 63);
		}
	else *d++=*c;

	c++;
	}
*d=0;
}

void utf8_to_char(char *s, char *d)
{
u8 *c= (u8 *) s;

while(*c!=0)
	{
	
	if((*c & 0xc0)==0xc0 && (*(c+1) & 0xc0)==0x80)
		{
		*d++= (((*c & 3)<<6) | (*(c+1) & 63));
		c++;
		}
	else *d++=*c;

	c++;
	}
*d=0;
}

char * get_name(char *name) // devuelve el nombre del fichero completo
{
static unsigned char name2[256];

int n,m;

	m=0;n=-1;

	while(name[m]!=0) {if(name[m]=='/') n=m;m++;}
	n++;

	for(m=0;m<255;m++)
		{
		name2[m]=name[n];
		if(name[n]==0) break;
		n++;
		}

	name2[m]=0;

return ((char *) name2);
}

char * get_name_from_UTF8(char *name) // devuelve el nombre del fichero completo
{
static unsigned char name2[256];

int n,m;

	m=0;n=-1;

	while(name[m]!=0) {if(name[m]=='/') n=m;m++;}
	n++;

	for(m=0;m<255;m++)
		{
		if((name[n] & 0xc0)==0xc0 && (name[n+1] & 0xc0)==0x80)
			{
			name2[m]= (((name[n] & 3)<<6) | (name[n+1] & 63));
			n++;
			}
		else
			name2[m]=name[n];

		if(name[n]==0) break;
		n++;
		}

	name2[m]=0;

return ((char *) name2);
}

int have_device=1;

int current_device=0;


void read_list_file(unsigned char *dirname, int flag)
{
DIR_ITER *fd;

static char namefile[256*4]; // reserva espacio suficiente para utf-8

static struct stat filestat;


static char swap[257];

int n,m,f;

if(!dirname) sprintf(swap, "%s", path_file);
else
	{
    sprintf(path_file, "%s", dirname);

	n=0;while(dirname[n]!=0) n++;
	if(dirname[n-1]=='/') // esto solo pasa si es directorio raiz
		{sprintf(swap, "%s", dirname);dirname=0;}
	else  
		{
		sprintf(swap, "%s/", dirname);
		if(swap[n-1]=='.') 
			{n--;while(swap[n]!='/') n--;n--;while(swap[n]!='/') n--; if(swap[n-1]==':') dirname=0; n++;swap[n]=0;} // para volver al directorio anterior
		}
	}

ndirs=0;
nfiles = 0;
files[nfiles].is_selected=0;
if(!dirname)
	{
	
	if(current_device==0 && (have_device & 2)!=0)
		{
		sprintf(files[nfiles].name, ".[USB]");
		ndirs++;files[nfiles].is_directory=1;
		files[nfiles].is_selected=0;
		nfiles++;
		}
	if(current_device==1 && (have_device & 1)!=0)
		{
		sprintf(files[nfiles].name, ".[SD]");
		ndirs++;files[nfiles].is_directory=1;
		files[nfiles].is_selected=0;
		nfiles++;
		}
	}

	fd = diropen(swap);

	if(fd != NULL) 
		{

		while(nfiles < MAX_ENTRY)
			{
			files[nfiles].is_directory=0;
			files[nfiles].is_selected=0;

			if(dirnext(fd, namefile, &filestat)!=0) break;

			if(namefile[0]=='.' && namefile[1]==0) continue;
		   
			sprintf(files[nfiles].name, "%s%s", swap, namefile);
		
			if((filestat.st_mode & S_IFDIR)) 
				{
				files[nfiles].size=0;
				ndirs++;files[nfiles].is_directory=1; 
				}
			else 
				{
				/*if(flag==0)
					{
					if(!(is_ext(files[nfiles].name,".mp3") || is_ext(files[nfiles].name,".ogg") || is_ext(files[nfiles].name,".m3u") )) continue;
					if(!is_ext(files[nfiles].name,".m3u")) test_music_file(nfiles, 0);
					}

				if(flag==1)
					if(!(is_ext(files[nfiles].name,".jpg") || is_ext(files[nfiles].name,".jpeg"))) continue;
                 */
				if(flag==0)
					if(!(is_ext(files[nfiles].name,".png") )) continue;

				files[nfiles].size=filestat.st_size;
				}
			
			nfiles++;
			}
	
	dirclose(fd);	
	}
	
  if(!nfiles)
		{
		
		if(current_device==0 && (have_device & 2)!=0)
			{
			sprintf(files[nfiles].name, ".[USB]");
			ndirs++;files[nfiles].is_directory=1;
			files[nfiles].is_selected=0;
			nfiles++;
			}
		if(current_device==1 && (have_device & 1)!=0)
			{
			sprintf(files[nfiles].name, ".[SD]");
			ndirs++;files[nfiles].is_directory=1;
			files[nfiles].is_selected=0;
			nfiles++;
			}
			
		}

	for(n=0;n<(nfiles);n++) //ordena
	for(m=n+1;m<nfiles;m++)
		{
		f=0;
	 		
		if(files[m].is_directory==1 && files[n].is_directory==0) f=1;
		if(f==0) 
			if(files[m].is_directory==files[n].is_directory) 
				if(strcmp(files[m].name,files[n].name)<0) f=1;
		if(f)
			{
			f=0;
		    swap_file=files[m];
			files[m]=files[n];
			files[n]=swap_file;
			}
		}

} 


