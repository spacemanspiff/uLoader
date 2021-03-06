/*
 partition.c
 Functions for mounting and dismounting partitions
 on various block devices.

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "partition.h"
#include "bit_ops.h"
#include "file_allocation_table.h"
#include "directory.h"
#include "mem_allocate.h"
#include "fatfile.h"

#include <string.h>
#include <ctype.h>
#include <sys/iosupport.h>

/* 
This device name, as known by devkitPro toolchains
*/
const char* DEVICE_NAME = "fat";

/*
Data offsets
*/

// BIOS Parameter Block offsets
enum BPB {
	BPB_jmpBoot = 0x00,
	BPB_OEMName = 0x03,
	// BIOS Parameter Block
	BPB_bytesPerSector = 0x0B,
	BPB_sectorsPerCluster = 0x0D,
	BPB_reservedSectors = 0x0E,
	BPB_numFATs = 0x10,
	BPB_rootEntries = 0x11,
	BPB_numSectorsSmall = 0x13,
	BPB_mediaDesc = 0x15,
	BPB_sectorsPerFAT = 0x16,
	BPB_sectorsPerTrk = 0x18,
	BPB_numHeads = 0x1A,
	BPB_numHiddenSectors = 0x1C,
	BPB_numSectors = 0x20,
	// Ext BIOS Parameter Block for FAT16
	BPB_FAT16_driveNumber = 0x24,
	BPB_FAT16_reserved1 = 0x25,
	BPB_FAT16_extBootSig = 0x26,
	BPB_FAT16_volumeID = 0x27,
	BPB_FAT16_volumeLabel = 0x2B,
	BPB_FAT16_fileSysType = 0x36,
	// Bootcode
	BPB_FAT16_bootCode = 0x3E,
	// FAT32 extended block
	BPB_FAT32_sectorsPerFAT32 = 0x24,
	BPB_FAT32_extFlags = 0x28,
	BPB_FAT32_fsVer = 0x2A,
	BPB_FAT32_rootClus = 0x2C,
	BPB_FAT32_fsInfo = 0x30,
	BPB_FAT32_bkBootSec = 0x32,
	// Ext BIOS Parameter Block for FAT32
	BPB_FAT32_driveNumber = 0x40,
	BPB_FAT32_reserved1 = 0x41,
	BPB_FAT32_extBootSig = 0x42,
	BPB_FAT32_volumeID = 0x43,
	BPB_FAT32_volumeLabel = 0x47,
	BPB_FAT32_fileSysType = 0x52,
	// Bootcode
	BPB_FAT32_bootCode = 0x5A,
	BPB_bootSig_55 = 0x1FE,
	BPB_bootSig_AA = 0x1FF
};

static const char FAT_SIG[3] = {'F', 'A', 'T'};


PARTITION* _FAT_partition_constructor (const DISC_INTERFACE* disc, uint32_t cacheSize, sec_t startSector) {
	PARTITION* partition;
	int i;
	
	static uint8_t sectorBuffer[BYTES_PER_READ]__attribute__((aligned(32))) = {0};

	sectorBuffer[0]=0;
	
	// Read first sector of disc
	if (!_FAT_disc_readSectors (disc, startSector, 1, sectorBuffer)) {
		return NULL;
	}

	// Make sure it is a valid MBR or boot sector
	if ( (sectorBuffer[BPB_bootSig_55] != 0x55) || (sectorBuffer[BPB_bootSig_AA] != 0xAA)) {
		return NULL;
	}

	if (startSector != 0) {
		// We're told where to start the partition, so just accept it
	} else if (!memcmp(sectorBuffer + BPB_FAT16_fileSysType, FAT_SIG, sizeof(FAT_SIG))) {
		// Check if there is a FAT string, which indicates this is a boot sector
		startSector = 0;
	} else if (!memcmp(sectorBuffer + BPB_FAT32_fileSysType, FAT_SIG, sizeof(FAT_SIG))) {
		// Check for FAT32
		startSector = 0;
	} else {
		#if 0
		// This is an MBR
		// Find first valid partition from MBR
		// First check for an active partition
		for (i=0x1BE; (i < 0x1FE) && (sectorBuffer[i] != 0x80); i+= 0x10);
		// If it didn't find an active partition, search for any valid partition
		if (i == 0x1FE) {
			for (i=0x1BE; (i < 0x1FE) && (sectorBuffer[i+0x04] == 0x00); i+= 0x10);
		}
		
		if ( i != 0x1FE) {
			// Go to first valid partition
			startSector = u8array_to_u32(sectorBuffer, 0x8 + i);
			// Load the BPB
			if (!_FAT_disc_readSectors (disc, startSector, 1, sectorBuffer)) {
					return NULL;
			}
			// Make sure it is a valid BPB
			if ( (sectorBuffer[BPB_bootSig_55] != 0x55) || (sectorBuffer[BPB_bootSig_AA] != 0xAA)) {
				return NULL;
			}
		} else {
			// No partition found, assume this is a MBR free disk
			startSector = 0;
		}
	#else

		uint8_t part_table[16*4],*ptr;

		//find FAT partition
		if(sectorBuffer[0x1fe]!=0x55 || sectorBuffer[0x1ff]!=0xaa) memset(part_table,0,16*4);
		else memcpy(part_table,sectorBuffer+0x1be,16*4);

        ptr = part_table;

		

        for(i=0;i<4;i++,ptr+=16)
        { 
		u32 part_lba = u8array_to_u32(ptr, 0x8 );
		
		if(ptr[4]==0) continue;

		if(ptr[4]==0xf)
			{
			u32 part_lba2=part_lba;
			u32 next_lba2=0;
			int n;
			
			for(n=0;n<8;n++) // max 8 logic partitions (i think it is sufficient!)
				{
					startSector=part_lba+next_lba2;
					if (!_FAT_disc_readSectors (disc, startSector, 1, sectorBuffer)) {return NULL;}
		

					part_lba2=part_lba+next_lba2+u8array_to_u32(sectorBuffer, 0x1C6);
					next_lba2=u8array_to_u32(sectorBuffer, 0x1D6);

					startSector=part_lba2;
					if (!_FAT_disc_readSectors (disc, startSector, 1, sectorBuffer)) {return NULL;}

					if (!memcmp(sectorBuffer + BPB_FAT16_fileSysType, FAT_SIG, sizeof(FAT_SIG)) 
						|| !memcmp(sectorBuffer + BPB_FAT32_fileSysType, FAT_SIG, sizeof(FAT_SIG))) goto fat_finded;

					if(next_lba2==0) break;
				}
			}  
          else
				{
					startSector=part_lba;
					if (!_FAT_disc_readSectors (disc, startSector, 1, sectorBuffer)) {return NULL;}

					// verify there is the magic.
					if (!memcmp(sectorBuffer + BPB_FAT16_fileSysType, FAT_SIG, sizeof(FAT_SIG)) 
						|| !memcmp(sectorBuffer + BPB_FAT32_fileSysType, FAT_SIG, sizeof(FAT_SIG))) goto fat_finded;
				}
       
		
		}
	

		#endif
	}
fat_finded:
	// Now verify that this is indeed a FAT partition
	if (memcmp(sectorBuffer + BPB_FAT16_fileSysType, FAT_SIG, sizeof(FAT_SIG)) && 
		memcmp(sectorBuffer + BPB_FAT32_fileSysType, FAT_SIG, sizeof(FAT_SIG)))
	{
		return NULL;
	}

	partition = (PARTITION*) _FAT_mem_allocate (sizeof(PARTITION));
	if (partition == NULL) {
		return NULL;
	}

	// Init the partition lock
	_FAT_lock_init(&partition->lock);
	
	// Set partition's disc interface
	partition->disc = disc;

	// Store required information about the file system
	partition->fat.sectorsPerFat = u8array_to_u16(sectorBuffer, BPB_sectorsPerFAT);
	if (partition->fat.sectorsPerFat == 0) {
		partition->fat.sectorsPerFat = u8array_to_u32( sectorBuffer, BPB_FAT32_sectorsPerFAT32); 
	}

	partition->numberOfSectors = u8array_to_u16( sectorBuffer, BPB_numSectorsSmall); 
	if (partition->numberOfSectors == 0) {
		partition->numberOfSectors = u8array_to_u32( sectorBuffer, BPB_numSectors);	
	}

	partition->bytesPerSector = BYTES_PER_READ;	// Sector size is redefined to be 512 bytes
	partition->sectorsPerCluster = sectorBuffer[BPB_sectorsPerCluster] * u8array_to_u16(sectorBuffer, BPB_bytesPerSector) / BYTES_PER_READ;
	partition->bytesPerCluster = partition->bytesPerSector * partition->sectorsPerCluster;
	partition->fat.fatStart = startSector + u8array_to_u16(sectorBuffer, BPB_reservedSectors); 

	partition->rootDirStart = partition->fat.fatStart + (sectorBuffer[BPB_numFATs] * partition->fat.sectorsPerFat);
	partition->dataStart = partition->rootDirStart + 
		(( u8array_to_u16(sectorBuffer, BPB_rootEntries) * DIR_ENTRY_DATA_SIZE) / partition->bytesPerSector);

	partition->totalSize = ((uint64_t)partition->numberOfSectors - (partition->dataStart - startSector)) * (uint64_t)partition->bytesPerSector;

	// Store info about FAT
	partition->fat.lastCluster = (partition->numberOfSectors - (uint32_t)(partition->dataStart - startSector)) / partition->sectorsPerCluster;
	partition->fat.firstFree = CLUSTER_FIRST;

	if (partition->fat.lastCluster < CLUSTERS_PER_FAT12) {
		partition->filesysType = FS_FAT12;	// FAT12 volume
	} else if (partition->fat.lastCluster < CLUSTERS_PER_FAT16) {
		partition->filesysType = FS_FAT16;	// FAT16 volume
	} else {
		partition->filesysType = FS_FAT32;	// FAT32 volume
	}

	if (partition->filesysType != FS_FAT32) {
		partition->rootDirCluster = FAT16_ROOT_DIR_CLUSTER;
	} else {
		// Set up for the FAT32 way
		partition->rootDirCluster = u8array_to_u32(sectorBuffer, BPB_FAT32_rootClus); 
		// Check if FAT mirroring is enabled
		if (!(sectorBuffer[BPB_FAT32_extFlags] & 0x80)) {
			// Use the active FAT
			partition->fat.fatStart = partition->fat.fatStart + ( partition->fat.sectorsPerFat * (sectorBuffer[BPB_FAT32_extFlags] & 0x0F));
		}
	}

	// Create a cache to use
	uint32_t sectorsPerPage= partition->sectorsPerCluster > PAGE_SECTORS ? PAGE_SECTORS  : partition->sectorsPerCluster;

	if(sectorsPerPage<16 && PAGE_SECTORS>=16) sectorsPerPage=16;

	partition->cache = _FAT_cache_constructor (partition->fat.fatStart, cacheSize, sectorsPerPage, partition->disc, startSector+partition->numberOfSectors);


	// Set current directory to the root
	partition->cwdCluster = partition->rootDirCluster;
	
	// Check if this disc is writable, and set the readOnly property appropriately
	partition->readOnly = !(_FAT_disc_features(disc) & FEATURE_MEDIUM_CANWRITE);
	
	// There are currently no open files on this partition
	partition->openFileCount = 0;
	partition->firstOpenFile = NULL;

	return partition;
}

void _FAT_partition_destructor (PARTITION* partition) {
	FILE_STRUCT* nextFile;
	
	_FAT_lock(&partition->lock);

	// Synchronize open files
	nextFile = partition->firstOpenFile;
	while (nextFile) {
		_FAT_syncToDisc (nextFile);
		nextFile = nextFile->nextOpenFile;
	}
	
	// Free memory used by the cache, writing it to disc at the same time
	_FAT_cache_destructor (partition->cache);

	// Unlock the partition and destroy the lock
	_FAT_unlock(&partition->lock);
	_FAT_lock_deinit(&partition->lock);
	
	// Free memory used by the partition
	_FAT_mem_free (partition);
}

extern const devoptab_t* My_GetDeviceOpTab (const char *name);

PARTITION* _FAT_partition_getPartitionFromPath (const char* path) {
	const devoptab_t *devops;
	
	devops = My_GetDeviceOpTab (path);
	
	if (!devops) {
		return NULL;
	}
	
	return (PARTITION*)devops->deviceData;
}
