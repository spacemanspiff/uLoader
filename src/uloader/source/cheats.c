
#include "cheats.h"

#define MAX_LIST_CHEATS 25

int txt_cheats=0;

int num_list_cheats=0;
int actual_cheat=0;

struct _data_cheats data_cheats[MAX_LIST_CHEATS];

_cheat_file cheat_file ATTRIBUTE_ALIGN(32);

static char buff_rcheat[260];

static int gettype_line(u8 *b, u8 *v, int *nv)
{

int n;

while(*b==32 || *b==10) b++;

if(*b==0) return 0; // linea separadora

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[n>>1]<<=4; v[n>>1]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return 1; // cadena
	}

b+=8;

*nv=1;
while(*b==32 || *b==10) b++;
if(*b==0) return 2; // numero

for(n=0;n<8;n++)
	{
	if((b[n]>='0' &&  b[n]<='9') || (b[n]>='A' &&  b[n]<='F') || (b[n]>='a' &&  b[n]<='f'))
		{
		v[4+(n>>1)]<<=4; v[4+(n>>1)]|=(b[n])-48-7*(b[n]>='A')-41*(b[n]>='a');
		}
	else return -1; // error en numero
	}

*nv=2;
return 2; // numero
}

void set_cheats_list(u8 *id)
{
int n,m;

int free=-1;

	if(!txt_cheats) return;

	for(n=0;n<1024;n++)
		{
		if(cheat_file.cheats[n].magic!=0xae && free<0) free=n;

		if(cheat_file.cheats[n].magic==0xae && !strncmp((char *) cheat_file.cheats[n].id, (char *) id, 6))
			{
			for(m=0;m<25;m++) cheat_file.cheats[n].sel[m]=(data_cheats[m].apply!=0);
			return;
			}
		}

	if(free<0)
		{
		free=1023;
		memcpy((char *) &cheat_file, ((char *) &cheat_file)+32, 32768-32);
		}

	memcpy((char *) cheat_file.cheats[free].id, (char *) id, 6);
	cheat_file.cheats[free].magic=0xae;
	for(m=0;m<25;m++) cheat_file.cheats[free].sel[m]=(data_cheats[m].apply!=0);
}

void get_cheats_list(u8 *id)
{
int n, m;

	if(!txt_cheats) return;

	for(n=0;n<1024;n++)
		{
		if(cheat_file.cheats[n].magic==0xae && !strncmp((char *) cheat_file.cheats[n].id, (char *) id, 6))
			{
			for(m=0;m<25;m++) data_cheats[m].apply= (cheat_file.cheats[n].sel[m]==1);
			return;
			}
		}

}


void create_cheats()
{
int n,m,f;

static u8 data_head[8] = {
	0x00, 0xD0, 0xC0, 0xDE, 0x00, 0xD0, 0xC0, 0xDE
};
static u8 data_end[8] = {
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

u8* temp_buff_cheats;
if(!txt_cheats) return;
	len_cheats=0;
if(!buff_cheats) return;

	len_cheats=0;
	
	temp_buff_cheats =  malloc (16384);
	if(temp_buff_cheats == 0){
		free(buff_cheats);buff_cheats=NULL;
		return;
	}

	f=0;
	memcpy(temp_buff_cheats,data_head,8);
	m=0;
	for(n=0;n<MAX_LIST_CHEATS;n++)
		{
		if(data_cheats[n].title && data_cheats[n].apply)
			{
			if(f==0) m+=8;f=1;
			memcpy(temp_buff_cheats+m,data_cheats[n].values,data_cheats[n].len_values);
			m+=data_cheats[n].len_values;
			}
		}
	if(f)
		{
		memcpy(temp_buff_cheats+m,data_end,8);m+=8;
		len_cheats=m;
		}

	if(buff_cheats) free(buff_cheats);
	buff_cheats=temp_buff_cheats;

}

int load_cheats(u8 *discid)
{
char file_cheats[]="sd:/codes/000000.txt";

u8 data_readed[8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

u8 sub_head[32]="";
u8 temp_sub_head[32]="";

FILE *fp;
int ret;
int n,m;
int mode=0;

if(!sd_ok && !ud_ok) return 0;

#ifdef ALTERNATIVE_VERSION
PauseOgg(1);usleep(200*1000);
#endif

actual_cheat=0;
num_list_cheats=0;
memcpy(&file_cheats[10],(char *) discid, 6);
len_cheats=0;

txt_cheats=1;
memset(data_cheats,0,sizeof(struct _data_cheats)*MAX_LIST_CHEATS);
fp=NULL;
if(sd_ok)
	{
	fp = fopen(file_cheats, "rb");
	if (!fp) 
		{
		txt_cheats=0;
		file_cheats[17]='g';file_cheats[18]='c';file_cheats[19]='t';
		fp = fopen(file_cheats, "rb");
		}
	}
if(!fp && ud_ok)
	{
	txt_cheats=1;
	file_cheats[0]='u';file_cheats[17]='t';file_cheats[18]='x';file_cheats[19]='t';
	fp = fopen(file_cheats, "rb");
	if (!fp) 
		{
		txt_cheats=0;
		file_cheats[17]='g';file_cheats[18]='c';file_cheats[19]='t';
		fp = fopen(file_cheats, "rb");
		}
	}


	if (!fp) {
		#ifdef ALTERNATIVE_VERSION
		PauseOgg(0);
		#endif
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len_cheats = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	buff_cheats =  malloc (16384);
	if(buff_cheats == 0){
		fclose(fp);
		len_cheats=0;
		#ifdef ALTERNATIVE_VERSION
		PauseOgg(0);
		#endif
		return 0;
	}

	if(txt_cheats)
		{
		
		n=m=0;
		mode=0;

		while(1)
			{
			int force_exit=0;
		
			if(fgets(buff_rcheat,256,fp)) 
				{
				int g=0,ret, ndatas=0;
				while(buff_rcheat[g]!=0 && buff_rcheat[g]!=10 && buff_rcheat[g]!=13) g++;
				buff_rcheat[g]=0;

				ret=gettype_line((u8 *) buff_rcheat, data_readed, &ndatas);

				switch(mode)
					{
					case 0: // get ID
						if(ret==1)
							{
							if(strncmp((char *) discid, (char *) buff_rcheat,6)!=0) force_exit=1; // error! código no coincide
							mode++;
							}
						break;

					case 1: // get name
						if(ret==0)
							mode++;
						break;

					case 2: // get entry name
						if(ret!=0)
							{
							int sub;
							if(ret!=1) force_exit=1;

							memcpy(temp_sub_head, buff_rcheat, (g>31) ? 31 : g);temp_sub_head[(g>31) ? 31 : g]=0;

							sub=strlen((char *) sub_head);

							if(g>(63-sub)) 
								{
								if(g>39) 
									g=39;
								if((63-g)<sub) sub=63-g;
								}

							memset(buff_cheats+m,32,63); // rellena de espacios
							buff_cheats[m+63]=0;
							
							
                            memcpy(buff_cheats+m+(63-(g+sub))/2,sub_head,sub);// centra nombre
							if(sub>0) buff_cheats[m+(63-(g+sub))/2+sub-1]='>';
							memcpy(buff_cheats+m+(63-(g+sub))/2+sub,buff_rcheat,g);// centra nombre
							g=63;

							data_cheats[n].title=buff_cheats+m;
							m+=g+1;
							data_cheats[n].values=buff_cheats+m;
							data_cheats[n].len_values=0;
							data_cheats[n].description=NULL;
							mode++;
							}
						break;
					case 3: // get entry codes
					case 4:
					    if(ret==0 || ret==1) 
							{
						    if(mode==4) 
								{	
								if(ret==1)
									{
									memcpy(buff_cheats+m,buff_rcheat,g+1);
									data_cheats[n].description=buff_cheats+m;
									m+=g+1;
									}
								n++; 
								}
							else 
								{
								int sub;
								if(data_cheats[n].title)
								  {memcpy(sub_head,temp_sub_head,31);sub_head[31]=0;
								   sub=strlen((char *) sub_head); if(sub<31) {sub_head[sub]='>';sub_head[sub+1]=0;}
								   data_cheats[n].title=NULL;}
							    }
							mode=2;if(n>=MAX_LIST_CHEATS) force_exit=1;
							}
						else
						if(ret==2)
							{
							memcpy(buff_cheats+m,data_readed,ndatas*4);
							data_cheats[n].len_values+=ndatas*4;
							m+=ndatas*4;
							mode=4;
							}
						else
						if(ret<0) {data_cheats[n].title=NULL;data_cheats[n].len_values=0;force_exit=1;}
						break;
					
					}
				}
			 else {
				  if(mode==4) n++; else data_cheats[n].title=NULL;n++;
				  break;
				  }
			
			if(force_exit) break;
			}
		fclose(fp);

		for(n=0;n<MAX_LIST_CHEATS;n++)
			if(data_cheats[n].title!=NULL) num_list_cheats=n+1; else break;

		get_cheats_list(discid);
		
		}
	else
		{
		ret = fread(buff_cheats, 1, len_cheats, fp);
		fclose(fp);

		if(ret != len_cheats){
			len_cheats=0;
			free(buff_cheats);buff_cheats=NULL;
			#ifdef ALTERNATIVE_VERSION
			PauseOgg(0);
			#endif
			return 0;
			}
		}
		
	#ifdef ALTERNATIVE_VERSION
	PauseOgg(0);
	#endif
return 1;
}

