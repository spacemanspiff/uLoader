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

int generaSalidaSPF(struct SPF *spf, const char *prefijo);

int liberaSPF(struct SPF *spf);

#endif
