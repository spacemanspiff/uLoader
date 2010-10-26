/*
	From Custom IOS Module (FAT)

	Copyright (C) 2009 Waninkoko.
	Copyright (C) 2010 Hermes.
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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <malloc.h>

#include "fat.h"
#include "utils.h"

#include "wads.h"

int FAT_copy_file ( const char *filepath_ori, const char *filepath_dest )
{
        FILE *fp, *fp2;
        int ret=-1;
	int written;
        int len;
        int p;
        char *buffer;

        if ( !strcmp ( filepath_ori, filepath_dest ) )
                return 0;

        buffer= malloc ( 0x40000 );
        if ( !buffer )
                return -2;

        fp=fopen ( filepath_ori, "r" );
        if ( !fp ) {
                free ( buffer );
                return -1;
        }

        fseek ( fp, 0, SEEK_END );

        len=ftell ( fp );

        if ( len<=0 )
                goto error;

        fseek ( fp, 0, SEEK_SET );

        fp2=fopen ( filepath_dest, "w" );
        if ( !fp2 ) {
                ret=-3;
                goto error;
        }
        p=0;
        while ( p<len ) {
                ret=len-p;
                if ( ret>0x40000 )
                        ret=0x40000;

                ret=fread ( buffer, 1, ret, fp );

                if ( ret<0 )
                        goto error_w;
                if ( ret==0 )
                        break;

		written = fwrite ( buffer, 1, ret, fp2 );
                if (  written !=ret )
                        goto error_w;
                p+=ret;
        }

        ret=0;
        fclose ( fp2 );

error:

        free ( buffer );
        fclose ( fp );
        return ret;

error_w:
        free ( buffer );
        fclose ( fp );
        fclose ( fp2 );
        remove ( filepath_dest );
        return -4;
}

int FAT_write_file ( const char *filepath, void *buffer, int len )
{
        FILE *fp;
        int ret=-1;

        fp=fopen ( filepath, "w" );
        if ( !fp )
                goto error;

        ret=fwrite ( buffer, 1, len, fp );

        if ( ret!=len ) {
                ret=-2;
        } else {
                ret=0;
        }
        fclose ( fp );

error:

        return ret;
}

int FAT_read_file ( const char *filepath, void **buffer, int *len )
{
        FILE *fp;
        int ret=-1;

        *buffer=NULL;

        fp=fopen ( filepath, "r" );
        if ( !fp )
                return -1;

        fseek ( fp, 0, SEEK_END );

        *len=ftell ( fp );

        if ( *len<=0 )
                goto error;

//	if(*len==0) {
//		ret=0;
//		goto error;
//	}

        fseek ( fp, 0, SEEK_SET );

        *buffer= ( u8 * ) malloc ( *len+32 );

        if ( !*buffer ) {
                ret=-2;
                goto error;
        }

        ret=fread ( *buffer, 1, *len, fp );

        if ( ret!=*len ) {
                free ( *buffer );
                ret=-3;
                *buffer=NULL;
        } else {
                ret=0;
        }

error:

        fclose ( fp );
        return ret;
}


s32 FAT_DeleteDir ( const char *dirpath )
{
        DIR *dir;

        s32 ret;

        /* Open directory */
        dir = opendir ( dirpath );
        if ( !dir )
                return -1;

        /* Read entries */
        for ( ;; ) {
                char   filename[256], newpath[256];
                struct stat filestat;
                struct dirent *de;

                /* Read entry */
                de = readdir ( dir );
                if ( !de )
                        break;

                strcpy ( newpath, dirpath );
                strcat ( newpath,"/" );
                strcat ( newpath, de->d_name );

                if ( stat ( newpath, &filestat ) == -1 )
                        continue;

                /* Non valid entry */
                if ( de->d_name[0]=='.' )
                        continue;

                lower_caps ( filename );

                /* Generate entry path */
                strcpy ( newpath, dirpath );
                strcat ( newpath, "/" );
                strcat ( newpath, filename );

                /* Delete directory contents */
                if ( filestat.st_mode & S_IFDIR )
                        FAT_DeleteDir ( newpath );

                /* Delete object */
                ret = remove ( newpath );

                /* Error */
                if ( ret != 0 )
                        break;
        }

        /* Close directory */
        closedir ( dir );

        return 0;
}

static void FAT_Install_Dir ( const char *nandPath, const char *install_path )
{
        DIR *dir;

        /* Open directory */
        dir = opendir ( install_path );
        if ( !dir )
                return;

        /* Read entries */
        for ( ;; ) {
                char filename[256], newpath[256];
                struct stat filestat;
                struct dirent *de;

                /* Read entry */
                de = readdir ( dir );
                if ( !de )
                        break;

                sprintf ( newpath,"%s/%s", install_path, de->d_name );

                if ( stat ( newpath, &filestat ) == -1 )
                        continue;

                /* Non valid entry */
                if ( de->d_name[0]=='.' )
                        continue;
                printf ( "\tProcesando entrada %s - ", de->d_name );

                if ( ! ( filestat.st_mode & S_IFDIR ) ) {
                        lower_caps ( filename );
                        // test for wads
                        if ( !is_ext ( de->d_name,".wad" ) ) {
                                printf ( "no es un archivo wad - ignorado\n" );
                                continue;
                        }

                        printf ( "wad encontrado - instalando - " );
                        if ( Install_from_wad ( nandPath, newpath ) <0 ) {
                                printf ( "Error al instalar\n" );
                        } else {
                                printf ( "instalado correctamente.\n" );
                        }
                        continue;
                } else {
                        printf ( "es un directorio, ignorando\n" );
                }


        }
        /* Close directory */
        closedir ( dir );
}

void FAT_Install ( const char *nandPath, const char **install_paths, int count )
{
        struct stat stat_info;

        char dir_path[256];

        if ( stat ( nandPath, &stat_info ) != 0 ) {
                printf ( "No existe: %s\n", nandPath );
                return;
        }

        if ( !S_ISDIR ( stat_info.st_mode ) ) {
                printf ( "%s no es un directorio.", nandPath );
                return;
        }

        // make sure shared folder exist
        sprintf ( dir_path, "%s/shared", nandPath );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        sprintf ( dir_path, "%s/shared/00010001", nandPath );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        while ( count-- ) {
                const char *install_path = * ( install_paths++ );
                printf ( "Procesando: %s - ",install_path );

                if ( stat ( install_path, &stat_info ) != 0 ) {
                        printf ( "no existe esta entrada - ignorado.\n" );
                        continue;
                }

                if ( S_ISDIR ( stat_info.st_mode ) ) {
                        printf ( "encontrado directorio.\n" );
                        FAT_Install_Dir ( nandPath, install_path );
                }
                if ( S_ISREG ( stat_info.st_mode ) ) {
                        printf ( "encontrado archivo - " );
                        if ( !is_ext ( install_path,".wad" ) ) {
                                printf ( "no es un archivo wad - ignorado\n" );
                                continue;
                        }
                        if ( Install_from_wad ( nandPath, install_path ) <0 ) {
                                printf ( "Error al instalar\n" );
                        } else {
                                printf ( "instalado correctamente.\n" );
                        }
                }

        }
}

int init_nand ( const char *nandPath )
{
        char dir_path[256];
        struct stat dirstat;

        if ( stat ( nandPath, &dirstat ) == -1 )
                return 0;

        if ( ! ( dirstat.st_mode & S_IFDIR ) )
                return 0;

        sprintf ( dir_path, "%s/shared", nandPath );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        sprintf ( dir_path, "%s/shared/00010001", nandPath );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        return 1;

}


