
/*  PINTOR for GP32
    Copyright (C) 2005  Hermes/PS2Reality 

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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <gccore.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include "screen.h"
#include "asndlib.h"

#define WIDTH	320
#define HEIGHT	240



#define BORDER	10

#define ticks_to_msecs(ticks)  (ticks/TB_TIMER_CLOCK)

extern WPADData * wmote_datas;

extern unsigned gettick();

unsigned GetTicks()
{
return ticks_to_msecs(gettick());
}

void Wait_Event()
{

}



// VARIABLES DEL PROGRAMA


#define PAD_RIGHT WPAD_BUTTON_DOWN
#define PAD_LEFT WPAD_BUTTON_UP
#define PAD_DOWN WPAD_BUTTON_LEFT
#define PAD_UP WPAD_BUTTON_RIGHT



int PADEVENT=0; // si 1 se puede leer el PAD



/****************************************************************************************************************************************/
// PANTALLA
/****************************************************************************************************************************************/

extern unsigned char msx[];  // define la fuente externa usada para dibujar letras y numeros 
unsigned *video=NULL;       // memoria que almacena la pantalla virtual
unsigned *video_text=NULL;  // memoria que almacena la textura
GXTexObj text_pintor;

#define SIZEVIDEO 4*256*192 // tamaño en bytes de la pantalla vir.
#define SCANVIDEO 256       // ancho de la pantalla vir.
#define DESPL8VID 11        // usado para ajustar Y a scans (se utilizan graficos de 8x8 pixeles, 8*SCANVIDEO=2048=2^11
#define ANCHO 32            // ancho de la pantalla en bloques de 8 pixeles
#define ALTO 24             // alto d ela pantalla en bloques de 8 pixeles
#define ALTO2 22            // se usa para presentar la puntuacion, vidas, etc en pantalla

unsigned COLORFONDO=0x80804000; // color de fondo (vaya noticia :P)



void ClearVideo(unsigned val) // se usa para 'borrar' la pantalla virtual con un color
{
int n;
for(n=0;n<SIZEVIDEO/4;n++)
	{
	video[n]=val;
	}
}

void DrawScreen() // funcion evento que dibuja todos los elementos de la pantalla virtaul al LCD
{
int n,m;
unsigned *p,c,c2;
volatile unsigned *v;
int cx1,cy1,cy2;


c2=0xffff0000;
cy1=0,cy2=0;
for(n=0;n<HEIGHT;n++) // convierte una superficie de 256x192 32 bits a 320x240 16 bits de la pantalla
	{
	p=video_text + n*WIDTH;
	v=(unsigned *) &video[cy2<<8];
	cx1=0;
	c=*v;
	if(c==0 || ((c>>24) & 255)<0x10) c=c2;
	for(m=0;m<WIDTH;m++) 
		{
		*p++=c;
		cx1+=256;
		if(cx1>=320) {v++;cx1-=320;c=*v;if(c==0 || ((c>>24) & 255)<0x10) c=c2;
					  }
		}
	cy1+=192;if(cy1>=240) {cy1-=240;cy2++;}
	}


CreateTexture(&text_pintor, TILE_SRGBA8, video_text, WIDTH, HEIGHT, 0);

SetTexture(NULL);
DrawFillBox(-128, 0, SCR_WIDTH+256, SCR_HEIGHT, 0, 0xffff0000);

SetTexture(&text_pintor);
DrawFillBox(8, 8, SCR_WIDTH-16, SCR_HEIGHT-16, 0, 0xffffffff);
SetTexture(NULL);
/*#define WIDTH	320
#define HEIGHT	240*/
	
}

void v_putchar( unsigned x, unsigned y, unsigned color, unsigned char ch) // rutina usada para dibujar caracteres (coordenadas de 8x8)
{
   int 	i,j,v;
   unsigned char	*font;
  
   if(x>=ANCHO || y>=ALTO) return;
   v=(y<<DESPL8VID);
   font = &msx[ (int)ch * 8];
   for (i=0; i < 8; i++, font++)
		{  
		 for (j=0; j < 8; j++)
          {
          if ((*font & (128 >> j)))
			  { 
               video[v+(((x<<3)+j))]=color;
			  }
			else video[v+(((x<<3)+j))]=0;
          }
	  v+=SCANVIDEO;
	 }
 
}

// display array of chars

void v_putcad(int x,int y,unsigned color,char *cad)  // dibuja una cadena de texto
{
	
while(cad[0]!=0) {v_putchar(x,y,color,cad[0]);cad++;x++;}
}


/****************************************************************************************************************************************/
//  rutinas y definicion de UDG (User Defined Graphic) (8x8 pixeles)
/****************************************************************************************************************************************/

// paleta de conversion de caracter/color

unsigned PALETA[10]={0x0,0xff0000ff,0xff00ff00,0xff00ffff,0xffff0000,0xffff00ff,0xffffff00,0xffffffff,0xff000000,1};

void SetUDG(unsigned x,unsigned y,unsigned char *punt) // dibuja un UDG
{
int n,m,v;
unsigned col;
if(x>=ANCHO || y>=ALTO) return;
v=(y<<DESPL8VID);

for(n=0;n<8;n++)
	{
	for(m=0;m<8;m++)
		{
		col=(unsigned )*punt++;
		col-=48;
		col=PALETA[col];
		//if(col==1) col=COLORFONDO;
		if(col!=0)
        video[v+(((x<<3)+m))]=col;
		}
	v+=SCANVIDEO;
	}
}

// lista de UDG's usados en el juego
unsigned char gdu_sprite[20][8][8]={
{
"33388333",
"33388333",
"33383383",
"38888833",
"83383333",
"33838833",
"33833833",
"38333383",
},
{
"99999999",
"99999999",
"99999999",
"99999999",
"99999999",
"99999999",
"99999999",
"99999999",
},
{
"77777777",
"77777777",
"77777777",
"77777777",
"77777777",
"77777777",
"77777777",
"77777777",
},
{
"33333333",
"33333333",
"33333333",
"33333333",
"33333333",
"33333333",
"33333333",
"33333333",
},
{
"33388333",
"33388333",
"38338333",
"33888883",
"33338338",
"33883833",
"33833833",
"38333383",
},
{
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
"22222222",
},
{
// 6
"00001100",
"10111110",
"11111011",
"01111111",
"01111000",
"00110111",
"00111110",
"00001100",
},
{
"00001100",
"00111110",
"01111011",
"01111111",
"01111000",
"11110001",
"10111110",
"00001100",
},
{
// 8
"00110000",
"01111101",
"11011111",
"11111110",
"00011110",
"11101100",
"01111100",
"00110000",
},
{
"00110000",
"01111100",
"11011110",
"11111110",
"00011110",
"10001111",
"01111101",
"00110000",
},


};


/****************************************************************************************************************************************/
// definicion de mapas del juego
/****************************************************************************************************************************************/


// definicion d elos mapas: A,B,C,D->representa donde aparecen los bichos, 1,2->caminos
unsigned char map_screens[8][22][32]={

{
////////////////////////////////////
"            2      2            ",
"             2    2             ",
"                                ",
"            22222222            ",
"                                ",
"11111111111                     ",
" 1        11                    ",
" 1         1     1              ",
" 1        11     1              ",
" 1111111111   111111111         ",
" 1               1          1 11",
" 1  2    1 11111 1    1111  111 ",
" 1       111   1 1   11  11 1   ",
" 1  1    1     111   1    1 1   ",
" 1111    1     1 1   1    111   ",
" 1  1    1     1 1   1    1 1   ",
" 1  11   1     1 11  11  11 1   ",
" A   1111D     1  11B11111  C   ",
"                                ",
"22222222222222222222222222222222",
"22222222222222222222222222222222",
"                                ",
},
{
//////////////////////////////////
"A1111111                1111111D",
"1      1                1      1",
"1      11111111$111111111      1",
"1      1                1      1",
"1      1                1      1",
"1      111111111111111111      1",
"1      1    1      1    1      1",
"11111111    1      1    11111111",
"  1  1    111111111111    1  1  ",
"  1  1    1          1    1  1  ",
"  1  1    1          1    1  1  ",
"  1  1    1          1    1  1  ",
"  1  1    1          1    1  1  ",
"  1  1    111111111111    1  1  ",
"11111111    1      1    11111111",
"1      1    1      1    1      1",
"1      111111111111111111      1",
"1      1                1      1",
"1      1                1      1",
"1      111111111111111111      1",
"1      1                1      1",
"C1111111                1111111B",
},
{
////////////////////////////////////
"   A11111              11111D   ",
"   1    1              1    1   ",
"111111111111        111111111111",
"1          1        1          1",
"1          1        1          1",
"111111111111        111111111111",
"   1    1   11111111   1    1   ",
"   1    1   1      1   1    1   ",
"   1    1   1      1   1    1   ",
"1111111111111111$111111111111111",
"1                              1",
"1                              1",
"1                              1",
"11111111111111111111111111111111",
"   1        1      1        1   ",
"   1        1      1        1   ",
"   1        1      1        1   ",
"   1        1      1        1   ",
"   1        1      1        1   ",
"   C111111111      111111111B   ",
"                                ",
"                                ",
////////////////////////////////////
},
{
////////////////////////////////////
"A1111                11111      ",
"1   1                1   1      ",
"1   1                1   1      ",
"1   111111111111111111111111111D",
"1   1                1   1     1",
"1   1                1   1     1",
"1   1111111111111$11111111111111",
"1   1                1   1      ",
"1   1                1   1      ",
"1   1         1111111111111111  ",
"1   1         1              1  ",
"1   1         1              1  ",
"1   1         1              1  ",
"1   1   111111111111111111111111",
"1   1   1                      1",
"1   1   1                      1",
"1   1111111111111111111111111111",
"1   1                       1   ",
"1   1                       1   ",
"1   111111111111111111111111B   ",
"1   1    1            1         ",
"C1111    11111111111111         ",
},
{
////////////////////////////////////
"    A11111      111111111111D   ",
"    1    1      1           1   ",
" 111111111  11111111111111111111",
" 1       1  1      1      1    1",
" 1       1  1      1      1    1",
" 111111111  1      1      1    1",
"    1    1  1      1      1    1",
"    1    1  1      1      1    1",
"    1    1  1      1111111111111",
"    1    1  1      1           1",
"    1    1  1      1           1",
"    1    1  1      1           1",
"    1    1  1111$111111111111111",
"    1    1  1      1      1    1",
"    1    1  1      1      1    1",
"    1    1  1      1      1    1",
"    1    1  1      1      1    1",
"11111111111111111111111111111111",
"1                            1  ",
"1                            1  ",
"1                            1  ",
"C1111111111111111111111111111B  ",
////////////////////////////////////
},
{
////////////////////////////////////
"A11111111   11111111   11111111D",
"1       1   1      1   1       1",
"1       1   1      1   1       1",
"1       1 111111111111 1       1",
"111111111 1          1 111111111",
"    1     1          1     1    ",
"1111111111111111$111111111111111",
"1         1          1         1",
"1         1          1         1",
"1         1          1         1",
"1         111111111111         1",
"1         1  1    1  1         1",
"1         1  1    1  1         1",
"11111111111  1    1  11111111111",
"  1     1    1    1    1     1  ",
"  1     1111111111111111     1  ",
"  1     1              1     1  ",
"  1     1              1     1  ",
"11111111111111111111111111111111",
"1          1        1          1",
"1          1        1          1",
"C11111111111        11111111111B",
////////////////////////////////////
},
{
////////////////////////////////////
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
////////////////////////////////////
},


};
// numero de anillos por mapa
int map_screen_rings[10]={66,11,10,10,12,13,0,0,0,0};

/****************************************************************************************************************************************/
// variables del juego
/****************************************************************************************************************************************/


int pausa=0; // 1- juego pausado

unsigned puntos=0; // si te tengo que explicar esto, mejor lo dejamos :P
unsigned hiscore=0;
int vidas=3;
int nbichos=2;

int rings=0; // anillos completados
unsigned protadir=0; // para donde miramos
int bichodir[4]={0,0,0,0}; // para donde miran los bichos

int ACTMAP=0; // mapa actual
unsigned char map_screen[24][32]; // aqui se trabaja (se copia el mapa aqui para ello)


//  control de eventos de sonido
int message_scroll=0;
int message_effect1=0;
int message_effect2=0;
int message_camina[4]={0,0,0,0};
int piok=0;
int message_piok=0; // me han 'picado' me temo :D

/****************************************************************************************************************************************/
// manejo y control de mapas del juego
/****************************************************************************************************************************************/


void copyMAP()
{
int n,m;
unsigned char c;
protadir=0;

rings=0;
piok=0;
message_piok=0;
for(n=0;n<4;n++) 
	{bichodir[n]=0;
    message_camina[n]=0;
    }
 message_effect2=0;
message_effect1=0;
message_scroll=0;
ClearVideo(0);

switch(ACTMAP)
	{
	case 0:COLORFONDO=0x80800000;PALETA[2]=0x80ff8000;break;
	case 2:COLORFONDO=0x80004020;PALETA[2]=0x8000ff80;break;
	case 3:COLORFONDO=0x80404040;PALETA[2]=0x808000ff;break;
	case 4:COLORFONDO=0x80404000;PALETA[2]=0x8000ff00;break;
	case 5:COLORFONDO=0x80402040;PALETA[2]=0x80ff00ff;break;
	default:COLORFONDO=0x80804000;PALETA[2]=0x80ff8000;
	}

//memcpy(map_screen,&map_screens[ACTMAP][0][0],32*22);
for(n=0;n<22;n++)
{
for(m=0;m<32;m++)
	{
    c=map_screens[ACTMAP][n][m];
    // esto sucede la primera vez
	if(c==' ') c=0;
	else
	if(c=='1') c=1;
	else
	if(c=='2') c=2;
	else
	if(c=='$') c=130;
	else
	if(c=='A') c=65;
	else
	if(c=='B' && nbichos>1) c=33;
    else
	if(c=='C' && nbichos>2) c=17;
    else
	if(c=='D' && nbichos>3) c=9;
	else c=1;
	map_screen[n][m]=c;
	}
}
}

int AnalizeMAP();

void scrollMAP() // efecto de scroll lateral de cuando completamos un mapa
{
int n,m;

for(n=0;n<22;n++)
	{
	for(m=30;m>=0;m--)
		{
		map_screen[n][m+1]=map_screen[n][m];
		}
	map_screen[n][0]=0;
	}
}



void DrawMAP() // dibuja el mapa y actualiza variables
{
int n,m,v;
unsigned char c;
char cad[256];
static int paso=0,count=0,count2=0;
count2++;
if(count2>=50) {count2=0;}
count++;
if(count>=32){count=0;paso^=1;}


if(rings>=map_screen_rings[ACTMAP] || piok || pausa || ACTMAP==0) count2=20; // evita efectos de sonido
else
rings+=AnalizeMAP();
//if(rings>map_screen_rings[ACTMAP]) return ; // si se han comppletado los anillos retorna

for(n=0;n<22;n++)
{Wait_Event();
for(m=0;m<32;m++)
	{
    c=map_screen[n][m];
    // esto sucede la primera vez
	/*if(c=='$') {c='2' | 128;map_screen[n][m]=c; }
	if(c=='A') {c='1' | 128;map_screen[n][m]=c; }*/
	// pinta fondo mapa
	
	v=(c & 7)+1;
	
	SetUDG(m,n,(unsigned char *) &gdu_sprite[v][0][0]);
	// mi personaje esta aqui?
	if(c & 128) {if((c & 120) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}SetUDG(m,n,(unsigned char *) &gdu_sprite[4*(protadir==1 || protadir==4)][0][0]);c=(c & 0xfc) | 130;map_screen[n][m]=c;}
	// veo bichos!!
	if(c & 64) {if((c & 128) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
		       if(count2==0) message_camina[0]=(m-16)+(m==16);SetUDG(m,n,(unsigned char *) &gdu_sprite[6+paso+2*(bichodir[0]==1 || bichodir[0]==4)][0][0]);}
	if(c & 32) {if((c & 128) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
		       if(count2==0) message_camina[1]=(m-16)+(m==16);SetUDG(m,n,(unsigned char *) &gdu_sprite[6+paso+2*(bichodir[1]==1 || bichodir[1]==4)][0][0]);}
	if(c & 16) {if((c & 128) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
		       if(count2==0) message_camina[2]=(m-16)+(m==16);SetUDG(m,n,(unsigned char *) &gdu_sprite[6+paso+2*(bichodir[2]==1 || bichodir[2]==4)][0][0]);}
	if(c & 8) {if((c & 128) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
		      if(count2==0) message_camina[3]=(m-16)+(m==16);SetUDG(m,n,(unsigned char *) &gdu_sprite[6+paso+2*(bichodir[3]==1 || bichodir[3]==4)][0][0]);}
	}
}


if(ACTMAP!=0) // si no es pantalla 0
{if(puntos>hiscore) hiscore=puntos;

sprintf(cad,"SCORE:%6.6u HI:%6.6u ",puntos,hiscore); 
v_putcad(0,23,0xffffFFff,cad);
v_putcad(23,23,0xffffFFff,"LIVES:");
for(n=0;n<vidas;n++) SetUDG(29+n,23,(unsigned char *) &gdu_sprite[0]);
}
else
{
v_putcad(3,22,0xffffffff,"by Hermes/PS2REALITY 2005");
v_putcad(8,23,0xffffffff,"uLoader Version");
}

}

int subanalizeMAP(int x,int y) // llamada por analizeMAP(), me gustan los nombres rarus
{
int n,m;

int xx1,yy1,xx2,yy2;

if(x>29 || y>19) return 0;
if((map_screen[y][x+1] & 3)!=2 || (map_screen[y+1][x] & 3)!=2) return 0;
Wait_Event();
xx1=xx2=x;yy1=yy2=y;
// busca una bifurcacion hacia abajo
for(m=x+1;m<32;m++)
	{
	if((map_screen[y][m] & 3)==2)
	{
	if((map_screen[y+1][m] & 3)==1) break;
	if((map_screen[y+1][m] & 3)==2) {xx2=m;break;}
	} else break;
	}
// busca una bifurcacion a la derecha
for(m=y+1;m<22;m++)
	{
	if((map_screen[m][x] & 3)==2)
	{
	if((map_screen[m][x+1] & 3)==1) break; 
	if((map_screen[m][x+1] & 3)==2) {yy2=m;break;}
	} else break;
	}
if(xx1==xx2 || yy1==yy2) return 0;

for(m=xx1;m<=xx2;m++)
	{
	if((map_screen[yy2][m] & 3)!=2) break;
	}
if(m<=xx2) return 0; // no esta completo el circulo
for(m=yy1;m<=yy2;m++)
	{
	if((map_screen[m][xx2] & 3)!=2) break;
	}
if(m<=yy2) return 0; // no esta completo el circulo
// recuadro a iluminar
for(n=yy1+1;n<yy2;n++)
	{
	for(m=xx1+1;m<xx2;m++)
		{
		if(map_screen[n][m]==4) return 0; // ya estaba pintado
		map_screen[n][m]=4;
		}
	}
message_effect1=1; // emite un sonido
return 1;
}

int AnalizeMAP() // rutina que sirve para que el programa se entere de lo que pasa en el mapa (busca anillos)
{
int numrec=0;
int n,m;

for(n=0;n<22;n++)
{
for(m=0;m<32;m++)
	{
	if((map_screen[n][m] & 3)==2)  
		{
		if(subanalizeMAP(m,n)) numrec++;
		}
	}
}
puntos+=10*numrec*nbichos;
return numrec;
}

void Borra_Prota()
{
int n,m;
for(n=0;n<22;n++)
{
for(m=0;m<32;m++)
	{
    map_screen[n][m]&=127;
	}
}
}

void Move_Bicho(unsigned nbi) // mola el spanglish :D
{
int n,m;
unsigned char c,mask,mask2;
int testmove=0,contmove=0;
switch(nbi)
{
case 1: mask=32;mask2=255-32;break;
case 2: mask=16;mask2=255-16;break;
case 3: mask=8;mask2=255-8;break;
default: mask=64;mask2=255-64;
}
//busca al bichin :P
for(n=0;n<22;n++)
{
for(m=0;m<32;m++)
	{
    c=map_screen[n][m];
	if(c & mask) // detectado bicho
		{
		do
		{Wait_Event();
		if(bichodir[nbi]==0)
			{bichodir[nbi]=(rand() & 1)+1; message_effect2=1;}
         if(bichodir[nbi]==1 || bichodir[nbi]==2)
			{
			if(n>0) if(map_screen[n-1][m] & 3) if((rand() & 15)<4) {bichodir[nbi]=4;message_effect2=1;} 
			if(n<21) if(map_screen[n+1][m] & 3) if((rand() & 15)<4) {bichodir[nbi]=8;message_effect2=1;} 
			}
         else
			{
			{
			if(m>0) if(map_screen[n][m-1] & 3) if((rand() & 15)<4) {bichodir[nbi]=1;message_effect2=1;} 
			if(m<31) if(map_screen[n][m+1] & 3) if((rand() & 15)<4) {bichodir[nbi]=2;message_effect2=1;}
			}
			}
		if(bichodir[nbi] & 1)
			{
			if(m>0) if(map_screen[n][m-1] & 3) {map_screen[n][m]&=mask2;map_screen[n][m-1]|=mask;testmove++;}
			}
		else
		if(bichodir[nbi] & 2)
			{
			if(m<31) if(map_screen[n][m+1] & 3) {map_screen[n][m]&=mask2;map_screen[n][m+1]|=mask;testmove++;}
			}
		else
		if(bichodir[nbi] & 4)
			{
			if(n>0) if(map_screen[n-1][m] & 3) {map_screen[n][m]&=mask2;map_screen[n-1][m]|=mask;testmove++;}
			}
		else
		if(bichodir[nbi] & 8)
			{
			if(n<21) if(map_screen[n+1][m] & 3) {map_screen[n][m]&=mask2;map_screen[n+1][m]|=mask;testmove++;}
			}
		contmove++;
		if(contmove>32 && testmove==0) { // anticuelgue
			           contmove=0;
					   bichodir[nbi]=0;
						}
		}while(testmove==0);
	if((c & 128) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
	return;
		}
		
	}
}

}


void Move_Prota(unsigned pad)
{
int n,m;
unsigned char c;
for(n=0;n<nbichos;n++) Move_Bicho(n);

//busca a mi personaje
for(n=0;n<22;n++)
{Wait_Event();
for(m=0;m<32;m++)
	{
    c=map_screen[n][m];
	if(c & 128) // detectado protagonista
		{
		if((c & 120) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
		if((pad & PAD_LEFT) && protadir!=1)
			{
			if(m>0) if(map_screen[n][m-1] & 3) protadir=1;
			}
		else
	    if((pad & PAD_RIGHT) && protadir!=2)
			{
			if(m<31) if(map_screen[n][m+1] & 3)protadir=/*(protadir  & 12) |*/ 2;
			}	
		else
       if((pad & PAD_UP) && protadir!=4)
			{
			if(n>0) if(map_screen[n-1][m] & 3)protadir=/*(protadir  & 3) |*/ 4;
			}
		else
	    if((pad & PAD_DOWN) && protadir!=8)
			{
			if(n<21) if(map_screen[n+1][m] & 3) protadir=/*(protadir  & 3) |*/ 8;
			}	
			

//protadir=1;
		if(protadir & 1)
			{
			if(m>0) if(map_screen[n][m-1] & 3) {map_screen[n][m]&=127;map_screen[n][m-1]=(map_screen[n][m-1] & 0xfc) | 130;}
			}
		else
		if(protadir & 2)
			{
			if(m<31) if(map_screen[n][m+1] & 3) {map_screen[n][m]&=127;map_screen[n][m+1]=(map_screen[n][m+1] & 0xfc) | 130;}
			}
		else
		if(protadir & 4)
			{
			if(n>0) if(map_screen[n-1][m] & 3) {map_screen[n][m]&=127;map_screen[n-1][m]=(map_screen[n-1][m] & 0xfc) | 130;}
			}
		else
		if(protadir & 8)
			{
			if(n<21) if(map_screen[n+1][m] & 3) {map_screen[n][m]&=127;map_screen[n+1][m]=(map_screen[n+1][m] & 0xfc) | 130;}
			}
	if((c & 120) && piok==0){piok=16384+(n<<5)+m;message_piok=1;}
	return;
		}
		
	}
}

}


/****************************************************************************************************************************************/
//  SONIDO (generación)
/****************************************************************************************************************************************/



int time1_sound=0;
int time2_sound=0;
int time3_sound=0;
int time4_sound=0;
int count_sound=0;
int count2_sound=0;
int vol_sound=0;
int mod_sound=1;

/* set_sonido
vol=volumen (0-0x7fff)
hz=frecuencia en hz del sonido
time_on=tiempo en ms que se reproduce la onda
time_off=tiempo en ms deonda en off (intermitencia)
times=numero de veces que se repite (-1=infinito)
*/

void set_sonido(int vol,int hz,int time_on,int time_off,int times) // programa un sonido
{
int a;
time4_sound=0;
//time_on>>=1;
//time_off>>=1;
vol_sound=vol;
a=44100/hz;
if(a==0) a=2;
time1_sound=a;
time_on=(11025*time_on)/(a*1000);
if(time_on==0) time_on=1;
time2_sound=time_on;
time3_sound=(11025*time_off)/1000;
time4_sound=times;

}

// mezclador de audio de las SDL llamado como eventos

void mixaudio(void *unused, u8 *stream, int len)
{

int n;

int mid;
short *pu;
mid=time1_sound>>1;
pu=(short *) stream;
for(n=0;n<(len>>2);n++)
	{
	if(time4_sound!=0)
		{
		if(count2_sound<time2_sound)
			{
			if(count_sound==mid) {pu[0]=pu[1]=0;pu+=2;} else 
			if(count_sound<mid) {pu[0]=pu[1]=vol_sound;pu+=2;} else {pu[0]=pu[1]=-vol_sound;pu+=2;}
			count_sound++;if(count_sound>=time1_sound) {count_sound=0;count2_sound++;}
			}
		else
			{
			if(count_sound<time3_sound) {pu[0]=pu[1]=0;pu+=2;count_sound++;} 
			else {count_sound=count2_sound=0;if(time4_sound>0) time4_sound--;}
			}
		}
	}

}
   

/****************************************************************************************************************************************/
//  MAIN
/****************************************************************************************************************************************/

unsigned oldtime=0; // se usa para conocer el tiempo transcurrido en ms
int count20ms=0; // variable rara vinculada al temporizador

//int temp_keys,new_keys=0,old_keys=0; // usadas para leer el pad 

// from uLoader
extern unsigned temp_pad,new_pad,old_pad;
extern int exit_by_reset;
extern unsigned wiimote_read();
extern void wiimote_rumble(int status);
extern int time_sleep;

static short pcmout[2][2048] ATTRIBUTE_ALIGN(32);
int pcm_flip=0;

static void audio_add_callback(int voice)
{
	if(ASND_TestVoiceBufferReady(1)==1)
	{
	//void mixaudio(void *unused, u8 *stream, int len)
	mixaudio(NULL,  (void *) &pcmout[pcm_flip][0],2048);
	if(ASND_AddVoice(1, (void *) &pcmout[pcm_flip][0],2048)==0) 
		{
		pcm_flip^=1;
		}
	}
}

int pintor()
{

unsigned time1,n;
    

video=malloc(SIZEVIDEO);
video_text=memalign(32,WIDTH*HEIGHT*4);

ASND_SetVoice(1, VOICE_STEREO_16BIT, 11025*2,0, (void *) &pcmout[pcm_flip][0], 2048, 63, 63, audio_add_callback);
pcm_flip^=1;


/*
// ajuste
set_sonido(127<<8,440,10000,0,1);
while(time4_sound) Wait_Event();
return 0;
*/

/////////////////////////////////////////////////////////////
do
{
ACTMAP=0;
vidas=3;
puntos=0;
pausa=0;

do
{
copyMAP();

new_pad=old_pad=0;

while(1==1) // bucle principal donde se desarrolla toda la accion :O
{

if(!time4_sound && !pausa) 
	{if(message_effect1) {mod_sound=0;message_effect1=0; set_sonido(127<<8,1200,125,0,1);}
	 else
	if(message_effect2) {mod_sound=1;message_effect2=0; set_sonido((63<<8),300,75,125,1);}
       else /*if(message_effect1)*/ {mod_sound=1;message_effect1=0;  set_sonido((32<<8),150,60,125,1);}
	}

if(rings>=map_screen_rings[ACTMAP]) break; // pasa a sigueinte pantalla

		
if(PADEVENT) // lectura del PAD por interrupciones :D
	    {
		PADEVENT=0; // anula el evento (lectura cada 20ms, recuerda)

		wiimote_read();

		if(wmote_datas!=NULL && wmote_datas->exp.type==WPAD_EXP_CLASSIC)
			{
 
			if(temp_pad & WPAD_CLASSIC_BUTTON_UP) temp_pad|=WPAD_BUTTON_RIGHT;

			if(temp_pad & WPAD_CLASSIC_BUTTON_DOWN) temp_pad|=WPAD_BUTTON_LEFT;
		
			if(temp_pad & WPAD_CLASSIC_BUTTON_LEFT) temp_pad|=WPAD_BUTTON_UP;

			if(temp_pad & WPAD_CLASSIC_BUTTON_RIGHT) temp_pad|=WPAD_BUTTON_DOWN;
			
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
			}
        
		if(exit_by_reset) goto sal_del_juego;

        if(new_pad & WPAD_BUTTON_HOME) {goto sal_del_juego;}
		
		if(ACTMAP==0)
		{
		if((new_pad & WPAD_BUTTON_A)) {pausa=0;rings=500;} // empieza
		// ajusta bichos
		if(new_pad & (WPAD_BUTTON_UP | WPAD_CLASSIC_BUTTON_UP)) {nbichos--;if(nbichos<2) nbichos=2;copyMAP();}
		if(new_pad & (WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_DOWN)) {nbichos++;if(nbichos>4) nbichos=4;copyMAP();}
		}
		// pausa
		
		if(new_pad & WPAD_BUTTON_PLUS) pausa^=1;
		
		 
		 }
time1=GetTicks();
count20ms=(time1-oldtime)/20;
//if(count20ms & 1) {
if(PADEVENT==0) 
	{
	wiimote_rumble(0);

	WPAD_ScanPads();
	
	PADEVENT=1;}
//	}
//if(pausa) count20ms=0;

if(count20ms>8 || pausa)
	{
	oldtime=time1;
	
	if(!pausa) Move_Prota(temp_pad);Wait_Event();DrawMAP();Wait_Event();DrawScreen();Wait_Event();Screen_flip();
	count20ms=0;
	}



Wait_Event();


if(piok) break;
}
n=0;
if(piok==0) 
{ // rutina fin de pantalla
message_scroll=1;
set_sonido(127<<8,200,100,100,-1);mod_sound=0;
//n=100;
while(n<36)
	{
	time1=GetTicks();
	count20ms=(time1-oldtime)/20;
	Wait_Event();
	if(count20ms>2)
	{
	oldtime=time1;
	n++;scrollMAP();
	count20ms=0;
	}
	
	
	DrawMAP();DrawScreen();Wait_Event();Screen_flip();
	}
	
ACTMAP++;mod_sound=1;time4_sound=0;
}
else
	{ 
		 
	Borra_Prota();
	mod_sound=0;

	while(n<26)
	{

	
	time1=GetTicks();   
	count20ms=(time1-oldtime)/20;
	Wait_Event();	
	if(count20ms>2)
	{
	n++;set_sonido(127<<8,1900-300*n/26,50,0,1);

	oldtime=time1;
    map_screen[(piok>>5) & 31][piok & 31]&=127;
    if(((piok>>5) & 31)<21)
		{
		piok+=32;
		map_screen[(piok>>5) & 31][piok & 31]&=0xf8;
		map_screen[(piok>>5) & 31][piok & 31]|=130;
		}
	count20ms=0;
	}
	
	DrawMAP();DrawScreen();Wait_Event();Screen_flip();
	}
	mod_sound=1;time4_sound=0;
	
    vidas--;
	}
	
if(vidas<=0) break;

if(map_screen_rings[ACTMAP]==0) ACTMAP=1;
}while(map_screen_rings[ACTMAP]!=0);

}while(1);

sal_del_juego:

ASND_StopVoice(1);
if(video) free(video); video=NULL;
if(video_text) free(video_text); video_text=NULL;

return 0;
}
