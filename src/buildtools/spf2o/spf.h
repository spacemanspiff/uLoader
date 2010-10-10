#ifndef __SPH_H__
#define __SPH_H__

#define MAX_COLORES 256

struct COLOR {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
};

struct SPRITES {
	int largo;
	int alto;
	unsigned char *bitmap;
};

struct SPF {
	struct COLOR paleta[MAX_COLORES];
	int num_colores;

	int num_sprites;
	struct SPRITES *sprites;

};

struct SPF *cargarSPF(const char *archivo);

int generaSalidaSPF(struct SPF *spf, const char *salida, const char *prefijo);

int liberaSPF(struct SPF *spf);

// Macro to convert RGBA8 values to RGB5A3
#define PNGU_u16 unsigned short
#define PNGU_RGB8_TO_RGB5A3(r,g,b,a)    (PNGU_u16) (((a & 0xE0U) == 0xE0U) ? \
                                (0x8000U | ((((PNGU_u16) r) & 0xF8U) << 7) | ((((PNGU_u16) g) & 0xF8U) << 2) | (((PNGU_u16) b) >> 3)) : \
                           (((((PNGU_u16) a) & 0xE0U) << 7) | ((((PNGU_u16) r) & 0xF0U) << 4) | (((PNGU_u16) g) & 0xF0U) | ((((PNGU_u16) b) & 0xF0U) >> 4)))

#endif
