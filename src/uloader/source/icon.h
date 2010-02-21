/* Sprite_gen (c) 2007-2008 Hermes/Entuwii.net */


#ifndef icon_def
#define icon_def


#ifdef __cplusplus
extern "C" {
#endif


#define icon_bits_per_color 8


#define icon_palette_colors 18
#define icon_palette_bits 16
#define icon_palette_base 0


extern unsigned short icon_palette[18];

typedef struct
{
void *ptr;
unsigned short sx,sy;
} type_sprite_list_icon;  // Sprite list Struct type


#define icon_num_sprites 2 // Number of Sprites


extern type_sprite_list_icon icon_sprites[2]; // Sprite List


#define icon_sprite_1_sx 32
#define icon_sprite_1_sy 32


extern unsigned char icon_sprite_1[1024];

#define icon_sprite_2_sx 32
#define icon_sprite_2_sy 32


extern unsigned char icon_sprite_2[1024];

#ifdef __cplusplus
}
#endif


#endif


