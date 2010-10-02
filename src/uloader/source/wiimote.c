/*
Copyright (c) 2009 Francisco Muñoz 'Hermes' <www.elotrolado.net>
Copyright (c) 2010 Spaceman Spiff 
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of 
  conditions and the following disclaimer. 
- Redistributions in binary form must reproduce the above copyright notice, this list 
  of conditions and the following disclaimer in the documentation and/or other 
  materials provided with the distribution. 
- The names of the contributors may not be used to endorse or promote products derived 
  from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "wiimote.h"
#include "global.h"
#include "gfx.h"

//---------------------------------------------------------------------------------
/* Wiimote's routines */
//---------------------------------------------------------------------------------

// TODO: Get Rid of this ugly globals
extern int time_sleep;
extern int frames2;
extern int is_16_9;

unsigned temp_pad = 0,
	new_pad = 0,
	old_pad = 0;

WPADData * wmote_datas = NULL;

// wiimote position (temp)
int px = -100;
int py = -100;

// wiimote position emulated for Guitar
int guitar_pos_x, guitar_pos_y;

static int w_index = -1;

int rumble = 0;

void wiimote_rumble(int status)
{
	if (config_file.rumble_off)
		status = 0;
	
	/*
	if(status == 0)
		rumble = 0;
	*/
	
	if(w_index < 0) {
		if (status == 0) {
			WPAD_Rumble(0, status);
			WPAD_Rumble(1, status);
			WPAD_Rumble(2, status);
			WPAD_Rumble(3, status);
		}
		return;
	}
	WPAD_Rumble(w_index, status);
}

void make_rumble_off()
{
	int n;
	rumble = 0;

	for(n = 0; n < 3; n++) {
		usleep(30 * 1000);
		wiimote_rumble(0);
		WPAD_Flush(w_index);
		WPAD_ScanPads();
	}
}

unsigned wiimote_read()
{
	int n;

	int ret=-1;

	unsigned type=0;
	unsigned butt=0;

	wmote_datas=NULL;
	w_index=-1;
	for (n = 0; n < 4; n++) { // busca el primer wiimote encendido y usa ese
		ret = WPAD_Probe(n, &type);

		if(ret >= 0) {
			
			butt = WPAD_ButtonsHeld(n);
			wmote_datas = WPAD_Data(n);
			w_index = n;
			break;
		}
	}

	if (n == 4)
		butt = 0;

	temp_pad = butt;

	new_pad = temp_pad & (~old_pad);
	old_pad = temp_pad;

	if (new_pad) {
		time_sleep = TIME_SLEEP_SCR;
		SetVideoSleep(0);
	}

	return butt;
}


void wiimote_ir()
{
	px = wmote_datas->ir.x-104*is_16_9;
	py = wmote_datas->ir.y;
	angle_icon = wmote_datas->orient.roll;

	time_sleep = TIME_SLEEP_SCR;
	SetVideoSleep(0);
	
	SetTexture(NULL);
	DrawIcon(px, py, frames2);
}

void wiimote_guitar()
{
	angle_icon=0.0f;

	if (wmote_datas->exp.gh3.js.pos.x>=wmote_datas->exp.gh3.js.center.x + 8) {
		guitar_pos_x += 8;
		if (guitar_pos_x > (SCR_WIDTH+104*is_16_9-16)) 
			guitar_pos_x = (SCR_WIDTH+104*is_16_9-16);
	}

	if (wmote_datas->exp.gh3.js.pos.x <= wmote_datas->exp.gh3.js.center.x - 8) {
		guitar_pos_x -= 8;
		if (guitar_pos_x<16-104*is_16_9) 
			guitar_pos_x = 16-104*is_16_9;
	}
		
	if (wmote_datas->exp.gh3.js.pos.y >= wmote_datas->exp.gh3.js.center.y + 8) {
		guitar_pos_y -= 8;
		if (guitar_pos_y < 16)
			guitar_pos_y = 16;
	}
	if (wmote_datas->exp.gh3.js.pos.y <= wmote_datas->exp.gh3.js.center.y - 8) {
		guitar_pos_y += 8;
		if (guitar_pos_y > (SCR_HEIGHT-16)) 
			guitar_pos_y = (SCR_HEIGHT-16);
	}

	if (guitar_pos_x < -104*is_16_9) 
		guitar_pos_x = 104*is_16_9;

	if (guitar_pos_x > (SCR_WIDTH - 16 + 104 * is_16_9)) 
		guitar_pos_x = (SCR_WIDTH - 16 + 104 * is_16_9);

	if (guitar_pos_y < 0) 
		guitar_pos_y = 0;
	if (guitar_pos_y > (SCR_HEIGHT-16)) 
		guitar_pos_y = SCR_HEIGHT-16;

	if(px != guitar_pos_x || py != guitar_pos_y) {
		time_sleep = TIME_SLEEP_SCR;
		SetVideoSleep(0);
	}

	px = guitar_pos_x; 
	py = guitar_pos_y;

	SetTexture(NULL);
	DrawIcon(px, py, frames2);

	if (new_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN) 
		new_pad |= WPAD_BUTTON_A;
	if (old_pad & WPAD_GUITAR_HERO_3_BUTTON_GREEN)
		old_pad |= WPAD_BUTTON_A;

	if (new_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) 
		new_pad |= WPAD_BUTTON_B;
	if (old_pad & WPAD_GUITAR_HERO_3_BUTTON_RED) 
		old_pad |= WPAD_BUTTON_B;

	if (new_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW)
		new_pad |= WPAD_BUTTON_1;
	if (old_pad & WPAD_GUITAR_HERO_3_BUTTON_YELLOW) 
		old_pad |= WPAD_BUTTON_1;

	if (new_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS)
		new_pad |= WPAD_BUTTON_MINUS;
	if (old_pad & WPAD_GUITAR_HERO_3_BUTTON_MINUS)
		old_pad |= WPAD_BUTTON_MINUS;

	if (new_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) 
		new_pad |= WPAD_BUTTON_PLUS;
	if (old_pad & WPAD_GUITAR_HERO_3_BUTTON_PLUS) 
		old_pad |= WPAD_BUTTON_PLUS;
}


void wiimote_classic()
{
	angle_icon = 0.0f;

	if (wmote_datas->exp.classic.ljs.pos.x >= wmote_datas->exp.classic.ljs.center.x + 8) {
		guitar_pos_x += 8;
		if (guitar_pos_x > (SCR_WIDTH+104*is_16_9-16)) 
			guitar_pos_x = (SCR_WIDTH + 104*is_16_9 - 16);
	}
	if (wmote_datas->exp.classic.ljs.pos.x <= wmote_datas->exp.classic.ljs.center.x - 8) {
		guitar_pos_x -= 8;
		if (guitar_pos_x < 16 - 104*is_16_9) 
			guitar_pos_x = 16 - 104*is_16_9;
	}
		
	if (wmote_datas->exp.classic.ljs.pos.y >= wmote_datas->exp.classic.ljs.center.y + 8) {
		guitar_pos_y -= 8;
		if (guitar_pos_y < 16)
			guitar_pos_y = 16;
	}
	if (wmote_datas->exp.classic.ljs.pos.y <= wmote_datas->exp.classic.ljs.center.y - 8) {
		guitar_pos_y += 8;
		if (guitar_pos_y > (SCR_HEIGHT - 16))
			guitar_pos_y = (SCR_HEIGHT - 16);
	}
 
	if (guitar_pos_x < -104*is_16_9) 
		guitar_pos_x = 104 * is_16_9;
	if (guitar_pos_x > (SCR_WIDTH-16+104*is_16_9))
		guitar_pos_x = (SCR_WIDTH - 16 + 104*is_16_9);
	if (guitar_pos_y < 0)
		guitar_pos_y = 0;
	if (guitar_pos_y > (SCR_HEIGHT - 16))
		guitar_pos_y = SCR_HEIGHT - 16;

	if (px != guitar_pos_x || py != guitar_pos_y) {
		time_sleep = TIME_SLEEP_SCR;
		SetVideoSleep(0);
	}

	px = guitar_pos_x; 
	py = guitar_pos_y;


	SetTexture(NULL);
	DrawIcon(px, py, frames2);

	if (new_pad & WPAD_CLASSIC_BUTTON_UP)
		new_pad |= WPAD_BUTTON_UP;
	if (old_pad & WPAD_CLASSIC_BUTTON_UP) 
		old_pad |= WPAD_BUTTON_UP;

	if (new_pad & WPAD_CLASSIC_BUTTON_DOWN) 
		new_pad |= WPAD_BUTTON_DOWN;
	if (old_pad & WPAD_CLASSIC_BUTTON_DOWN)
		old_pad |= WPAD_BUTTON_DOWN;

	if (new_pad & WPAD_CLASSIC_BUTTON_LEFT)
		new_pad |= WPAD_BUTTON_LEFT;
	if (old_pad & WPAD_CLASSIC_BUTTON_LEFT) 
		old_pad |= WPAD_BUTTON_LEFT;

	if (new_pad & WPAD_CLASSIC_BUTTON_RIGHT) 
		new_pad |= WPAD_BUTTON_RIGHT;
	if (old_pad & WPAD_CLASSIC_BUTTON_RIGHT) 
		old_pad |= WPAD_BUTTON_RIGHT;

	if (new_pad & WPAD_CLASSIC_BUTTON_A) 
		new_pad |= WPAD_BUTTON_A;
	if (old_pad & WPAD_CLASSIC_BUTTON_A) 
		old_pad |= WPAD_BUTTON_A;

	if (new_pad & WPAD_CLASSIC_BUTTON_B) 
		new_pad |= WPAD_BUTTON_B;
	if (old_pad & WPAD_CLASSIC_BUTTON_B) 
		old_pad |= WPAD_BUTTON_B;

	if (new_pad & WPAD_CLASSIC_BUTTON_X) 
		new_pad |= WPAD_BUTTON_1;
	if (old_pad & WPAD_CLASSIC_BUTTON_X)
		old_pad |= WPAD_BUTTON_1;

	if (new_pad & WPAD_CLASSIC_BUTTON_Y) 
		new_pad |= WPAD_BUTTON_2;
	if (old_pad & WPAD_CLASSIC_BUTTON_Y)
		old_pad |= WPAD_BUTTON_2;

	if (new_pad & WPAD_CLASSIC_BUTTON_HOME) 
		new_pad |= WPAD_BUTTON_HOME;
	if (old_pad & WPAD_CLASSIC_BUTTON_HOME)
		old_pad |= WPAD_BUTTON_HOME;

	if (new_pad & WPAD_CLASSIC_BUTTON_PLUS)
		new_pad |= WPAD_BUTTON_PLUS;
	if (old_pad & WPAD_CLASSIC_BUTTON_PLUS)
		old_pad |= WPAD_BUTTON_PLUS;

	if (new_pad & WPAD_CLASSIC_BUTTON_MINUS)
		new_pad |= WPAD_BUTTON_MINUS;
	if (old_pad & WPAD_CLASSIC_BUTTON_MINUS)
		old_pad |= WPAD_BUTTON_MINUS;

}
