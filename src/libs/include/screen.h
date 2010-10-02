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

#ifndef SCREEN_H
#define SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>    // for the s_printf function

#define	FIFO_SIZE (1024*1024)


/*********************************************************************************************************************************************************/
/* fonts and s_printf                                                                                                                                    */
/*********************************************************************************************************************************************************/


extern int xclip;      // screen width (don´t modify)

extern int autocenter; // screen autocenter for s_printf function

extern unsigned sizeletter; // size letter index (better if yuo use letter_size() to change the size)

extern unsigned bkcolor;  // background color for letters. Usually 0 (transparent)

extern unsigned color;	  // color for letters (s_printf)

extern unsigned PX,PY;    // pixel coordinates for s_printf

extern int s_printf_z; // Z for s_printf (obvious)

extern unsigned char font_b[28672];

#define INTERNAL_EXTFONT &font_b[0] // pointer to the internal font used as secondary (external) font. This font is loaded by default

void SelectFontTexture(int select); // select the font texture (0-> Internal font; 1-> External font)

void UploadFontTextureExt(unsigned char *punt); // upload an external font created with TTF2RAW

void letter_size(u16 tamx, u16 tamy); // size for letters in pixel (default size for the fonts are 16x32)

void print_str(u32 x, u32 y, u32 z, u32 color, char *string,u16 tamx, u16 tamy); // print string at screen coordinates

void s_printf( char *format, ...); // similar to printf standard function


/*********************************************************************************************************************************************************/
/* textures                                                                                                                                              */
/*********************************************************************************************************************************************************/

#define TILE_LITTLE_ENDIAN 128 // change the byte order in the color
#define TILE_CI4       0 // for CI4  (color index of 4 bits)
#define TILE_CI8       1 // for CI8  (color index of 8 bits)
#define TILE_RGB5A3    2 // for RGB5A3 
#define TILE_RGB5A1    2 // pseudo RGB5A1
#define TILE_RGB565    3 // for RGB565 
#define TILE_SRGB565   4 // for RGB565 with R and B swapped
#define TILE_SRGB5A1   5 // for RGB5A1 with R and B swapped
#define TILE_RGBA8     6 // for RGBA8
#define TILE_SRGBA8    7 // for RGBA8 with R and B swapped

void tiling4x4(void *mem, int format, int tx, int ty); // function for tile texture conversion

/* formats for ctlut: */

#define TLUT_LITTLE_ENDIAN 128 // change the byte order in the color
#define TLUT_RGB5A3    2 // for RGB5A3 
#define TLUT_RGB5A1    2 // pseudo RGB5A1
#define TLUT_RGB565    3 // for RGB565 
#define TLUT_SRGB565   4 // for RGB565 with R and B swapped
#define TLUT_SRGB5A1   5 // for RGB5A1 with R and B swapped


void ctlut(void *mem,int format,int entries); // function for palette conversion

// u32 color Big to Little Endian
#define Get_LE(x) ((((x)>>24) & 0xff) | (((x)>>8) & 0xff00) | (((x)<<8) & 0xff0000) | (((x)<<24) & 0xff000000))

#define Get_FASTLE(x) ((((x)>>24)) | (((x)>>8) & 0xff00) | (((x)<<8) & 0xff0000) | (((x)<<24)))

#define Color5551(x) ((((x) & 0xff000000) ? 0x8000 : 0) | (((x)>>9) & 0x7c00) | (((x)>>6) & 0x3e0) | (((x)>>3) & 0x1f))


/*********************************************************************************************************************************************************/
/* screen functions                                                                                                                                      */
/*********************************************************************************************************************************************************/
extern int SCR_WIDTH,SCR_HEIGHT;

void InitScreen();
void Screen_flip();

void SetVideoSleep(int on_off);

/*********************************************************************************************************************************************************/
/* screen draw                                                                                                                                           */
/*********************************************************************************************************************************************************/

extern GXTexObj tex_pattern[4];

#define STRIPED_PATTERN &tex_pattern[0]
#define CROSS_PATTERN   &tex_pattern[1]
#define SQUARE_PATTERN  &tex_pattern[2]
#define MOSAIC_PATTERN  &tex_pattern[3]
#define NULL_PATTERN	NULL

// create one texture using TILE_CI4, TILE_CI8, TILE_RGB565, TILE_RGB5A3, TILE_SRGB565, TILE_SRGB5A3, TILE_RGBA8, TILE_SRGBA8
void CreateTexture(GXTexObj *texture, int format, void *mem, int width, int height, int repeat);

void SetTexture(GXTexObj *texture); // set the draw texture

void CreatePalette(GXTlutObj *palette, int format, int pal_index, u16 *mem, int entries); /* create and select a palette object to use with CreateTexture()
																						you can create 0 to 15 palettes */

void SetPalette(int pal_index); // Select the palette object to use with CreateTexture() (from 0 to 15). You must create one with CreatePalette()

void DrawSurface(GXTexObj *surface,int x,int y, int width, int height, int layer, u32 color);

void DrawBox(int x,int y, int width, int height, int layer, int thickness, u32 color);
void DrawFillBox(int x,int y, int width, int height, int layer, u32 color);

void DrawRoundBox(int x,int y, int width, int height, int layer, int thickness, u32 color);
void DrawRoundFillBox(int x,int y, int width, int height, int layer, u32 color);

void DrawRoundBox2(int x,int y, int width, int height, int layer, int thickness, u32 color);
void DrawRoundFillBox2(int x,int y, int width, int height, int layer, u32 color);

void DrawSlice(int x,int y, int rx, int ry, int layer, int thickness, int degrees_start, int degrees_end, u32 color);
void DrawFillSlice(int x,int y, int rx, int ry, int layer, int degrees_start, int degrees_end, u32 color);


void DrawEllipse(int x,int y, int rx, int ry, int layer, int thickness, u32 color);
void DrawFillEllipse(int x,int y, int rx, int ry, int layer, u32 color);

void DrawLine(int x,int y, int x2, int y2, int layer, int thickness, u32 color);


void UseTexLOD(int near, int far); // used to change the LOD method creating one texture (by default GX_LINEAR, GX_LINEAR)

/*********************************************************************************************************************************************************/
/* Direct access to GX functions                                                                                                                         */
/*********************************************************************************************************************************************************/

void Recover2DContext();             // use this function if you change to a 3D context or change projection, GX matrix, lights, etc

void ConfigureForColor();			 /* configure to use GX_Begin method for vertex color */
void ConfigureForTexture(int scale); /* configure to use GX_Begin method for vertex textured (scale is a displacement for th GX, texture coordinates uses a
										range from 0 to 1 , but you are using integers!. So if you put 10 as scale the coordinate range is from 0 to 1<<10 
										(0 to 1024) and 1024 represent 1.0 */

static void inline AddColorVertex(s16 x, s16 y, s16 z, unsigned color)
{
	GX_Position3s16(x, y, -z);
	GX_Color1u32(Get_LE(color));
}

static void inline AddTextureVertex(s16 x, s16 y, s16 z, unsigned color, s16 tx, s16 ty)
{
	GX_Position3s16(x, y, -z);
	GX_Color1u32(Get_FASTLE(color));
	GX_TexCoord2s16(tx, ty);
}

/*********************************************************************************************************************************************************/
/* Projection functions                                                                                                                                  */
/*********************************************************************************************************************************************************/

void ChangeProjection(int x, int y, int w, int h); // change the screen projection matrix

void RestoreProjection(); // restore the default projection

#ifdef __cplusplus
  }
#endif

#endif

//
