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

#include "wads.h"
#include "rijndael.h"
#include "types.h"
#include "es.h"
#include "fat.h"
#include "utils.h"
#include "endian.h"
#include "wiidisc.h"
#include "shared.h"

static u8 title_iv[16];

int Install_from_wad ( const char *nandPath, const char *filename )
{
        FILE *fp  = NULL;
        FILE *fp2 = NULL;
        wadHeader *header=NULL;
        void *mem     =NULL;
        void *decrypt =NULL;
        u8 * tmd_data =NULL;
        u8 * original_tmd_data = NULL;

        u8 title_key[16];

        tmd *Tmd;
        tmd_content *Content;

        char *tik=NULL;

        u32 *title_id=0;

        short n;

        int offset;

        int error=0;

        int wad_len=0;

        int delete_content=0;

        char dir_path[256];


        fp=fopen ( filename, "r" );
        if ( !fp )
                return -1;

        header=malloc ( sizeof ( wadHeader ) );
        if ( !header ) {
                error=-2;
                goto error;
        }

        if ( fread ( header, 1, sizeof ( wadHeader ), fp ) !=sizeof ( wadHeader ) ) {
                error=-3;
                goto error;
        }
        fix_wad_header_endian ( header );

        offset= round_up ( header->header_len, 64 )
                + round_up ( header->certs_len, 64 )
                + round_up ( header->crl_len, 64 );


        tik=malloc ( round_up ( header->tik_len, 64 ) );
        if ( !tik ) {
                error=-2;
                goto error;
        }

        fseek ( fp, offset, SEEK_SET );

        if ( fread ( tik, 1, header->tik_len, fp ) !=header->tik_len ) {
                free ( tik );
                tik=NULL;
                error=-3;
                goto error;
        }


        offset+= round_up ( header->tik_len, 64 );

        original_tmd_data= ( u8 * ) malloc ( header->tmd_len );
        tmd_data= ( u8 * ) malloc ( header->tmd_len );
        if ( !tmd_data ) {
                error=-2;
                goto error;
        }

        fseek ( fp, 0, SEEK_END );
        wad_len=ftell ( fp );

        fseek ( fp, offset, SEEK_SET );

        if ( fread ( tmd_data, 1, header->tmd_len, fp ) !=header->tmd_len ) {
                error=-3;
                goto error;
        }

        memcpy ( original_tmd_data, tmd_data, header->tmd_len );

        offset += round_up ( header->tmd_len, 64 );

        //printf("tmd: %02x %02x %02x %02x\n", tmd_data[0], tmd_data[1], tmd_data[2], tmd_data[3]);
        Tmd= ( tmd* ) SIGNATURE_PAYLOAD ( ( signed_blob * ) tmd_data );

        fix_tmd_endian ( Tmd );

        Content = Tmd->contents;

        title_id= ( u32 * ) ( void * ) &Tmd->title_id;

        // create folder destination

        sprintf ( dir_path, "%s/title", nandPath );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );
        sprintf ( dir_path, "%s/title/%08lx", nandPath, title_id[0] );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );
        sprintf ( dir_path, "%s/title/%08lx/%08lx", nandPath, title_id[0], title_id[1] );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );
        sprintf ( dir_path, "%s/title/%08lx/%08lx/content", nandPath, title_id[0], title_id[1] );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );
        sprintf ( dir_path, "%s/title/%08lx/%08lx/data", nandPath, title_id[0], title_id[1] );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        if ( 1 ) {
                sprintf ( dir_path, "%s/ticket", nandPath );
                mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );
                sprintf ( dir_path, "%s/ticket/%08lx", nandPath, title_id[0] );
                mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

                sprintf ( dir_path, "%s/ticket/%08lx/%08lx.tik", nandPath, title_id[0], title_id[1] );
                if ( FAT_write_file ( dir_path, tik, header->tik_len ) < 0 ) {
                        free ( tik );
                        tik=NULL;
                        error=-9;
                        goto error;
                }
        }

        delete_content=1;
        // get title_key for decript content
        _decrypt_title_key ( ( void * ) tik, title_key );
        aes_set_key ( title_key );

        // create shared folder
        sprintf ( dir_path, "%s/shared/%08lx", nandPath, title_id[0] );
        mkdir ( dir_path, S_IRWXO | S_IRWXG | S_IRWXU );

        free ( tik );
        tik=NULL;

        decrypt= ( u8 * ) malloc ( 256*1024+32 );
        if ( !decrypt ) {
                error=-2;
                goto error;
        }

        mem= ( u8 * ) malloc ( 256*1024+32 );
        if ( !mem ) {
                error=-2;
                goto error;
        }

        for ( n=0; n < Tmd->num_contents; n++ ) {
                int len = Content[n].size; //round_up(Content[n].size, 64);
                int is_shared=0;
                int readed=0;
                int size;
                int decryp_state;

                sprintf ( dir_path, "%s/title/%08lx/%08lx/content/%08lx.app", nandPath, title_id[0], title_id[1], Content[n].cid );

                fp2=fopen ( dir_path, "wb" );
                if ( !fp2 ) {
                        error=-4;
                        goto error;
                }

                // fix private content
                if ( Content[n].type & 0xC000 ) {
                        is_shared=1;
//	    printf("file: %s is shared\n", dir_path);
                }

                memset ( title_iv, 0, 16 );
                short fixed_endian_n = be_u16 ( n );
                memcpy ( title_iv, &fixed_endian_n, 2 );

                decryp_state=0; // start

                fseek ( fp, offset, SEEK_SET );
                //size=ftell(fp);
                //if(size>=0 && size<wad_len)
                while ( readed<len ) {
                        int res=0;
			int written;

                        size = ( len - readed );
                        if ( size > 256*1024 )
                                size = 256*1024;

                        memset ( mem ,0, size );
                        res = fread ( mem, 1, size, fp );
                        if ( res<0 ) {
                                error=-6;
                                goto error;
                        }
                        if ( res==0 )
                                break;

                        if ( ( readed+size ) >=len || size<256*1024 )
                                decryp_state=1; // end

                        aes_decrypt2 ( title_iv, mem, decrypt, size, decryp_state );

			written = fwrite ( decrypt, 1, size, fp2 );
                        if (  written !=size ) {
                                error=-4;
                                goto error;
                        }
                        readed += size;
                }

                fclose ( fp2 );
                fp2 = NULL;

                offset += round_up ( Content[n].size, 64 );

                if ( readed==0 ) {
                        if ( !is_shared ) {
                                // special for non title 0x00010001
                                if ( title_id[0]!=0x00010001 ) {
                                        // remove content
                                        sprintf ( dir_path, "%s/title/%08lx/%08lx/content/%08lx.app", nandPath, title_id[0], title_id[1], Content[n].cid );
                                        remove ( dir_path );
                                        break;
                                }
                                error = -8;
                                goto error;
                        } else {
                                sprintf ( dir_path, "%s/title/%08lx/%08lx/content/%08lx.app", nandPath, title_id[0], title_id[1], Content[n].cid );
                                remove ( dir_path );
                        }
                } else {
                }
        }

        //fclose(fp);
        //fp=fp2=NULL;

        // save the original tmd
        sprintf ( dir_path, "%s/title/%08lx/%08lx/content/title.tmd", nandPath, title_id[0], title_id[1] );
        if ( FAT_write_file ( dir_path, original_tmd_data, header->tmd_len ) <0 ) {
                error=-7;
                goto error;
        }

error:

        if ( mem )
                free ( mem );

        if ( decrypt )
                free ( decrypt );

        if ( tmd_data )
                free ( tmd_data );

        if ( original_tmd_data )
                free ( original_tmd_data );

        if ( header )
                free ( header );

        if ( fp )
                fclose ( fp );

        if ( fp2 )
                fclose ( fp2 );

        if ( error ) {
                char mess[4096];
                switch ( -error ) {
                case 2:
                        sprintf ( mess,"Out of Memory" );
                        break;

                case 3:
                        sprintf ( mess,"Error Reading WAD" );
                        break;

                case 4:
                        sprintf ( mess,"Error Creating Content" );
                        break;

                case 5:
                        sprintf ( mess,"Error Creating Shared Content" );
                        break;

                case 6:
                        sprintf ( mess,"Error Reading Content" );
                        break;

                case 7:
                        sprintf ( mess,"Error Creating TMD" );
                        break;

                case 8:
                        sprintf ( mess,"Error truncated WAD" );
                        break;

                case 9:
                        sprintf ( mess,"Error Creating TIK" );
                        break;

                default:
                        sprintf ( mess,"Undefined Error" );
                        break;
                }
                printf ( "Error: %s", mess );

                if ( title_id && error && delete_content ) {
                        // deletes the content
                        sprintf ( dir_path, "%s/title/%08lx/%08lx", nandPath, title_id[0], title_id[1] );
                        FAT_DeleteDir ( dir_path );
                        remove ( dir_path );
                }
        }

        //if ( !error && title_id && title_id[0]==0x00010001 )
        //    scan_for_shared (nandPath);

        return error;
}
