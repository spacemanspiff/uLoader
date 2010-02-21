// tpl2ppm.c -Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Modified by Hermes
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "global.h"
#include "gfx.h"

/*static*/ u16 tpl_w=256, tpl_h=128;

static u16 *mem=NULL;
static u8 *buf= NULL;

static u16 be16(const u8 *p)
{
	return (p[0] << 8) | p[1];
}

static u32 be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

u16 rgb32to16(u32 color)
{
u16 c16;

	c16=((color>>3) & 0x1f) | ((color>>6) & 0x3e0) | ((color>>9) & 0x7c00) | 0x8000;

return c16;
}
static void out_rgb(u32 color, int x, int y, int w, int h)
{
int x2, y2,l;

u16 *p;
u16 color2;
int h2=76;


x2=(x+1)*tpl_w/w;
x=x*tpl_w/w;
	

y2=((y+1)*h2)/h;
y=(y*h2)/h;

// align to uLoader final icon
y+=16;
y2+=16;

color2=rgb32to16(color);

for(; y<=y2; y++)
	{
	if(y>=tpl_h) break;

	p= &mem [tpl_w*y+x];
    l=x;
	for(; l<=x2; l++)
		{
		if(l>=tpl_w) break;
		*p++= color2;
		}
	}

}

static void i4(int w, int h, int o)
{
	
	int x, y;

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			int x0, x1, y0, y1, off;
			int ww = round_up(w, 8);

			x0 = x & 7;
			x1 = x >> 3;
			y0 = y & 7;
			y1 = y >> 3;
			off = x0 + 8*y0 + 64*x1 + 8*ww*y1;

			raw = buf[o + off/2];
			if ((x0 & 1) == 0)
				raw >>= 4;
			else
				raw &= 0x0f;

			pix[0] = raw * 0x11;
			pix[1] = raw * 0x11;
			pix[2] = raw * 0x11;

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);

		}

	
}

static void i8(int w, int h, int o)
{
	
	int x, y;

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			int x0, x1, y0, y1, off;
			int ww = round_up(w, 8);

			x0 = x & 7;
			x1 = x >> 3;
			y0 = y & 3;
			y1 = y >> 2;
			off = x0 + 8*y0 + 32*x1 + 4*ww*y1;

			raw = buf[o + off];

			pix[0] = raw;
			pix[1] = raw;
			pix[2] = raw;

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);
		}

	
}


static void ia4(int w, int h, int o)
{
	
	int x, y;


	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			int x0, x1, y0, y1, off;
			int ww = round_up(w, 8);

			x0 = x & 7;
			x1 = x >> 3;
			y0 = y & 3;
			y1 = y >> 2;
			off = x0 + 8*y0 + 32*x1 + 4*ww*y1;

			raw = buf[o + off];

			//raw = (raw >> 4) * 0x11;
			raw = (raw & 0xf) * 0x11;

			pix[0] = raw;
			pix[1] = raw;
			pix[2] = raw;

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);
		}

	
}

static void rgb5a3(int w, int h, int o)
{
	
	int x, y;

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			int x0, x1, y0, y1, off;
			int ww = round_up(w, 4);

			x0 = x & 3;
			x1 = x >> 2;
			y0 = y & 3;
			y1 = y >> 2;
			off = x0 + 4*y0 + 16*x1 + 4*ww*y1;

			raw = buf[o + 2*off] << 8;
			raw |= buf[o + 2*off + 1];

			// RGB5A3
			if (raw & 0x8000) {
				pix[0] = (raw >> 7) & 0xf8;
				pix[1] = (raw >> 2) & 0xf8;
				pix[2] = (raw << 3) & 0xf8;
			} else {
				pix[0] = (raw >> 4) & 0xf0;
				pix[1] =  raw       & 0xf0;
				pix[2] = (raw << 4) & 0xf0;
			}

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);
		}

	
}

static void rgb5(int w, int h, int o)
{
	
	int x, y;

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			int x0, x1, y0, y1, off;
			int ww = round_up(w, 4);

			x0 = x & 3;
			x1 = x >> 2;
			y0 = y & 3;
			y1 = y >> 2;
			off = x0 + 4*y0 + 16*x1 + 4*ww*y1;

			raw = buf[o + 2*off] << 8;
			raw |= buf[o + 2*off + 1];

			// RGB5A3
			
				pix[0] = (raw >> 8) & 0xf8;
				pix[1] = (raw >> 3) & 0xfc;
				pix[2] = (raw << 3) & 0xf8;
			

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);
		}

	
}

static u16 avg(u16 w0, u16 w1, u16 c0, u16 c1)
{
	u16 a0, a1;
	u16 a, c;

	a0 = c0 >> 11;
	a1 = c1 >> 11;
	a = (w0*a0 + w1*a1) / (w0 + w1);
	c = a << 11;

	a0 = (c0 >> 5) & 63;
	a1 = (c1 >> 5) & 63;
	a = (w0*a0 + w1*a1) / (w0 + w1);
	c |= a << 5;

	a0 = c0 & 31;
	a1 = c1 & 31;
	a = (w0*a0 + w1*a1) / (w0 + w1);
	c |= a;

	return c;
}

static void cmp(int w, int h, int o)
{
	
	int x, y;	

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			u8 pix[3];
			u16 raw;
			u16 c[4];
			int x0, x1, x2, y0, y1, y2, off;
			int ww = round_up(w, 8);
			int ix;
			u32 px;

			x0 = x & 3;
			x1 = (x >> 2) & 1;
			x2 = x >> 3;
			y0 = y & 3;
			y1 = (y >> 2) & 1;
			y2 = y >> 3;
			off = 8*x1 + 16*y1 + 32*x2 + 4*ww*y2;

			c[0] = be16(buf + o + off);
			c[1] = be16(buf + o + off + 2);
			if (c[0] > c[1]) {
				c[2] = avg(2, 1, c[0], c[1]);
				c[3] = avg(1, 2, c[0], c[1]);
			} else {
				c[2] = avg(1, 1, c[0], c[1]);
				c[3] = 0;
			}

			px = be32(buf + o + off + 4);
			ix = x0 + 4*y0;
			raw = c[(px >> (30 - 2*ix)) & 3];

			pix[0] = (raw >> 8) & 0xf8;
			pix[1] = (raw >> 3) & 0xf8;
			pix[2] = (raw << 3) & 0xf8;

			out_rgb(0xff000000 | (pix[0]<<16) | (pix[1]<<8) | (pix[2]), x, y, w, h);
		}

	
}


void * tpl_2_rgba(void *tpl)
{
u16 w, h, o, t;
u32 n;

	buf=tpl;

	h = be16(buf + 0x14);
	w = be16(buf + 0x16);
	t = be32(buf + 0x18);
	o = be32(buf + 0x1c);

	mem=memalign(32,(u32) tpl_w * (u32) tpl_h*2+32);
	if(!mem) return NULL;
    
	// fill color
	for(n=0;n< ((u32) tpl_w * (u32) tpl_h);n++) mem[n]=rgb32to16(0xffc0c0d0);

	// draw line
	for(n=0;n<tpl_w;n++) {mem[(tpl_h-20)*tpl_w+n]=rgb32to16(0xff808090);mem[(tpl_h-16)*tpl_w+n]=rgb32to16(0xff808090);}
	//memset(mem, 255, tpl_w * tpl_h*4+32);

	// XXX: check more header stuff here
	switch (t) {
	case 0:
		i4(w, h, o);
		break;

	case 1:
		i8(w, h, o);
		break;

	case 2:
		ia4(w, h, o);
		break;

	case 4: 
		rgb5(w, h, o);
		break;

	case 5:
		rgb5a3(w, h, o);
		break;

	case 14:
		cmp(w, h, o);
		break;
	
	default:
	
		free(mem);mem=NULL;
	}

return mem;
}

