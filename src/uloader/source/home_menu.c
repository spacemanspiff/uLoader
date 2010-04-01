
#include "gfx.h"
#include "global.h"
#include "http.h"
#include "ehcmodule.h"

extern int frames2;

extern int abort_signal;

u32 sector_size;

char month[2][12][4]=
	{
	{"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"},
	{"ENE", "FEB", "MAR", "ABR", "MAY", "JUN", "JUL", "AGO", "SEP", "OCT", "NOV", "DIC"}
	
	};

void select_file(int flag, struct discHdr *header);
int menu_format();
void add_game();
int delete_test(int ind, char *name);
int rename_game(char *old_title);
void set_parental_control();
int uloader_update();
int uloader_hacks(void);

int call_home_menu(struct discHdr *header, int function)
{

	// add png
	if(function==0) {snd_fx_yes();draw_snow();Screen_flip(); in_black|=2;select_file(0,header); in_black^=2;return 3;}

	// delete png
	if(function==1) {snd_fx_yes();draw_snow();Screen_flip(); 
						   if(delete_test(25, header->title))
							   {
							   memset(disc_conf,0,256*1024);global_GetProfileDatas(header->id, disc_conf);
							   disc_conf[0]='H'; disc_conf[1]='D'; disc_conf[2]='R';disc_conf[3]=0;disc_conf[9]=0; // break PNG signature
							   global_SetProfileDatas(header->id, disc_conf);
							   } 
						   return 3;
						   }
	// rename
	if(function==2) {snd_fx_yes();draw_snow();Screen_flip(); if(rename_game(header->title))  {global_RenameGame(header->id, header->title);WBFS_Close();return 2;} else return 0;}
	// delete game
	if(function==3) {snd_fx_yes();draw_snow();Screen_flip(); if(delete_test(29, header->title)){ WBFS_RemoveGame(header->id);WBFS_Close();return 2;} else return 0;}

return -1;
}

/*********************************************************************************************************************************************************/
// home_menu
/*********************************************************************************************************************************************************/
int home_menu(struct discHdr *header)
{

int n;
int old_select=-1;
char *punt;
char buff[64];

int index=0;

	while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=8; color= INK0; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}

		letter_size(16,32);
		

    for(n=0;n<6;n++)
		{
		int m=n+index;

		memset(buff,32,56);buff[56]=0;
		if(m>11)
			{
			Draw_button2(30+48, ylev+32+64*n, buff, 0);
			continue;
			}
		
		punt=&letrero[idioma][22+n+index][0];
		memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));

		if(Draw_button2(30+48, ylev+32+64*n, buff,
			(
			(!header && !mode_disc && (m==2 || m==3 || m==6 || m==7 || m==9 ))  
			|| (parental_control_on && m!=0 && m!=5)
			|| (mode_disc && (m==1 || m==2 || m==3 ||  m==6 || m==7 || m==8)) 
			|| (!sd_ok  && m==10)
			|| (is_fat && (m==1 ||  m==7 || (m==8 && dvd_only)))
			|| (is_fat && header && (header->version & 2) && (m==6 || m==9))
		 ) ? -1 : 0))
			     select_game_bar=m+1;
	//
		}
		int z=-is_16_9*80-10 * (!is_16_9); 
		if(px>=z && px<=60+z && py>=ylev+220-40 && py<=ylev+220+40)
			{
			circle_select(40+z, ylev+220, '-', 1);
			select_game_bar=50;
			}
		else
		if(frames2 & 32)
			{
			circle_select(40+z, ylev+220, '-', 0);
			}
		

		z=is_16_9*80+10 * (!is_16_9);
		if(px>=SCR_WIDTH-82+z && px<=SCR_WIDTH-2+z && py>=ylev+220-40 && py<=ylev+220+40)
			{
			circle_select(SCR_WIDTH-42+z, ylev+220, '+', 1);
			select_game_bar=51;
			}
		else
		if(frames2 & 32)
			{
			circle_select(SCR_WIDTH-42+z, ylev+220, '+', 0);
			}

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;


	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

				
				if(new_pad & (WPAD_BUTTON_B | WPAD_BUTTON_HOME))
					{
					snd_fx_no();draw_snow();Screen_flip();
					return 0;
					}

                if(new_pad & WPAD_BUTTON_MINUS)
					{
						index-=6;if(index<0) index=6;snd_fx_tick();
					}
				
				if(new_pad & WPAD_BUTTON_PLUS)
					{
						index+=6;if(index>6) index=0;snd_fx_tick();
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==50) {index-=6;if(index<0) index=6;snd_fx_yes();}
					if(select_game_bar==51) {index+=6;if(index>6) index=0;snd_fx_yes();}
					
					// return
					if(select_game_bar==1) {snd_fx_yes();draw_snow();Screen_flip(); return 0;}

					// add game
					if(select_game_bar==2) {snd_fx_yes();draw_snow();Screen_flip(); add_game(); return 2;}
				
					if(header)
						{
						// add png
						if(select_game_bar==3) {snd_fx_yes();draw_snow();Screen_flip(); in_black|=2;select_file(0,header); in_black^=2;return 3;}

						// delete png
                        if(select_game_bar==4) {snd_fx_yes();draw_snow();Screen_flip(); 
						                       if(delete_test(25, header->title))
												   {
												   memset(disc_conf,0,256*1024);global_GetProfileDatas(header->id, disc_conf);
												   disc_conf[0]='H'; disc_conf[1]='D'; disc_conf[2]='R';disc_conf[3]=0;disc_conf[9]=0; // break PNG signature
											       global_SetProfileDatas(header->id, disc_conf);
												   } 
											   return 3;
											   }
						// rename
						if(select_game_bar==7) {snd_fx_yes();draw_snow();Screen_flip(); if(rename_game(header->title))  {global_RenameGame(header->id, header->title);WBFS_Close();return 2;} else return 0;}
						// delete game
						if(select_game_bar==8) {snd_fx_yes();draw_snow();Screen_flip(); if(delete_test(29, header->title)) WBFS_RemoveGame(header->id);WBFS_Close();return 2;}
						// alternative dol
						if(select_game_bar==10) {snd_fx_yes();draw_snow();Screen_flip();load_alt_game_disc=(mode_disc & 3)!=0; menu_alternativedol(header->id);return 0;}
						}
					
					// parental control
					if(select_game_bar==5) {snd_fx_yes();draw_snow();Screen_flip();in_black|=2;load_alt_game_disc=(mode_disc & 3)!=0;set_parental_control();in_black^=2;draw_snow();Screen_flip();return 0;}
					
					// menu wii
					if(select_game_bar==6) {snd_fx_yes();draw_snow();Screen_flip(); return 1;}
					
					

					// format
					if(select_game_bar==9) {snd_fx_yes();WBFS_Close();draw_snow();Screen_flip(); if(menu_format()==0) {WBFS_Open();return 2;} else return 1;}

					// update
					if(select_game_bar==11) {snd_fx_yes();draw_snow();Screen_flip(); if(uloader_update()) return 1; else  return 0;}

					// uLoader Hacks
					if(select_game_bar==12) {snd_fx_yes();draw_snow();Screen_flip(); if(uloader_hacks()) return 1; else  return 0;}

					
					}
			}
	
	frames2++;step_button++;
	draw_snow();
	Screen_flip();
	if(exit_by_reset)  
		{	
		break;
		}
	}

return 0;
}


int menu_format()
{

int n,m;
int ret;
char type[5][9]={"Primary ", "Extended", "WBFS Pri", "WBFS Ext","Unknown "};

int current_partition=0;

int level=0;
//int it_have_wbfs=0;

int last_select=-1;

frames2=0;
re_init:

autocenter=0;
ret=get_partitions();
current_partition=0;
level=0;

if(ret==-1 /*|| num_partitions<=0*/) return -1;



while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	draw_background();
		

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	SelectFontTexture(1); // selecciona la fuente de letra extra
    
	
	PX= 0; PY=ylev-32; color= INK0; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][13][0]);
	bkcolor=0;
	autocenter=0;
	

	color= INK0; 
	letter_size(8,24);
	SetTexture(NULL);

	//it_have_wbfs=0;

	/*for(n=0;n<num_partitions;n++)
		{
		if(type[partition_type[n].type>>8][0]>1) {it_have_wbfs=1;break;}
		}*/

	if(level>=100)
		{
		bkcolor=0;
		
		
		if(level>=1000)
			{
			PX= 0; PY=ylev+(352)/2-16;
			letter_size(16,32);
			autocenter=1;
			if(level>=2048+128)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff00ff00);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, INK0);

				s_printf("%s", &letrero[idioma][20][0]);
				}
			else
			if(level>=2048)
				{
				DrawRoundFillBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 0xff0000ff);
				DrawRoundBox((SCR_WIDTH-500)>>1, PY-16, 500, 64, 0, 4, INK0);

				s_printf("%s", &letrero[idioma][21][0]);
				}
			else
				s_printf("%s", &letrero[idioma][19][0]);

			autocenter=0;
			}
		else
			{
			PX= 0; PY=ylev+32;
			letter_size(16,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][14][0]);
			letter_size(8,32);
			PX= ((SCR_WIDTH-54*8)>>1)-28; PY=ylev+80+32;
			m=level-100;autocenter=0;

			DrawRoundFillBox(PX-32, PY-12, 60*8+64, 56, 0, 0xcf0000ff);
							
			DrawRoundBox(PX-32, PY-12, 60*8+64, 56, 0, 4, INK0);
			
			color= INK0; 
			/*s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));*/
			if((partition_type[m].type>>8)==4)
					{
					s_printf("Unpartitioned Disc (%s) LBA: %.10u LEN: %.2fGB", &type[partition_type[m].type>>8][0],
						partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				else
					{
					s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
			autocenter=1;
			PX= 0; PY=ylev+180;
			letter_size(12,32);
			s_printf("%s", &letrero[idioma][16][0]);
		
			PX= 0; PY=ylev+180+48;
			/*if(it_have_wbfs)
				s_printf("%s", &letrero[idioma][15][0]);*/
			autocenter=0;

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
			}
		}

    if(level==0)
		{
		for(n=0;n<6;n++)
			{
			PX=((SCR_WIDTH-54*8)>>1)-28; PY=ylev+32+n*50;
			m=(current_partition+n);
			if(m>=num_partitions)
				{
				DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xcf808080);
				DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
				if(num_partitions<1 && n==0)
					{
					s_printf("No Partitions in Disc Detected");
					}
				}
			else
				{
				if(px>=PX-32 && px<=PX+60*8+64 && py>=PY-8 && py<PY+32) 
					{
					if((partition_type[m].type>>8)>1 && (partition_type[m].type>>8)!=4) DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-40, PY-12, 60*8+80, 48, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-40, PY-12, 60*8+80, 48, 0, 5, 0xfff08000);select_game_bar=100+m;
					}
				else 
					{
					if((partition_type[m].type>>8)>1)  DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xff00cf00);
					else DrawRoundFillBox(PX-32, PY-8, 60*8+64, 40, 0, 0xffcfcfcf);
					
					DrawRoundBox(PX-32, PY-8, 60*8+64, 40, 0, 4, 0xff606060);
					}
				if((partition_type[m].type>>8)==4)
					{
					s_printf("Unpartitioned Disc (%s) LBA: %.10u LEN: %.2fGB", &type[partition_type[m].type>>8][0],
						partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				else
					{
					s_printf("Partition %2.2i -> %2.2xh (%s) LBA: %.10u LEN: %.2fGB", m+1, partition_type[m].type & 255, &type[partition_type[m].type>>8][0],
					partition_type[m].lba, ((float)partition_type[m].len* (float)sector_size)/(1024*1024*1024.0));
					}
				}
			}
		
	
			SetTexture(NULL);
		
			if(current_partition>=6)
				{
				int z=-is_16_9*80-10 * (!is_16_9); 
				if(px>=z && px<=z+60 && py>=ylev+220-40 && py<=ylev+220+40)
					{
					circle_select(40+z, ylev+220, '-', 1);
					select_game_bar=50;
					}
				else
				if(frames2 & 32)
					{
					circle_select(40+z, ylev+220, '-', 0);
					}
				}

				if((current_partition+6)<num_partitions)
				{
				int z=is_16_9*80+10 * (!is_16_9);
				if(px>=SCR_WIDTH-82+z && px<=SCR_WIDTH-2+z && py>=ylev+220-40 && py<=ylev+220+40)
					{
					circle_select(SCR_WIDTH-42+z, ylev+220, '+', 1);
					
					select_game_bar=51;
					}
				else
				if(frames2 & 32)
					{
					circle_select(SCR_WIDTH-42+z, ylev+220, '+', 0);
					}
				}

			if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
			
			if(Draw_button(600-32-strlen(&letrero[idioma][0][0])*8, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=1;
		} // level==0

	frames2++;step_button++;

	if(select_game_bar>=0)
		{
		if(select_game_bar!=last_select)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			last_select=select_game_bar;
			}
		}
	else last_select=-1;
	

	wiimote_read();

	if(level<1000)
		{

		if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

				

				if(new_pad & WPAD_BUTTON_B)
					{
					snd_fx_no();
					if(level==0) break;
					else level=0;
					}

				if((new_pad &  WPAD_BUTTON_MINUS) && level==0)
					{
					current_partition-=6;if(current_partition<0) current_partition=0;
					}
				
				if((new_pad &  WPAD_BUTTON_PLUS) && level==0)
					{
					current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==1) {snd_fx_yes();break;}
					if(select_game_bar==50) {snd_fx_yes();current_partition-=6;if(current_partition<0) current_partition=0;} //page -6
					if(select_game_bar==51) {snd_fx_yes();current_partition+=6;if(current_partition>(num_partitions+6)) current_partition-=6;} // page +6

					if(select_game_bar>=100 && select_game_bar<150) // select partition to format
						{
						level=select_game_bar;snd_fx_yes();
						}
					
					if(select_game_bar==60) {level+=900;snd_fx_yes();} // Yes
					if(select_game_bar==61) {snd_fx_no();level=0;} // No

                   
					}
				}

		}
	
	draw_snow();
	Screen_flip();
	
	if(level>=1000)
		{
		if(level<2000)
			{
			if(WBFS_Format(partition_type[level-1000].lba, partition_type[level-1000].len)<0) level=2048; // error
			else level=2048+128; // ok
			}

        level++;
		if((level & 127)>120) 
			{	 
			if(level>=2048+128) break;
			level=0;goto re_init;
			}
		}


	if(exit_by_reset) break;
	}

return 0;
}

void add_game()
{

int ret;
int n;
static struct discHdr header ATTRIBUTE_ALIGN(32);
static f32 used,free;

char str_id[7];	
//ASND_Pause(1);

    
	WDVD_SetUSBMode(NULL, 0);

	WBFS_Init();

	{
	int ylev=(SCR_HEIGHT-440);


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;
	
	
	while(1)
		{
		if(exit_by_reset)
			{
			autocenter=0;
			goto out;
			}

		cabecera( &letrero[idioma][23][0]);
        color=INK0;
	 
		PX=0;PY=ylev+352/2-16;
		s_printf("%s",&letrero[idioma][38][0]);
		PX=0;PY+=64;
		s_printf("%s",&letrero[idioma][40][0]);

		if(rumble) 
			{
			if(rumble<=7) wiimote_rumble(0); 
			rumble--;
			}
		else wiimote_rumble(0);
	
		WPAD_ScanPads(); // esto lee todos los wiimotes
		wiimote_read();
		
		draw_snow();
		Screen_flip();

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				PX=0;PY=ylev+352/2-16;
				color=INK0;

				s_printf("%s",&letrero[idioma][39][0]);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			//sleep(4);
			goto out;
			}
	  
		ret = Disc_Wait();
		if(ret==0) break;
		if (ret < 0) 
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				color=INK0;
				PX=0;PY=ylev+352/2-16;
				s_printf("ERROR! (ret = %d)", ret);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			goto out;
			}
		}

	/*cabecera( &letrero[idioma][23][0]);
	PX=0;PY=ylev+352/2-16;

	s_printf("%s",&letrero[idioma][41][0]);
	Screen_flip();	*/

	remote_call(draw_add_game_mess);

		/* Open disc */
	ret = Disc_Open();
	if (ret < 0) 
		{
		
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			color=INK0;
			PX=0;PY=ylev+352/2-16;
			s_printf("ERROR! (ret = %d)", ret);
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			if(n==0) snd_fx_no();
			autocenter=0;
			}
		//sleep(4);
		goto out;
		}
  
	/* Check disc */
	ret = Disc_IsWii();
	if (ret < 0) 
		{
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera(&letrero[idioma][23][0]);
			color=INK0;
			PX=0;PY=ylev+352/2-16;
			s_printf("%s",&letrero[idioma][42][0]);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			autocenter=0;
			}
		//sleep(4);
		goto out;
	
	}


	/* Read header */
	Disc_ReadHeader(&header);

	
	memcpy(str_id,header.id,6); str_id[6]=0;
	
	/* Check if game is already installed */
	ret = WBFS_CheckGame((u8*) str_id);
	if (ret) 
		{
		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			color=INK0;
			PX=0;PY=ylev+352/2-16;
			
			s_printf("%s",&letrero[idioma][43][0]);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			wiimote_rumble(0);
			WPAD_ScanPads();
			autocenter=0;
			}
		//sleep(4);
		goto out;

	}

/////////////////////

    WBFS_DiskSpace(&used, &free);
	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

    while(1)
		{
		if(rumble) 
			{
			if(rumble<=7) wiimote_rumble(0); 
			rumble--;
			}
		else wiimote_rumble(0);
	
		WPAD_ScanPads(); // esto lee todos los wiimotes

		wiimote_read();
		
		cabecera( &letrero[idioma][23][0]);

		
		PX=0;PY=ylev+32; color= 0xffff3000; bkcolor=0;
		letter_size(12,32);
		s_printf("HDD: Used: %.2fGB Free: %.2fGB", used,free);

		if(strlen(header.title)<=37) letter_size(16,32);
		else if(strlen(header.title)<=45) letter_size(12,32);
		else letter_size(8,32);		


		PX=0;PY=ylev+352/2-32; color= INK0;
				
		bkcolor=0;
		
		s_printf("%s",header.title);
		
		letter_size(12,32);

		PX=0;PY=ylev+352/2+32; 

		s_printf(&letrero[idioma][37][0]);
		
		draw_snow();
		Screen_flip();

        if(exit_by_reset)
			{
			autocenter=0;
			goto out;
			}

	    if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_RED | WPAD_BUTTON_B))
			{
			for(n=0;n<240;n++)
				{
				cabecera( &letrero[idioma][23][0]);
				color=INK0;
				PX=0;PY=ylev+352/2-16;
				s_printf("%s",&letrero[idioma][39][0]);
				if(n==0) snd_fx_no();
				draw_snow();
				Screen_flip();
				wiimote_rumble(0);
				WPAD_ScanPads();
				autocenter=0;
				}
			//sleep(4);
			goto out;
			}
		if(new_pad & (WPAD_GUITAR_HERO_3_BUTTON_GREEN | WPAD_BUTTON_A)) {snd_fx_yes();break;}
		}
    

////////////////////
    for(n=0;n<60;n++)
		{
		cabecera( &letrero[idioma][23][0]);
		color=INK0;
		PX=0;PY=ylev+352/2-16;
		
		s_printf("%s",&letrero[idioma][44][0]);
		if(n==0) snd_fx_yes();
		draw_snow();
		Screen_flip();
		wiimote_rumble(0);
		WPAD_ScanPads();
		}
    //sleep(1);

	USBStorage2_Watchdog(0); // to increase the speed
	WPAD_Shutdown();
	time_sleep=0;
	SetVideoSleep(0);
	in_black|=2;

	/* Install game */
	ret = WBFS_AddGame(0);
	in_black^=2;

	while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

	time_sleep=TIME_SLEEP_SCR;
	SetVideoSleep(0);
	USBStorage2_Watchdog(1); // to avoid hdd sleep

	if(exit_by_reset) {exit_by_reset=0;}

	if (ret < 0) {
		

		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH+is_16_9*208, SCR_HEIGHT);	
		
		if(ret==-666) goto out;
		for(n=0;n<240;n++)
			{
			cabecera( &letrero[idioma][23][0]);
			color=INK0;
			PX=0;PY=ylev+352/2-16;
			s_printf("Installation ERROR! (ret = %d)", ret);
			if(n==0) snd_fx_no();
			draw_snow();
			Screen_flip();
			autocenter=0;
			
			
			//sleep(1);
			wiimote_rumble(0);
			WPAD_ScanPads();
			}
			//sleep(3);
		goto out;
	}

    for(n=0;n<240;n++)
		{
		cabecera( &letrero[idioma][23][0]);
		PX=0;PY=ylev+352/2-16;
		color=INK0;
		
		s_printf("%s",&letrero[idioma][45][0]);
		if(n==0) snd_fx_yes();
		draw_snow();
		Screen_flip();
		WPAD_Init();
		WPAD_SetIdleTimeout(60*5); // 5 minutes 

		WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); // ajusta el formato para acelerometros en todos los wiimotes
		WPAD_SetVRes(WPAD_CHAN_ALL, SCR_WIDTH+is_16_9*208, SCR_HEIGHT);	

		wiimote_rumble(0);
		WPAD_ScanPads();
		}

	}
	color=INK0;
autocenter=0;

out:
	autocenter=0;
	WBFS_Close();
}


int delete_test(int ind, char *name)
{
int old_select=-1;

frames2=0;

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= INK1; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", &letrero[idioma][ind][0]);
	bkcolor=0;
	
	PX=0;PY=ylev+352/2-32;color= INK0;
	autocenter=1;letter_size(12,32);
	s_printf("%s", &letrero[idioma][36][0]);

	if(strlen(name)<=37) letter_size(16,32);
	else if(strlen(name)<=45) letter_size(12,32);
	else letter_size(8,32);		

	PX=0;PY=ylev+352/2+32; color= INK0; 
			
	bkcolor=0;
	
	s_printf("%s",name);
	autocenter=0;
	letter_size(16,32);

	
	if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
	if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;
		

	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					draw_snow();
					Screen_flip();snd_fx_no();break;
					}

				

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_yes();return 1;} // Yes
					if(select_game_bar==61) {draw_snow();Screen_flip();snd_fx_no();break;} // No
                   
					}
				}

	
	draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

// No!!!

return 0;
}

void set_parental_control()
{

int n;

int mode=0;
int old_select=-1;

frames2=0;

parental_control_on=1;



char parental_str[4]={0,0,0,0};

char title_str[24];


while(1)
	{
	int select_game_bar=-1;
	int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	PX= 0; PY=8; color= INK0; 
	letter_size(16,32);
	
	bkcolor=0;//0xc0f08000;
			
	autocenter=1;
	s_printf("%s","Parental Control");
	autocenter=0;

    if(mode==1)
			{
			ylev+=32;

			SetTexture(NULL);

			DrawRoundFillBox(20+148, ylev, 148*2, 352, 0, 0xcfafafaf);

			letter_size(16,32);

			

			select_game_bar=-1;
			

			// change partition draw and selection
			letter_size(16,24);
			bkcolor=0;
			autocenter=0;
			
			for(n=0;n<4;n++)
				{
				int my_x=SCR_WIDTH/2-100+50*n;
				int my_y=ylev+50;

				if(parental_str[n]==0) DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0ffffff);
				else DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa000ff00);

				PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("%c", parental_str[n]==0 ? ' ' : parental_str[n]);
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);

				}

			for(n=0;n<10;n++)
				{
				int my_x=SCR_WIDTH/2-75+50*((n==0) ? 1 :((n-1) % 3));
				int my_y=ylev+64+50*((n==0) ? 4 :3-((n-1)/3));
				
				DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

				PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("%c", 48+n); 

				if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
					{
					DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
					select_game_bar=20000+n;
					}
				else
					DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
			
				}
			ylev-=32;
			}
    if(mode==0)
		{
		int m,l,k;
		
		m=64;
		for(n=0;n<8;n++)
			{
			PX=36;PY=m;
            if(config_file.game_log[n].id[0])
				{
				letter_size(12,32);
				color=INK0;
			
				s_printf("%c%c%c%c%c%c ",config_file.game_log[n].id[0],config_file.game_log[n].id[1],config_file.game_log[n].id[2],config_file.game_log[n].id[3],config_file.game_log[n].id[4],config_file.game_log[n].id[5]);

			    memset(title_str,32,22); title_str[22]=0;

				if(gameList && gameCnt>0)
					{
					for(l=0;l<gameCnt;l++)
						{
						if(!strncmp( (void *)gameList[l].id, (void *) config_file.game_log[n].id, 6))
							{
							for(k=0;k<22;k++) if(gameList[l].title[k]==0) break; else title_str[k]=gameList[l].title[k]<32 ? ' ': gameList[l].title[k];
							}
						}
					}

                color=0xffff1000; s_printf("%s ",title_str);
				color=INK0;

				s_printf("%c%c-%s-%c%c%c%c %c%c:%c%c",
					(config_file.game_log[n].bcd_time[0]>>4)+48, (config_file.game_log[n].bcd_time[0] & 15)+48,
					 &month[idioma][((config_file.game_log[n].bcd_time[1]>>4)*10+(config_file.game_log[n].bcd_time[1] & 15)-1) % 12][0],
					(config_file.game_log[n].bcd_time[2]>>4)+48, (config_file.game_log[n].bcd_time[2] & 15)+48,
					(config_file.game_log[n].bcd_time[3]>>4)+48, (config_file.game_log[n].bcd_time[3] & 15)+48,
					(config_file.game_log[n].bcd_time[4]>>4)+48, (config_file.game_log[n].bcd_time[4] & 15)+48,
					(config_file.game_log[n].bcd_time[5]>>4)+48, (config_file.game_log[n].bcd_time[5] & 15)+48);
				}
			else {autocenter=1;s_printf("< Empty >");autocenter=0;}
			m+=40;
		    }
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=61;
		if(Draw_button(600-32-strlen(&letrero[idioma][46][0])*8, ylev+108*4-64, &letrero[idioma][46][0])) select_game_bar=70;
		}
	
	if(mode==2)
		{

		SetTexture(&text_button[0]);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
		SetTexture(NULL);
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

		letter_size(16,24);

		for(n=0;n<5;n++)
			{
			int my_x=SCR_WIDTH/2-125+50*n;
			int my_y=ylev+50;

			if(n==4) DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0ffffff);
			else DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa000ff00);

			PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("%c", n==4 ? '0' : parental_str[n]);
			DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);

			}

		letter_size(16,32);

		color= INK0; bkcolor=0;
		PX=0;PY=ylev+352/2-32;
		autocenter=1;letter_size(12,32);
		s_printf("%s", &letrero[idioma][47][0]);

		autocenter=0;
		
		letter_size(16,32);

		

		
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][17][0])) select_game_bar=60;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][18][0])*8, ylev+108*4-64, &letrero[idioma][18][0])) select_game_bar=61;
		}
	
	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;

	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					for(n=0;n<4;n++) parental_str[n]=0;
					snd_fx_no();
					select_game_bar=-1;

					if(mode==0)
						{
						draw_snow();Screen_flip();break;
						}
					else mode=0;
					}

				

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_yes(); for(n=0;n<4;n++) config_file.parental[n]=parental_str[n]-48;save_cfg(mode_disc!=0); return;} // Yes
					if(select_game_bar==61) {
											for(n=0;n<4;n++) parental_str[n]=0; 
											snd_fx_no();
											if(mode==0) {draw_snow();Screen_flip();break;} else mode=0;
											} // No

					if(select_game_bar==70) {snd_fx_yes();mode=1;}

					if(select_game_bar>=20000 && select_game_bar<20010)
								{
								for(n=0;n<4;n++) if(parental_str[n]==0) break;
								if(n<4) {parental_str[n]=select_game_bar+48-20000;snd_fx_yes();}
								if(n>=3) {
										 mode=2;
										 }
								select_game_bar=-1;
								}
                   
					}
				}

	
	draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	
	}
}


int rename_game(char *old_title)
{

int n,m;

int old_select=-1;

int shift=0;

int cursor=0;
int len_str=0;
int sx;

static char keyboard[8][10]={
	{'1','2','3','4','5','6','7','8','9','0',},
	{'Q','W','E','R','T','Y','U','I','O','P',},
	{'A','S','D','F','G','H','J','K','L','Ñ',},
	{'Z','X','C','V','B','N','M','&',':','_',},

	{'1','2','3','4','5','6','7','8','9','0',},
	{'q','w','e','r','t','y','u','i','o','p',},
	{'a','s','d','f','g','h','j','k','l','ñ',},
	{'z','x','c','v','b','n','m','ç','.','-',},
	//{'','','','','','','','','','',},

};

char title[64];

memcpy(title,old_title,64);

title[63]=0;

len_str=strlen(title);

cursor=len_str;

while(1)
	{
	int select_game_bar=-1;
	int ylev=(SCR_HEIGHT-440);

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	PX= 0; PY=8; color= INK1; 
	letter_size(16,32);
	
	bkcolor=0;//0xc0f08000;
			
	autocenter=1;
	s_printf("%s",&letrero[idioma][28][0]);
	autocenter=0;

    SetTexture(NULL);

    //DrawRoundFillBox(20+148, ylev+32, 148*2, 352, 0, 0xcfafafaf);

        sx=8;
		if(len_str<=37) {letter_size(16,32);sx=16;}
		else if(len_str<=45) {letter_size(12,32);sx=12;}
		else letter_size(8,32);		

		color= INK1; 
				
		bkcolor=0x20000000;
		
        PX=(SCR_WIDTH-sx*len_str)/2;PY=56;
		for(n=0;n<len_str;n++)
			{
			PX=(SCR_WIDTH-sx*len_str)/2+n*sx; PY=56;
			if(n==cursor && (frames2 & 8)==0) {bkcolor=0xa000ff00;s_printf(" ");bkcolor=0x20000000;PX-=sx;PY=56;}
			s_printf("%c",title[n]);
			}

		if(n==cursor && (frames2 & 8)==0) {PY=56;bkcolor=0xa000ff00;s_printf(" ");bkcolor=0x20000000;}
		

    letter_size(16,32);

	select_game_bar=-1;

	color= INK0; 

	letter_size(16,24);
	bkcolor=0;
	autocenter=0;

	for(m=0;m<4;m++)
		{
		for(n=0;n<10;n++)
			{
			int my_x=SCR_WIDTH/2-5*50+50*n;
			int my_y=ylev+64+50*m;

			DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

			PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("%c", keyboard[m+shift][n]); 

			if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=1024+keyboard[m+shift][n];
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
			}
		}

		{
		int my_x=SCR_WIDTH/2-5*50;
		int my_y=ylev+64+50*4+16;

		// shift
		DrawRoundFillBox(my_x, my_y, 48+16*1, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("Aa");

		if(px>=my_x && px<my_x+48+16*1 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48+16*1, 48, 0, 4, 0xfff08000);
				select_game_bar=100;
				}
			else
				DrawRoundBox(my_x, my_y, 48+16*1, 48, 0, 4, 0xa0000000);
		
		my_x+=50+16*1;

		// <
		DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("<");

		if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=101;
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
		
		my_x+=50;


		// Space

		DrawRoundFillBox(my_x, my_y, 48*5, 48, 0, 0xa0cfffff);

		PX=my_x+16*5; PY=my_y+6+8; color= INK0;s_printf("Space");

		if(px>=my_x && px<my_x+48*5 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48*5, 48, 0, 4, 0xfff08000);
				select_game_bar=1024+32;
				}
			else
				DrawRoundBox(my_x, my_y, 48*5, 48, 0, 4, 0xa0000000);
		
		my_x+=5*50;

		// >
		DrawRoundFillBox(my_x, my_y, 48, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf(">");

		if(px>=my_x && px<my_x+48 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xfff08000);
				select_game_bar=102;
				}
			else
				DrawRoundBox(my_x, my_y, 48, 48, 0, 4, 0xa0000000);
		
		my_x+=50;

		// Del

		DrawRoundFillBox(my_x, my_y, 48+16*2, 48, 0, 0xa0cfffff);

		PX=my_x+16; PY=my_y+6+8; color= INK0;s_printf("Del");

		if(px>=my_x && px<my_x+48+16*2 && py>=my_y && py<my_y+48)
				{
				DrawRoundBox(my_x, my_y, 48+16*2, 48, 0, 4, 0xfff08000);
				select_game_bar=104;
				}
			else
				DrawRoundBox(my_x, my_y, 48+16*2, 48, 0, 4, 0xa0000000);
		
		//my_x+=5*50;

		}

		
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=60;

		if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][48][0])) select_game_bar=65;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=61;
	
	
	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;

	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

				if(new_pad & WPAD_BUTTON_B)
					{
					snd_fx_no();
					select_game_bar=-1;
					
					draw_snow();
					
					Screen_flip();break;
						
					}

				
			    if(new_pad & (WPAD_BUTTON_MINUS | WPAD_BUTTON_LEFT))
					{
						cursor--;if(cursor<0) {cursor=0;snd_fx_no();} else snd_fx_yes();
					}
				
				if(new_pad & (WPAD_BUTTON_PLUS | WPAD_BUTTON_RIGHT))
					{
						cursor++;if(cursor>len_str) {cursor=len_str;snd_fx_no();} else snd_fx_yes();
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==60) {if(len_str<2) {select_game_bar=65;}
						                    else {draw_snow();Screen_flip();snd_fx_yes(); title[63]=0;memcpy(old_title,title,64);return 1;}
					                        }// OK
					if(select_game_bar==61) {
											draw_snow();Screen_flip();
											snd_fx_no();
											return 0;
											} // discard

					if(select_game_bar==65) {
											memcpy(title,old_title,64);title[63]=0;len_str=strlen(title);cursor=0;
											snd_fx_no();
					
											} // No


											





					if(select_game_bar==100) {shift^=4;snd_fx_yes();}

					if(select_game_bar==101) {cursor--;if(cursor<0) {cursor=0;snd_fx_no();} else snd_fx_yes();}
					if(select_game_bar==102) {cursor++;if(cursor>len_str) {cursor=len_str;snd_fx_no();} else snd_fx_yes();}

					if(select_game_bar==104) {if(cursor==0) snd_fx_no();
											  else
												{
												memcpy(&title[cursor-1], &title[cursor], 64-cursor);
												len_str=strlen(title);snd_fx_yes();
												cursor--;
												}
											  }

					if(select_game_bar>=1024)
											  {
											  if(len_str>62) snd_fx_no();
											  else
												  {
												  if(cursor>=len_str) cursor=len_str;
												  for(n=62;n>=cursor;n--) title[n+1]=title[n];
												  title[cursor]=select_game_bar-1024; title[63]=0;
                                                  len_str=strlen(title);snd_fx_yes();
												  cursor++;if(cursor>=len_str) cursor=len_str;
												  }
											  
											  }
                   
					}
				}

	
    draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	
	}
return 0;
}
extern int WBFS_getdols(u8 *id);


int load_alt_game_disc=0; // 0->from WBFS 1-> from DVD

u8 alt_dol_disc_tab[32768]  __attribute__ ((aligned (32))); // temp dol alternative table for DVD mode


void menu_alternativedol(u8 *id)
{

int old_select=-1;

char buff[65];
int mode=0,n,m;
dol_infodat *dol_infop=(dol_infodat *) temp_data;
int indx=0;
int last_indx=1023;

int current_dol=0;
int max_dol=0;

memset(temp_data,0,32768);

make_rumble_off();

if(!load_alt_game_disc && is_fat==0)
	global_LoadDolInfo(temp_data);
else
	{
    dol_infop=(dol_infodat *) alt_dol_disc_tab;
	//load_cfg(1);
	//memset((void *) alt_dol_disc_tab,0,32768);
	}


for(indx=0;indx<1024;indx++)
	{
	if(dol_infop[indx].id[0]==0 && last_indx==1023)  last_indx=indx;
	if(!strncmp((void *) dol_infop[indx].id, (void *) id, 6)) break;
	}

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();

	SetTexture(&text_button[0]);
    DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
	SetTexture(NULL);
	DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);

	PX= 0; PY=ylev-32; color= INK0; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=0;
	s_printf("%s", &letrero[idioma][31][0]);
	bkcolor=0;
	
	
	autocenter=0;
	letter_size(16,32);
	
	if(mode==0)
		{
		PX=0;PY=ylev+352/2-32;
		autocenter=1;letter_size(16,32);
		s_printf("Based in Wiiscrubber Code");
		autocenter=0;
			
		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;

		if(indx<1024)
			{
			if(dol_infop[indx].offset!=0 && dol_infop[indx].size!=0)
				if(Draw_button(x_temp+16, ylev+108*4-64, &letrero[idioma][49][0])) select_game_bar=61;	
			//PX= 20; PY+=32;
			//s_printf("%llx %i",dol_infop[indx].offset, dol_infop[indx].size);
			}

		if(Draw_button(600-32-strlen(&letrero[idioma][50][0])*8, ylev+108*4-64, &letrero[idioma][50][0])) select_game_bar=62;

		}
	else
	if(mode==1)
		{
		iso_files *file =CWIIDisc_first_file;

		for(n=0;n<current_dol;n++)
			{
			file = file->next;
			if(!file) break;
			}
		

		for(m=0;m<5;m++,n++)
		{
		memset(buff,32,64);buff[64]=0;
		
		if (file != NULL)
			{
			
			int l1=strlen((char *)file->path);
			
			int l2=strlen((char *)file->name);
			int c1=l1,c2=l2;
			
            if((l1+l2)>64)
				{
				if(l1>16) 
					{
				    c1=16;
					if(l2<(64-16)) c1=64-l2;
					if(c1>l1) c1=l1;
					}
				c2=64-c1;
				
            
				if(l1<=c1)
					{
					memcpy(buff, (char *) file->path, c1);buff[c1]=0;
					}
				else
					{
					int c=c1/2-3;
					memcpy(buff, (char *)file->path, c);buff[c]=0;
					strcat(buff,"...");
					c=c1-c-3;
					strcat(buff, (char *)&file->path[l1-c]);
					}

				if(l2<=c2)
					{
					strcat(buff, (char *) &file->name);
					}
				else
					{
					int c=c2/2-3;
					int k=strlen(buff);

					memcpy(&buff[k], (char *) file->name, c);buff[k+c]=0;
					strcat(buff,"...");
					c=c2-c-3;
					strcat(buff, (char *)&file->name[l2-c]);
					}
				}
			else
				{
				memcpy(buff+(64-(l1+l2))/2, (char *) file->path, l1);
				memcpy(buff+(64-(l1+l2))/2+l1, (char *) file->name, l2);
				}

			if(Draw_button2(30+16, ylev+32-8+64*m, buff, 0)) select_game_bar=1024+n;
			file = file->next;
			}
	      else
			{
			Draw_button2(30+16, ylev+32-8+64*m, buff, -1);
			
			}
		
		//punt=&letrero[idioma][22+n+index][0];
		//memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));
		}
		        int z=-is_16_9*80-10 * (!is_16_9); 
				if(px>=z && px<=60+z && py>=ylev+220-40 && py<=ylev+220+40)
					{
					circle_select(40+z, ylev+220, '-', 1);
					
					select_game_bar=50;
					}
				else
				if(frames2 & 32)
					{
					circle_select(40+z, ylev+220, '-', 0);
					}
			

			    z=is_16_9*80+10 * (!is_16_9);
				if(px>=SCR_WIDTH-82+z && px<=SCR_WIDTH-2+z && py>=ylev+220-40 && py<=ylev+220+40)
					{
					circle_select(SCR_WIDTH-42+z, ylev+220, '+', 1);
					select_game_bar=51;
					}
				else
				if(frames2 & 32)
					{
					circle_select(SCR_WIDTH-42+z, ylev+220, '+', 0);
					}
				


		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;
			
		if(Draw_button(600-32-strlen(&letrero[idioma][0][0])*8, ylev+108*4-64, &letrero[idioma][0][0])) select_game_bar=60;
		}
		else
		if(mode==2)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][51][0]);
			autocenter=0;
			mode=128;
			}
		
		else
		if(mode==3)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][52][0]);
			autocenter=0;
			mode=129;
			}
		else
		if(mode==4)
			{
			PX=0;PY=ylev+352/2-32;
			autocenter=1;letter_size(16,32);
			s_printf("%s", &letrero[idioma][53][0]);
			autocenter=0;
			mode=129;
			}

	if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;
		
   if(mode<128)
		{

		wiimote_read();

		if(wmote_datas!=NULL)
				{
				SetTexture(NULL);		// no uses textura

						if(wmote_datas->ir.valid)
							{
							wiimote_ir();
							}
						 else 
						 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
							{
							wiimote_guitar();
							}
						 else
						 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
							{
							wiimote_classic();
							}
						 else
						   {px=-100;py=-100;}

					if(mode==1)
						{
						if(new_pad & WPAD_BUTTON_MINUS)
							{
								current_dol-=5;if(current_dol<0) current_dol=0;snd_fx_yes();
							}
						
						if(new_pad & WPAD_BUTTON_PLUS)
							{
								current_dol+=5;if(current_dol>(max_dol/5)*5) current_dol=(max_dol/5)*5;snd_fx_yes();
							}
						}

					if(new_pad & WPAD_BUTTON_B)
						{
						draw_snow();Screen_flip();snd_fx_no();return;
						}

					

					if(new_pad & WPAD_BUTTON_A) 
						{
						if(select_game_bar==50) {current_dol-=5;if(current_dol<0) current_dol=0;snd_fx_yes();}
						if(select_game_bar==51) {current_dol+=5;if(current_dol>(max_dol/5)*5) current_dol=(max_dol/5)*5;snd_fx_yes();}
					

						if(select_game_bar==60) {draw_snow();Screen_flip();snd_fx_no();return;} // exit
						
						if(select_game_bar==61) {
												draw_snow();Screen_flip();snd_fx_yes();
						                        if(indx<1024) 
													{
													if(!load_alt_game_disc && is_fat==0)
														{
														memset((void *) &dol_infop[indx],0,32);
														}
													else // for DVD mode
														{
														dol_infop[indx].offset=0;
                                                        dol_infop[indx].size=0;
														}
													}
													
												mode=4;
												 }
						
						if(select_game_bar==62) {draw_snow();Screen_flip();snd_fx_yes();mode=2;}
					   //
						if(select_game_bar>=1024)
							{
							iso_files *file =CWIIDisc_first_file;

							if(indx>=1024)
								{
								if(last_indx==1023)
									memcpy((char *) &dol_infop[0], ((char *) &dol_infop[0])+32, 32768-32);
								indx=last_indx;
								}
							memset((void *) &dol_infop[indx],0,32);
							
							n=1024;

							while(n<select_game_bar) {if(!file) break; file=file->next;n++;}

							if(file)
								{
								snd_fx_yes();
								dol_infop[indx].offset= file->offset;
								dol_infop[indx].size=	 file->size;
								memcpy((void *) &dol_infop[indx].id, id, 6);
								}
							else snd_fx_no(); // error!

							mode=3;
							}
							//

						}
					}
		}

	
    draw_snow();
	Screen_flip();
	if(exit_by_reset) break;

	if(mode==128) 
		{
		iso_files *file;

		make_rumble_off();

		altdol_frames2=0;
		remote_call(draw_altdolscr);

        if(!load_alt_game_disc)
			{
			if(is_fat && !(mode_disc & 3))
				{
				FILE *fp;
				char *name;

				name=get_fat_name(id);
				if(name)
					{
					fp=fopen(name,"rb"); // lee el fichero de texto
					if(fp)
						{
						extern int CWIIDisc_getdols(wbfs_disc_t *d);
						CWIIDisc_getdols((void *) fp);
						}
					fclose(fp);
					}
				}
			else
			WBFS_getdols(id);
			}
		else
			{
			int temp=is_fat;
			is_fat=0;
			disc_getdols(id);
			is_fat=temp;
			}


		remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
		/*
		// simulate more dols
        file =CWIIDisc_first_file;
		for(n=0;n<7;n++)
			{
			file->next=malloc(sizeof(iso_files));
			*file->next=*file;
			file->next->next=NULL;
			file=file->next;
			}
		*/

		file =CWIIDisc_first_file;
		max_dol=0;
		while(file)
			{
			max_dol++;
			file = file->next;
			if(!file) break;
			}
				
		mode=1;
		}
	if(mode==129) break;

	frames2++;
	}


	// FREE FILES
	{
	iso_files *file =CWIIDisc_first_file;
	while(file)
		{
		iso_files *file2=file->next;
		free(file); file=file2;
		}
	CWIIDisc_first_file=CWIIDisc_last_file=NULL;
	}

make_rumble_off();
if(!load_alt_game_disc  && is_fat==0)
	global_SaveDolInfo(temp_data);
else
	save_cfg(1);

if(mode==129) sleep(2);
}

int current_partition=0;

int partition_cnt[4]={-1,-1,-1,-1};


extern void *ulo_cfg;

int uloader_update()
{
int n,r;

int old_select=-1;

int update=0;

int ret=0;

int retorno=0;

static char url[512];

char cad[5];


u8 *temp_buf=NULL;
u32 temp_size=0;
int select_game_bar=-1;

u8 *temp_buf2=NULL;
u32 temp_size2=0;

u8 cadena[256];
u8 name[256];

if(!sd_ok) return 0;

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes


	draw_background();

		{

		PX= 0; PY=8; color= INK0; 
				
		bkcolor=0;
		letter_size(16,32);
		autocenter=1;
		s_printf("%s","uLoader Update Utility");
		autocenter=0;
		}

		letter_size(16,32);

		if(Draw_button2(30+48, ylev+32+64*3-80, "                    Download uLoader                    ",0)) select_game_bar=1;

		if(Draw_button2(30+48, ylev+32+64*3,    "              Download Alternative uLoader              ",0)) select_game_bar=2;

		if(Draw_button2(30+48, ylev+32+64*3+80, "                          Exit                          ",0)) select_game_bar=3;
	

		if(select_game_bar>=0)
		{
		if(old_select!=select_game_bar)
			{
			snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
			old_select=select_game_bar;
			}
		}
	else 
		old_select=-1;


	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

			if(new_pad & (WPAD_BUTTON_B | WPAD_BUTTON_HOME))
					{
					snd_fx_no();draw_snow();Screen_flip();
					return 0;
					}
			if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==1) {snd_fx_yes();draw_snow();Screen_flip(); break;}
					if(select_game_bar==2) {snd_fx_yes();draw_snow();Screen_flip(); break;}
					if(select_game_bar==3) {snd_fx_yes();draw_snow();Screen_flip(); return 0;}
					}
			
			
			}

	frames2++;step_button++;
	draw_snow();
	Screen_flip();
	if(exit_by_reset)  
		{	
		return 1;
		}
	}


	force_reload_ios222=1;

	down_frame=0;

	down_mess="Verifying uloader Update";
	remote_call(down_uload_gfx);

		sprintf(url, "http://mods.elotrolado.net/~hermes/wii/update_uloader/update.txt");
		ret=download_file(url, &temp_buf, &temp_size);

		if(ret==0 && temp_buf!=NULL) 
			{
			if(!strncmp((char *) temp_buf, "<!DOCTYPE HTML", 14) /*strstr((char *) temp_buf, "Not Found")*/)
				{
				down_frame=-1;
				down_mess="update.txt Not Found";
				snd_fx_no();
				sleep(4);
				goto exit;
				}
			n=0;
			memset(cad,0,5);

            while(temp_buf[n]>32 && n<temp_size && n<4) {cad[n]=temp_buf[n];n++;}
			while(temp_buf[n]<32 && n<temp_size) n++;
            
			if(strncmp((void *) uloader_version, (void *) cad,4)==0) update=2;
			else
			if(strncmp((void *) uloader_version, (void *) cad,4)<0) update=1;
			}
		else
			{
			down_frame=-1;
			down_mess="Error Downloading update.txt";
			snd_fx_no();
			sleep(4);
			goto exit;
			}

        
		
		if(update==0)
			{
			down_frame=-1;
			down_mess="No New uLoader Version For You";
			snd_fx_yes();
			sleep(4);
			goto exit;
			}
		else		
			{
			down_frame=-1;
			if(update==2) 
				 {
				 #ifndef ALTERNATIVE_VERSION
				 if(select_game_bar==1)
					 {
					 sprintf((char *) cadena,"You Have The Current Version %s",cad);
					 down_mess=(char *) cadena;
					 snd_fx_yes();
					 sleep(4);
					 goto exit;
					 }
				 #else
				 if(select_game_bar==2)
					 {
					 sprintf((char *) cadena,"You Have The Current Version %s",cad);
					 down_mess=(char *) cadena;
					 snd_fx_yes();
					 sleep(4);
					 goto exit;
					 }
				 #endif
				 sprintf((char *) cadena,"Updating to v%s",cad);
				 }
				 sprintf((char *) cadena,"New Update v%s",cad);

			down_mess=(char *) cadena;
			sleep(4);
			}
		
		temp_buf2=temp_buf;
		temp_size2=temp_size;

		#if 1

		down_frame=0;
		down_mess="Downloading boot.dol";

        if(select_game_bar==1)
			sprintf(url, "http://mods.elotrolado.net/~hermes/wii/update_uloader/uloader.dol");
		else 
			sprintf(url, "http://mods.elotrolado.net/~hermes/wii/update_uloader/uloader_alt.dol");
		    
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!strncmp((char *) temp_buf, "<!DOCTYPE HTML", 14)/*strstr((char *) temp_buf, "Not Found")*/)
					{
					down_frame=-1;
					if(temp_buf) free(temp_buf); temp_buf=NULL;

					if(select_game_bar==1)
						down_mess= "uloader.dol Not Found";
					else
						down_mess= "uloader_alt.dol Not Found";
				

					snd_fx_no();
					sleep(4);
					goto exit;
					}

				if(sd_ok && temp_size!=0)
					{
					FILE *fp;

					// PATCH FOR PORT
					u8 *ehc_data= search_for_ehcmodule_cfg((void *) temp_buf, temp_size);
					u8 *ehc_data_old=search_for_ehcmodule_cfg((void *) ehcmodule, size_ehcmodule);
					u8 * uload_data= search_for_uloader_cfg((void *) temp_buf, temp_size);
					u8 * uload_data_old=(u8*) &ulo_cfg;

					if(ehc_data && ehc_data_old)
						{
						ehc_data+=12;
						ehc_data_old+=12;
						memcpy(&ehc_data[0], &ehc_data_old[0], 4);
						}

					if(uload_data && uload_data_old)
						{
						uload_data+=12;
						uload_data_old+=12;
						memcpy(&uload_data[0], &uload_data_old[0], 8);
						}

					
					fp=fopen("sd:/apps/uloader/boot.dol","wb"); // escribe el fichero

					if(fp!=0)
						{
						r=fwrite(temp_buf,1, temp_size ,fp);
						fclose(fp);
						}
					if(!fp)
						{
						down_frame=-1;
						down_mess="Error Creating boot.dol in the SD";snd_fx_no();sleep(4);goto exit;
						}
					else
					if(r!=temp_size) {down_frame=-1;down_mess="Error Writing boot.dol in the SD";snd_fx_no();sleep(4);goto exit;}

			        
					}
				}
			else
			   {
			   down_frame=-1;
			  

			   if(select_game_bar==1)
					down_mess= "Error Downloading uloader.dol";
				else
					down_mess= "Error Downloading uloader_alt.dol";
					
			   snd_fx_no();
			   sleep(4);goto exit;
			   }

			if(temp_buf) free(temp_buf); temp_buf=NULL;

			#endif

			while(n<temp_size2)
			{
			int m;

			down_frame=0;
			memset((char *) name,0,256);
			m=0;
            while(temp_buf2[n]>32 && n<temp_size2) {name[m]=temp_buf2[n];n++;m++;}
			while(temp_buf2[n]<32 && n<temp_size2) n++;

			if(m<4) break;

			sprintf((char *) cadena,"Downloading %s", name);
			down_mess=(char *) cadena;

			sprintf(url, "http://mods.elotrolado.net/~hermes/wii/update_uloader/%s", name);
	
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!strncmp((char *) temp_buf, "<!DOCTYPE HTML", 14) /*strstr((char *) temp_buf, "Not Found")*/)
					{
					down_frame=-1;

					if(temp_buf) free(temp_buf); temp_buf=NULL;
					
					sprintf((char *) cadena,"%s Not Found", name);
					down_mess=(char *) cadena;
					snd_fx_no();
					sleep(4);
					goto exit;
					}

				if(sd_ok && temp_size!=0)
					{
					FILE *fp;
                    
					sprintf(url, "sd:/apps/uloader/%s", name);
					fp=fopen(url,"wb"); // escribe el fichero

					if(fp!=0)
						{
						r=fwrite(temp_buf,1, temp_size ,fp);
						fclose(fp);
						}
					if(!fp)
						{
						down_frame=-1;
						sprintf((char *) cadena,"Error Creating %s in the SD", name);
					    down_mess=(char *) cadena;
						snd_fx_no();sleep(4);goto exit;
						}
					else
					if(r!=temp_size) 
						{
						down_frame=-1;sprintf((char *) cadena,"Error Writing %s in the SD", name);
						down_mess=(char *) cadena;snd_fx_no();sleep(4);goto exit;
						}
					if(temp_buf) free(temp_buf); temp_buf=NULL;
			        
					}
				}
			else
			   {
			   down_frame=-1;
			   sprintf((char *) cadena,"Error Downloading %s", name);
			   down_mess=(char *) cadena;
			   snd_fx_no();
			   sleep(4);goto exit;
			   }
			
			}// while
			
			down_frame=-1;
			down_mess="uLoader Update Successful";
			snd_fx_yes();
            sleep(4);
			  
		
	
	retorno=1;

exit:
	remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
	if(temp_buf) free(temp_buf); temp_buf=NULL;
	if(temp_buf2) free(temp_buf2); temp_buf2=NULL;
	http_deinit();
	sleep(1);

return retorno;
}



int uloader_hacks(void)
{

int n;
int old_select=-1;
char *punt;
char buff[64];

int mi_letrero=60*3;

int index=0;

u8 temp_hacks[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

u8 * ehc_data=search_for_ehcmodule_cfg((void *) ehcmodule, size_ehcmodule);
u8 * uload_data=(u8*) &ulo_cfg;

if(ehc_data)
	{
	ehc_data+=12;

	temp_hacks[0]=ehc_data[0];
	temp_hacks[1]=ehc_data[1];
	temp_hacks[2]=ehc_data[2] & 1;
	temp_hacks[3]=(ehc_data[2] & 2)!=0;
	temp_hacks[4]=ehc_data[3];
	}

if(uload_data)
	{
	uload_data+=12;
	memcpy((void *) temp_hacks+5, (void *) uload_data, 3);
	temp_hacks[8]=(uload_data[3] & 1)!=0;
	temp_hacks[9]=(uload_data[3] & 2)!=0;
	temp_hacks[10]=(uload_data[3] & 4)!=0;
	temp_hacks[11]=(uload_data[3] & 8)!=0;
	temp_hacks[12]=(uload_data[3] & 16)!=0;
	temp_hacks[13]=(uload_data[3] & 32)!=0;
	temp_hacks[14]=(uload_data[3] & 64)!=0;
	temp_hacks[15]=(uload_data[3] & 128)!=0;

	}

	while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	//SetTexture(NULL);
    //DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);

	draw_background();


		letter_size(16,32);
		
		PX= 0; PY=8; color= INK0; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",&letrero[idioma][33][0]);
		autocenter=0;
		

		letter_size(16,32);
		
	if(mi_letrero==0)
		{

		for(n=0;n<5;n++)
			{
			int m=n+index;

			memset(buff,32,56);buff[56]=0;
			if(m>10)
				{
				Draw_button2(30+48, ylev+32+64*n, buff, 0);
				continue;
				}
			
			punt=&letrero[idioma][56+n+index][0];
			memcpy(buff+(56-strlen(punt))/2, punt, strlen(punt));

			if(Draw_button2(30+48, ylev+32+64*n, buff,
				temp_hacks[m]))
					 select_game_bar=100+m;
		//
			}

		if(Draw_button(36, ylev+108*4-64, &letrero[idioma][8][0])) select_game_bar=10;

		if(Draw_button(600-32-strlen(&letrero[idioma][9][0])*8, ylev+108*4-64, &letrero[idioma][9][0])) select_game_bar=11;
		
		int z=-is_16_9*80-10 * (!is_16_9); 

		if(px>=z && px<=60+z && py>=ylev+220-40 && py<=ylev+220+40)
			{
			circle_select(40+z, ylev+220, '-', 1);
		
			select_game_bar=50;
			}
		else
		if(frames2 & 32)
			{
			circle_select(40+z, ylev+220, '-', 0);
			}
		

		z=is_16_9*80+10 * (!is_16_9);
		if(px>=SCR_WIDTH-82+z && px<=SCR_WIDTH-2+z && py>=ylev+220-40 && py<=ylev+220+40)
			{
			circle_select(SCR_WIDTH-42+z, ylev+220, '+', 1);
			select_game_bar=51;
			}
		else
		if(frames2 & 32)
			{
			circle_select(SCR_WIDTH-42+z, ylev+220, '+', 0);
			}

		if(select_game_bar>=0)
			{
			if(old_select!=select_game_bar)
				{
				snd_fx_tick();if(rumble==0) {wiimote_rumble(1);rumble=10;}
				old_select=select_game_bar;
				}
			}
		else 
			old_select=-1;
		}
	else
		{// letrero

		SetTexture(&text_button[0]);
		DrawRoundFillBox(20, ylev, 148*4, 352, 0, 0xffafafaf);
		SetTexture(NULL);
		DrawRoundBox(20, ylev, 148*4, 352, 0, 4, 0xff303030);
		
		PX= 0; PY=ylev+128; color= INK0; 
				
		bkcolor=0;
		
		autocenter=1;
		letter_size(12,24);
		
		if(idioma)
			s_printf("%s", "Esta función modifica el .dol de uLoader en:");
		else
			s_printf("%s", "This function modify uLoader .dol in:");
		PY+=64;
		letter_size(16,32);
		s_printf("%s", "sd:/apps/uloader/boot.dol");
		autocenter=0;
		
		mi_letrero--;
		}


	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					if(wmote_datas->ir.valid)
						{
						wiimote_ir();
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						wiimote_guitar();
						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						wiimote_classic();
						}
					 else
					   {px=-100;py=-100;}

			if(mi_letrero==0)
				{
				if(new_pad & (WPAD_BUTTON_B | WPAD_BUTTON_HOME))
					{
					snd_fx_no();draw_snow();Screen_flip();
					return 0;
					}

                if(new_pad & WPAD_BUTTON_MINUS)
					{
						index-=5;if(index<0) index=10;snd_fx_tick();
					}
				
				if(new_pad & WPAD_BUTTON_PLUS)
					{
						index+=5;if(index>10) index=0;snd_fx_tick();
					}

				if(new_pad & WPAD_BUTTON_A) 
					{
					if(select_game_bar==50) {index-=5;if(index<0) index=10;}
					if(select_game_bar==51) {index+=5;if(index>10) index=0;}

					// done
					if(select_game_bar==10)
						{
						FILE *fp;
						u8 *mem=NULL;
						int size=0;

						make_rumble_off();
						wiimote_read();

					    snd_fx_yes();draw_snow();Screen_flip();

						down_frame=0;
						down_mess="Reading boot.elf";
						remote_call(down_uload_gfx);

						if(sd_ok)
							{
							fp=fopen("sd:/apps/uloader/boot.dol","rb");

							if(fp!=0)
								{
								fseek(fp, 0, SEEK_END);
								size = ftell(fp);
								mem= memalign(32, size);
								if(!mem) 
									{
									down_frame=-1;
									down_mess="Can't Alloc Memory";
									remote_call(down_uload_gfx);
									sleep(3);
									fclose(fp);
									}
								else
									{
									fseek(fp, 0, SEEK_SET);

									if(fread(mem,1, size ,fp)!=size)
										{
										down_frame=-1;
										down_mess="Can't Read File";
										remote_call(down_uload_gfx);
										sleep(3);
										free(mem); mem=NULL;
										}
								
									fclose(fp);
									}
								}
							else
								{
								down_frame=-1;
								down_mess="Can't Open boot.dol";
								remote_call(down_uload_gfx);
								sleep(3);
								}

							if(mem)
								{
								ehc_data=   search_for_ehcmodule_cfg((void *) mem, size);
								uload_data= search_for_uloader_cfg((void *) mem, size);

								if(ehc_data)
									{
									ehc_data+=12;
									memcpy((void *) (void *) ehc_data, temp_hacks, 4);

									ehc_data[0]=temp_hacks[0];
									ehc_data[1]=temp_hacks[1];
									ehc_data[2]=temp_hacks[2]!=0;
									ehc_data[2]|=(temp_hacks[3]!=0)<<1;
									ehc_data[3]=temp_hacks[4];
									}

								if(uload_data)
									{
									int n;
									uload_data+=12;
									memcpy((void *) uload_data, (void *) temp_hacks+5, 3);
									   
									uload_data[3]=0;
									for(n=0;n<8;n++)
										{
										uload_data[3]|= (temp_hacks[8+n]!=0)<<n;
										}

									}
								
								if(!ehc_data || !uload_data)
									{
									down_frame=-1;
									down_mess="Can't Find Config Area";
									remote_call(down_uload_gfx);
									sleep(3);
									free(mem); mem=NULL;
									}
								}

							if(mem)
								{
								down_mess="Writing patched boot.elf";
								fp=fopen("sd:/apps/uloader/boot.dol","wb");

									if(fp!=0)
										{
										if(fwrite(mem,1, size ,fp)!=size)
											{
											down_frame=-1;
											down_mess="Can't Write File";
											remote_call(down_uload_gfx);
											sleep(3);
											free(mem); mem=NULL;
											}
									
										fclose(fp);
										
										}
									else
										{
										down_frame=-1;
										down_mess="Can't Write boot.dol";
										remote_call(down_uload_gfx);
										sleep(3);free(mem); mem=NULL;
										}
								}

							
							if(mem) 
								{
								free(mem);
								down_frame=-1;
								down_mess="Done!!! Exiting from uLoader...";
								remote_call(down_uload_gfx);
								sleep(3);
								}
							
							remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);

							if(mem) return 1;

							return 0;
							}
						else
							{
							down_frame=-1;
							down_mess="Can't Access To The SD";
							remote_call(down_uload_gfx);
							sleep(3);
							}
						remote_call_abort();while(remote_ret()==REMOTE_BUSY) usleep(1000*50);
						}
					// return
					if(select_game_bar==11) {snd_fx_yes();draw_snow();Screen_flip(); return 0;}

					if(select_game_bar==110)
						{
						snd_fx_yes();draw_snow();Screen_flip();
						happy_new_year(-1);
						}
					else

					if(select_game_bar>=100 && select_game_bar<115) {snd_fx_yes();temp_hacks[select_game_bar-100]^=1;}


					
					}
			}
			
			} // letrero
	
	frames2++;step_button++;
	draw_snow();
	Screen_flip();
	if(exit_by_reset)  
		{	
		break;
		}
	}

return 0;
}

void get_covers()
{
int n,m;
int ret=0;

static char url[512];
static char region[3][8]={"pal", "ntsc", "ntscj"};
//static char region2[3][8]={"PAL", "NTSC", "NTSC-J"};
static char region3[11][4]={"US", "EN", "FR", "DE", "ES", "IT", "NL", "PT", "AU", "JA", "KO"};

char *flag;
int sizex=0,sizex2=0;

u8 *temp_buf=NULL;
u32 temp_size=0;

int reg_ind=0;

//if(!sd_ok) return;

//mkdir("sd:/covers",S_IREAD | S_IWRITE);
if(!gameCnt) return;

	switch(CONF_GetLanguage())
			{
		    case CONF_LANG_JAPANESE:
				reg_ind=9;break;
			case CONF_LANG_ENGLISH:
				reg_ind=1;break;
			case CONF_LANG_GERMAN:
				reg_ind=3;break;
			case CONF_LANG_FRENCH:
				reg_ind=2;break;
			case CONF_LANG_SPANISH:
				reg_ind=4;break;
			case CONF_LANG_ITALIAN:
				reg_ind=5;break;
			case CONF_LANG_DUTCH:
				reg_ind=6;break;
			case CONF_LANG_KOREAN:
				reg_ind=10;break;
			default:
				reg_ind=1;break;
			}

flag=malloc(gameCnt);
if(!flag) return;

memset(flag,0,gameCnt);

if(gameCnt==1) sizex=600;
else sizex=600/(gameCnt);

if(sizex<=2) sizex2=1;
else {if(sizex>4) sizex2=sizex-2; else sizex2=sizex-1;}

//draw_snow();
Screen_flip();

for(n=0;n<=gameCnt;n++)
	{
	struct discHdr *header = &gameList[n];

	int ylev=(SCR_HEIGHT-440);

	


	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	draw_background();

	PX= 0; PY=ylev-32; color= INK0; 
				
	bkcolor=0;
	letter_size(16,32);

	autocenter=1;
	bkcolor=num_partitions=0;//0xb0f0f0f0;
	s_printf("%s", "Downloading...");
	bkcolor=0;
   
	if(header && n<gameCnt)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=112; color= INK0; 
				
		bkcolor=0;
		
		autocenter=1;
		s_printf("%s",header->title);
		autocenter=0;
		}

	letter_size(16,32);
	SetTexture(NULL);
	DrawFillBox(10, SCR_HEIGHT/2-16 , 620, 32, 2, INK0);

	if(is_fat && header && (header->version & 2)) flag[n]=2;

    for(m=0;m<gameCnt;m++)
		{
		switch(flag[m])
			{
			case 1:
			case 3:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0xff00cf00);break;
			case 2:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0xff0000ff);break;
			default:
				DrawFillBox(20+m*sizex, SCR_HEIGHT/2-8 , sizex2, 16, 0, 0x30800000);break;
			}
		}

   PX= 0; PY=SCR_HEIGHT/2+120; color= INK0; 
				
	bkcolor=0;
		
	autocenter=1;
    if(n>=gameCnt) s_printf("Done."); else s_printf("Press B to abort");
		
		autocenter=0;
	Screen_flip();

    if(is_fat && header && (header->version & 2)) continue;

	if(n>=gameCnt) break;

	//////////////////////
	wiimote_read();

	if(wmote_datas!=NULL)
			{

					if(wmote_datas->ir.valid)
						{
						angle_icon=0.0f;
						time_sleep=TIME_SLEEP_SCR;
						SetVideoSleep(0);
						}
					 else 
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						angle_icon=0.0f;
						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						 angle_icon=0.0f;
						if(new_pad & WPAD_CLASSIC_BUTTON_B) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_CLASSIC_BUTTON_B) old_pad|=WPAD_BUTTON_B;
						}
					 else
					   {px=-100;py=-100;}

	

				if(new_pad & WPAD_BUTTON_B)
					{
					break;
					}
			}
	/////////////////////

	//if(!strncmp((void *) discid, (void *) header->id,6))
   
   /*
	sprintf(url, "http://www.theotherzone.com/wii/resize/%s/160/224/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
	*/

/*	sprintf(url, "http://www.wiiboxart.com/wii/resize/%s/160/224/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);*/


   
	memset(disc_conf,0,256*1024);
	global_GetProfileDatas(header->id, disc_conf);
		
	if(!(disc_conf[0]=='H' && disc_conf[1]=='D' && disc_conf[2]=='R') || !(disc_conf[9]=='P' && disc_conf[10]=='N' && disc_conf[11]=='G'))
		{
		ret=-1;
		for(m=0;m<nfiles;m++)
			{
			if(!files[m].is_directory)
				if(!strncmp((char *) header->id,get_name_from_UTF8(&files[m].name[0]), 6))
					{
					FILE *fp;

					fp=fopen(&files[m].name[0],"rb"); // lee el fichero de texto
					if(fp)
						{
						fseek(fp, 0, SEEK_END);

						temp_size = ftell(fp);

						temp_buf= (u8 *) malloc(temp_size);

						if(!temp_buf) {fclose(fp);break;}
						else
							{
							fseek(fp, 0, SEEK_SET);
							if(fread(temp_buf,1, temp_size ,fp)==temp_size) ret=0;
							
							if(ret<0) {free(temp_buf);temp_buf=NULL;}

							fclose(fp);break;
							}
						}
					}
			}


		if(header->id[3]=='E') reg_ind=0;
		if(header->id[3]=='J') reg_ind=9;

		//reg_ind=1;
	

		sprintf(url, "http://wiitdb.com/wiitdb/artwork/cover/%s/%c%c%c%c%c%c.png",
		&region3[reg_ind][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
        if(reg_ind!=1)
			{
			sprintf(url, "http://wiitdb.com/wiitdb/artwork/cover/%s/%c%c%c%c%c%c.png",
			&region3[1][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
			
			if(ret!=0)
				{
				force_reload_ios222=1;
				ret=download_file(url, &temp_buf, &temp_size);

				if(ret==0 && temp_buf!=NULL) 
					{
					if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
					}
				}
			}
		
		sprintf(url, "http://www.wiiboxart.com/%s/%c%c%c%c%c%c.png", 
		&region[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
		
		#if 0
		sprintf(url, "http://www.muntrue.nl/covers/%s/160/225/boxart/%c%c%c%c%c%c.png", 
		&region2[(header->id[3]=='E')+(header->id[3]=='J')*2][0], header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
		
		if(ret!=0)
			{
			force_reload_ios222=1;
			ret=download_file(url, &temp_buf, &temp_size);
			if(ret==0 && temp_buf!=NULL) 
				{
				if(!(temp_buf[1]=='P' && temp_buf[2]=='N' && temp_buf[3]=='G')) {free(temp_buf);temp_buf=NULL;ret=-1;}
				}
			}
		#endif

		if(ret==0)
			{
			#if 0
			sprintf(url, "sd:/caratulas/%c%c%c%c%c%c.png", header->id[0], header->id[1], header->id[2], header->id[3], header->id[4], header->id[5]);
            if(sd_ok && temp_size!=0)
				{
				FILE *fp;
				//flag[n]=1;
				fp=fopen(url,"wb"); // escribe el fichero

				if(fp!=0)
					{
					fwrite(temp_buf,1, temp_size ,fp);
					fclose(fp);
					}
				}
			#endif
			flag[n]=2;
			if(temp_size>4 && temp_size<(200*1024-8))
				{
				void * texture_png=create_png_texture(&png_texture, temp_buf, 0);
				if(texture_png)
					{
					free(texture_png);
					disc_conf[0]='H'; disc_conf[1]='D'; disc_conf[2]='R';disc_conf[3]=((temp_size+8+1023)/1024)-1;
									
					memcpy(&disc_conf[8], temp_buf, temp_size);

					global_SetProfileDatas(header->id, disc_conf);
					flag[n]=1;
					}
					
				
				}
			if(temp_buf) free(temp_buf); temp_buf=NULL;

			
			}
		  else flag[n]=2;
		}
	else flag[n]=3;

	
	
	}

http_deinit();
sleep(1);
}

void select_file(int flag, struct discHdr *header)
{
int n,m,signal2=2,f=0,len,max_entry;
static  int posfile=0;
static int old_flag=-1;
int flash=0;
int signal=1;

frames2=0;

u8 *mem_png=NULL;
int len_png=0;
void * texture_png=NULL;
int mode=0;

int flag_auto=1;

have_device=(sd_ok!=0) | 2*(ud_ok!=0);

if(ud_ok && !sd_ok) path_file[0]='u';

read_list_file((void *) path_file, flag);

if(flag!=old_flag)
	{
	posfile=0;
	old_flag=flag;
	}

while(1)
	{
	int ylev=(SCR_HEIGHT-440);
	//int select_game_bar=-1;

	if(SCR_HEIGHT>480) ylev=(SCR_HEIGHT-440)/2;

	if(rumble) 
		{
		if(rumble<=7) wiimote_rumble(0); 
		rumble--;
		}
	else wiimote_rumble(0);

	WPAD_ScanPads(); // esto lee todos los wiimotes

	draw_background();

	SetTexture(NULL);
	DrawRoundFillBox(8,40, SCR_WIDTH-16, 336+8-32-40, 0, 0x6fffff00);			
	DrawRoundBox(8, 40, SCR_WIDTH-16, 336+8-32-40, 0, 2, 0xff30ffff);

	if(header)
		{
		if(strlen(header->title)<=37) letter_size(16,32);
		else if(strlen(header->title)<=45) letter_size(12,32);
		else letter_size(8,32);		

		PX= 0; PY=ylev-32; color= INK0; 
				
		bkcolor=0;
		
		autocenter=1;

		s_printf("%s",header->title);
		autocenter=0;

		// get automatic .png
		if(flag_auto)
			{
			for(n=0;n<nfiles;n++)
			{
			if(!files[n].is_directory)
				if(!strncmp((char *) header->id,get_name_from_UTF8(&files[n].name[0]), 6)) {signal=1;posfile=n;flag_auto=0;break;}
			}
			}
		
		}


	SetTexture(NULL);

	PX= 0; PY=ylev-32; color= INK0; 
				
	bkcolor=0;
	letter_size(16,32);


	autocenter=0;

	max_entry=(320-32)/30;

	flash++;

	m=0;
	
	sizeletter=4;


	for(n=(posfile-max_entry/2)*(nfiles>=max_entry);n<posfile+max_entry;n++)
			{
			if(n<0) continue;
			if(n>=nfiles) break;
			if(m>=max_entry) break;
			
            
			PX=16;PY=48+2+m*30;
			color=INK0;
			
			
			if(n== posfile)
				{
				len=38;

				DrawFillBox(PX-8,PY-2-8, len*16+16, 36, 0, 0x6f00ff00);
				
				DrawBox(PX-8,PY-2-8, len*16+16, 36, 0, 2, 0xff30ffff);

				}

			if(files[n].is_directory)
				{
				if(files[n].name[0]=='.' && files[n].name[1]=='[')
					{
					color=0xffff6f00;
					s_printf("%s",&files[n].name[1]);
					}
				else
					{
					color=0xffff6f00;
				
					s_printf("<%s>",get_name_short_fromUTF8(&files[n].name[0]));
					}
				}
			else 
				{
				color=INK0;
	            if(files[n].is_selected) color=INK0;

				s_printf("%s", (char *) get_name_short_fromUTF8(&files[n].name[0]));
				}
			
			bkcolor=0;color=0xffffffff;
			m++;
			}
	if(mode==0) 
		{
		letter_size(12,32);
		color=INK0;
		PX=16;PY=480-160;
		s_printf(" Press PLUS to");
		PX=16;PY+=40;
		s_printf("Download Covers");
		PX=16;PY+=40;
		s_printf(" From Internet");
		PX=16;PY+=40;
		s_printf("or this folder");

		PX=396;PY=480-160;
		s_printf("  Pulsa MAS Para");
		PX=396;PY+=40;
		s_printf("Descargar Caratulas");
		PX=396;PY+=40;
		s_printf("  Desde Internet");
		PX=396;PY+=40;
		s_printf("%s","o éste directorio");
		}
	if(texture_png)
		{
		SetTexture(NULL);

		/*DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffafafaf);
		SetTexture(&png_texture);
		DrawRoundFillBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 0xffffffff);
		SetTexture(NULL);
		DrawRoundBox(SCR_WIDTH/2-100-200*mode/100, 480-128-312*mode/100, 200+400*mode/100, 128+256*mode/100, 0, 4, 0xff303030);
		*/

		DrawFillBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 0xffafafaf);
		SetTexture(&png_texture);
		DrawFillBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 0xffffffff);
		SetTexture(NULL);
		DrawBox(SCR_WIDTH/2-64-100*mode/100, 480-170-308*mode/100, 128+200*mode/100, 170+260*mode/100, 0, 2, 0xff303030);
		

		if(mode>0) {mode+=signal2;if(mode==0) signal2=2;}
		if(mode>100) mode=100;
		
		if(mode!=0 && mode !=100) snd_fx_fx(mode);

		if(mode==100)
			{
			bkcolor=0;
			PX= 0; PY=480-40; color= INK0; 
			letter_size(12,32);
			autocenter=1;
			s_printf("%s", &letrero[idioma][37][0]);
			autocenter=0;
			}
		
		}
	else
		{
		SetTexture(NULL);

		//DrawRoundFillBox(SCR_WIDTH/2-100, 480-128, 200, 128, 0, 0xffafafaf);
		//DrawRoundBox(SCR_WIDTH/2-100, 480-128, 200, 128, 0, 4, 0xff303030);

		DrawFillBox(SCR_WIDTH/2-64, 480-170, 128, 170, 0, 0xffafafaf);
		DrawBox(SCR_WIDTH/2-64, 480-170, 128, 170, 0, 2, 0xff303030);
		}

	SetTexture(NULL);

	wiimote_read();

	if(wmote_datas!=NULL)
			{
			SetTexture(NULL);		// no uses textura

					
					 if(wmote_datas->exp.type==WPAD_EXP_GUITARHERO3)
						{
						angle_icon=0.0f;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) new_pad|=WPAD_BUTTON_UP;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_UP) old_pad|=WPAD_BUTTON_UP;

						if(new_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) new_pad|=WPAD_BUTTON_DOWN;
						if(old_pad & WPAD_GUITAR_HERO_3_BUTTON_STRUM_DOWN) old_pad|=WPAD_BUTTON_DOWN;

						}
					 else
					 if(wmote_datas->exp.type==WPAD_EXP_CLASSIC)
						{
						angle_icon=0.0f;

						if(new_pad & WPAD_CLASSIC_BUTTON_UP) new_pad|=WPAD_BUTTON_UP;
						if(old_pad & WPAD_CLASSIC_BUTTON_UP) old_pad|=WPAD_BUTTON_UP;

						if(new_pad & WPAD_CLASSIC_BUTTON_DOWN) new_pad|=WPAD_BUTTON_DOWN;
						if(old_pad & WPAD_CLASSIC_BUTTON_DOWN) old_pad|=WPAD_BUTTON_DOWN;

						if(new_pad & WPAD_CLASSIC_BUTTON_LEFT) new_pad|=WPAD_BUTTON_LEFT;
						if(old_pad & WPAD_CLASSIC_BUTTON_LEFT) old_pad|=WPAD_BUTTON_LEFT;

						if(new_pad & WPAD_CLASSIC_BUTTON_RIGHT) new_pad|=WPAD_BUTTON_RIGHT;
						if(old_pad & WPAD_CLASSIC_BUTTON_RIGHT) old_pad|=WPAD_BUTTON_RIGHT;

						if(new_pad & WPAD_CLASSIC_BUTTON_A) new_pad|=WPAD_BUTTON_A;
						if(old_pad & WPAD_CLASSIC_BUTTON_A) old_pad|=WPAD_BUTTON_A;

						if(new_pad & WPAD_CLASSIC_BUTTON_B) new_pad|=WPAD_BUTTON_B;
						if(old_pad & WPAD_CLASSIC_BUTTON_B) old_pad|=WPAD_BUTTON_B;

						if(new_pad & WPAD_CLASSIC_BUTTON_X) new_pad|=WPAD_BUTTON_1;
						if(old_pad & WPAD_CLASSIC_BUTTON_X) old_pad|=WPAD_BUTTON_1;

						if(new_pad & WPAD_CLASSIC_BUTTON_Y) new_pad|=WPAD_BUTTON_2;
						if(old_pad & WPAD_CLASSIC_BUTTON_Y) old_pad|=WPAD_BUTTON_2;

						if(new_pad & WPAD_CLASSIC_BUTTON_HOME) new_pad|=WPAD_BUTTON_HOME;
						if(old_pad & WPAD_CLASSIC_BUTTON_HOME) old_pad|=WPAD_BUTTON_HOME;

						if(new_pad & WPAD_CLASSIC_BUTTON_PLUS) new_pad|=WPAD_BUTTON_PLUS;
						if(old_pad & WPAD_CLASSIC_BUTTON_PLUS) old_pad|=WPAD_BUTTON_PLUS;

						if(new_pad & WPAD_CLASSIC_BUTTON_MINUS) new_pad|=WPAD_BUTTON_MINUS;
						if(old_pad & WPAD_CLASSIC_BUTTON_MINUS) old_pad|=WPAD_BUTTON_MINUS;
						}
					 else
					   {px=-100;py=-100;}

				
				if(mode==0 || mode==100)
				{

				if(new_pad & WPAD_BUTTON_B)
					{
					if(mode) signal2=-2;
					else break;
					}
				}


				if(nfiles>0) if(new_pad & WPAD_BUTTON_A) 
				{
				
					if(!files[posfile].is_directory) 
						{
						
						if(is_ext(files[posfile].name,".png") && (mode==0 || mode==100))
							{
							if(texture_png!=NULL && mem_png!=NULL)
								{
								
								if(mode==0) mode=1;
								else
									{
									u8 discid[7];

									memcpy(discid,header->id,6); discid[6]=0;

									memset(disc_conf,0,256*1024);
									global_GetProfileDatas(discid, disc_conf);
									disc_conf[0]='H'; disc_conf[1]='D'; disc_conf[2]='R';disc_conf[3]=((len_png+8+1023)/1024)-1;
									
									memcpy(&disc_conf[8], mem_png, len_png);

									global_SetProfileDatas(discid, disc_conf);

									Screen_flip();

									if(mem_png!=NULL) free(mem_png);mem_png=NULL;
									if(texture_png!=NULL) free(texture_png);texture_png=NULL;
									break;

									}
								}
							else mode=0;
								
							}
						}
							
					  else if(mode==0)
						{
						  signal=1;
		                snd_fx_yes();
						 // cambio de device
						 if(files[posfile].name[0]=='.' && files[posfile].name[1]=='[')
							{
							
							if(files[posfile].name[2]=='U') 
								{
								
								current_device=1;
								sprintf(path_file, "ud:/");
								read_list_file(NULL, flag);
								posfile=0;flag_auto=1;
							
								}
							else
							if(files[posfile].name[2]=='S') 
								{

								current_device=0;
							    sprintf(path_file, "sd:/");
								read_list_file(NULL, flag);
								posfile=0;flag_auto=1;
								
								}
							}
						 else
							{read_list_file((void *) &files[posfile].name[0], flag);posfile=0;flag_auto=1;}
						 
						 
						}
				}

			
				if(mode==0)
				{
				if(!(old_pad  & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN))) f=0;
				
				if(old_pad  & WPAD_BUTTON_UP)
					{
					if(f==0) f=2;
					else if(f & 1) f=2;
					else {f+=2;if(f>40) {f=38;new_pad|=WPAD_BUTTON_UP;}}
					}
				if(old_pad  & WPAD_BUTTON_DOWN)
					{
					if(f==0) f=1;
					else if(!(f & 1)) f=1;
					else {f+=2;if(f>41) {f=39;new_pad|=WPAD_BUTTON_DOWN;}}
					}
				if(new_pad & (WPAD_BUTTON_HOME | WPAD_BUTTON_1 | WPAD_BUTTON_2)) break;
				if(new_pad & WPAD_BUTTON_PLUS)
					{
					get_covers();
					if(mem_png!=NULL) free(mem_png);mem_png=NULL;
					if(texture_png!=NULL) free(texture_png);texture_png=NULL;
					snd_fx_yes();return;
					}

				if((new_pad & WPAD_BUTTON_UP)) {snd_fx_tick();signal=1;posfile--;if(posfile<0) posfile=nfiles-1;}
				if((new_pad & WPAD_BUTTON_DOWN)){snd_fx_tick();signal=1;posfile++;if(posfile>=nfiles) posfile=0;} 
               
				if(signal)
				{
				FILE *fp=NULL;
				signal=0;

				if(mem_png!=NULL) free(mem_png);mem_png=NULL;
				if(texture_png!=NULL) free(texture_png);texture_png=NULL;

				if(!files[posfile].is_directory) 
					{
					
					fp=fopen(files[posfile].name,"r");
					if(fp!=0)
						{
						fseek(fp,0,SEEK_END);
						len_png=ftell(fp);
						fseek(fp,0,SEEK_SET);
						if(len_png<(200*1024-8))
							{
							mem_png=malloc(len_png+128);
							if(mem_png) 
								{n=fread(mem_png,1,len_png,fp);
								if(n<0) {len_png=0;free(mem_png);mem_png=0;} else len_png=n;
								}
							} else len_png=0;
						fclose(fp);
						} else len_png=0;

					if(mem_png)
						texture_png=create_png_texture(&png_texture, mem_png, 0);
					}
				}
				} // mode==0

			}

	

	Screen_flip();
	if(exit_by_reset) break;

	frames2++;
	}

snd_fx_yes();
Screen_flip();
if(mem_png!=NULL) free(mem_png);
if(texture_png!=NULL) free(texture_png);texture_png=NULL;
}

