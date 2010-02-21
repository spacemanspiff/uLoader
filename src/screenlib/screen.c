/*
Copyright (c) 2008 Francisco Muñoz 'Hermes' <www.entuwii.net>
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

#include "screen.h"

#include "font.h"
#include "font_b.h"

//extern char *debug_str;

static GXRModeObj	*screenMode;


static void *frameBuffer[2] = {NULL,NULL};
static int frame=0;
static volatile int update_gfx=0,update_scr=0;

/* fonts and s_printf */

static struct tagsizeletter // font size table (use 'sizeletter' as index)
{
 unsigned tx,ty;
} sizelet[10] = {
	{64,64},{32,32},{16,24},{12,16},{16,16},{12,24},{15,24},{128,192},{128,128}//,{userdefined,userdefined} -> see letter_size function
};

int xclip=640; // screen width

int autocenter=0;
unsigned sizeletter=2;    
unsigned bkcolor=0x00000000; 
unsigned color=0xffffffff;
unsigned PX=0,PY=0;
int s_printf_z=0;

void letter_size(u16 tamx, u16 tamy)
{
	sizelet[9].tx=(unsigned) tamx;
	sizelet[9].ty=(unsigned) tamy;
	sizeletter=9;
}

/*********************************************************************************************************************************************************/
/* Font & s_printf                                                                                                                                       */
/*********************************************************************************************************************************************************/

static GXTexObj texObj,texObj2;

static u16 texture[256*256*2] __attribute__ ((aligned (32)));
static u16 texture2[256*256*2] __attribute__ ((aligned (32)));

static int text_font_selected=0;

void SelectFontTexture(int select)
{
text_font_selected=select & 1;
}


void UploadFontTexture()
{
#if 0
u16 *font2=0;
int n,m,l,pos,pos2;
unsigned char *punt;
unsigned short dat;
unsigned color;

int select;

for(select=0;select<2;select++)
	{

	font2=&texture[256*256*select];
	//if(!font2) return;

	update_gfx=1;

	punt=(unsigned char *) &letter[0];


	memset((void *) &font2[0],0,2*256*256);



	for(n=32;n<256;n++)
		{

		pos=((n & 15)<<4)+((n>>4)<<12);

		pos2=0;
	  
		for(m=0;m<16;m++)
			{
			dat=punt[0] | (punt[1]<<8);
			punt+=2;
			
			for(l=0;l<16;l++)
				{
				pos2=(l & 3)+((m & 3)<<2)+((l>>2)<<4)+((m>>2)<<10);
				if(dat & 32768) {color=0xffff; 
								}
								else color=0x0000;
				font2[pos+l]=color;dat<<=1;
				
				if(color && select!=0)
					{
					if(l>0) if(!font2[pos+l-256]) font2[pos+l-256]=0x8000;
					if(l<15) if(!font2[pos+l+256]) font2[pos+l+256]=0x8000;
					if(m>0) if(!font2[pos+l-1]) font2[pos+l-1]=0x8000;
					if(m<15) if(!font2[pos+l+1]) font2[pos+l+1]=0x8000;
					}

				}
			pos+=256;
			}
		
		}

	tiling4x4(&texture[256*256*select],TILE_SRGB5A1 ,256,256);
}

DCFlushRange(texture,256*256*2*2);

GX_InitTexObj(&texObj, texture, 256, 256, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
GX_InitTexObj(&texObj2, &texture[256*256], 256, 256, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
#endif

u16 *font2=0;
int n,m,l,pos;
unsigned char *punt;

unsigned color;
int k;

	font2=&texture[0];

	update_gfx=1;


	punt=(unsigned char *) &font[0];

	memset((void *) &font2[0],0,2*256*256*2);


    k=0;
	for(n=32;n<256;n++)
		{

		pos=((n & 15)<<4)+((n>>4)<<13);
		for(m=0;m<32;m++)
			{
		    for(l=0;l<16;l++)
				{
				
				color=((punt[k>>2]>>((k & 3)<<1)) & 3)*85;/*(*punt++) & ~7*/;
				if(color!=0)
					{
					color>>=3;
					color=32768 | (color<<10) | (color<<5) | color;
					}
				font2[pos+l]=color;
				k++;
				}
			pos+=256;
			}
			
		}

	
tiling4x4(&texture[0],TILE_SRGB5A1 ,256,256*2);
DCFlushRange(texture,256*256*2*2);

GX_InitTexObj(&texObj, texture, 256, 256*2, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
//GX_InitTexObj(&texObj2, texture, 256, 256*2, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);

}

void UploadFontTextureExt(unsigned char *punt)
{

u16 *font2=0;
int n,m,l,pos;
unsigned color;
int k;


	font2=&texture2[0];
	if(!font2) return;

	update_gfx=1;

	memset((void *) &font2[0],0,2*256*256*2);


    k=0;
	for(n=32;n<256;n++)
		{

		pos=((n & 15)<<4)+((n>>4)<<13);
		for(m=0;m<32;m++)
			{
		    for(l=0;l<16;l++)
				{
				
				color=((punt[k>>2]>>((k & 3)<<1)) & 3)*85;/*(*punt++) & ~7*/;
				if(color!=0)
					{
					color>>=3;
					color=32768 | (color<<10) | (color<<5) | color;
					}
				font2[pos+l]=color;
				k++;
				}
			pos+=256;
			}
			
		}

tiling4x4(&texture2[0],TILE_SRGB5A1 ,256,256*2);
DCFlushRange(texture2,256*256*2*2);

GX_InitTexObj(&texObj2, texture2, 256, 256*2, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);

}

#define XX(a) (a)
#define YY(a) (a)
#define ZZ(a) (a)


#define GX_POSITION(x,y,z) GX_Position3s16(x, y, z)

//#define GX_POSITION(x,y,z) GX_Position3f32(x, y, z)

void print_str(u32 x, u32 y, u32 z, u32 color, char *string,u16 tamx, u16 tamy)
{

int i,c,valueanc;
s16 x0, y0, x1, y1;
s32 sx=(s32) x;


update_gfx=1;

//GX_TexModeSync();

GX_ClearVtxDesc();

GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ, GX_S16,	0);
GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);
GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

GX_SetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);


  valueanc=0;
			
   for(i=0;i<strlen(string);i++)
	{if(i>=xclip/tamx) break;

     c=((unsigned char )string[i]) & 0xff;

	 if(c>=32 && sx<(xclip+128))
		{
		x0=((c & 15)<<4);
		x1=x0+16;
		y0=((c & 0xfff0));
		y1=y0+16;
				
        if(bkcolor!=0)	
	     {
         GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
         GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
         GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
	     
		 GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		 
		 GX_POSITION(XX(sx),YY(y),ZZ(z));
         GX_Color1u32(Get_FASTLE(bkcolor));
  
         GX_POSITION(XX(sx+tamx),YY(y),ZZ(z));
         GX_Color1u32(Get_FASTLE(bkcolor));

         GX_POSITION(XX(sx+tamx),YY(y+tamy),ZZ(z));
         GX_Color1u32(Get_FASTLE(bkcolor));
 
         GX_POSITION(XX(sx),YY(y+tamy),ZZ(z));
         GX_Color1u32(Get_FASTLE(bkcolor));

	     GX_End();
	     }
   
			
			
	    if(c!=32)
	     {
		 GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // enable texture
		 GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		 GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		 GX_SetNumChans(1);

         if(text_font_selected==0)
			 GX_LoadTexObj(&texObj, GX_TEXMAP0); // select texture
		 else
			 GX_LoadTexObj(&texObj2, GX_TEXMAP0);
	

         GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 9); // 9 bits of precision
		 x0<<=1;y0<<=1;
		 x1<<=1;y1<<=1;
		 y0+=1;
		 //y1-=1;

	     GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

		 GX_POSITION(XX(sx),YY(y),ZZ(z));
		 GX_Color1u32(Get_FASTLE(color));
         GX_TexCoord2s16(x0,y0);

		 GX_POSITION(XX(sx+tamx),YY(y),ZZ(z));
		 GX_Color1u32(Get_FASTLE(color));
		 GX_TexCoord2s16(x1,y0);


		 GX_POSITION(XX(sx+tamx),YY(y+tamy),ZZ(z));
		 GX_Color1u32(Get_FASTLE(color));
		 GX_TexCoord2s16(x1,y1);

		 GX_POSITION(XX(sx),YY(y+tamy),ZZ(z));
		 GX_Color1u32(Get_FASTLE(color));
		 GX_TexCoord2s16(x0,y1);


		 GX_End();
		}
	 }
				
   sx=sx+tamx;

   if(c=='\n') {sx=0;y=y+tamy;}
   }
   
   PY=y;PX=sx;

   GX_SetBlendMode(GX_BM_NONE,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_CLEAR);
  
}


void s_printf( char *format, ...)
{
   va_list	opt;
   static  u8 buff[2048];

   
    
   va_start(opt, format);
   vsprintf( (void *) buff,format, opt);
   va_end(opt);

if(autocenter) {PX=(xclip-(strlen((char *)buff)*sizelet[sizeletter].tx))/2;}
print_str(PX, PY ,-s_printf_z, color, (void *) buff, sizelet[sizeletter].tx, sizelet[sizeletter].ty);

return;
}

/*********************************************************************************************************************************************************/
/* textures                                                                                                                                              */
/*********************************************************************************************************************************************************/

void tiling4x4(void *mem,int format,int tx,int ty)
{
int n,m,l;
u16 mem_tile[1024*8];
int swap=(format & TILE_LITTLE_ENDIAN)!=0;

format &= ~TILE_LITTLE_ENDIAN;

#define INV_COLORA16(x) ((x & 0x83e0) | ((x & 31)<<10) | ((x>>10) & 31))
#define INV_COLOR16(x) ((x & 0x7e0) | ((x & 31)<<11) | ((x>>11) & 31))
#undef SWAP16
#define SWAP16(x) (((x)>>8) | ((x)<<8))

if(format==TILE_CI4)
	{
	u8 *p1,*p2;
	
	p1=(u8 *) mem;
    p2=(u8 *) &mem_tile[0];
    tx>>=1;
    for(n=0;n<ty;n+=8)
		{
		for(l=0;l<8;l++)
			{
			for(m=0;m<tx;m+=4)
				{
				p2[((l+(m<<1))<<2)]=p1[(n+l)*tx+m];
				p2[((l+(m<<1))<<2)+1]=p1[(n+l)*tx+m+1];
				p2[((l+(m<<1))<<2)+2]=p1[(n+l)*tx+m+2];
				p2[((l+(m<<1))<<2)+3]=p1[(n+l)*tx+m+3];
				}
			}
		for(l=0;l<8;l++)
			{
			for(m=0;m<tx;m++)
				{
				p1[(n+l)*tx+m]=p2[(l)*tx+m];
				}
			}
		}

	DCFlushRange(mem,tx*ty);
	return;
	}
            
if(format==TILE_CI8)
	{
	u16 *p1,*p2;
	
	p1=(u16 *) mem;
    p2=(u16 *) &mem_tile[0];
    tx>>=1;
    for(n=0;n<ty;n+=4)
		{
		for(l=0;l<4;l++)
			{
			for(m=0;m<tx;m+=4)
				{
				p2[((l+m)<<2)]=p1[(n+l)*tx+m];
				p2[((l+m)<<2)+1]=p1[(n+l)*tx+m+1];
				p2[((l+m)<<2)+2]=p1[(n+l)*tx+m+2];
				p2[((l+m)<<2)+3]=p1[(n+l)*tx+m+3];
				}
			}
		for(l=0;l<4;l++)
			{
			for(m=0;m<tx;m++)
				p1[(n+l)*tx+m]=p2[(l)*tx+m];
			}
		}

	DCFlushRange(mem,tx*2*ty);
	return;
	}

if(format<TILE_RGBA8) // color 16 bits
	{
	u16 *p1,*p2;
	
	p1=(u16 *) mem;
    p2=(u16 *) &mem_tile[0];

    for(n=0;n<ty;n+=4)
		{
		for(l=0;l<4;l++)
			{
			for(m=0;m<tx;m+=4)
				{
				p2[((l+m)<<2)]=p1[(n+l)*tx+m];
				p2[((l+m)<<2)+1]=p1[(n+l)*tx+m+1];
				p2[((l+m)<<2)+2]=p1[(n+l)*tx+m+2];
				p2[((l+m)<<2)+3]=p1[(n+l)*tx+m+3];
				}
			}
	if(format==TILE_RGB5A3 || format==TILE_RGB565)
		{
		for(l=0;l<4;l++)
			{
			if(swap) // for little endian to big endian order
			  for(m=0;m<tx;m++) p1[(n+l)*tx+m]= SWAP16(p2[(l)*tx+m]);
			else
			  for(m=0;m<tx;m++) p1[(n+l)*tx+m]=p2[(l)*tx+m];
	
			}
		}
	else
	if(format==TILE_SRGB565)
		{
		for(l=0;l<4;l++)
			{
			if(swap)  // for little endian to big endian order
			  for(m=0;m<tx;m++) p1[(n+l)*tx+m]=INV_COLOR16(SWAP16(p2[(l)*tx+m]));
		    else
			  for(m=0;m<tx;m++) p1[(n+l)*tx+m]=INV_COLOR16(p2[(l)*tx+m]);
			
			}
		}
	else
	if(format==TILE_SRGB5A1)
		{
		for(l=0;l<4;l++)
			{
			if(swap)  // for little endian to big endian order
				for(m=0;m<tx;m++) p1[(n+l)*tx+m]=INV_COLORA16(SWAP16(p2[(l)*tx+m]));  // color 16 bits (With R and B swap)
			else
			    for(m=0;m<tx;m++) p1[(n+l)*tx+m]=INV_COLORA16(p2[(l)*tx+m]);  // color 16 bits (With R and B swap)
				
			}
		}
		
		}
	}
if(format==TILE_RGBA8 || format==TILE_SRGBA8)
	{
	u16 *p1,*p2;
	u32 *p32;
	int tx2=tx*2;

	p1=(u16 *) mem;
    p2=(u16 *) &mem_tile[0];
    p32=(u32 *) p1;
    
	if(format==TILE_SRGBA8) // swap R and B
		{
		for(n=0;n<ty;n++)
		for(m=0;m<tx;m++)
			{
			u32 rgba;
		
			rgba=p32[n*tx+m];

			if(swap)  // for little endian to big endian order
				rgba=(rgba<<24) | ((rgba<<8) & 0xff0000) | ((rgba>>8) & 0xff00) | ((rgba>>24) & 0xff0000);
		
			rgba=(rgba & 0xff00ff00) | ((rgba>>16) & 0xff)  | ((rgba<<16) & 0xff0000);

			p32[n*tx+m]=rgba;

			}
		}

    for(n=0;n<ty;n+=4)
		{
		for(l=0;l<4;l++)
			{
		
			for(m=0;m<tx;m+=4)
				{
				int m2=m<<1;
				p2[((l+m2)<<2)]=p1[(((n+l)*tx+m)<<1)];
				p2[((l+m2)<<2)+1]=p1[(((n+l)*tx+m+1)<<1)];
				p2[((l+m2)<<2)+2]=p1[(((n+l)*tx+m+2)<<1)];
				p2[((l+m2)<<2)+3]=p1[(((n+l)*tx+m+3)<<1)];
               
                p2[((l+m2)<<2)+16]=p1[(((n+l)*tx+m)<<1)+1];
				p2[((l+m2)<<2)+17]=p1[(((n+l)*tx+m+1)<<1)+1];
				p2[((l+m2)<<2)+18]=p1[(((n+l)*tx+m+2)<<1)+1];
				p2[((l+m2)<<2)+19]=p1[(((n+l)*tx+m+3)<<1)+1];
				
				}
			}
	
		for(l=0;l<4;l++)
			{
			for(m=0;m<tx2;m++)
				{
				p1[(n+l)*tx2+m]=p2[(l)*tx2+m];
				}
			}
		
		}
	}

#undef INV_COLOR16
#undef INV_COLORA16
#undef SWAP16

if(format<TILE_RGBA8)
    DCFlushRange(mem,tx*ty*2);
else 
	DCFlushRange(mem,tx*ty*4);

}

void ctlut(void *mem,int format,int entries)
{
int n;
int swap=(format & TLUT_LITTLE_ENDIAN)!=0;

format &= ~TLUT_LITTLE_ENDIAN;

#define INV_COLORA16(x) ((x & 0x83e0) | ((x & 31)<<10) | ((x>>10) & 31))
#define INV_COLOR16(x) ((x & 0x7e0) | ((x & 31)<<11) | ((x>>11) & 31))
#define SWAP16(x) (((x)>>8) | ((x)<<8))

if(format==TLUT_RGB5A3 || format==TLUT_RGB565) 
	{
	if(swap) // for little endian to big endian order
		{
		u16 *p1=(u16 *) mem;

	    for(n=0;n<entries;n++)  p1[n]=SWAP16(p1[n]);

		}
	DCFlushRange(mem, entries*2); return;
	}

if(format==TLUT_SRGB565)
	{
	u16 *p1=(u16 *) mem;
  
    if(swap) // for little endian to big endian order
	     for(n=0;n<entries;n++) p1[n]=INV_COLOR16(SWAP16(p1[n]));
	else
		 for(n=0;n<entries;n++) p1[n]=INV_COLOR16(p1[n]);
		 
		
	DCFlushRange(mem, entries*2); return;
	}
if(format==TLUT_SRGB5A1)
	{
	u16 *p1=(u16 *) mem;

    if(swap) // for little endian to big endian order
	     for(n=0;n<entries;n++) p1[n]=INV_COLORA16(SWAP16(p1[n]));
	else
		 for(n=0;n<entries;n++) p1[n]=INV_COLORA16(p1[n]);
		
	DCFlushRange(mem, entries*2); return;
	}

#undef INV_COLOR16
#undef INV_COLORA16
#undef SWAP16
}

/*********************************************************************************************************************************************************/
/* screen functions                                                                                                                                      */
/*********************************************************************************************************************************************************/

Mtx44 projection;
Mtx	modelView;

static GXTexObj *new_texture=NULL;
static int new_texture_sx=1;
static int new_texture_sy=1;
static int new_texture_dspl=10;
static int new_texture_mult=1023;
static int new_texture_repeat=0;

GXColor	backgroundColor	= {0, 0x20*0, 0x40*0,	255};


static void *fifoBuffer = NULL;

int SCR_WIDTH=0,SCR_HEIGHT=0;


static int tablaseno[16384];
static int tablacoseno[16384];

#define  FLOAT_FIX (16384)
#define  PID 6.283185307179586476925286766559

static void tabsenofunc()
{
int n;
//u32 stat;
//stat=IRQ_Disable();


	for(n=0;n<16384;n++) 
		tablaseno[n]=(int) ((double) FLOAT_FIX*sin((PID*(float) n)/16384.0));
	

	for(n=0;n<16384;n++) 
		tablacoseno[n]=(int) ((double) FLOAT_FIX*cos((PID*(float) n)/16384.0));

//IRQ_Restore(stat);	
}

/*static float seno(float ang) // fast sin
{
int n;

	n=(int) ((ang*16384.0f)/PID);
	if(n<0) n=16384-n;
	n&=16383;

return(tablaseno[n]);
}

static float coseno(float ang) // fast cos
{
int n;


	n=(int) ((ang*16384.0f)/PID);
	if(n<0) n=16384-n;
	n&=16383;

return(tablacoseno[n]);
}
*/
/*static*/ int seno2(int ang) // fast sin (ang=16384= 360 degrees)
{
int n;

	n=ang;
	if(n<0) n=16384-n;
	n&=16383;

return(tablaseno[n]);
}

/*static*/ int coseno2(int ang)
{
int n;

	n=ang;
	if(n<0) n=16384-n;
	n&=16383;

return(tablacoseno[n]);
}


GXTexObj tex_pattern[4];

static void create_patterns()
{
u16 *m_tex1;
int n,m;

m_tex1=memalign(32,8*8*2);
memset(m_tex1,0xff,8*8*2);

for(n=0;n<8;n++)
	{
	m_tex1[n*8+n]=0x8000;
	}
CreateTexture(&tex_pattern[0], TILE_SRGB5A1, m_tex1, 8, 8, 1);

m_tex1=memalign(32, 16*16*2);
memset(m_tex1,0xff, 16*16*2);
 //X
for(n=0;n<8;n++)
	{
	m_tex1[n*16+7-n]=0x8000;
	m_tex1[n*16+8+n]=0x8000;
	m_tex1[(15-n)*16+7-n]=0x8000;
	m_tex1[(15-n)*16+8+n]=0x8000;
	}
CreateTexture(&tex_pattern[1], TILE_SRGB5A1, m_tex1, 16, 16, 1);

m_tex1=memalign(32, 16*16*2);
memset(m_tex1,0xff, 16*16*2);

 // square
for(n=0;n<16;n++)
	{
	m_tex1[n]=0x8000;
	m_tex1[n*16]=0x8000;
	}
CreateTexture(&tex_pattern[2], TILE_SRGB5A1, m_tex1, 16, 16, 1);

m_tex1=memalign(32, 8*8*2);
memset(m_tex1,0xff, 8*8*2);
{ // mosaic
u16 color=Color5551(0xffe0e0e0);
n=0;m=0;
m_tex1[n*8+m]=color;
m_tex1[n*8+m+1]=color;
m_tex1[n*8+m+2]=color;
m_tex1[n*8+m+3]=color;
m_tex1[(n+1)*8+m]=color;
m_tex1[(n+1)*8+m+1]=color;
m_tex1[(n+1)*8+m+2]=color;
m_tex1[(n+1)*8+m+3]=color;
m_tex1[(n+2)*8+m]=color;
m_tex1[(n+2)*8+m+1]=color;
m_tex1[(n+2)*8+m+2]=color;
m_tex1[(n+2)*8+m+3]=color;
m_tex1[(n+3)*8+m]=color;
m_tex1[(n+3)*8+m+1]=color;
m_tex1[(n+3)*8+m+2]=color;
m_tex1[(n+3)*8+m+3]=color;


n=4;m=4;
m_tex1[n*8+m]=color;
m_tex1[n*8+m+1]=color;
m_tex1[n*8+m+2]=color;
m_tex1[n*8+m+3]=color;
m_tex1[(n+1)*8+m]=color;
m_tex1[(n+1)*8+m+1]=color;
m_tex1[(n+1)*8+m+2]=color;
m_tex1[(n+1)*8+m+3]=color;
m_tex1[(n+2)*8+m]=color;
m_tex1[(n+2)*8+m+1]=color;
m_tex1[(n+2)*8+m+2]=color;
m_tex1[(n+2)*8+m+3]=color;
m_tex1[(n+3)*8+m]=color;
m_tex1[(n+3)*8+m+1]=color;
m_tex1[(n+3)*8+m+2]=color;
m_tex1[(n+3)*8+m+3]=color;
	
CreateTexture(&tex_pattern[3], TILE_SRGB5A1, m_tex1, 8, 8, 1);
}

}


static int video_sleep=0;

static void copy_buffers(u32 count __attribute__ ((unused)))
{
	/*
if(update_gfx && update_scr)
		{
		update_gfx=0;
		
		
		GX_CopyDisp(frameBuffer[frame],GX_TRUE);
		GX_Flush();
		frame^=1;
		VIDEO_SetNextFramebuffer(frameBuffer[frame]);
		
		//VIDEO_Flush();
		update_scr=0;
		}
*/
}

void SetVideoSleep(int on_off)
{
	video_sleep=on_off;
}

void InitScreen()
{
//char *debug_str2=debug_str;

//	debug_str="InitScreen()";

	// Inicializa el sistema de video

	VIDEO_Init();

	tabsenofunc();
	

// Inicializa la pantalla para doble buffer

    screenMode = VIDEO_GetPreferredMode (NULL);
	
	frameBuffer[0]	= MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));
	frameBuffer[1]	= MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenMode));

	VIDEO_Configure(screenMode);
	VIDEO_SetNextFramebuffer(frameBuffer[frame]);
	VIDEO_SetPostRetraceCallback(copy_buffers);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();

    if(screenMode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	 // asigna la memoria para el FIFO

	fifoBuffer = MEM_K0_TO_K1(memalign(32,FIFO_SIZE));
	memset(fifoBuffer,0, FIFO_SIZE);

	GX_Init(fifoBuffer, FIFO_SIZE);
    
	// formato RGB y Buffer Z a 24 bits
	GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	// asigna los valores de borrado de pantalla (color) y de relleno de buffer Z
	GX_SetCopyClear(backgroundColor, 0x00ffffff);

	SCR_WIDTH=screenMode->fbWidth;
	SCR_HEIGHT=screenMode->efbHeight;

	GX_SetViewport(0,0,screenMode->fbWidth,screenMode->efbHeight,0.0f,1.0f);
	GX_SetDispCopyYScale((f32)screenMode->xfbHeight/(f32)screenMode->efbHeight);
	GX_SetScissor(0,0,screenMode->fbWidth,screenMode->efbHeight);
	GX_SetDispCopySrc(0,0,screenMode->fbWidth,screenMode->efbHeight);
	GX_SetDispCopyDst(screenMode->fbWidth,screenMode->xfbHeight);
	GX_SetCopyFilter(screenMode->aa,screenMode->sample_pattern,
					 GX_TRUE,screenMode->vfilter);
	GX_SetFieldMode(screenMode->field_rendering,
					((screenMode->viHeight==2*screenMode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	GX_SetCullMode(GX_CULL_NONE);

	GX_SetZMode(GX_TRUE, GX_LEQUAL,	GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);

	GX_CopyDisp(frameBuffer[frame],GX_TRUE);
    
	GX_SetDispCopyGamma(GX_GM_1_0);

	// Ajusta la Matriz de Proyeccion para trabajar con Sprites en 2D
    if(screenMode->efbHeight<=480)
       guOrtho(projection, -12.0f ,(f32) (screenMode->efbHeight+16), 0.0f ,(f32)(screenMode->fbWidth-1), 0.0f, 1000.0f); // z desde 0 a 1000.0f
	else
	   guOrtho(projection, 0.0f ,(f32) (screenMode->efbHeight), 0.0f ,(f32)(screenMode->fbWidth-1), 0.0f, 1000.0f); // z desde 0 a 1000.0f

	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

	GX_PixModeSync(); // función de sincronización.
    GX_SetZCompLoc(GX_FALSE); /* esto hace que no se actualize el buffer Z hasta después de texturizar */


    // aqui ajustamos la iluminacion en Off a un solo canal y para que tome el color de los vertices
	GX_SetNumChans(1); 
	GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_CLAMP, GX_AF_NONE );

	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

// ajusta a una textura
	GX_SetNumTexGens(1);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	guMtxIdentity(modelView);

	/* NOTA: con ésta matriz, podrias jugar para rotar los sprites*/

	GX_LoadPosMtxImm(modelView,	GX_PNMTX0); // carga la matriz mundial como identidad
	GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz
    
	UploadFontTexture();
	UploadFontTextureExt(&font_b[0]);
	SelectFontTexture(0);
	create_patterns();

	

	// limpia la pantalla
	update_gfx=1;
	Screen_flip();
	update_gfx=1;
	Screen_flip();

//	debug_str=debug_str2;
}


void Recover2DContext()
{
	GX_SetCullMode(GX_CULL_NONE);

	GX_SetZMode(GX_TRUE, GX_LEQUAL,	GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);

	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);

	GX_PixModeSync(); // función de sincronización.
    GX_SetZCompLoc(GX_FALSE); /* esto hace que no se actualize el buffer Z hasta después de texturizar */

	GX_LoadPosMtxImm(modelView,	GX_PNMTX0); // carga la matriz mundial como identidad
	GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz

	// aqui ajustamos la iluminacion en Off a un solo canal y para que tome el color de los vertices
	GX_SetNumChans(1); 
	GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_CLAMP, GX_AF_NONE );

	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

// ajusta a una textura
	GX_SetNumTexGens(1);
}

void Screen_flip()
{
Mtx temp_mtx;
//char *debug_str2=debug_str;

//	debug_str="Screen_flip()";
    
if(video_sleep) 
	{
	guMtxIdentity(temp_mtx);
	
	GX_LoadPosMtxImm(temp_mtx,	GX_PNMTX0); // carga la matriz mundial como identidad
	GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz

	DrawFillBox(-128, -128, 848+256, SCR_HEIGHT+256, 0, 0xd0000000);

	
	GX_LoadPosMtxImm(modelView,	GX_PNMTX0); // carga la matriz mundial como identidad
	GX_SetCurrentMtx(GX_PNMTX0); // selecciona la matriz
	}

	if(update_gfx)
		{
		GX_DrawDone();
		
		}

    if(update_gfx) update_scr=1; else update_scr=0;

	do
	{
	VIDEO_WaitVSync();
	if(update_gfx && update_scr)
		{
		update_gfx=0;
		
		
		GX_CopyDisp(frameBuffer[frame],GX_TRUE);
		GX_Flush();
		
		VIDEO_SetNextFramebuffer(frameBuffer[frame]);
		frame^=1;
		VIDEO_Flush();
		update_scr=0;
		}
	} while(update_scr==1);
	
    GX_InvVtxCache();
	GX_InvalidateTexAll();
	
//	debug_str=debug_str2;
}

/*********************************************************************************************************************************************************/
/* screen draw                                                                                                                                           */
/*********************************************************************************************************************************************************/

static void inline set_text_vtx(int x,int y,int sx,int sy)
{
if(new_texture_repeat)
	{
	sx=new_texture_sx;
	sy=new_texture_sy;
	if(sx<=0) sx=1;
	if(sy<=0) sy=1;
	}

	GX_TexCoord2s16(new_texture_mult*x/sx, new_texture_mult*y/sy);
}

void SetTexture(GXTexObj *texture)
{
int n;
	new_texture=texture;
	//GX_TexModeSync();
	if(texture!=NULL)
		{
		new_texture_sx=(texture->val[2] & 0x3ff)+1;
		new_texture_sy=((texture->val[2]>>10) & 0x3ff)+1;
		new_texture_repeat=(texture->val[0] & 0x3)==GX_REPEAT;
		new_texture_dspl=1;
		n=new_texture_sx;
		if(new_texture_sy<n) n=new_texture_sy;

		while((1<<new_texture_dspl)<n) {new_texture_dspl++;}
		new_texture_mult=(1<<new_texture_dspl)-1;
		
		}
}

static int cur_palette=GX_TLUT0;
static int cur_texLOD_near=GX_LINEAR, cur_texLOD_far=GX_LINEAR;

void UseTexLOD(int near, int far)
{
	 cur_texLOD_near=near, cur_texLOD_far= far;
}

void CreateTexture(GXTexObj *texture, int format, void *mem, int width, int height, int repeat)
{
int flag;
int palette=0;

	if(width<=0 || height<=0) return;

	tiling4x4(mem, format, width, height);

	format&=7;

	switch(format)
		{
		case TILE_RGB5A3:
		case TILE_RGB565:
		case TILE_SRGB565:
		case TILE_SRGB5A1:
			flag=GX_TF_RGB5A3;
			break;
		case TILE_RGBA8:
		case TILE_SRGBA8:
			flag=GX_TF_RGBA8;
			break;
		case TILE_CI4:
			palette=1;
			flag=GX_TF_CI4;
			break;
		case TILE_CI8:
			palette=1;
			flag=GX_TF_CI8;
			break;
		default:
			return;
		}

    GX_TexModeSync();
	if(palette) GX_InitTexObjCI(texture, mem, width, height, flag, (repeat==0) ? GX_CLAMP : GX_REPEAT, (repeat==0) ? GX_CLAMP : GX_REPEAT, GX_FALSE,
				cur_palette);
	else
		GX_InitTexObj(texture, mem, width, height, flag, (repeat==0) ? GX_CLAMP : GX_REPEAT, (repeat==0) ? GX_CLAMP : GX_REPEAT, GX_FALSE);	

	GX_InitTexObjLOD(texture, // objeto de textura
						 cur_texLOD_near, // filtro Linear para cerca
						 cur_texLOD_far, // filtro Linear para lejos
						 0, 0, 0, 0, 0, GX_ANISO_1);
}

void CreatePalette(GXTlutObj *palette, int format, int pal_index, u16 *mem, int entries)
{
	ctlut(mem, format, entries);

	GX_InitTlutObj(palette, mem, GX_TL_RGB5A3, entries);

	cur_palette=GX_TLUT0+(pal_index & 15);
	GX_LoadTlut(palette, cur_palette);
}

void SetPalette(int pal_index)
{
	cur_palette=GX_TLUT0+(pal_index & 15);
}

void DrawSurface(GXTexObj *surface,int x,int y, int width, int height, int layer, u32 color)
{


	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

    //GX_TexModeSync(); // esto evita problemas
    // carga la textura para trabajar como Mapa de textura 0
    GX_LoadTexObj(surface, GX_TEXMAP0);

	//  ajusta el texture environment a 1 y selecciona para que use el color proporcionado en los vertices 
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    
	// ajusta a una textura
	GX_SetNumTexGens(1);

    // aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)

	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 10);  /* coordenadas de textura como S16 para S y T y desplazamiento 10
	                                                                  las coords de texturas están en el rango de 0.0f a 1.0f por lo que al meter
																	  un desplazamiento de 10, 1024==1.0f dado que 2 elevado a 10 es 1024
																	  */
    
	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4); // le decimos que pinte 4 vertices de un quad (si fueran mas quads, añadir mas vertices de 4 en 4)
		 
		 GX_Position3s16(x,y,layer); 
         GX_Color1u32(color);
		 GX_TexCoord2s16(0,0); 
		 GX_Position3s16(x+width,y,layer); 
		 GX_Color1u32(color);
		 GX_TexCoord2s16(1023,0); 
		 GX_Position3s16(x+width,y+height,layer); 
		 GX_Color1u32(color);
		 GX_TexCoord2s16(1023,1023); 
		 GX_Position3s16(x,y+height,layer);
		 GX_Color1u32(color);
		 GX_TexCoord2s16(0,1023); 

	GX_End();

}


void DrawBox(int x,int y, int width, int height, int layer, int thickness, u32 color)
{
int xx,yy;

	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	if(thickness<=0) thickness=1;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

   
    
	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4*4);
		 
		 xx=x;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

         xx=x;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+thickness;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+thickness;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-thickness;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-thickness;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	GX_End();

}

void DrawFillBox(int x,int y, int width, int height, int layer, u32 color)
{
int xx,yy;

	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}
    
	// ajusta a una textura
	GX_SetNumTexGens(1);

	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		 
		 xx=x;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	GX_End();

}

void DrawRoundBox(int x,int y, int width, int height, int layer, int thickness, u32 color)
{
int n;
int dx;
int dx2;
int xx,yy;
//u32 stat;

	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	dx=width;

	if(height<dx) dx=height;

	if(dx>32) dx=16;
	else dx=dx/4;

	if(thickness<=0) thickness=1;

	dx2=dx-thickness;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

    //stat=IRQ_Disable();

	for(n=0;n<4096;n+=1024)
	{
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6*4);
		 
		// 1
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

         xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	     // 2
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 // 3
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		//4
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		

	GX_End();
	}
	//IRQ_Restore(stat);
    
	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4*4);
		 
		 xx=x+dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

         xx=x;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+thickness;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+thickness;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-thickness;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-thickness;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+dx;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	GX_End();

}

void DrawRoundFillBox(int x,int y, int width, int height, int layer, u32 color)
{
int n;
int dx;
int xx,yy;
//u32 stat;


	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	dx=width;

	if(height<dx) dx=height;

	if(dx>32) dx=16;
	else dx=dx/4;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}
    
	// ajusta a una textura
	GX_SetNumTexGens(1);

	
	//stat=IRQ_Disable();

	for(n=0;n<4096;n+=1024)
	{
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3*4);
		 
		 xx=x+dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+dx;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-dx;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+height-dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		

	GX_End();
	}
    //IRQ_Restore(stat);

	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4*3);
		 
		 xx=x+dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		
		 xx=x;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
	
		 xx=x+width-dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height-dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		

	GX_End();

}


void DrawRoundBox2(int x,int y, int width, int height, int layer, int thickness, u32 color)
{
int n;
int dx;
int dx2;
int xx,yy;

	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	dx=height;

	if(width<dx) dx=width;

	dx=dx/2;
	

	if(thickness<=0) thickness=1;

	dx2=dx-thickness;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

	for(n=0;n<8192;n+=1024)
	{
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6*2);
		 
		// 1
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

         xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	  
		 // 3
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx2*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}


	GX_End();
	}
	
    
	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4*2);
		 
		 xx=x+dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+dx;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height-thickness;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	GX_End();

}

void DrawRoundFillBox2(int x,int y, int width, int height, int layer, u32 color)
{
int n;
int dx;
int xx,yy;


	if(width<=0 || height<=0) return;

	color=Get_FASTLE(color); // ordena los colores

	dx=height;

	if(width<dx) dx=width;

	dx=dx/2;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}
    
	// ajusta a una textura
	GX_SetNumTexGens(1);

	
	for(n=0;n<8192;n+=1024)
	{
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3*2);
		 
		 xx=x+dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx+(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx-(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx+(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 

		 xx=x+width-dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx+(int)(dx*seno2(n+1024)/FLOAT_FIX);
		 yy=y+dx-(int)(dx*coseno2(n+1024)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

	GX_End();
	}

	// dibujando el QUAD del Sprite

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4*2);
		 
		 xx=x+dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}

		 xx=x+dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer); 
         GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+dx;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+width-dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}
		 xx=x+dx;
		 yy=y+height;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx-x, yy-y, width, height);}


	GX_End();

}


void DrawSlice(int x,int y, int rx, int ry, int layer, int thickness, int degrees_start, int degrees_end, u32 color)
{
int n;
int rx2;
int ry2;
int xx,yy;
int width, height;
//u32 stat;


	if(rx<=0 || ry<=0) return;

	if(degrees_start==degrees_end) return;

	color=Get_FASTLE(color); // ordena los colores

	if(thickness<=0) thickness=1;

	width=rx;
	height=ry;

	rx2=rx-thickness;
	ry2=ry-thickness;

	if(rx2<0) rx2=0;
	if(ry2<0) ry2=0;


	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

    //stat=IRQ_Disable();

    degrees_start=(degrees_start % 360)*16384/360;
	degrees_end=(degrees_end % 360)*16384/360;
	if(degrees_end<=0) degrees_end=16384;
	if(degrees_end<degrees_start) degrees_end=degrees_end+16384;

	for(n=degrees_start;n<degrees_end;n+=512)
	{
	int na=512;

	if((n+512)>degrees_end) na=degrees_end-n;

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		 
		// 1
		 xx=x+(int)(rx*seno2(n)/FLOAT_FIX);
		 yy=y-(int)(ry*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x+(int)(rx*seno2(n+na)/FLOAT_FIX);
		 yy=y-(int)(ry*coseno2(n+na)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x+(int)(rx2*seno2(n)/FLOAT_FIX);
		 yy=y-(int)(ry2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}

         xx=x+(int)(rx*seno2(n+na)/FLOAT_FIX);
		 yy=y-(int)(ry*coseno2(n+na)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x+(int)(rx2*seno2(n+na)/FLOAT_FIX);
		 yy=y-(int)(ry2*coseno2(n+na)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x+(int)(rx2*seno2(n)/FLOAT_FIX);
		 yy=y-(int)(ry2*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}


	GX_End();
	}
    //IRQ_Restore(stat);
}

void DrawFillSlice(int x,int y, int rx, int ry, int layer, int degrees_start, int degrees_end, u32 color)
{
int n;
int xx,yy;

int width, height;
//u32 stat;

	if(rx<=0 || ry<=0) return;

	if(degrees_start==degrees_end) return;

	color=Get_FASTLE(color); // ordena los colores

	width=rx;
	height=ry;

	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

   //stat=IRQ_Disable();

    degrees_start=(degrees_start % 360)*16384/360;
	degrees_end=(degrees_end % 360)*16384/360;
	if(degrees_end<=0) degrees_end=16384;
	if(degrees_end<degrees_start) degrees_end=degrees_end+16384;

	for(n=degrees_start;n<degrees_end;n+=512)
	{
	int na=512;

	if((n+512)>degrees_end) na=degrees_end-n;

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
		 
		// 1
		 xx=x+(int)(rx*seno2(n)/FLOAT_FIX);
		 yy=y-(int)(ry*coseno2(n)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x+(int)(rx*seno2(n+na)/FLOAT_FIX);
		 yy=y-(int)(ry*coseno2(n+na)/FLOAT_FIX);
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}

       

	GX_End();
	}

    //IRQ_Restore(stat);
}
void DrawEllipse(int x,int y, int rx, int ry, int layer, int thickness, u32 color)
{
	DrawSlice( x, y, rx, ry,layer, thickness, 0, 360, color);
}

void DrawFillEllipse(int x,int y, int rx, int ry, int layer, u32 color)
{
	DrawFillSlice( x, y, rx, ry,layer, 0, 360, color);
}


void DrawLine(int x,int y, int x2, int y2, int layer, int thickness, u32 color)
{
int xx,yy;

int x3,x4,y3,y4;

int width, height;

	if(x==x2 && y==y2) return;

	color=Get_FASTLE(color); // ordena los colores


	if(thickness<=0) thickness=1;

	// ordena en base a X
	if(x2<x) {x3=x2;x2=x;x=x3;x3=y2;y2=y;y=x3;}


    if(y==y2 && x!=x2)
		{
		x3=x;y3=y+thickness;
		x4=x2;y4=y2+thickness;
		width=x2-x;
		height=thickness;
		}
	else
	if(y!=y2 && x==x2)
		{
		x3=x+thickness;y3=y;
		x4=x2+thickness;y4=y2;
		width=thickness;
		height=y-y2;
		if(height<0) height=-height; height+=1;
		}
	else
		{
	    
		width=x2-x;
		if(width<0) width=-width; width+=1;
		height=y-y2;
		if(height<0) height=-height; height+=1;

		if(width>=height)
			{
			x3=x;y3=y+thickness;
			x4=x2;y4=y2+thickness;
			width=x2-x;
			height+=thickness;
			}
        else
			{
			x3=x+thickness;y3=y;
			x4=x2+thickness;y4=y2;
			width+=thickness;
			}
		}


	update_gfx=1;

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
	GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

	layer=-layer; // layer en realidad, es Z y tiene que tener progresion negativa

	GX_SetNumTevStages(1);

	// aqui se describe los vertices y su formato (el orden sería, pos, norm, color, textura)
	GX_ClearVtxDesc();                    // borra los descriptores
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo  
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,	 GX_POS_XYZ,GX_S16,	0);      // formato de posicion S16 para X, Y ,Z
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,	0);  // color RGBA8 para color0

	if(new_texture)
		{
		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, new_texture_dspl); 
		}
	else
		{
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 
		}

    
	// ajusta a una textura
	GX_SetNumTexGens(1);

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		 
		// 1
		 xx=x;
		 yy=y;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x2;
		 yy=y2;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x3;
		 yy=y3;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}

         xx=x2;
		 yy=y2;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x3;
		 yy=y3;
		 GX_Position3s16(xx, yy, layer);
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}
		 xx=x4;
		 yy=y4;
		 GX_Position3s16(xx, yy, layer); 
		 GX_Color1u32(color);
		 if(new_texture) {set_text_vtx(xx, yy, width, height);}


	GX_End();
    
}

/////////////////////////////////////////

void ConfigureForColor()
{
		// blending (translucidez)
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

		// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
		GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

		GX_SetNumTevStages(1);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxDesc(GX_VA_TEX0, GX_NONE); 

		GX_ClearVtxDesc();                    // borra los descriptores
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo 
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,    GX_POS_XYZ,GX_S16,   0);      // formato de posicion S16 para X, Y ,Z
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,   0);  // color RGBA8 para color0

		update_gfx=1;
}

void ConfigureForTexture(int scale)
{

		// blending (translucidez)
		GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,GX_BL_INVSRCALPHA, GX_LO_CLEAR);

		// activa la transparencia de texturas: los colores con alpha<8 no se dibujan
		GX_SetAlphaCompare(GX_GEQUAL, 8, GX_AOP_AND, GX_ALWAYS, 0); // esto deja pasar solo valores entre 8 y 255

		GX_SetNumTevStages(1);

		GX_ClearVtxDesc();                    // borra los descriptores
		GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);  // selecciona Posicion como directo
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT); // selecciona Color como directo 
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,    GX_POS_XYZ,GX_S16,   0);      // formato de posicion S16 para X, Y ,Z
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8,   0);  // color RGBA8 para color0

		GX_LoadTexObj(new_texture, GX_TEXMAP0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT); // selecciona Textura como directa
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, scale); 

		// ajusta a una textura
		GX_SetNumTexGens(1);

		update_gfx=1;

}

void ChangeProjection(int left, int top, int right, int bottom)
{
static Mtx44 projection2;

	guOrtho(projection2, (f32) top ,(f32) bottom, (f32) left ,(f32) right, 0.0f, 1000.0f); // z desde 0 a 1000.0f

	GX_LoadProjectionMtx(projection2, GX_ORTHOGRAPHIC);
}

void RestoreProjection()
{
	GX_LoadProjectionMtx(projection, GX_ORTHOGRAPHIC);
}

/////////////////////////

