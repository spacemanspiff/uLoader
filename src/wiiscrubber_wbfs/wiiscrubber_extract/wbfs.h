#ifndef WBFS_H
#define WBFS_H

//#include <windows.h>
//#include <wchar.h>
#include "global.h"

#include <stdio.h>

#define GB (1024 * 1024 * 1024.)

#define wbfs_fatal(x) fatal(x)
#define wbfs_error(x) fatal(x)

#include <stdlib.h>
#define wbfs_malloc(x) malloc(x)
#define wbfs_free(x) free(x)

// alloc memory space suitable for disk io
#define wbfs_ioalloc(x) malloc(x)
#define wbfs_iofree(x) free(x)

//#include <arpa/inet.h>

// endianess tools
#define wbfs_ntohl(x) ntohl(x)
#define wbfs_ntohs(x) ntohs(x)
#define wbfs_htonl(x) htonl(x)
#define wbfs_htons(x) htons(x)

#include <string.h>
#define wbfs_memcmp(x,y,z) memcmp(x,y,z)
#define wbfs_memcpy(x,y,z) memcpy(x,y,z)
#define wbfs_memset(x,y,z) memset(x,y,z)

typedef u32 be32_t;
typedef u16 be16_t;

// callback definition. Return 1 on fatal error (callback is supposed to make retries until no hopes..)
typedef int (*rw_sector_callback_t)(void*fp,u32 lba,u32 count,void*iobuf);
typedef void (*progress_callback_t)(int status,int total);
typedef void (*close_callback_t)(void*fp);

#define WBFS_MAGIC (('W'<<24)|('B'<<16)|('F'<<8)|('S'))
/*
#ifdef WIN32
#pragma pack(1)
#endif
*/
typedef struct wbfs_head
{
        be32_t magic;
        // parameters copied in the partition for easy dumping, and bug reports
        be32_t n_hd_sec;	       // total number of hd_sec in this partition
        u8  hd_sec_sz_s;       // sector size in this partition
        u8  wbfs_sec_sz_s;     // size of a wbfs sec
        u8  padding3[2];
        u8  disc_table[0];	// size depends on hd sector size
}
/*
#ifndef WIN32
__attribute((packed))
#endif
*/
wbfs_head_t;
/*
#ifdef WIN32
#pragma pack()
#endif
*/

typedef struct wbfs_disc_info
{
        u8 disc_header_copy[0x100];
        be16_t wlba_table[0];
}wbfs_disc_info_t;

typedef struct wbfs_s
{
        wbfs_head_t *head;

        /* hdsectors, the size of the sector provided by the hosting hard drive */
        u32 hd_sec_sz;
        u8  hd_sec_sz_s; // the power of two of the last number
        u32 n_hd_sec;	 // the number of hd sector in the wbfs partition

        /* standard wii sector (0x8000 bytes) */
        u32 wii_sec_sz; 
        u8  wii_sec_sz_s;
        u32 n_wii_sec;
        u32 n_wii_sec_per_disc;
        
        /* The size of a wbfs sector */
        u32 wbfs_sec_sz;
        u32 wbfs_sec_sz_s; 
        u16 n_wbfs_sec;   // this must fit in 16 bit!
        u16 n_wbfs_sec_per_disc;   // size of the lookup table

        u32 part_lba;
        /* virtual methods to read write the partition */
        rw_sector_callback_t read_hdsector;
        rw_sector_callback_t write_hdsector;
		close_callback_t close_hd;

        void *callback_data;

        u16 max_disc;
        u32 freeblks_lba;
        u32 *freeblks;
        u16 disc_info_sz;

        u8  *tmp_buffer;  // pre-allocated buffer for unaligned read
        
        u32 n_disc_open;
       
}wbfs_t;


typedef struct wbfs_disc_s
{
        wbfs_t *p;
        wbfs_disc_info_t  *header;	  // pointer to wii header
        int i;		  		  // disc index in the wbfs header (disc_table)
}wbfs_disc_t;

wbfs_t *wbfs_try_open(char *disc, char *partition, int reset);
wbfs_disc_t *wbfs_open_disc(wbfs_t* p, u8 *discid);
void wbfs_close(wbfs_t*p);
void wbfs_close_disc(wbfs_disc_t*d);
u32 wbfs_get_disc_info(wbfs_t*p, u32 index,u8 *header,int header_size,u32 *size);//size in 32 bit
u32 wbfs_count_discs(wbfs_t*p);
bool wbfs_read_disc(wbfs_disc_t*d, u64 offset, s32 size, char *dst);

#endif