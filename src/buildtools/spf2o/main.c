#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "spf.h"

int main(int argc, char **argv)
{
	const char *entrada = "icono.spf";
	const char *salida = "icon";

	struct SPF *sprites = cargarSPF(entrada);

	if (!sprites) {
		printf("Error leyendo el archivo %s.\n", entrada);
		exit(-1);
	}

	int res = generaSalidaSPF(sprites, salida);
	liberaSPF(sprites);

	if (!res) {
		printf("Error leyendo el archivo %s.\n", entrada);
		exit(-1);
	}
	return 0;
}

