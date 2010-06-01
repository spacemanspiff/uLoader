/*
	Copyright (C) 2010 Spaceman Spiff.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "fat.h"
#include "shared.h"

void showUsage ( const char *bin )
{
        printf ( "Uso: %s rutaNand instalar [<wads>|directorios]\n", bin );
        exit ( 0 );
}

int main ( int argc, const char **argv )
{
        printf ( "Instalador WiiWare - compatible uLoader 4.8+\n" );
        printf ( "(C) 2010 Hermes, Spaceman Spiff\n" );

        if ( argc < 3 )
                showUsage ( *argv );

        const char *nandPath = argv[1];
        const char *cmd = argv[2];

        if ( strcmp ( cmd, "crear" ) == 0 ) {
                // Create basic nand structure
                exit ( 0 );
        }

        printf ( "Probando nand en %s... ", nandPath );

        int init = init_nand ( nandPath );

        init_shared_list ( nandPath );

        if ( !init ) {
                printf ( "error, no es un directorio o no puedo acceder.\n" );
                exit ( -1 );
        }
        printf ( "exito.\n" );

        if ( strcmp ( cmd, "instalar" ) == 0 ) {
                FAT_Install ( nandPath, argv + 3, argc -3 );
        } else if ( strcmp ( cmd,"listar" ) == 0 ) {

        } else if ( strcmp ( cmd,"borrar" ) == 0 ) {

        } else if ( strcmp ( cmd,"chequear" ) == 0 ) {

        } else {
                printf ( "Comando %s no reconocido.\n", cmd );
        }
        update_shared_list ( nandPath );
        write_shared_list ( nandPath );
        release_shared_list();
        return 0;
}

