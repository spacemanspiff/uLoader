// ported by Hermes from wiiscrubber_extract

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "wiiscrubber.h"

#ifndef ATTRIBUTE_ALIGN
# define ATTRIBUTE_ALIGN(v)	__attribute__((aligned(v)))
#endif
#ifndef ATTRIBUTE_PACKED
# define ATTRIBUTE_PACKED	__attribute__((packed))
#endif

#define AES_DECRYPT 1



void nullprintf(char *c, ...)
{
}

#define AddToLog nullprintf

#define ROUNDUP64B(x) (((u64)(x) + 64 - 1) & ~(64 - 1))


#define SIZE_H0						0x0026CUL
#define SIZE_H1						0x000A0UL
#define SIZE_H2						0x000A0UL
#define SIZE_H3						0x18000UL
#define SIZE_H4						0x00014UL
#define SIZE_PARTITION_HEADER		0x20000UL
#define SIZE_CLUSTER				0x08000UL
#define SIZE_CLUSTER_HEADER			0x00400UL
#define SIZE_CLUSTER_DATA			(SIZE_CLUSTER - SIZE_CLUSTER_HEADER)

/*
 * ADDRESSES
 */
/* Absolute addresses */
#define OFFSET_GAME_TITLE			0x00020UL
#define OFFSET_PARTITIONS_INFO		0x40000UL
#define OFFSET_REGION_BYTE			0x4E003UL
#define OFFSET_REGION_CODE			0x4E010UL
/* Relative addresses */
#define OFFSET_H0					0x00000UL
#define OFFSET_H1					0x00280UL
#define OFFSET_H2					0x00340UL
#define OFFSET_PARTITION_TITLE_KEY	0x001BFUL
#define OFFSET_PARTITION_TITLE_ID	0x001DCUL
#define OFFSET_PARTITION_TMD_SIZE	0x002A4UL
#define OFFSET_PARTITION_TMD_OFFSET	0x002A8UL
#define OFFSET_PARTITION_H3_OFFSET	0x002B4UL
#define OFFSET_PARTITION_INFO		0x002B8UL
#define OFFSET_CLUSTER_IV			0x003D0UL
#define OFFSET_FST_NB_FILES			0x00008UL
#define OFFSET_FST_ENTRIES			0x0000CUL
#define OFFSET_TMD_HASH				0x001F4UL

/*
 * OTHER
 */
#define NB_CLUSTER_GROUP			64
#define NB_CLUSTER_SUBGROUP			8

enum tmd_sig {
        SIG_UNKNOWN = 0,
        SIG_RSA_2048,
        SIG_RSA_4096
};

struct tmd_content {
        u32 cid;
        u16 index;
        u16 type;
        u64 size;
        u8 hash[20];
};

struct tmd {
        enum tmd_sig sig_type; 
        u8 *sig;
        char issuer[64];
        u8 version;
        u8 ca_crl_version;
        u8 signer_crl_version;
        u64 sys_version;
        u64 title_id;
        u32 title_type;
        u16 group_id;
        u32 access_rights;
        u16 title_version;
        u16 num_contents;
        u16 boot_index;
        struct tmd_content *contents;
};


struct part_header {
        char console;
        u8 is_gc;
        u8 is_wii;

        char gamecode[2];
        char region;
        char publisher[2];

        u8 has_magic;
        char name[0x60];

        u64 dol_offset;
        u64 dol_size;

        u64 fst_offset;
        u64 fst_size;
};

enum partition_type {
        PART_UNKNOWN = 0,
        PART_DATA,
        PART_UPDATE,
        PART_INSTALLER,
        PART_VC
};

struct partition {
        u64 offset;

        struct part_header header;

        u64 appldr_size;

        u8 is_encrypted;

        u64 tmd_offset;
        u64 tmd_size;

        struct tmd * tmd;

		u64	h3_offset;

        char title_id_str[17];

        enum partition_type type;
        char chan_id[5];

        char key_c[35];
        //AES_KEY key;
		//u8 key[16];

		u8 title_key[16];

        u64 data_offset;
        u64 data_size;

        u64 cert_offset;
        u64 cert_size;

        u8 dec_buffer[0x8000] ATTRIBUTE_ALIGN(32);

        u32 cached_block;
        u8 cache[0x7c00] ATTRIBUTE_ALIGN(32);
};

struct image_file {
		wbfs_disc_t *d;
		bool encrypted;


        u8 is_wii;

        u32 nparts;
        struct partition *parts;

        u64 nfiles;
        u64 nbytes;

		u8	PartitionCount;
		u8	ChannelCount;

		u64 part_tbl_offset;
		u64	chan_tbl_offset;

		u8 key[16];
};


static unsigned char * pFreeTable;
static unsigned char * pBlankSector;
static unsigned char * pBlankSector0;

static s64 nImageSize;

iso_files * CWIIDisc_first_file=NULL;
iso_files * CWIIDisc_last_file=NULL;

void aes_set_key(u8 *key);
void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
int wbfs_wiiscrub_read_disc(wbfs_disc_t*d, u64 offset, s32 size, char *dst);

void AES_cbc_encrypt(unsigned char *in, unsigned char *out,
		     unsigned long length, u8 *key,
		     unsigned char *ivec, int enc) {

static u8 *oldkey=NULL;

if(key!=oldkey)
	{
	aes_set_key(key);
	oldkey=key;
	}

if(enc==AES_DECRYPT)
	aes_decrypt(ivec, in, out, length);
else 
	aes_encrypt(ivec, in, out, length);
}

static char	hPartition[20][255];
static char hDisc[255];

static char m_csText[255];

static char m_csFilename[255];

u16 be16 (const u8 *p) {
	return (p[0] << 8) | p[1];
}

u32 be32 (const u8 *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

u64 be64 (const u8 *p) {
	return ((u64) be32 (p) << 32) | be32 (p + 4);
}


size_t g_strnlen (const char *s, size_t size) {
        size_t i;

        for (i = 0; i < size; ++i)
                if (s[i] == 0)
                        return i;

        return size;
}

u32 get_dol_size (const u8 *header) {
        u8 i;
        u32 offset, size;
        u32 max = 0;

        // iterate through the 7 code segments
        for (i = 0; i < 7; ++i) {
                offset = be32 (&header[i * 4]);
                size = be32 (&header[0x90 + i * 4]);
                if (offset + size > max)
                        max = offset + size;
        }

        // iterate through the 11 data segments
        for (i = 0; i < 11; ++i) {
                offset = be32 (&header[0x1c + i * 4]);
                size = be32 (&header[0xac + i * 4]);
                if (offset + size > max)
                        max = offset + size;
        }

        return max;
}

/////////////////////////////////////////////
void MarkAsUsed(u64 nOffset, u64 nSize);
void MarkAsUsedDC(u64 nPartOffset, u64 nOffset, u64 nSize, bool bIsEncrypted);
int CWIIDisc_CheckAndLoadKey(int bLoadCrypto, struct image_file *image);

#define CONST_TABLE (587497*2) //((u64)(4699979776) / (u64)(0x8000))*2
void CWIIDisc_create()
{
	int i;

	CWIIDisc_first_file = NULL;
	// create and blank the wii blank table
	pFreeTable = (unsigned char *) memalign(32, CONST_TABLE);
	//set them all to clear first
	memset(pFreeTable, 0, CONST_TABLE);
	// then set the header size to used
	MarkAsUsed(0, 0x40000);

	pBlankSector = (unsigned char *) memalign(32, 0x8000);
	memset(pBlankSector, 0xFF, 0x8000);

	pBlankSector0 = (unsigned char *) memalign(32, 0x8000);
	memset (pBlankSector0, 0, 0x8000);


	for (i = 0; i < 20; i++)
	{
		strcpy(hPartition[i],"");
	}
	strcpy(hDisc,"");
	
}

void CWIIDisc_destroy()
{
	// free up the memory
	free(pFreeTable);
	free(pBlankSector);
	free(pBlankSector0);
}

extern s32 WDVD_UnencryptedRead(void *buf, u32 len, u64 offset);

extern int is_fat;

static u32 table_lba[2048];
static u8 mem_index[2048];
static u8 buff[32768];
static int ciso_size;


static int one=1;
static FILE * fp_disc=NULL;


int CWIIDisc_io_read (unsigned char  *ptr, size_t size, struct image_file *image, u64 offset)
{
	u32 my_offset=(u32) (offset/4);
	#if 0
	if (image->fp)
	{
        size_t bytes;

		__int64 nSeek;

		nSeek = _lseeki64 (image->fp, offset, SEEK_SET);

        if (-1==nSeek) {
			DWORD x = GetLastError();
                ///AfxMessageBox("io_seek");
                return -1;
        }

		MarkAsUsed(offset, size);

        bytes = (size_t)_read(image->fp, ptr, (unsigned int)size);

        //if (bytes != size)
                ///AfxMessageBox("io_read");

        return (int)bytes;
	}
	
	else	
		
	#else

	if(is_fat)
		{
		int ret,l;
		

		if(one)
			{
			u32 lba_glob=(16)<<9;

			fseek(fp_disc, 0, SEEK_SET);
			
			ret=fread(buff,1, 32768 ,fp_disc);
				if(ret<=0) {return 0;}
				else
					if(!(buff[0]=='C' && buff[1]=='I' && buff[2]=='S' && buff[3]=='O')) return 0;
			ciso_size=(((u32)buff[4])+(((u32)buff[5])<<8)+(((u32)buff[6])<<16)+(((u32)buff[7])<<24))/4;
		
			memset(mem_index,0,2048);


			for(l=0;l<16384;l++)
				{

				if((l & 7)==0) table_lba[l>>3]=lba_glob;
				
				if(buff[(8+l)])
					{
					mem_index[l>>3]|=1<<(l & 7);
					lba_glob+=ciso_size;
					}
				}
            
			one=0;
			}

		if(!one)
			{
			u32 temp=((u32) my_offset)/ciso_size;
	

			u32 read_lba=table_lba[temp>>3];

			for(l=0;l<(temp & 7);l++) if((mem_index[temp>>3]>>l) & 1) read_lba+=ciso_size;

			read_lba<<=2;

			read_lba+=(offset) & (((ciso_size)<<2)-1);
			

			fseek(fp_disc, read_lba, SEEK_SET);
			ret=fread(ptr,1, size ,fp_disc);


			
			/*if(ret>0)
				{
				
				if(read_lba+ret>=1024 && read_lba<1024*201)
					{
					l=1024*201-(read_lba-1024);
					if(l>ret) l=ret;
                    
					memset(ptr+read_lba-1024,0, l);
					}
				return ret;
				}
			*/
			return ret;
		
			}
		//exit(0);
		}
	// from WBFS
	if (image->d)
	{

		return size *(wbfs_wiiscrub_read_disc(image->d, offset, size ,(char*)ptr));
	} 
	// from DVD DISC
	else
	{
	void *temp_ptr;
	int size2=(size+31+(offset & 3)) & ~31;

		temp_ptr=memalign(32, size2+32);

		if(!temp_ptr) return 0;


		if(WDVD_UnencryptedRead(temp_ptr, size2, offset & ~3)<0) size=0;


		if(size)
			{
			memcpy(ptr, temp_ptr+(offset & 3), size);
			}

		free(temp_ptr);

	return size;
	}
	//else return 0;

	return 0;
	#endif
}

int CWIIDisc_io_open(struct image_file *image, wbfs_disc_t *d)
{
	#if 0
	if (filename[1] != '-')
	{
        int fp = _open (filename, _O_BINARY|_O_RDWR);
		image->fp = fp;
		image->d = NULL;

		image->encrypted = 1;

		return fp;
	} else
	#else
	{
		//wbfs_t *p;
		//p = wbfs_try_open(NULL, (char*)filename, 0);
		//if (!p) return 0;

		image->d = d;
		//image->fp = NULL;

		image->encrypted = 1;

		return 1;
	}
	
	

		return 0;
	#endif
}

void CWIIDisc_io_close(struct image_file *image)
{
	#if 0
	if (image->fp)
	{
		_close(image->fp);
	} 
	
	else
	#endif
	if (image->d)
	{
		//wbfs_close_disc(image->d);
		//wbfs_close(image->d->p);
	}
	
}
int CWIIDisc_io_size(struct image_file *image)
{
	#if 0 
	
	if (image->fp)
	{
		return _lseeki64(image->fp, 0L, SEEK_END);
	}
	
		else
			
	#endif
	if (image->d)
	{
		int i;
		u32 size=0;
		u8 *b = (u8*)wbfs_ioalloc(0x100);
		int count = wbfs_count_discs(image->d->p);
		if(!b) return 0;

		for (i = 0; i < count; i++)
		{
		size=0;
		
		memset(b, 0, 0x100);

			if (!wbfs_get_disc_info(image->d->p, i, b, 0x100, &size))
			{
				if (strcmp((const char*)b,(const char*)image->d->header->disc_header_copy) == 0) break;
//				printf( "%c%c%c%c%c%c %40s %.2fG\n", b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20, size * 4ULL / (GB));
			}
		}

		free(b);

		return size;
	}
	
	return 0;
}

int CWIIDisc_decrypt_block (struct image_file *image, u32 part, u32 block)
{
        if (block == image->parts[part].cached_block)
                return 0;


        if (CWIIDisc_io_read (image->parts[part].dec_buffer, 0x8000, image,
                     image->parts[part].offset +
                     image->parts[part].data_offset + (u64)(0x8000) * (u64)(block))
            != 0x8000) {
///                AfxMessageBox("decrypt read");
                return -1;
        }

#if 1
        AES_cbc_encrypt (&image->parts[part].dec_buffer[0x400],
                         image->parts[part].cache, 0x7c00,
                         (void *) &image->parts[part].title_key,
                         (void *) &image->parts[part].dec_buffer[0x3d0], AES_DECRYPT);
#endif

        image->parts[part].cached_block = block;

        return 0;
}

size_t CWIIDisc_io_read_part (unsigned char *ptr, size_t size, struct image_file *image, u32 part, u64 offset)
{
        u32 block = (u32)(offset / (u64)(0x7c00));
        u32 cache_offset = (u32)(offset % (u64)(0x7c00));
        u32 cache_size;
        unsigned char *dst = ptr;

        if (!image->parts[part].is_encrypted)
                return CWIIDisc_io_read (ptr, size, image,
                                image->parts[part].offset + offset);

        while (size) {
                if (CWIIDisc_decrypt_block (image, part, block))
                        return (dst - ptr);

                cache_size = size;
                if (cache_size + cache_offset > 0x7c00)
                        cache_size = 0x7c00 - cache_offset;

                memcpy (dst, image->parts[part].cache + cache_offset,
                        cache_size);
                dst += cache_size;
                size -= cache_size;
                cache_offset = 0;

                block++;
        }

        return dst - ptr;
}
void MarkAsUsed(u64 nOffset, u64 nSize)
{
		u64 nStartValue = nOffset;
		u64 nEndValue = nOffset + nSize;
		while((nStartValue < nEndValue)&&
			  (nStartValue < ((u64)(4699979776ULL) * (u64)(2ULL))))
		{

			pFreeTable[nStartValue / (u64)(0x8000)] = 1;
			nStartValue = nStartValue + ((u64)(0x8000));
		}

}
void MarkAsUsedDC(u64 nPartOffset, u64 nOffset, u64 nSize, bool bIsEncrypted)
{
	u64 nTempOffset;
	u64 nTempSize;
	
	if (bIsEncrypted!=0)
	{
		// the offset and size relate to the decrypted file so.........
		// we need to change the values to accomodate the 0x400 bytes of crypto data
		
		nTempOffset = nOffset / (u64)(0x7c00);
		nTempOffset = nTempOffset * ((u64)(0x8000));
		nTempOffset += nPartOffset;
		
		nTempSize = nSize / (u64)(0x7c00);
		nTempSize = (nTempSize + 1) * ((u64)(0x8000));
		
		// add on the offset in the first nblock for the case where data straddles blocks
		
		nTempSize += nOffset % (u64)(0x7c00);
	}
	else
	{
		// unencrypted - we use the actual offsets
		nTempOffset = nPartOffset + nOffset;
		nTempSize = nSize;
	}
	MarkAsUsed(nTempOffset, nTempSize);

}


void CWIIDisc_add_iso_file(const char *name, const char *path, u32 part, u64 offset, u32 size, u32 i)
{
	iso_files *mem;
	// only accept dol files
	if(!strstr(name,".dol")) { return;}
	mem= memalign(32, sizeof(iso_files));
	memset(mem, 0, sizeof(iso_files));
	strcpy(mem->name,name);
	strcpy(mem->path,path);
	mem->part   = part;
	mem->offset = offset;
	mem->size   = size;
	mem->i      = i;


	if (CWIIDisc_first_file == NULL)
	{
		CWIIDisc_first_file = mem;
//		last_file = mem;
	} else CWIIDisc_last_file->next = mem;
	
	CWIIDisc_last_file = mem;
	
	CWIIDisc_last_file->next = NULL;
	
}

u8 CWIIDisc_image_parse_header (struct part_header *header, u8 *buffer) 
{
        memset (header, 0, sizeof (struct part_header));

        header->console = buffer[0];
  
		header->is_gc = 0;
		header->is_wii = 1;

        header->gamecode[0] = buffer[1];
        header->gamecode[1] = buffer[2];
        header->region = buffer[3];
        header->publisher[0] = buffer[4];
        header->publisher[1] = buffer[5];

        header->has_magic = be32 (&buffer[0x18]) == 0x5d1c9ea3;
        strncpy (header->name, (char *) (&buffer[0x20]), 0x60);

        header->dol_offset = be32 (&buffer[0x420]);

        header->fst_offset = be32 (&buffer[0x424]);
        header->fst_size = be32 (&buffer[0x428]);

        if (header->is_wii) {
                header->dol_offset *= 4;
                header->fst_offset *= 4;
                header->fst_size *= 4;
        }

        return header->is_gc || header->is_wii;
}

struct image_file * CWIIDisc_image_init (wbfs_disc_t *d)
{
        //int fp;
        struct image_file *image;
        struct part_header *header;

        static u8 buffer[0x440] ATTRIBUTE_ALIGN(32);

		strcpy(m_csFilename,"");

//        fp = _open (filename, _O_BINARY|_O_RDWR);
//        if (fp <= 0) {
///                AfxMessageBox(filename);
//                return NULL;
//        }

        image = (struct image_file *) malloc (sizeof (struct image_file));
        if (!image) {
                // LOG_ERR ("out of memory");
//                io_close(image);
                return NULL;
        }
        memset (image, 0, sizeof (struct image_file));

		if (!CWIIDisc_io_open(image,d)) return NULL;
		

		// get the filesize and set the range accordingly for future
		// operations
		
		nImageSize = CWIIDisc_io_size(image); 
		

//        image->fp = fp;

        if (!CWIIDisc_io_read (buffer, 0x440, image, 0)) {
//                AfxMessageBox("reading header");

                CWIIDisc_io_close(image);
                free (image);
                return NULL;
        }
       
        header = (struct part_header *) (memalign(32, sizeof (struct part_header)));

        if (!header) {

                CWIIDisc_io_close(image);
                free (image);
                // LOG_ERR ("out of memory");
                return NULL;
        }

        CWIIDisc_image_parse_header (header, buffer);

        if (!header->is_gc && !header->is_wii) {
			
                // LOG_ERR ("unknown type for file: %s", filename);
                CWIIDisc_io_close(image);
                free (header);
                free (image);
                return NULL;
        }

        if (!header->has_magic)
		{
               // AddToLog("image has an invalid magic");

		}

        image->is_wii = header->is_wii;

        if (header->is_wii) {

			if (0==CWIIDisc_CheckAndLoadKey(1, image))
			{
				free (header);
				free (image);
				return NULL;
			}
        }

		// Runtime crash fixed :)
		// Identified by Juster over on GBATemp.net
		// the free was occuring before in the wrong location
		// As a free was being carried out and the next line was checking
		// a value it was pointing to
        free (header);
        return image;
 }

void CWIIDisc_tmd_load (struct image_file *image, u32 part)
{
        struct tmd *tmd;
        u32 tmd_offset, tmd_size;
        enum tmd_sig sig = SIG_UNKNOWN;

        u64 off, cert_size, cert_off;
        static u8 buffer[64] ATTRIBUTE_ALIGN(32);
        u16 i, s;

        off = image->parts[part].offset;
        CWIIDisc_io_read (buffer, 16, image, off + 0x2a4);

        tmd_size = be32 (buffer);
        tmd_offset = be32 (&buffer[4]) * 4;
        cert_size = be32 (&buffer[8]);
        cert_off = be32 (&buffer[12]) * 4;

        // TODO: ninty way?
        /*
        if (cert_size)
                image->parts[part].tmd_size =
                        cert_off - image->parts[part].tmd_offset + cert_size;
        */

        off += tmd_offset;

        CWIIDisc_io_read (buffer, 4, image, off);
        off += 4;

        switch (be32 (buffer)) {
        case 0x00010001:
                sig = SIG_RSA_2048;
                s = 0x100;
                break;

        case 0x00010000:
                sig = SIG_RSA_4096;
                s = 0x200;
                break;
        }

        if (sig == SIG_UNKNOWN)
                return;

        tmd = (struct tmd *) memalign (32, sizeof (struct tmd)+1024);
        memset (tmd, 0, sizeof (struct tmd));

        tmd->sig_type = sig;

        image->parts[part].tmd = tmd;
        image->parts[part].tmd_offset = tmd_offset;
        image->parts[part].tmd_size = tmd_size;

		image->parts[part].cert_offset = cert_off;
		image->parts[part].cert_size = cert_size;

        tmd->sig = (unsigned char *) memalign (32, s+1024);
        CWIIDisc_io_read (tmd->sig, s, image, off);
        off += s;
        
        off = ROUNDUP64B(off);

        CWIIDisc_io_read ((unsigned char *)&tmd->issuer[0], 0x40, image, off);
        off += 0x40;

        CWIIDisc_io_read (buffer, 26, image, off);
        off += 26;

        tmd->version = buffer[0];
        tmd->ca_crl_version = buffer[1];
        tmd->signer_crl_version = buffer[2];

        tmd->sys_version = be64 (&buffer[4]);
        tmd->title_id = be64 (&buffer[12]);
        tmd->title_type = be32 (&buffer[20]);
        tmd->group_id = be16 (&buffer[24]);

        off += 62;

        CWIIDisc_io_read (buffer, 10, image, off);
        off += 10;

        tmd->access_rights = be32 (buffer);
        tmd->title_version = be16 (&buffer[4]);
        tmd->num_contents = be16 (&buffer[6]);
        tmd->boot_index = be16 (&buffer[8]);

        off += 2;

        if (tmd->num_contents < 1)
                return;

        tmd->contents =
                (struct tmd_content *)
                malloc (sizeof (struct tmd_content) * tmd->num_contents);

        for (i = 0; i < tmd->num_contents; ++i) {
                CWIIDisc_io_read (buffer, 0x30, image, off);
                off += 0x30;

                tmd->contents[i].cid = be32 (buffer);
                tmd->contents[i].index = be16 (&buffer[4]);
                tmd->contents[i].type = be16 (&buffer[6]);
                tmd->contents[i].size = be64 (&buffer[8]);
                memcpy (tmd->contents[i].hash, &buffer[16], 20);

        }

        return;
}



void CWIIDisc_tmd_free (struct tmd *tmd)
{
        if (tmd == NULL)
                return;

        if (tmd->sig)
                free (tmd->sig);

        if (tmd->contents)
                free (tmd->contents);

        free (tmd);
}

u8 CWIIDisc_get_partitions (struct image_file *image)
{
        static u8 buffer[16] ATTRIBUTE_ALIGN(32);
        u64 part_tbl_offset;
        u64 chan_tbl_offset;
        u32 i;

        u8 title_key[16];
        u8 iv[16];
        u8 partition_key[16];

        u32 nchans;


		// clear out the old memory allocated
		if (NULL!=image->parts)
		{
			free (image->parts);
			image->parts = NULL;
		}
        CWIIDisc_io_read (buffer, 16, image, 0x40000);
        image->nparts = 1 + be32 (buffer);

		
        nchans = be32 (&buffer[8]);

		// number of partitions is out by one
        AddToLog("number of partitions: %i\n", image->nparts);
        AddToLog("number of channels: %i\n", nchans);
				
		// store the values for later bit twiddling
		image->ChannelCount = nchans;
		image->PartitionCount = image->nparts -1;

        image->nparts += nchans;

 
        part_tbl_offset = (u64) (be32 (&buffer[4])) * ((u64)(4));
        chan_tbl_offset = (u64 )(be32 (&buffer[12])) * ((u64) (4));
        AddToLog("partition table offset: %x\n", part_tbl_offset);
        AddToLog("channel table offset: %x\n", chan_tbl_offset);



		image->part_tbl_offset = part_tbl_offset;
		image->chan_tbl_offset = chan_tbl_offset;

        image->parts = (struct partition *)
                        malloc (image->nparts * sizeof (struct partition));
        memset (image->parts, 0, image->nparts * sizeof (struct partition));

        for (i = 1; i < image->nparts; ++i) {
				AddToLog("--------------------------------------------------------------------------\n");
                AddToLog("partition: %i\n", i);

                if (i < image->nparts - nchans)
				{
                        CWIIDisc_io_read (buffer, 8, image,
                                 part_tbl_offset + (i - 1) * 8);

                        switch (be32 (&buffer[4]))
						{
                        case 0:
                                image->parts[i].type = PART_DATA;
                                break;

                        case 1:
                                image->parts[i].type = PART_UPDATE;
                                break;

                        case 2:
                                image->parts[i].type = PART_INSTALLER;
                                break;

                        default:
                                break;
                        }

                } else {
						AddToLog("Virtual console\n");
                        
						// error in WiiFuse as it 'assumes' there are only two
						// partitions before VC games

						// changed to a generic version
						CWIIDisc_io_read (buffer, 8, image,
                                 chan_tbl_offset + (i - image->PartitionCount - 1) * 8);

                        image->parts[i].type = PART_VC;
                        image->parts[i].chan_id[0] = buffer[4];
                        image->parts[i].chan_id[1] = buffer[5];
                        image->parts[i].chan_id[2] = buffer[6];
                        image->parts[i].chan_id[3] = buffer[7];
                }

                image->parts[i].offset = (u64)(be32 (buffer)) * ((u64)(4));

                AddToLog("partition offset: %x\n", image->parts[i].offset);

				// mark the block as used
				MarkAsUsed(image->parts[i].offset, 0x8000);


                CWIIDisc_io_read (buffer, 8, image, image->parts[i].offset + 0x2b8);
                image->parts[i].data_offset = (u64)(be32 (buffer)) << 2ULL;
                image->parts[i].data_size = (u64)(be32 (&buffer[4])) << 2ULL;

				// now get the H3 offset
				if(CWIIDisc_io_read (buffer,4, image, image->parts[i].offset + 0x2b4)<=0)
					{
					
					}
				image->parts[i].h3_offset = (u64)(be32 (buffer)) << 2 ;

                AddToLog("partition data offset: %lx\n", image->parts[i].data_offset);
                AddToLog("partition data size: %i\n", (int) image->parts[i].data_size);
                AddToLog("H3 offset: %lx\n", image->parts[i].h3_offset);

               
                CWIIDisc_tmd_load (image, i);
                if (image->parts[i].tmd == NULL) {
                        AddToLog("partition has no valid tmd\n");

                        continue;
                }
			


               sprintf (image->parts[i].title_id_str, "%016llx",
                         image->parts[i].tmd->title_id);

			   image->parts[i].is_encrypted = image->encrypted;
                image->parts[i].cached_block = 0xffffffff;

                memset (title_key, 0, 16);
                memset (iv, 0, 16);

                CWIIDisc_io_read (title_key, 16, image, image->parts[i].offset + 0x1bf);
                CWIIDisc_io_read (iv, 8, image, image->parts[i].offset + 0x1dc);

                
              
				AES_cbc_encrypt (title_key, partition_key, 16,
                                 &image->key[0], iv, AES_DECRYPT);
               

				memcpy(image->parts[i].title_key, partition_key, 16);
					
				
				//AES_set_decrypt_key (partition_key, 128, &image->parts[i].key);
				

                sprintf (image->parts[i].key_c, "0x"
                         "%02x%02x%02x%02x%02x%02x%02x%02x"
                         "%02x%02x%02x%02x%02x%02x%02x%02x",
                          partition_key[0], partition_key[1],
                          partition_key[2], partition_key[3],
                          partition_key[4], partition_key[5],
                          partition_key[6], partition_key[7],
                          partition_key[8], partition_key[9],
                          partition_key[10], partition_key[11],
                          partition_key[12], partition_key[13],
                          partition_key[14], partition_key[15]);


        }

        return image->nparts == 0;
}

u32 CWIIDisc_parse_fst (u8 *fst, const char *names, u32 i, struct image_file *image, u32 part, char **partition)
 {
        u64 offset;
        u32 size;
        const char *name;
        u32 j;
		char old_m_csText[255];
		//char buffer[255];

        name = names + (be32 (fst + 12 * i) & 0x00ffffff);
        size = be32 (fst + 12 * i + 8);

		
        if (i == 0)
		{
			// directory so need to go through the directory entries
                for (j = 1; j < size; )
				{
                        j = CWIIDisc_parse_fst (fst, names, j, image, part, partition);
				}
                return size;
        }

        if (fst[12 * i])
		{
				strcpy(old_m_csText,m_csText);
				strcat(m_csText,name);///				m_csText += name;
				strcat(m_csText,"\\");///				m_csText += "\\";
///                AddToLog(m_csText);
///				pParent = m_pParent->AddItemToTree(name, pParent);

                for (j = i + 1; j < size; ) j = CWIIDisc_parse_fst (fst, names, j, image, part, partition);

				// now remove the directory name we just added
				strcpy(m_csText,old_m_csText);///				m_csText = m_csText.Left(m_csText.GetLength()-strlen(name) - 1);
				return size;
        }
		else
		{
                offset = be32(fst + 12 * i + 4);
                if (image->parts[part].header.is_wii) offset *= 4;

				CWIIDisc_add_iso_file(name,m_csText,part,offset,size,i);

///				printf("PAR%i\\%s%s\n",part,m_csText,name);

///				CString	csTemp;
///				csTemp.Format("%s [0x%lX] [0x%I64X] [0x%lX] [%d]",  name,
///															  part,
///															  offset,
///															  size,
///															  i);
///
///				m_pParent->AddItemToTree(csTemp, pParent);


///				csTemp.Format("%s%s", m_csText, name);
///                AddToLog(csTemp, image->parts[part].offset + offset, size);
                

				MarkAsUsedDC(image->parts[part].offset+image->parts[part].data_offset, offset, size, image->parts[part].is_encrypted);

                image->nfiles++;
                image->nbytes += size;

                return i + 1;
        }
}

int CWIIDisc_image_parse (struct image_file *image)
{
        static u8 buffer[0x440] ATTRIBUTE_ALIGN(32);

        u8 *fst;
        u32 i;
        u8 j, valid, nvp;

        u32 nfiles;
		
//		HTREEITEM hPartitionBin;

		char csText[255];


        if (image->is_wii)
		{
                AddToLog("wii image detected\n");
				strcpy(hDisc,"WII DISC");

                CWIIDisc_get_partitions (image);
        } else return 0;

      //  _fstat (image->fp, &image->st);


        nvp = 0;
        for (i = 0; i < image->nparts; ++i)
        {

		if(image->parts[i].type!=PART_DATA) continue;

		
				AddToLog("------------------------------------------------------------------------------\n");

                AddToLog("partition: %i\n", i);

				switch (image->parts[i].type)
				{
				case PART_DATA:
					sprintf(csText,"Partition:%d - DATA",i);
                                break;

				case PART_UPDATE:
					sprintf(csText,"Partition:%d - UPDATE",i);
                                break;

				case PART_INSTALLER:
					sprintf(csText,"Partition:%d - INSTALLER",i);
                                break;
				case PART_VC:
					sprintf(csText,"Partition:%d - VC GAME [%s]",i,image->parts[i].chan_id );
                                break;
				default:
					if (0!=i)
					{
						sprintf(csText,"Partition:%d - UNKNOWN",i);
					}
					else
					{
						sprintf(csText,"Partition:0 - PART INFO");
					}
					break; 
				}

				strcpy(hPartition[i],csText);
///				hPartition[i] = m_pParent->AddItemToTree(csText, hDisc);

				

                if (!CWIIDisc_io_read_part (buffer, 0x440, image, i, 0)) {
///                        AfxMessageBox("partition header");
                        return 1;
                }

				

                valid = 1;
                for (j = 0; j < 6; ++j) {
                        if (!isprint (buffer[j])) {
                                valid = 0;
                                break;
                        }
                }

                if (!valid) {
                        AddToLog("invalid header for partition: %i\n", i);
                        continue;
                }
                nvp++;


                CWIIDisc_image_parse_header (&image->parts[i].header, buffer);




				if (PART_UNKNOWN!=image->parts[i].type)
				{
				//	AddToLog("\\partition.bin %lx %lx", image->parts[i].offset, image->parts[i].data_offset);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-4]",
						"partition.bin",
						i,
						image->parts[i].offset,
						image->parts[i].data_offset);*/
					MarkAsUsed(image->parts[i].offset, image->parts[i].data_offset);
///					hPartitionBin = m_pParent->AddItemToTree(csText, hPartition[i]);

					// add on the boot.bin
				//	AddToLog("\\boot.bin %lx %lx", image->parts[i].offset + image->parts[i].data_offset, (u64)0x440);
					MarkAsUsedDC(image->parts[i].offset + image->parts[i].data_offset, 0, (u64)0x440, image->parts[i].is_encrypted);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [0]",
						"boot.bin",
						i,
						(u64) 0x0,
						(u64)0x440);*/
					
///					m_pParent->AddItemToTree(csText, hPartition[i]);
					
					
					// add on the bi2.bin
				//	AddToLog("\\bi2.bin", image->parts[i].offset + image->parts[i].data_offset + 0x440, (u64)0x2000);
					MarkAsUsedDC(image->parts[i].offset + image->parts[i].data_offset, 0x440, (u64)0x2000, image->parts[i].is_encrypted);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [0]",
						"bi2.bin",
						i,
						(u64) 0x440,
						(u64)0x2000);*/
					
///					m_pParent->AddItemToTree(csText, hPartition[i]);
					
				}
                CWIIDisc_io_read_part (buffer, 8, image, i, 0x2440 + 0x14);
                image->parts[i].appldr_size =
                        be32 (buffer) + be32 (&buffer[4]);
                if (image->parts[i].appldr_size > 0)
                        image->parts[i].appldr_size += 32;


				if (image->parts[i].appldr_size > 0)
				{
					//AddToLog("\\apploader.img", image->parts[i].offset+ image->parts[i].data_offset +0x2440, image->parts[i].appldr_size);
					MarkAsUsedDC(	image->parts[i].offset + image->parts[i].data_offset,
									0x2440,
									image->parts[i].appldr_size,
									image->parts[i].is_encrypted);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-3]",
										"apploader.img",
										i,
										(u64) 0x2440,
										(u64) image->parts[i].appldr_size);*/

///					m_pParent->AddItemToTree(csText, hPartition[i]);
				}
				else
				{
					AddToLog("apploader.img not present\n");
				}

                if (image->parts[i].header.dol_offset > 0)
				{
                        CWIIDisc_io_read_part (buffer, 0x100, image, i, image->parts[i].header.dol_offset);
                        image->parts[i].header.dol_size = get_dol_size (buffer);

						// now check for error condition with bad main.dol
						if (image->parts[i].header.dol_size >=image->parts[i].data_size)
						{
							// almost certainly an error as it's bigger than the partition
							image->parts[i].header.dol_size = 0;
						}
						MarkAsUsedDC(	image->parts[i].offset+ image->parts[i].data_offset,
										image->parts[i].header.dol_offset,
										image->parts[i].header.dol_size,
										image->parts[i].is_encrypted);
						
						//AddToLog("\\main.dol ", image->parts[i].offset + image->parts[i].data_offset + image->parts[i].header.dol_offset, image->parts[i].header.dol_size);

/*						csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-2]",
										"main.dol",
										i,
										image->parts[i].header.dol_offset,
										image->parts[i].header.dol_size);*/

///						m_pParent->AddItemToTree(csText, hPartition[i]);
				 AddToLog("main.dol\n");
                } else{

                        AddToLog("partition has no main.dol\n");
				}

				if (image->parts[i].is_encrypted)
				{
					// Now add the TMD.BIN and cert.bin files - as these are part of partition.bin
					// we don't need to mark them as used - we do put them undr partition.bin in the
					// tree though

					//AddToLog("\\tmd.bin", image->parts[i].offset + image->parts[i].tmd_offset, image->parts[i].tmd_size);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-5]",
						"tmd.bin",
						i,
						image->parts[i].tmd_offset,
						image->parts[i].tmd_size);*/
					
///					m_pParent->AddItemToTree(csText, hPartitionBin);

					//AddToLog("\\cert.bin", image->parts[i].offset + image->parts[i].cert_offset, image->parts[i].cert_size);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-6]",
						"cert.bin",
						i,
						(u64)image->parts[i].cert_offset,
						(u64)image->parts[i].cert_size);*/
					
///					m_pParent->AddItemToTree(csText, hPartitionBin);


				
					// add on the H3
					//AddToLog("\\h3.bin", image->parts[i].offset + image->parts[i].h3_offset, (u64)0x18000);
					MarkAsUsedDC(	image->parts[i].offset,
									image->parts[i].h3_offset,
									(u64)0x18000,
									0);

/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-7]",
										"h3.bin",
										i,
										(u64) image->parts[i].h3_offset,
										(u64)0x18000);*/

///					m_pParent->AddItemToTree(csText, hPartitionBin);

				}
				
                
				if (image->parts[i].header.fst_offset > 0 &&
                    image->parts[i].header.fst_size > 0) {

                        //AddToLog("\\fst.bin ", image->parts[i].offset+image->parts[i].data_offset+image->parts[i].header.fst_offset,image->parts[i].header.fst_size);

						MarkAsUsedDC( image->parts[i].offset+ image->parts[i].data_offset,
									  image->parts[i].header.fst_offset,
									  image->parts[i].header.fst_size,
									  image->parts[i].is_encrypted);
/*						csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-1]",
										"fst.bin",
										i,
										image->parts[i].header.fst_offset,
										image->parts[i].header.fst_size);*/
						
///						m_pParent->AddItemToTree(csText, hPartition[i]);

                        fst = (u8 *) (malloc ((u32)(image->parts[i].header.fst_size)));
                        if (CWIIDisc_io_read_part (fst, (u32)(image->parts[i].header.fst_size),
                                          image, i,
                                          image->parts[i].header.fst_offset) !=
                            image->parts[i].header.fst_size)
						{
///                                AfxMessageBox("fst.bin");
								free (fst);
                                return 1;
                        }

                        nfiles = be32 (fst + 8);

                        if (12 * nfiles > image->parts[i].header.fst_size) {
                                AddToLog("invalid fst for partition %i\n", i);
                        } else {
///								m_csText = "\\";

                               CWIIDisc_parse_fst (fst, (char *) (fst + 12 * nfiles), 0, image, i, (char**)hPartition[i]);
                        }

                        free (fst);
                } else
				{
                        AddToLog("partition has no fst\n");
				}


        }

        if (!nvp) {
                AddToLog("no valid partition were found, exiting\n");
                return 1;
        }

        return 0;
}

#if 0
bool CWIIDisc::SaveDecryptedFile(const char *csDestinationFilename,  struct image_file *image, u32 part, u64 nFileOffset, u64 nFileSize, BOOL bOverrideEncrypt)
{
	FILE * fOut;

		u32 block = (u32)(nFileOffset / (u64)(0x7c00));
        u32 cache_offset = (u32)(nFileOffset % (u64)(0x7c00));
        u64 cache_size;


        unsigned char cBuffer[0x8000];

		fOut = fopen(csDestinationFilename, "wb");

        if ((!image->parts[part].is_encrypted)||
			(1==bOverrideEncrypt))
		{
			if (-1==_lseeki64 (image->fp, nFileOffset, SEEK_SET)) {
				DWORD x = GetLastError();
///				AfxMessageBox("io_seek");
				return -1;
			}

			while(nFileSize)
			{
				cache_size = nFileSize;
				
				if (cache_size  > 0x8000)
					cache_size = 0x8000;
				
				
				_read(image->fp, &cBuffer[0], (u32)(cache_size));
				
				fwrite(cBuffer, 1, (u32)(cache_size), fOut);
				
				nFileSize -= cache_size;

			}
		}
		else
		{
			while (nFileSize) {
				    if (decrypt_block (image, part, block))
					{
						fclose(fOut);
					        return 0;
					}

					cache_size = nFileSize;
					if (cache_size + cache_offset > 0x7c00)
						    cache_size = 0x7c00 - cache_offset;

					if (cache_size!=fwrite(image->parts[part].cache + cache_offset, 1,
						    (u32)(cache_size), fOut))
					{
						AddToLog("Error writing file");
						fclose(fOut);
						return 0;
					}
 
					nFileSize -= cache_size;
					cache_offset = 0;

					block++;
			}
		}
		fclose (fOut);

	return 1;
}
#endif
extern u8 common_key[16];

int CWIIDisc_CheckAndLoadKey(int bLoadCrypto, struct image_file *image)
{
	
	
	memcpy(&image->key[0], common_key,16);

	//	AES_set_decrypt_key (LoadedKey, 128, );
	
	return 1;
}
#if 0 // control
void CWIIDisc_Reset()
{
	int i;
	//set them all to clear first
	memset(pFreeTable, 0, ((4699979776L / 32768L)* 2L));
	// then set the header size to used
	MarkAsUsed(0,0x50000);
	strcpy(hDisc,"");
	for(i=0;i<20;i++)
	{
		strcpy(hPartition[i],"");
	}

	// then clear the decrypt key
	u8 key[16];

	memset(key,0,16);

	AES_KEY nKey;

	memset(&nKey, 0, sizeof(AES_KEY));
	strcpy(m_csText,"");
}
#endif

int CWIIDisc_getdols(wbfs_disc_t *d)
{
struct image_file *image;

one=1;

if(is_fat)
	{
	fp_disc=(void *) d;
	d=NULL;
	}
//printf("Searching...\n");
CWIIDisc_create();


	if (1)
	{
	
		//CWIIDisc_Reset();
		image = CWIIDisc_image_init(d);
		//image = CWIIDisc_image_init("O:\\wii\\MySims.Racing.PAL.WII-LoCAL\\MSR.iso");
		if (image == NULL)
		{
			CWIIDisc_destroy();
			//printf("Unable to load \n");
			return -1;
		}
	
		CWIIDisc_image_parse(image);
	}

	CWIIDisc_destroy();
	
	#if 0
	printf("Ok\n");
	
	
	{
	iso_files *file =CWIIDisc_first_file;
	while (1)
	{
		if (file == NULL) break;
			printf("%i) %s ->%s %llx %x\n",file->part, file->path, file->name, file->offset, file->size);
		file = file->next;
	}
	}
#endif

return 0;
}


