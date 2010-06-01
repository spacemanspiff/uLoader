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

#include "es.h"
#include "fat.h"
#include "utils.h"
#include "endian.h"
#include "wiidisc.h"

static shared_entry *shared_list=NULL;
static int shared_entries=0;

int update_shared_list ( const char *nandPath )
{
        char dirpath[256];
        DIR *dir;
        void *title_tmd;
        int title_tmd_len;

        tmd *Tmd;
        tmd_content *Content;

        u8 title_ios;
        u32 title_id;

        int n;
        int m;

        sprintf ( dirpath, "%s/title/00010001", nandPath );

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
                strcat ( newpath, "/" );
                strcat ( newpath, de->d_name );

                if ( stat ( newpath, &filestat ) == -1 )
                        continue;

                /* Non valid entry */
                if ( de->d_name[0]=='.' )
                        continue;

                if ( ! ( filestat.st_mode & S_IFDIR ) ) continue;

                lower_caps ( filename );

                /* Generate entry path */
                strcpy ( newpath, dirpath );
                strcat ( newpath, "/" );
                strcat ( newpath, de->d_name );
                strcat ( newpath, "/content/#title.tmd" );

                n=FAT_read_file ( newpath, &title_tmd, &title_tmd_len );
                if ( n<0 ) {
                        /* Generate entry path */
                        strcpy ( newpath, dirpath );
                        strcat ( newpath, "/" );
                        strcat ( newpath, de->d_name );
                        strcat ( newpath, "/content/title.tmd" );
                        n=FAT_read_file ( newpath, &title_tmd, &title_tmd_len );
                }
                //printf("shared_content2: read_file: %s\n", newpath);

                if ( n==0 ) {
                        Tmd= ( tmd* ) SIGNATURE_PAYLOAD ( ( signed_blob * ) title_tmd );

                        title_ios = ( ( ( u8 * ) title_tmd ) [0x18b] );

                        fix_tmd_endian ( Tmd );
                        title_id= * ( ( ( u32 * ) ( void * ) &Tmd->title_id ) +1 );
                        Content = Tmd->contents;

                        for ( n=0;n<Tmd->num_contents;n++ ) {
                                if ( Content[n].index==0 )
                                        continue;
                                if ( ! ( Content[n].type & 0x8000 ) )
                                        continue;

                                // shared content: search
                                for ( m=0;m<shared_entries;m++ ) {
                                        if ( memcmp ( shared_list[m].hash, Content[n].hash, 20 ) == 0 )
                                                break;
                                }

                                if ( m==shared_entries ) { // new entry in database
                                        memcpy ( shared_list[m].hash, Content[n].hash, 20 );
                                        shared_list[m].title_id= title_id;
                                        shared_list[m].cindex= Content[n].cid;
                                        shared_list[m].ios=  title_ios;
                                        shared_list[m].minor_ios=  title_ios;
                                        shared_list[m].n_shared=1;
                                        shared_entries++;
                                } else {
                                        // if new>old update database
                                        if ( ( u32 ) title_ios > ( u32 ) shared_list[m].ios ) {
                                                FILE *fp;
                                                sprintf ( newpath,"%s/%08lx/content/%08lx.app", dirpath, title_id, Content[n].cid );

                                                // if content exist update
                                                fp=fopen ( newpath, "r" );
                                                if ( fp ) {
                                                        fclose ( fp );
                                                        sprintf ( newpath, "%s/shared/00010001/%08lx%08lx.app", nandPath, shared_list[m].title_id, shared_list[m].cindex );
                                                        remove ( newpath );
                                                        shared_list[m].title_id= title_id;
                                                        shared_list[m].cindex= Content[n].cid;
                                                        shared_list[m].ios=  title_ios;
                                                        //shared_list[m].n_shared=0;
                                                }
                                        }

                                        if ( ( u32 ) title_ios < ( u32 ) shared_list[m].minor_ios )
                                                shared_list[m].minor_ios=  title_ios;

                                        shared_list[m].n_shared++;
                                        //printf("saliendo shared_list[%d].n_shared=%d\n", m, shared_list[m].n_shared);
                                }
                        }
                }
        }

        /* Close directory */
        closedir ( dir );
        return 0;
}

int write_shared_list ( const char *nandPath )
{
        char dir_path[256];
        char dir_path2[256];
        FILE *fp, *fp2;
        sprintf ( dir_path, "%s/shared/#shared.dtb", nandPath );
        if ( shared_entries ) {
                int n = 1;
                void *data=NULL;
                int len_data;

                // Check if shared database needs updating
                if ( FAT_read_file ( dir_path, &data, &len_data ) ==0 ) {
                        if ( len_data== ( (int) sizeof ( shared_entry ) *shared_entries ) && len_data!=0 ) {
                                int entries = len_data / sizeof ( shared_entry );
                                int i = 0;
                                int different = 0;
                                while ( i < entries && !different ) {
                                        shared_entry *e = ( shared_entry * ) ( data + i*sizeof ( shared_entry ) );
                                        fix_shared_entry_endian ( e );
                                        if ( memcmp ( data,e, sizeof ( shared_entry ) ) != 0 )
                                                different=1;
                                        i++;
                                }
                                if ( !different )
                                        n=0;
                        }
                        free ( data );
                        data=NULL;
                }

                if ( n ) {
                        //printf("Rebuilding shared content database.\n");
                        fp=fopen ( dir_path, "w" ); // update the database list
                } else {
                        //printf("Shared content database is already updated.\n");
                        fp=NULL; // only test/update the shared content
                }

                if ( fp ) {
                        for ( n=0; n<shared_entries; n++ ) {
                                sprintf ( dir_path, "%s/shared/00010001/%08lx%08lx.app", nandPath, shared_list[n].title_id, shared_list[n].cindex );

                                // if n_shared == 0 -> this shared entry is not needed anymore
                                if ( shared_list[n].n_shared == 0 ) {
                                        // remove if it is present
                                        remove ( dir_path );
                                        continue;
                                }
                                fp2=fopen ( dir_path,"r" );
                                if ( fp2 ) {
                                        fclose ( fp2 ); // if exist don't write the content
                                } else {
                                        sprintf ( dir_path2, "%s/title/00010001/%08lx/content/%08lx.app", nandPath, shared_list[n].title_id, shared_list[n].cindex );
                                        if ( FAT_copy_file ( dir_path2, dir_path ) <0 ) {
                                                continue;
                                        }
                                }
                                // Write shared entry to the database
                                fix_shared_entry_endian ( &shared_list[n] );
                                fwrite ( &shared_list[n], sizeof ( shared_entry ), 1, fp );
                        }
                        fclose ( fp );
                }
        }
        return 1;
}

void init_shared_list ( const char *nandPath )
{
        char dir_path[256];
        void *data=NULL;
        int len_data;

        shared_list= ( void * ) malloc ( sizeof ( shared_list ) * 2048 );
        if ( !shared_list )
                return ;

        memset ( shared_list, 0, sizeof ( shared_list ) *2048 );
        shared_entries=0;

        // Try to load existing database
        sprintf ( dir_path, "%s/shared/#shared.dtb", nandPath );
        if ( FAT_read_file ( dir_path, &data, &len_data ) ==0 ) {
                int offset = 0;
                while ( offset + ((int) sizeof ( shared_entry )) <= len_data ) {
                        memcpy ( ( u8 * ) ( shared_list +shared_entries ), ( data + offset ), sizeof ( shared_entry ) );
                        fix_shared_entry_endian ( shared_list + shared_entries );
                        shared_list[shared_entries].n_shared = 0; // Reset count
                        shared_entries++;
                        offset += sizeof ( shared_entry );
                }
        }
        free ( data );
        data=NULL;
}

void release_shared_list()
{
        if ( shared_list )
                free ( shared_list );
}

