#ifndef WIIDISC_H
#define WIIDISC_H

#include <windows.h>
#include <wchar.h>
#include "include/openssl/aes.h"
#include "wbfs.h"
#include "global.h"


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

#define KEYFILE "key.bin"

#define _CRT_SECURE_NO_DEPRECATE 1


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
        AES_KEY key;

		u8 title_key[16];

        u64 data_offset;
        u64 data_size;

        u64 cert_offset;
        u64 cert_size;

        u8 dec_buffer[0x8000];

        u32 cached_block;
        u8 cache[0x7c00];
};

struct image_file {
 
		int fp;
		wbfs_disc_t *d;
		bool encrypted;
//        void * mutex;

        u8 is_wii;

        u32 nparts;
        struct partition *parts;

 //       struct tree *tree;

        struct _stat st;

        u64 nfiles;
        u64 nbytes;

		u8	PartitionCount;
		u8	ChannelCount;

		u64 part_tbl_offset;
		u64	chan_tbl_offset;

        AES_KEY key;
};

struct iso_files {
	char name[255];
	char path[255];
	u32 part;
	u64 offset;
	u32 size;
	u32 i;
	struct iso_files *next;
};


class CWIIDisc {
public:
	unsigned char * pFreeTable;
	unsigned char * pBlankSector;
	unsigned char * pBlankSector0;

	_int64 nImageSize;

	iso_files *first_file;
	iso_files *last_file;

	class CWIIScrubberDlg * m_pParent;

	char	hPartition[20][255];
	char	hDisc[255];

	char m_csText[255];

	CWIIDisc();
	virtual ~CWIIDisc();
	void Reset();
	void add_iso_file(const char *name, const char *path, u32 part, u64 offset, u32 size, u32 i);
	void AddToLog(char *text);
	void AddToLog(char *text, u64 nValue);
	void AddToLog(char *text, u64 nValue1, u64 nValue2);
	void AddToLog(char *text, u64 nValue1, u64 nValue2, u64 nValue3);
	u8 image_parse_header (struct part_header *header, u8 *buffer);
	void MarkAsUsed(u64 nOffset, u64 nSize);
	void MarkAsUsedDC(u64 nPartOffset, u64 nOffset, u64 nSize, BOOL bIsEncrypted);
	int io_read (unsigned char  *ptr, size_t size, struct image_file *image, u64 offset);
	BOOL CheckAndLoadKey(BOOL bLoadCrypto, struct image_file *image);
	struct image_file *image_init (const char *filename);
	u32 parse_fst (u8 *fst, const char *names, u32 i, struct tree *tree, struct image_file *image, u32 part, char **partition);
	u32 parse_fst_and_save(u8 *fst, const char *names, u32 i, struct image_file *image, u32 part);
	BOOL SaveDecryptedFile(const char *csDestinationFilename,  struct image_file *image, u32 part, u64 nFileOffset, u64 nFileSize, BOOL bOverrideEncrypt = FALSE);
	int decrypt_block (struct image_file *image, u32 part, u32 block);
	size_t io_read_part (unsigned char *ptr, size_t size, struct image_file *image, u32 part, u64 offset);
	void tmd_load (struct image_file *image, u32 part);
	void tmd_free (struct tmd *tmd);
	u8 get_partitions (struct image_file *image);
	int image_parse (struct image_file *image);
	void io_close(struct image_file *image);
	int io_open(struct image_file *image, const char *filename);
	int io_size(struct image_file *image);
private:
	// save the tables instead of reading and writing them all the time
	u8 h3[SIZE_H3];
	u8 h4[SIZE_H4];

	char m_csFilename[255];
};

#endif