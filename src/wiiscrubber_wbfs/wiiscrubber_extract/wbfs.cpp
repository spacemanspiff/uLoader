#include <winsock.h>
#include "wbfs.h"

wbfs_t *wbfs_open_partition(rw_sector_callback_t read_hdsector,
                            rw_sector_callback_t write_hdsector,
						    close_callback_t close_hd,
                            void *callback_data,
                            int hd_sector_size, int num_hd_sector, u32 part_lba, int reset);

void fatal(char *msg)
{
	printf(msg);
	_exit;
}

static int read_sector(void *_handle, u32 lba, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD read;
	u64 offset = lba;
	
	offset *= 512ULL;
	large.QuadPart = offset;

	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		fprintf(stderr, "\n\n%lld %d %p\n", offset, count, _handle);
		wbfs_error("error seeking in hd sector (read)");
		return 1;
	}
	
	read = 0;
	if (ReadFile(handle, buf, count * 512ULL, &read, NULL) == FALSE)
	{
		wbfs_error("error reading hd sector");
		return 1;
	}
	
	return 0;
}

static int write_sector(void *_handle, u32 lba, u32 count, void *buf)
{
	HANDLE *handle = (HANDLE *)_handle;
	LARGE_INTEGER large;
	DWORD written;
	u64 offset = lba;

	offset *= 512ULL;
	large.QuadPart = offset;

	if (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == FALSE)
	{
		wbfs_error("error seeking in hd sector (write)");
		return 1;
	}
	
	written = 0;
	if (WriteFile(handle, buf, count * 512ULL, &written, NULL) == FALSE)
	{
		wbfs_error("error writing hd sector");
		return 1;
	}
	
	return 0;
  
}

static void close_handle(void *handle)
{
	CloseHandle((HANDLE *)handle);
}

static int get_capacity(char *fileName, u32 *sector_size, u32 *sector_count)
{
	DISK_GEOMETRY dg;
	PARTITION_INFORMATION pi;

	DWORD bytes;
	HANDLE handle = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		wbfs_error("could not open drive");
		return 0;
	}
	
	if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(DISK_GEOMETRY), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get drive geometry");
		return 0;
	}

	*sector_size = dg.BytesPerSector;

	if (DeviceIoControl(handle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &pi, sizeof(PARTITION_INFORMATION), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get partition info");
		return 0;
	}

	*sector_count = (u32)(pi.PartitionLength.QuadPart / dg.BytesPerSector);
	
	CloseHandle(handle);
	return 1;
}


wbfs_t *wbfs_try_open_partition(char *partitionLetter, int reset)
{
	HANDLE handle;
	char drivePath[8] = "\\\\?\\Z:";
	
	u32 sector_size, sector_count;
	
	if (strlen(partitionLetter) == 0)
	{
		wbfs_error("bad drive name");
		return NULL;
	}

	drivePath[4] = partitionLetter[0];
	
	if (!get_capacity(drivePath, &sector_size, &sector_count))
	{
		return NULL;
	}
	
	handle = CreateFileA(drivePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	
	if (handle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	
	return wbfs_open_partition(read_sector, write_sector, close_handle, handle, sector_size, sector_count, 0, reset);
}

wbfs_t *wbfs_try_open(char *disc, char *partition, int reset)
{
	wbfs_t *p = 0;
	
	if (partition)
	{
		p = wbfs_try_open_partition(partition,reset);
	}
	
	if (!p && !reset && disc)
	{
		p = 0;
	}
	else if(!p && !reset)
	{
		p = 0;
	}

	return p;
}

//---------------------------------------------------------------------------

#define ERR(x) do {wbfs_error(x);goto error;}while(0)
#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(p->hd_sec_sz-1)))
static int force_mode=0;
void wbfs_set_force_mode(int force)
{
        force_mode = force;
}

void wbfs_sync(wbfs_t*p)
{
        // copy back descriptors
        if(p->write_hdsector){
                p->write_hdsector(p->callback_data,p->part_lba+0,1, p->head);
                
                if(p->freeblks)
                        p->write_hdsector(p->callback_data,p->part_lba+p->freeblks_lba,ALIGN_LBA(p->n_wbfs_sec/8)>>p->hd_sec_sz_s, p->freeblks);
        }
}

static u8 size_to_shift(u32 size)
{
        u8 ret = 0;
        while(size)
        {
                ret++;
                size>>=1;
        }
        return ret-1;
}

wbfs_t *wbfs_open_partition(rw_sector_callback_t read_hdsector,
                            rw_sector_callback_t write_hdsector,
						    close_callback_t close_hd,
                            void *callback_data,
                            int hd_sector_size, int num_hd_sector, u32 part_lba, int reset)
{
        wbfs_t *p = (wbfs_t*)wbfs_malloc(sizeof(wbfs_t));
        
        wbfs_head_t *head = (wbfs_head_t*)wbfs_ioalloc(hd_sector_size?hd_sector_size:512);

        //constants, but put here for consistancy
        p->wii_sec_sz = 0x8000;
        p->wii_sec_sz_s = size_to_shift(0x8000);
        p->n_wii_sec = (num_hd_sector/0x8000)*hd_sector_size;
        p->n_wii_sec_per_disc = 143432*2;//support for double layers discs..
        p->head = head;
        p->part_lba = part_lba;
        // init the partition
        if (reset)
        {
                u8 sz_s;
                wbfs_memset(head,0,hd_sector_size);
                head->magic = wbfs_htonl(WBFS_MAGIC);
                head->hd_sec_sz_s = size_to_shift(hd_sector_size);
                head->n_hd_sec = wbfs_htonl(num_hd_sector);
                // choose minimum wblk_sz that fits this partition size
                for(sz_s=6;sz_s<11;sz_s++)
                {
                        // ensure that wbfs_sec_sz is big enough to address every blocks using 16 bits
                        if(p->n_wii_sec <((1U<<16)*(1<<sz_s)))
                                break;
                }
                head->wbfs_sec_sz_s = sz_s+p->wii_sec_sz_s;
        }else
                read_hdsector(callback_data,p->part_lba,1,head);
        if (head->magic != wbfs_htonl(WBFS_MAGIC))
                ERR("bad magic");
        if(!force_mode && hd_sector_size && head->hd_sec_sz_s !=  size_to_shift(hd_sector_size))
                ERR("hd sector size doesn't match");
        if(!force_mode && num_hd_sector && head->n_hd_sec != wbfs_htonl(num_hd_sector))
                ERR("hd num sector doesn't match");
        p->hd_sec_sz = 1<<head->hd_sec_sz_s;
        p->hd_sec_sz_s = head->hd_sec_sz_s;
        p->n_hd_sec = wbfs_ntohl(head->n_hd_sec);

        p->n_wii_sec = (p->n_hd_sec/p->wii_sec_sz)*(p->hd_sec_sz);
        
        p->wbfs_sec_sz_s = head->wbfs_sec_sz_s;
        p->wbfs_sec_sz = 1<<p->wbfs_sec_sz_s;
        p->n_wbfs_sec = p->n_wii_sec >> (p->wbfs_sec_sz_s - p->wii_sec_sz_s);
        p->n_wbfs_sec_per_disc = p->n_wii_sec_per_disc >> (p->wbfs_sec_sz_s - p->wii_sec_sz_s);
        p->disc_info_sz = ALIGN_LBA(sizeof(wbfs_disc_info_t) + p->n_wbfs_sec_per_disc*2);

        //printf("hd_sector_size %X wii_sector size %X wbfs sector_size %X\n",p->hd_sec_sz,p->wii_sec_sz,p->wbfs_sec_sz);
        p->read_hdsector = read_hdsector;
        p->write_hdsector = write_hdsector;
		p->close_hd = close_hd;
        p->callback_data = callback_data;

        p->freeblks_lba = (p->wbfs_sec_sz - p->n_wbfs_sec/8)>>p->hd_sec_sz_s;
        
        if(!reset)
                p->freeblks = 0; // will alloc and read only if needed
        else
        {
                // init with all free blocks
                p->freeblks = (u32*)wbfs_ioalloc(ALIGN_LBA(p->n_wbfs_sec/8));
                wbfs_memset(p->freeblks,0xff,p->n_wbfs_sec/8);
        }
        p->max_disc = (p->freeblks_lba-1)/(p->disc_info_sz>>p->hd_sec_sz_s);
        if(p->max_disc > p->hd_sec_sz - sizeof(wbfs_head_t))
                p->max_disc = p->hd_sec_sz - sizeof(wbfs_head_t);

        p->tmp_buffer = (u8*)wbfs_ioalloc(p->hd_sec_sz);
        p->n_disc_open = 0;
        wbfs_sync(p);
        return p;
error:
        wbfs_free(p);
        wbfs_iofree(head);
        return 0;
            
}

void wbfs_close(wbfs_t*p)
{
        wbfs_sync(p);

        if(p->n_disc_open)
                ERR("trying to close wbfs while discs still open");

        wbfs_iofree(p->head);
        wbfs_iofree(p->tmp_buffer);
        if(p->freeblks)
                wbfs_iofree(p->freeblks);
        
		p->close_hd(p->callback_data);

        wbfs_free(p);
        
error:
        return;
}

wbfs_disc_t *wbfs_open_disc(wbfs_t* p, u8 *discid)
{
        u32 i;
        int disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
        wbfs_disc_t *d = 0;
        for(i=0;i<p->max_disc;i++)
        {
                if (p->head->disc_table[i]){
                        p->read_hdsector(p->callback_data,
                                         p->part_lba+1+i*disc_info_sz_lba,1,p->tmp_buffer);
                        if(wbfs_memcmp(discid,p->tmp_buffer,6)==0){
                                d = (wbfs_disc_t*)wbfs_malloc(sizeof(*d));
                                if(!d)
                                        ERR("allocating memory");
                                d->p = p;
                                d->i = i;
                                d->header = (wbfs_disc_info_t*)wbfs_ioalloc(p->disc_info_sz);
                                if(!d->header)
                                        ERR("allocating memory");
                                p->read_hdsector(p->callback_data,
                                                  p->part_lba+1+i*disc_info_sz_lba,
                                                  disc_info_sz_lba,d->header);
                                p->n_disc_open ++;
//                                for(i=0;i<p->n_wbfs_sec_per_disc;i++)
//                                        printf("%d,",wbfs_ntohs(d->header->wlba_table[i]));
                                return d;
                        }
                }
        }
        return 0;
error:
        if(d)
                wbfs_iofree(d);
        return 0;
        
}

void wbfs_close_disc(wbfs_disc_t*d)
{
        d->p->n_disc_open --;
        wbfs_iofree(d->header);
        wbfs_free(d);
}

u32 wbfs_count_discs(wbfs_t*p)
{
        u32 i,count=0;
        for(i=0;i<p->max_disc;i++)
                if (p->head->disc_table[i])
                        count++;
        return count;
        
}

static u32 wbfs_sector_used(wbfs_t *p,wbfs_disc_info_t *di)
{
        u32 tot_blk=0,j;
        for(j=0;j<p->n_wbfs_sec_per_disc;j++)
                if(wbfs_ntohs(di->wlba_table[j]))
                        tot_blk++;
        return tot_blk;

}

u32 wbfs_get_disc_info(wbfs_t*p, u32 index,u8 *header,int header_size,u32 *size)//size in 32 bit
{
        u32 i,count=0;
        int disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
        for(i=0;i<p->max_disc;i++)
                if (p->head->disc_table[i]){
                        if(count++==index)
                        {
								u32 magic;
                                p->read_hdsector(p->callback_data,
                                                 p->part_lba+1+i*disc_info_sz_lba,1,p->tmp_buffer);
                                if(header_size > (int)p->hd_sec_sz)
                                        header_size = p->hd_sec_sz;
                                magic = wbfs_ntohl(*(u32*)(p->tmp_buffer+24));
                                if(magic!=0x5D1C9EA3){
                                        p->head->disc_table[i]=0;
                                        return 1;
                                }
                                memcpy(header,p->tmp_buffer,header_size);
                                if(size)
                                {
										u32 sec_used;    
										u8 *header = (u8*)wbfs_ioalloc(p->disc_info_sz);
                                        p->read_hdsector(p->callback_data,
                                                         p->part_lba+1+i*disc_info_sz_lba,disc_info_sz_lba,header);
                                        sec_used = wbfs_sector_used(p,(wbfs_disc_info_t *)header);
                                        wbfs_iofree(header);
                                        *size = sec_used<<(p->wbfs_sec_sz_s-2);
                                }
                                return 0;
                        }
                }
        return 1;
}

bool wbfs_read_disc(wbfs_disc_t*d, u64 offset, s32 size, char *dst)
{
        wbfs_t *p = d->p;
        u8* copy_buffer = 0;

		u32 sector = offset / p->wbfs_sec_sz;
		offset = offset % p->wbfs_sec_sz;
		u32 dst_pos = 0;
//		u32 count = (size / p->wbfs_sec_sz) + 1;

        int i;
        int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;
//        int dst_wbs_nlb=p->wbfs_sec_sz/p->wii_sec_sz;
		copy_buffer = (u8*)wbfs_ioalloc(p->wbfs_sec_sz);
        if(!copy_buffer) ERR("alloc memory");

        for( i=sector; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {
                       
                        //if(spinner) spinner(i,p->n_wbfs_sec_per_disc);
						p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);

						/////////////////////////////
                        /* uLoader patch */

						if(i==0)
						{
						if(copy_buffer[1024]=='H' && copy_buffer[1025]=='D' && copy_buffer[1026]=='R')
							{
							memset(&copy_buffer[1024+8], 0, copy_buffer[1027]*1024);
							memset(&copy_buffer[1024],0,8);
							}
						}

						/////////////////////////////
						if (offset)
						{
							memcpy(dst,copy_buffer+offset,size>(p->wbfs_sec_sz-offset)?(p->wbfs_sec_sz-offset):size);
							size-=(size>(p->wbfs_sec_sz-offset)?(p->wbfs_sec_sz-offset):size);
							dst_pos+=size>(p->wbfs_sec_sz-offset)?(p->wbfs_sec_sz-offset):size;
							offset = 0;
						} else
						{
							memcpy(dst+dst_pos,copy_buffer,size<p->wbfs_sec_sz?size:p->wbfs_sec_sz);
							size-=(size<p->wbfs_sec_sz?size:p->wbfs_sec_sz);
							dst_pos+=(size<p->wbfs_sec_sz?size:p->wbfs_sec_sz);
						}
						if (!size) break;
                        //write_dst_wii_sector(callback_data, i*dst_wbs_nlb, dst_wbs_nlb, copy_buffer);
						//j++;
						//if (j == count) break;
                } else return 0;
        }
        wbfs_iofree(copy_buffer);
//		memcpy(dst,copy_buffer,size);
        return 1;
error:
        return 0;
}
