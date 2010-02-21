#include "stdafx.h"
#include "wiidisc.h"
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <sys/stat.h>
#include "wbfs.h"

u8 verbose_level = 0;

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

CWIIDisc::CWIIDisc()
{
	first_file = NULL;
	// create and blank the wii blank table
	pFreeTable = (unsigned char *) malloc(((u64)(4699979776) / (u64)(0x8000))*2);
	//set them all to clear first
	memset(pFreeTable, 0, ((u64)(4699979776) / (u64)(0x8000))*2);
	// then set the header size to used
	MarkAsUsed(0, 0x40000);

	pBlankSector = (unsigned char *) malloc(0x8000);
	memset(pBlankSector, 0xFF, 0x8000);

	pBlankSector0 = (unsigned char *) malloc(0x8000);
	memset (pBlankSector0, 0, 0x8000);


	for (int i = 0; i < 20; i++)
	{
		strcpy(hPartition[i],"");
	}
	strcpy(hDisc,"");
	
	// then clear the decrypt key
	u8 key[16];

	memset(key,0,16);

	AES_KEY nKey;

	memset(&nKey, 0, sizeof(AES_KEY));
	AES_set_decrypt_key (key, 128, &nKey);
}

CWIIDisc::~CWIIDisc()
{
	// free up the memory
	free(pFreeTable);
	free(pBlankSector);
	free(pBlankSector0);
}

void CWIIDisc::AddToLog(char *text)
{
	char buffer[255];
	sprintf(buffer,"%s\n",text);
//	printf(buffer);
	//m_pParent->AddToLog(csText);
}
void CWIIDisc::AddToLog(char *text, u64 nValue)
{
	char buffer[255];
	sprintf(buffer,"%s [0x%I64X]\n",text,nValue);
//	printf(buffer);
}

void CWIIDisc::AddToLog(char *text, u64 nValue1, u64 nValue2)
{
	char buffer[255];
	sprintf(buffer,"%s [0x%I64X] [0x%I64X]\n",text,nValue1,nValue2);
//	printf(buffer);
	//CString csText1;
	//csText1.Format("%s [0x%I64X], [0x%I64X]", csText, nValue1, nValue2);
	//m_pParent->AddToLog(csText1);
}

void CWIIDisc::AddToLog(char *text, u64 nValue1, u64 nValue2, u64 nValue3)
{
	char buffer[255];
	sprintf(buffer,"%s [0x%I64X] [0x%I64X] [%I64d K]\n",text,nValue1,nValue2,nValue3);
//	printf(buffer);
	//CString csText1;
	//csText1.Format("%s [0x%I64X], [0x%I64X] [%I64d K]", csText, nValue1, nValue2, nValue3);
	//m_pParent->AddToLog(csText1);
}

void CWIIDisc::add_iso_file(const char *name, const char *path, u32 part, u64 offset, u32 size, u32 i)
{
	iso_files *mem = new iso_files;
	strcpy(mem->name,name);
	strcpy(mem->path,path);
	mem->part   = part;
	mem->offset = offset;
	mem->size   = size;
	mem->i      = i;

	if (first_file == NULL)
	{
		first_file = mem;
//		last_file = mem;
	} else last_file->next = mem;
	last_file = mem;
	last_file->next = NULL;
}

void CWIIDisc::Reset()
{
	//set them all to clear first
	memset(pFreeTable, 0, ((4699979776L / 32768L)* 2L));
	// then set the header size to used
	MarkAsUsed(0,0x50000);
	strcpy(hDisc,"");
	for(int i=0;i<20;i++)
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

u8 CWIIDisc::image_parse_header (struct part_header *header, u8 *buffer) 
{
        memset (header, 0, sizeof (struct part_header));

        header->console = buffer[0];
  
		header->is_gc = FALSE;
		header->is_wii = TRUE;

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

void CWIIDisc::MarkAsUsed(u64 nOffset, u64 nSize)
{
		u64 nStartValue = nOffset;
		u64 nEndValue = nOffset + nSize;
		while((nStartValue < nEndValue)&&
			  (nStartValue < ((u64)(4699979776) * (u64)(2))))
		{

			pFreeTable[nStartValue / (u64)(0x8000)] = 1;
			nStartValue = nStartValue + ((u64)(0x8000));
		}

}
void CWIIDisc::MarkAsUsedDC(u64 nPartOffset, u64 nOffset, u64 nSize, BOOL bIsEncrypted)
{
	u64 nTempOffset;
	u64 nTempSize;
	
	if (TRUE==bIsEncrypted)
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

int CWIIDisc::io_read (unsigned char  *ptr, size_t size, struct image_file *image, u64 offset)
{
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
	} else
	if (image->d)
	{
		return size * wbfs_read_disc(image->d, offset, size, (char*)ptr);
	} else return 0;
}

int CWIIDisc::io_open(struct image_file *image, const char *filename)
{
	if (filename[1] != '-')
	{
        int fp = _open (filename, _O_BINARY|_O_RDWR);
		image->fp = fp;
		image->d = NULL;

		image->encrypted = 1;

		return fp;
	} else
	{
		wbfs_t *p;
		p = wbfs_try_open(NULL, (char*)filename, 0);
		if (!p) return 0;

		wbfs_disc_t *d;
		d = wbfs_open_disc(p, (u8*)filename+2);
		if (!d) return 0;
		image->d = d;
		image->fp = NULL;

		image->encrypted = 1;

		return (int)p;
	}
}

void CWIIDisc::io_close(struct image_file *image)
{
	if (image->fp)
	{
		_close(image->fp);
	} else
	if (image->d)
	{
		wbfs_close_disc(image->d);
		wbfs_close(image->d->p);
	}
}

int CWIIDisc::io_size(struct image_file *image)
{
	if (image->fp)
	{
		return _lseeki64(image->fp, 0L, SEEK_END);
	} else
	if (image->d)
	{
		int i;
		u32 size;
		u8 *b = (u8*)wbfs_ioalloc(0x100);
		int count = wbfs_count_discs(image->d->p);
		for (i = 0; i < count; i++)
		{
			if (!wbfs_get_disc_info(image->d->p, i, b, 0x100, &size))
			{
				if (strcmp((const char*)b,(const char*)image->d->header->disc_header_copy) == 0) break;
//				printf( "%c%c%c%c%c%c %40s %.2fG\n", b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20, size * 4ULL / (GB));
			}
		}
		return size;
	}
	return 0;
}

BOOL CWIIDisc::CheckAndLoadKey(BOOL bLoadCrypto, struct image_file *image)
{
	FILE * fp_key;
	static u8 LoadedKey[16];

	if (FALSE==bLoadCrypto)
	{
		fp_key = fopen (KEYFILE, "rb");
	
		if (fp_key == NULL) {
///			AfxMessageBox("Unable to open key.bin");
			return FALSE;
		}
	
		if (16 != fread (LoadedKey, 1, 16, fp_key)) {
			fclose (fp_key);
///			AfxMessageBox("key.bin not 16 bytes in size");
			return FALSE;
		}
	
		fclose (fp_key);

		// now check to see if it's the right key
		// as we don't want to embed the key value in here then lets cheat a little ;)
		// by checking the Xor'd difference values
		if	((0x0F!=(LoadedKey[0]^LoadedKey[1]))||
			(0xCE!=(LoadedKey[1]^LoadedKey[2]))||
			(0x08!=(LoadedKey[2]^LoadedKey[3]))||
			(0x7C!=(LoadedKey[3]^LoadedKey[4]))||
			(0xDB!=(LoadedKey[4]^LoadedKey[5]))||
			(0x16!=(LoadedKey[5]^LoadedKey[6]))||
			(0x77!=(LoadedKey[6]^LoadedKey[7]))||
			(0xAC!=(LoadedKey[7]^LoadedKey[8]))||
			(0x91!=(LoadedKey[8]^LoadedKey[9]))||
			(0x1C!=(LoadedKey[9]^LoadedKey[10]))||
			(0x80!=(LoadedKey[10]^LoadedKey[11]))||
			(0x36!=(LoadedKey[11]^LoadedKey[12]))||
			(0xF2!=(LoadedKey[12]^LoadedKey[13]))||
			(0x2B!=(LoadedKey[13]^LoadedKey[14]))||
			(0x5D!=(LoadedKey[14]^LoadedKey[15])))
		{
			return FALSE;
			// handle the Korean key, in case it ever gets found
/*			if (AfxMessageBox("Doesn't seem to be the correct key.bin\nDo you want to use anyways??",
				MB_YESNO|MB_ICONSTOP|MB_DEFBUTTON2)==IDNO)
			{
				return FALSE;
			}*/
		}
	}
	else
	{
		AES_set_decrypt_key (LoadedKey, 128, &image->key);
	}
	return TRUE;
}

struct image_file * CWIIDisc::image_init (const char *filename)
{
        int fp;
        struct image_file *image;
        struct part_header *header;

        u8 buffer[0x440];

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

		if (!io_open(image,filename)) return NULL;

		// get the filesize and set the range accordingly for future
		// operations
		
		nImageSize = io_size(image);

//        image->fp = fp;

        if (!io_read (buffer, 0x440, image, 0)) {
//                AfxMessageBox("reading header");
                io_close(image);
                free (image);
                return NULL;
        }

        header = (struct part_header *) (malloc (sizeof (struct part_header)));

        if (!header) {
                io_close(image);
                free (image);
                // LOG_ERR ("out of memory");
                return NULL;
        }

        image_parse_header (header, buffer);

        if (!header->is_gc && !header->is_wii) {
                // LOG_ERR ("unknown type for file: %s", filename);
                io_close(image);
                free (header);
                free (image);
                return NULL;
        }

        if (!header->has_magic)
		{
                AddToLog("image has an invalid magic");

		}

        image->is_wii = header->is_wii;

        if (header->is_wii) {

			if (FALSE==CheckAndLoadKey(TRUE, image))
			{
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


u32 CWIIDisc::parse_fst (u8 *fst, const char *names, u32 i, struct tree *tree, struct image_file *image, u32 part, char **partition)
 {
        u64 offset;
        u32 size;
        const char *name;
        u32 j;
		char old_m_csText[255];
		char buffer[255];

        name = names + (be32 (fst + 12 * i) & 0x00ffffff);
        size = be32 (fst + 12 * i + 8);

        if (i == 0)
		{
			// directory so need to go through the directory entries
                for (j = 1; j < size; )
				{
                        j = parse_fst (fst, names, j, tree, image, part, partition);
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

                for (j = i + 1; j < size; ) j = parse_fst (fst, names, j, NULL, image, part, partition);

				// now remove the directory name we just added
				strcpy(m_csText,old_m_csText);///				m_csText = m_csText.Left(m_csText.GetLength()-strlen(name) - 1);
				return size;
        }
		else
		{
                offset = be32(fst + 12 * i + 4);
                if (image->parts[part].header.is_wii) offset *= 4;

				add_iso_file(name,m_csText,part,offset,size,i);

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
u32 CWIIDisc::parse_fst_and_save(u8 *fst, const char *names, u32 i, struct image_file *image, u32 part)
 {
//		MSG msg;
        u64 offset;
        u32 size;
        const char *name;
        u32 j;
///		CString	csTemp;

        name = names + (be32 (fst + 12 * i) & 0x00ffffff);
        size = be32 (fst + 12 * i + 8);

///		pProgressBox->SetPosition(i);
/*        if (PeekMessage(&msg,
            NULL,
            0,
            0,
            PM_REMOVE))
        {
            // PeekMessage has found a message--process it 
            if (msg.message != WM_CANCELLED)
            {
                TranslateMessage(&msg); // Translate virt. key codes 
                DispatchMessage(&msg);  // Dispatch msg. to window 
            }
			else
			{
				// show a complete exit
				return 0xFFFFFFFF;
			}
        }*/

        if (i == 0)
		{
			// directory so need to go through the directory entries
                for (j = 1; j < size; )
				{
                        j = parse_fst_and_save(fst, names, j, image, part);
				}
				if (j!=0xFFFFFFFF)
				{
					return size;
				}
				else
				{
					return 0xFFFFFFFF;
				}
        }

        if (fst[12 * i])
		{
			// directory so....
			// create a directory and change to it
			_mkdir(name);
			_chdir(name);
	
			
			for (j = i + 1; j < size; )
			{
				j = parse_fst_and_save(fst, names, j, image, part);
			}
			
			// now remove the directory name we just added
///			m_csText = m_csText.Left(m_csText.GetLength()-strlen(name) - 1);
			_chdir("..");
			if (j!=0xFFFFFFFF)
			{
				return size;
			}
			else
			{
				return 0xFFFFFFFF;
			}
        }
		else
		{
			// it's a file so......
			// create a filename and then save it out
			
			offset = be32(fst + 12 * i + 4);
			if (image->parts[part].header.is_wii)
			{
				offset *= 4;
			}

			// now save it
			if (TRUE==SaveDecryptedFile(name, image, part, offset, size))
			{
				return i + 1;
			}
			else
			{
				// Error writing file
				return 0xFFFFFFFF;
			}
        }
}

BOOL CWIIDisc::SaveDecryptedFile(const char *csDestinationFilename,  struct image_file *image, u32 part, u64 nFileOffset, u64 nFileSize, BOOL bOverrideEncrypt)
{
	FILE * fOut;

		u32 block = (u32)(nFileOffset / (u64)(0x7c00));
        u32 cache_offset = (u32)(nFileOffset % (u64)(0x7c00));
        u64 cache_size;


        unsigned char cBuffer[0x8000];

		fOut = fopen(csDestinationFilename, "wb");

        if ((!image->parts[part].is_encrypted)||
			(TRUE==bOverrideEncrypt))
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
					        return FALSE;
					}

					cache_size = nFileSize;
					if (cache_size + cache_offset > 0x7c00)
						    cache_size = 0x7c00 - cache_offset;

					if (cache_size!=fwrite(image->parts[part].cache + cache_offset, 1,
						    (u32)(cache_size), fOut))
					{
						AddToLog("Error writing file");
						fclose(fOut);
						return FALSE;
					}
 
					nFileSize -= cache_size;
					cache_offset = 0;

					block++;
			}
		}
		fclose (fOut);

	return TRUE;
}

int CWIIDisc::decrypt_block (struct image_file *image, u32 part, u32 block)
{
        if (block == image->parts[part].cached_block)
                return 0;


        if (io_read (image->parts[part].dec_buffer, 0x8000, image,
                     image->parts[part].offset +
                     image->parts[part].data_offset + (u64)(0x8000) * (u64)(block))
            != 0x8000) {
///                AfxMessageBox("decrypt read");
                return -1;
        }

        AES_cbc_encrypt (&image->parts[part].dec_buffer[0x400],
                         image->parts[part].cache, 0x7c00,
                         &image->parts[part].key,
                         &image->parts[part].dec_buffer[0x3d0], AES_DECRYPT);

        image->parts[part].cached_block = block;

        return 0;
}

void CWIIDisc::tmd_load (struct image_file *image, u32 part)
{
        struct tmd *tmd;
        u32 tmd_offset, tmd_size;
        enum tmd_sig sig = SIG_UNKNOWN;

        u64 off, cert_size, cert_off;
        u8 buffer[64];
        u16 i, s;

        off = image->parts[part].offset;
        io_read (buffer, 16, image, off + 0x2a4);

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

        io_read (buffer, 4, image, off);
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

        tmd = (struct tmd *) malloc (sizeof (struct tmd));
        memset (tmd, 0, sizeof (struct tmd));

        tmd->sig_type = sig;

        image->parts[part].tmd = tmd;
        image->parts[part].tmd_offset = tmd_offset;
        image->parts[part].tmd_size = tmd_size;

		image->parts[part].cert_offset = cert_off;
		image->parts[part].cert_size = cert_size;

        tmd->sig = (unsigned char *) malloc (s);
        io_read (tmd->sig, s, image, off);
        off += s;
        
        off = ROUNDUP64B(off);

        io_read ((unsigned char *)&tmd->issuer[0], 0x40, image, off);
        off += 0x40;

        io_read (buffer, 26, image, off);
        off += 26;

        tmd->version = buffer[0];
        tmd->ca_crl_version = buffer[1];
        tmd->signer_crl_version = buffer[2];

        tmd->sys_version = be64 (&buffer[4]);
        tmd->title_id = be64 (&buffer[12]);
        tmd->title_type = be32 (&buffer[20]);
        tmd->group_id = be16 (&buffer[24]);

        off += 62;

        io_read (buffer, 10, image, off);
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
                io_read (buffer, 0x30, image, off);
                off += 0x30;

                tmd->contents[i].cid = be32 (buffer);
                tmd->contents[i].index = be16 (&buffer[4]);
                tmd->contents[i].type = be16 (&buffer[6]);
                tmd->contents[i].size = be64 (&buffer[8]);
                memcpy (tmd->contents[i].hash, &buffer[16], 20);

        }

        return;
}

void CWIIDisc::tmd_free (struct tmd *tmd)
{
        if (tmd == NULL)
                return;

        if (tmd->sig)
                free (tmd->sig);

        if (tmd->contents)
                free (tmd->contents);

        free (tmd);
}

u8 CWIIDisc::get_partitions (struct image_file *image)
{
        u8 buffer[16];
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
        io_read (buffer, 16, image, 0x40000);
        image->nparts = 1 + be32 (buffer);

        nchans = be32 (&buffer[8]);

		// number of partitions is out by one
        AddToLog("number of partitions:", image->nparts);
        AddToLog("number of channels:", nchans);

		// store the values for later bit twiddling
		image->ChannelCount = nchans;
		image->PartitionCount = image->nparts -1;

        image->nparts += nchans;

 
        part_tbl_offset = u64 (be32 (&buffer[4])) * ((u64)(4));
        chan_tbl_offset = (u64 )(be32 (&buffer[12])) * ((u64) (4));
        AddToLog("partition table offset: ", part_tbl_offset);
        AddToLog("channel table offset: ", chan_tbl_offset);

		image->part_tbl_offset = part_tbl_offset;
		image->chan_tbl_offset = chan_tbl_offset;

        image->parts = (struct partition *)
                        malloc (image->nparts * sizeof (struct partition));
        memset (image->parts, 0, image->nparts * sizeof (struct partition));

        for (i = 1; i < image->nparts; ++i) {
				AddToLog("--------------------------------------------------------------------------");
                AddToLog("partition:", i);

                if (i < image->nparts - nchans)
				{
                        io_read (buffer, 8, image,
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
						AddToLog("Virtual console");
                        
						// error in WiiFuse as it 'assumes' there are only two
						// partitions before VC games

						// changed to a generic version
						io_read (buffer, 8, image,
                                 chan_tbl_offset + (i - image->PartitionCount - 1) * 8);

                        image->parts[i].type = PART_VC;
                        image->parts[i].chan_id[0] = buffer[4];
                        image->parts[i].chan_id[1] = buffer[5];
                        image->parts[i].chan_id[2] = buffer[6];
                        image->parts[i].chan_id[3] = buffer[7];
                }

                image->parts[i].offset = (u64)(be32 (buffer)) * ((u64)(4));

                AddToLog("partition offset: ", image->parts[i].offset);

				// mark the block as used
				MarkAsUsed(image->parts[i].offset, 0x8000);


                io_read (buffer, 8, image, image->parts[i].offset + 0x2b8);
                image->parts[i].data_offset = (u64)(be32 (buffer)) << 2;
                image->parts[i].data_size = (u64)(be32 (&buffer[4])) << 2;

				// now get the H3 offset
				io_read (buffer,4, image, image->parts[i].offset + 0x2b4);
				image->parts[i].h3_offset = (u64)(be32 (buffer)) << 2 ;

                AddToLog("partition data offset:", image->parts[i].data_offset);
                AddToLog("partition data size:", image->parts[i].data_size);
                AddToLog("H3 offset:", image->parts[i].h3_offset);

                tmd_load (image, i);
                if (image->parts[i].tmd == NULL) {
                        AddToLog("partition has no valid tmd");

                        continue;
                }

               sprintf (image->parts[i].title_id_str, "%016llx",
                         image->parts[i].tmd->title_id);

			   image->parts[i].is_encrypted = image->encrypted;
                image->parts[i].cached_block = 0xffffffff;

                memset (title_key, 0, 16);
                memset (iv, 0, 16);

                io_read (title_key, 16, image, image->parts[i].offset + 0x1bf);
                io_read (iv, 8, image, image->parts[i].offset + 0x1dc);


                AES_cbc_encrypt (title_key, partition_key, 16,
                                 &image->key, iv, AES_DECRYPT);
                
				memcpy(image->parts[i].title_key, partition_key, 16);

				AES_set_decrypt_key (partition_key, 128, &image->parts[i].key);

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

size_t CWIIDisc::io_read_part (unsigned char *ptr, size_t size, struct image_file *image, u32 part, u64 offset)
{
        u32 block = (u32)(offset / (u64)(0x7c00));
        u32 cache_offset = (u32)(offset % (u64)(0x7c00));
        u32 cache_size;
        unsigned char *dst = ptr;

        if (!image->parts[part].is_encrypted)
                return io_read (ptr, size, image,
                                image->parts[part].offset + offset);

        while (size) {
                if (decrypt_block (image, part, block))
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

int CWIIDisc::image_parse (struct image_file *image)
{
        u8 buffer[0x440];
        u8 *fst;
        u32 i;
        u8 j, valid, nvp;

        u32 nfiles;
		
//		HTREEITEM hPartitionBin;

		char csText[255];


        if (image->is_wii)
		{
                AddToLog("wii image detected");
				strcpy(hDisc,"WII DISC");

                get_partitions (image);
        } else return 0;

        _fstat (image->fp, &image->st);


        nvp = 0;
        for (i = 0; i < image->nparts; ++i)
        {
				AddToLog("------------------------------------------------------------------------------");

                AddToLog("partition:", i);

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

                if (!io_read_part (buffer, 0x440, image, i, 0)) {
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
                        AddToLog("invalid header for partition:", i);
                        continue;
                }
                nvp++;


                image_parse_header (&image->parts[i].header, buffer);




				if (PART_UNKNOWN!=image->parts[i].type)
				{
					AddToLog("\\partition.bin", image->parts[i].offset, image->parts[i].data_offset);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-4]",
						"partition.bin",
						i,
						image->parts[i].offset,
						image->parts[i].data_offset);*/
					MarkAsUsed(image->parts[i].offset, image->parts[i].data_offset);
///					hPartitionBin = m_pParent->AddItemToTree(csText, hPartition[i]);

					// add on the boot.bin
					AddToLog("\\boot.bin", image->parts[i].offset + image->parts[i].data_offset, (u64)0x440);
					MarkAsUsedDC(image->parts[i].offset + image->parts[i].data_offset, 0, (u64)0x440, image->parts[i].is_encrypted);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [0]",
						"boot.bin",
						i,
						(u64) 0x0,
						(u64)0x440);*/
					
///					m_pParent->AddItemToTree(csText, hPartition[i]);
					
					
					// add on the bi2.bin
					AddToLog("\\bi2.bin", image->parts[i].offset + image->parts[i].data_offset + 0x440, (u64)0x2000);
					MarkAsUsedDC(image->parts[i].offset + image->parts[i].data_offset, 0x440, (u64)0x2000, image->parts[i].is_encrypted);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [0]",
						"bi2.bin",
						i,
						(u64) 0x440,
						(u64)0x2000);*/
					
///					m_pParent->AddItemToTree(csText, hPartition[i]);
					
				}
                io_read_part (buffer, 8, image, i, 0x2440 + 0x14);
                image->parts[i].appldr_size =
                        be32 (buffer) + be32 (&buffer[4]);
                if (image->parts[i].appldr_size > 0)
                        image->parts[i].appldr_size += 32;


				if (image->parts[i].appldr_size > 0)
				{
					AddToLog("\\apploader.img", image->parts[i].offset+ image->parts[i].data_offset +0x2440, image->parts[i].appldr_size);
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
					AddToLog("apploader.img not present");
				}

                if (image->parts[i].header.dol_offset > 0)
				{
                        io_read_part (buffer, 0x100, image, i, image->parts[i].header.dol_offset);
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
						
						AddToLog("\\main.dol ", image->parts[i].offset + image->parts[i].data_offset + image->parts[i].header.dol_offset, image->parts[i].header.dol_size);

/*						csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-2]",
										"main.dol",
										i,
										image->parts[i].header.dol_offset,
										image->parts[i].header.dol_size);*/

///						m_pParent->AddItemToTree(csText, hPartition[i]);
                } else{

                        AddToLog("partition has no main.dol");
				}

				if (image->parts[i].is_encrypted)
				{
					// Now add the TMD.BIN and cert.bin files - as these are part of partition.bin
					// we don't need to mark them as used - we do put them undr partition.bin in the
					// tree though

					AddToLog("\\tmd.bin", image->parts[i].offset + image->parts[i].tmd_offset, image->parts[i].tmd_size);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-5]",
						"tmd.bin",
						i,
						image->parts[i].tmd_offset,
						image->parts[i].tmd_size);*/
					
///					m_pParent->AddItemToTree(csText, hPartitionBin);

					AddToLog("\\cert.bin", image->parts[i].offset + image->parts[i].cert_offset, image->parts[i].cert_size);
/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-6]",
						"cert.bin",
						i,
						(u64)image->parts[i].cert_offset,
						(u64)image->parts[i].cert_size);*/
					
///					m_pParent->AddItemToTree(csText, hPartitionBin);


				
					// add on the H3
					AddToLog("\\h3.bin", image->parts[i].offset + image->parts[i].h3_offset, (u64)0x18000);
					MarkAsUsedDC(	image->parts[i].offset,
									image->parts[i].h3_offset,
									(u64)0x18000,
									FALSE);

/*					csText.Format("%s [0x%lX] [0x%I64X] [0x%I64X] [-7]",
										"h3.bin",
										i,
										(u64) image->parts[i].h3_offset,
										(u64)0x18000);*/

///					m_pParent->AddItemToTree(csText, hPartitionBin);

				}
				
                
				if (image->parts[i].header.fst_offset > 0 &&
                    image->parts[i].header.fst_size > 0) {

                        AddToLog("\\fst.bin ", image->parts[i].offset+image->parts[i].data_offset+image->parts[i].header.fst_offset,image->parts[i].header.fst_size);

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
                        if (io_read_part (fst, (u32)(image->parts[i].header.fst_size),
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
                                AddToLog("invalid fst for partition", i);
                        } else {
///								m_csText = "\\";

                                parse_fst (fst, (char *) (fst + 12 * nfiles), 0, NULL, image, i, (char**)hPartition[i]);
                        }

                        free (fst);
                } else
				{
                        AddToLog("partition has no fst");
				}


        }

        if (!nvp) {
                AddToLog("no valid partition were found, exiting");
                return 1;
        }

        return 0;
}