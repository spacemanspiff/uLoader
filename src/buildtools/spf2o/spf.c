#include "spf.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include <string.h>

#define FIRMA_SPF 0x31465053

#define PALETA_RGBA 8

static inline s32 leeS32(FILE *fp, s32 *n)
{
	fread(n,4,1, fp);
	return *n;
}

static inline u32 leeU32(FILE *fp, u32 *n)
{
	fread(n,4,1, fp);
	return *n;
}

static int leerPaletaRGBA(FILE *fp, struct SPF *spf) {
	int i;
	spf->num_colores = MAX_COLORES;

	for (i = 0; i < MAX_COLORES; i++) {
		char color[4];
		fread(color, 4, 1, fp);
		spf->paleta[i].red = color[0];
		spf->paleta[i].green = color[1];
		spf->paleta[i].blue = color[2];
		spf->paleta[i].alpha = color[3];
	}
	return 1;
}

int optimizarPaleta(struct SPF *spf, const char *colores)
{
	int i;
	int j;
	int idx = 0;
	char mapa[MAX_COLORES];
	struct COLOR paletatmp[MAX_COLORES];

	for (i = 0; i < MAX_COLORES; i++) {
		paletatmp[i].red = spf->paleta[i].red;
		paletatmp[i].green = spf->paleta[i].green;
		paletatmp[i].blue = spf->paleta[i].blue;
		paletatmp[i].alpha = spf->paleta[i].alpha;
	}

	for (i = 0; i < MAX_COLORES; i++) {
		if (colores[i]) {
			spf->paleta[idx].red = paletatmp[i].red; 
			spf->paleta[idx].green = paletatmp[i].green; 
			spf->paleta[idx].blue = paletatmp[i].blue; 
			spf->paleta[idx].alpha = paletatmp[i].alpha; 
			mapa[i] = idx++;
		} else 
			mapa[i] = -1;
	}
	spf->num_colores = idx;
	printf(" * Colores a utilizar: %d\n", idx);

	for (i = 0;i < spf->num_sprites; i++) {
		for (j = 0; j < spf->sprites[i].largo * spf->sprites[i].alto; j++) {
			int color = spf->sprites[i].bitmap[j];
			if (mapa[color] == -1) {
				printf("Error optimizando paleta - %d - %d - color: %d\n", i, j, color);
				return 0;
			}
			spf->sprites[i].bitmap[j] = mapa[color];
		}
	}
}


struct SPF *cargarSPF(const char *archivo)
{
	int i;
	FILE *fp = fopen(archivo, "r");
	if (!fp)
		return NULL;

	struct SPF *spf = (struct SPF *) malloc(sizeof(struct SPF));

	u32 firma;
	leeU32(fp, &firma);

	if (FIRMA_SPF != firma) {
		printf("Firma no coincide. %x\n", firma);
		fclose(fp);
		return 0;
	}

	u32 tipo_paleta;
	leeU32(fp, &tipo_paleta);

	switch(tipo_paleta) {
	case PALETA_RGBA:
		leerPaletaRGBA(fp, spf);
		break;
	default:
		printf("Paleta no soportada.\n");
		fclose(fp);
		return 0;
	}

	u32 num_sprites;
	leeU32(fp, &num_sprites);

	printf("* Cantidad de Sprites: %d\n", num_sprites);

	spf->num_sprites = num_sprites;

	spf->sprites = (struct SPRITES *) malloc(sizeof(struct SPRITES) * num_sprites);
	
	char colores[MAX_COLORES];
	for (i = 0; i < MAX_COLORES; i++) {
		colores[i] = 0;
	}

	for (i = 0; i < num_sprites; i++) {
		s32 largo, alto;
		leeS32(fp, &largo);
		leeS32(fp, &alto);
		spf->sprites[i].largo = largo;
		spf->sprites[i].alto = alto;
		spf->sprites[i].bitmap = malloc(largo*alto);
		fread(spf->sprites[i].bitmap, largo*alto, 1, fp);
		int idx;
		for (idx = 0; idx < largo*alto; idx++) 
			colores[spf->sprites[i].bitmap[idx]]++;
		
		printf("    - Sprite %d: %dx%d\n", i+1, largo, alto);
	}

	int cant = 0;
	for (i = 0; i < MAX_COLORES; i++) {
		if (colores[i])	
			cant++;
	}
	printf("* Cantidad de colores usados: %d\n", cant);
	optimizarPaleta(spf, colores);

	fclose(fp);

	return spf;
}

int generaSalidaSPF(struct SPF *spf, const char *prefijo)
{
	char archivo_c[256];
	char archivo_h[256];

	snprintf(archivo_h,256,"%s.h", prefijo);
	snprintf(archivo_c,256,"%s.c", prefijo);

	printf("Generando: %s\n", archivo_h);
	FILE *fph = fopen(archivo_h, "w");
	if (!fph) {
		return 0;
	}
	fprintf(fph, "#ifndef __%s_H__\n",prefijo);	
	fprintf(fph, "#ifndef __%s_H__\n",prefijo);	
	fprintf(fph, "\n\n");	
	fprintf(fph, "#ifdef __cplusplus\n");	
	fprintf(fph, "extern \"C\" {\n");	
	fprintf(fph, "#endif\n",prefijo);	
	
	fprintf(fph, "#define %s_bits_per_color %d\n", prefijo, 8);
	fprintf(fph, "\n");	
	fprintf(fph, "#define %s_palette_colors %d\n", prefijo, spf->num_colores);
	fprintf(fph, "#define %s_palette_bits %d\n", prefijo, 16);
	fprintf(fph, "#define %s_palette_base %d\n", prefijo, 0);

	fprintf(fph, "extern unsigned short %s_palette[%d];\n", prefijo, spf->num_colores);
	fprintf(fph, "typedef struct\n" 
		     "{\n"
		     "     void *ptr;\n"
		     "     unsigned short sx,sy;\n"
		     "} type_sprite_list_icon;  // Sprite list Struct type\n");

	fprintf(fph,"\n");
	fprintf(fph,"#define %s_num_sprites %d // Number of Sprites\n", prefijo, spf->num_sprites);
	fprintf(fph,"extern type_sprite_list_%s %s_sprites[%d]; // Sprite List\n", prefijo, prefijo, spf->num_sprites);

	fprintf(fph,"\n");

	int i;
	for (i = 0; i < spf->num_sprites;i++) {
		fprintf(fph, "#define %s_sprite_%d_sx %d\n", prefijo, i+1, spf->sprites[i].largo);
		fprintf(fph, "#define %s_sprite_%d_sy %d\n", prefijo, i+1, spf->sprites[i].alto);
		fprintf(fph, "extern unsigned char %s_sprite_%d[%d];\n\n", prefijo, i+1, spf->sprites[i].largo * spf->sprites[i].alto);
	}

	fprintf(fph, "#ifdef __cplusplus\n");	
	fprintf(fph, "}\n");	
	fprintf(fph, "#endif\n",prefijo);	


	fprintf(fph, "#endif\n",prefijo);	

	fclose(fph);
	return 1;
}

int muestraPaleta(struct SPF *spf)
{
	int i;
	printf("Paleta: \n");
	for (i = 0; i < spf->num_colores; i++) {
		printf("\tColor %d: r: %d, g: %d, b: %d, a: %d\n", i + 1,
			spf->paleta[i].red,
			spf->paleta[i].green,
			spf->paleta[i].blue,
			spf->paleta[i].alpha);	
	}

	return 1;
}

int liberaSPF(struct SPF *spf)
{
	int i;
	for (i = 0;i < spf->num_sprites; i++) {
		free(spf->sprites[i].bitmap);
	}

	free(spf);
	return 1;
}

