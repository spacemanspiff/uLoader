/*
Copyright (c) 2009 Francisco Muñoz 'Hermes' <www.elotrolado.net>
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

#ifndef GFX_H
#define GFX_H

#ifdef __cplusplus
extern "C" {
#endif


#include <screen.h>
#include <unistd.h>
#include <libpng/pngu/pngu.h>
#include "remote.h"


#ifndef ALTERNATIVE_VERSION
#define BACK_COLOR 0xffffffff //0xffa0a0a0
#else
#define BACK_COLOR 0xffa0a0a0
#endif

extern u32 INK0;
extern u32 INK1;
extern int in_black;

extern int num_partitions;

extern int exit_by_reset;

extern void reset_call();

extern int idioma;

extern char letrero[2][70][64];

extern int mode_disc;

extern int is_16_9;

extern int time_sleep;

extern int use_icon2;

extern Mtx	modelView;

extern char uloader_version[5];

/*-----------------------------------------------------------------------------------------------------------------------------------*/

extern void *background_png;
extern int thread_in_second_plane;
extern char my_perror_error[256];

extern int flag_snow;

extern int scroll_text;

extern int step_button;

extern int x_temp;

extern int px, py;

extern int abort_signal;

extern GXTlutObj palette_icon;
extern GXTexObj text_icon[10];

extern GXTexObj text_button[4], default_game_texture, default_game_texture2, text_background[4], text_background2,text_game_empty[4];
extern GXTexObj text_screen_fx;

extern u32 *screen_fx;

extern GXTexObj text_move_chan;

extern GXTexObj png_texture;

extern void *mem_move_chan;

extern int signal_draw_cabecera2;

extern char cabecera2_str[128];

extern int altdol_frames2;

extern char *down_mess;

extern int down_frame;

/*-----------------------------------------------------------------------------------------------------------------------------------*/

void splash_scr();

void splash_scr_send();

void splash2_scr();

void create_background_text(int seed, int width,int height,unsigned *t);

void draw_snow();

void DrawIcon(int px,int py, u32 frames2);

void draw_text(char *text);

void draw_box_text(char *text);

int Draw_button(int x,int y,char *cad);

int Draw_button2(int x,int y,char *cad, int selected);

void * create_png_texture(GXTexObj *texture, void *png, int repeat);
void * create_png_texture_from_file(GXTexObj *texture, const char *png, int repeat);

void draw_background();

void display_spinner_draw();

void my_perror(char * err);

void display_spinner(int mode, int percent, char *str);

void draw_cabecera2();

void cabecera(char *cab);

void cabecera2(char *str);

void draw_add_game_mess();

extern int seno2(int ang);
extern int coseno2(int ang);

void draw_altdolscr();

void set_text_screen_fx();

void circle_select(int x, int y, char symb, int selected);

void wait_splash_scr();
void down_uload_gfx();

void snd_explo(int voice, int pos);
void happy_new_year(int nyear);

void cluster_warning();



#ifdef __cplusplus
  }
#endif

#endif
