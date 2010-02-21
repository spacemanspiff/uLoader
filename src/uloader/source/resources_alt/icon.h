/* Sprite_gen (c) 2007-2008 Hermes/www.elotrolado.net */


/* WARNING!: This file use RGB5A3 format */

#ifndef icon_def
#define icon_def


#ifdef __cplusplus
extern "C" {
#endif


#define icon_bits_per_color 8


#define icon_palette_colors 58
#define icon_palette_bits 16
#define icon_palette_base 0


extern unsigned short icon_palette[58];

typedef struct
{
void *ptr;
unsigned short sx,sy;
} type_sprite_list_icon;  // Sprite list Struct type


#define icon_num_sprites 9 // Number of Sprites


extern type_sprite_list_icon icon_sprites[9]; // Sprite List


#define icon_sprite_1_sx 64
#define icon_sprite_1_sy 64


extern unsigned char icon_sprite_1[4096];

#define icon_sprite_2_sx 64
#define icon_sprite_2_sy 64


extern unsigned char icon_sprite_2[4096];

#define icon_sprite_3_sx 16
#define icon_sprite_3_sy 16


extern unsigned char icon_sprite_3[256];

#define icon_sprite_4_sx 128
#define icon_sprite_4_sy 128


extern unsigned char icon_sprite_4[16384];

#define icon_sprite_5_sx 64
#define icon_sprite_5_sy 64


extern unsigned char icon_sprite_5[4096];

#define icon_sprite_6_sx 64
#define icon_sprite_6_sy 64


extern unsigned char icon_sprite_6[4096];

#define icon_sprite_7_sx 64
#define icon_sprite_7_sy 64


extern unsigned char icon_sprite_7[4096];

#define icon_sprite_8_sx 64
#define icon_sprite_8_sy 64


extern unsigned char icon_sprite_8[4096];

#define icon_sprite_9_sx 64
#define icon_sprite_9_sy 64


extern unsigned char icon_sprite_9[4096];

#ifdef __cplusplus
}
#endif


#endif


