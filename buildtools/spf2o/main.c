#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "spf.h"

int main(int argc, char **argv)
{
	const char *entrada = argv[1];
	const char *prefijo = "icon";
	const char *salida = argv[2];

	struct SPF *sprites = cargarSPF(entrada);

	if (!sprites) {
		printf("Error leyendo el archivo %s.\n", entrada);
		exit(-1);
	}

	int res = generaSalidaSPF(sprites, salida, prefijo);
	liberaSPF(sprites);

	if (!res) {
		printf("Error leyendo el archivo %s.\n", entrada);
		exit(-1);
	}
	return 0;
}

