// Copyright 2009 Kwiirk
// Modified by Hermes
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


#include "libwbfs.h"
#include <conio.h>
#include <ctype.h>

#ifndef WIN32
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)		(x)
#define unlikely(x)		(x)
#endif

#define ERR(x) do {wbfs_error(x);goto error;}while(0)
#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(p->hd_sec_sz-1)))

void set_ciso_mode(wiidisc_t*d, int mode);

static int force_mode=0;

void wbfs_set_force_mode(int force)
{
        force_mode = force;
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
#define read_le32_unaligned(x) ((x)[0]|((x)[1]<<8)|((x)[2]<<16)|((x)[3]<<24))

void wbfs_sync(wbfs_t*p);

wbfs_t*wbfs_open_hd(
					rw_sector_callback_t read_hdsector,
					rw_sector_callback_t write_hdsector,
					close_callback_t close_hd,
					void *callback_data,
					int hd_sector_size,
#ifdef WIN32
					int num_hd_sector,
#else
					int num_hd_sector __attribute((unused)),
#endif
					int reset)
{
        int i=num_hd_sector,ret;
        u8 *ptr,*tmp_buffer = wbfs_ioalloc(hd_sector_size);
        u8 part_table[16*4];
        ret = read_hdsector(callback_data,0,1,tmp_buffer);
        if(ret)
                return 0;
        //find wbfs partition
        wbfs_memcpy(part_table,tmp_buffer+0x1be,16*4);
        ptr = part_table;
        for(i=0;i<4;i++,ptr+=16)
        {
                u32 part_lba = read_le32_unaligned(ptr+0x8);
                wbfs_head_t *head = (wbfs_head_t *)tmp_buffer;
                ret = read_hdsector(callback_data,part_lba,1,tmp_buffer);
                // verify there is the magic.
                if (head->magic == wbfs_htonl(WBFS_MAGIC))
                {
					
                        wbfs_t*p = wbfs_open_partition(read_hdsector,write_hdsector,close_hd,
                                                callback_data,hd_sector_size,0,part_lba,reset);
                        return p;
                }
        }
        if(reset)// XXX make a empty hd partition..
        {
        }
        return 0;
}

extern int block_ciso;
wbfs_t*wbfs_open_partition(rw_sector_callback_t read_hdsector,
                           rw_sector_callback_t write_hdsector,
						   close_callback_t close_hd,
                           void *callback_data,
                           int hd_sector_size, int num_hd_sector, u32 part_lba, int reset)
{
        wbfs_t *p = wbfs_malloc(sizeof(wbfs_t));
        
        wbfs_head_t *head = wbfs_ioalloc(hd_sector_size?hd_sector_size:512);

        //constants, but put here for consistancy
        p->wii_sec_sz = 0x8000;
        p->wii_sec_sz_s = size_to_shift(0x8000);
       
        p->n_wii_sec_per_disc = 143432*2;//support for double layers discs..
        p->head = head;
        p->part_lba = part_lba;

		 p->n_wii_sec =(u32) ((u64) num_hd_sector)/ ((u64) 0x8000);
		 p->n_wii_sec=(u32) ( ((u64) p->n_wii_sec) * ((u64) hd_sector_size) );

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
						if(((u32)p->n_wii_sec) <((1U<<16)*(1U<<sz_s)))
                                break;
                }
				
                head->wbfs_sec_sz_s = sz_s+p->wii_sec_sz_s;

			{
				printf("sz :%u %u\n", sz_s, num_hd_sector);
				system("pause");
			}
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

        block_ciso=0;
        if(p->n_wbfs_sec_per_disc>32760)
			{
			block_ciso=1;
			printf("WARNING! n_wbfs_sec_per_disc is too big for .ciso extraction!!!\nSize: %u\n\nPress any key\n\n",  p->n_wbfs_sec_per_disc);
			getch();
			}
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
                p->freeblks = wbfs_ioalloc(ALIGN_LBA(p->n_wbfs_sec/8));
                wbfs_memset(p->freeblks,0xff,p->n_wbfs_sec/8);
        }
        p->max_disc = (p->freeblks_lba-1)/(p->disc_info_sz>>p->hd_sec_sz_s);
        if(p->max_disc > p->hd_sec_sz - sizeof(wbfs_head_t))
                p->max_disc = p->hd_sec_sz - sizeof(wbfs_head_t);

        p->tmp_buffer = wbfs_ioalloc(p->hd_sec_sz);
        p->n_disc_open = 0;
        wbfs_sync(p);
        return p;
error:
        wbfs_free(p);
        wbfs_iofree(head);
        return 0;
            
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

wbfs_disc_t *wbfs_open_index_disc(wbfs_t* p, u32 i)
{
        int disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
        wbfs_disc_t *d = 0;
       
                if (p->head->disc_table[i]){
                        p->read_hdsector(p->callback_data,
                                         p->part_lba+1+i*disc_info_sz_lba,1,p->tmp_buffer);
                       
                                d = wbfs_malloc(sizeof(*d));
                                if(!d)
                                        ERR("allocating memory");
                                d->p = p;
                                d->i = i;
                                d->header = wbfs_ioalloc(p->disc_info_sz);
                                if(!d->header)
                                        ERR("allocating memory");
                                p->read_hdsector(p->callback_data,
                                                  p->part_lba+1+i*disc_info_sz_lba,
                                                  disc_info_sz_lba,d->header);
                                p->n_disc_open ++;

                                return d;
                }
       
        return 0;
error:
        if(d)
                wbfs_iofree(d);
        return 0;
        
}

static void load_freeblocks(wbfs_t*p);

void wbfs_integrity_check(wbfs_t* p)
{
u32 i,i2,n,m, flag=0;
int ret=0;
int disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
int discn;
int keyinput;
int bad_error=0;
u8 id1[7],id2[7];

load_freeblocks(p);

i=ALIGN_LBA(p->n_wbfs_sec/8);

u32 *in_use = wbfs_ioalloc(i);

if(!in_use) {printf("Error: Out of Memory\n"); return;}

memset(in_use,0,i);


for(i=0;i<p->max_disc;i++)
        {
      	wbfs_disc_t *d= wbfs_open_index_disc(p, i);
		if(!d) continue;
		memcpy(id1,p->tmp_buffer,6);id1[6]=0;
		ret=0;
		flag=1;
		keyinput=1;

		printf("Checking %s\n",id1);

		for(i2=0;i2<p->max_disc;i2++)
			{
			wbfs_disc_t *d2;
			if(i==i2) continue;
			d2= wbfs_open_index_disc(p, i2);
			if(!d2) continue;
			memcpy(id2,p->tmp_buffer,6);id2[6]=0;
			 for(n=0; n<p->n_wbfs_sec_per_disc; n++)
				{
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[n]);
		
                if (iwlba)
					{
					if(flag==2) flag=0;
					if(flag==1) flag=2;

						{ // test if block is marked as free...
					    int i = (iwlba-1)/(32);
					    int j = (iwlba-1)&31;
        
                        u32 v = wbfs_ntohl(p->freeblks[i]);
                        if((v & (1<<j)) == (u32)(1<<j))
							{
							//wbfs_htonl(v | 1<<j);
							printf("Error! LBA %u is marked as 'Free' in Disc %s. Fixed!\n", iwlba, id1);
							p->freeblks[i] = wbfs_htonl(v & ~(1<<j));
							bad_error=1;
							}

						in_use[i]|= wbfs_htonl(v | (1<<j));
						}
					

					for(m=0; m<p->n_wbfs_sec_per_disc; m++)
						{
						u32 iwlba2 = wbfs_ntohs(d2->header->wlba_table[m]);
						if(iwlba)
							{// 4
							
							if(iwlba==iwlba2)
								{//3
								bad_error=1;
								printf("Error! Crossed LBA %u in Disc %s and Disc %s\n", iwlba, id1, id2);
								if(flag==2 && keyinput)
									{// 2
									unsigned char c;
									printf("\nThe begin of this disc is bad (LBA's Crossed)");
									printf("\nDelete Disc %s ? (y/n)\n\n",id1);

									keyinput=0;
									
									while(1) {if(!kbhit()) break; getch();}

									while(1)
										{//1
										c = getchar();
										if(c==10 || c==13 || c>127) continue;

										if (toupper(c) != 'Y')
											{
											flag=0;break;
											}
										else break;
										}// 1
									}// 2
								if(flag==2 || ret) {iwlba=0;d->header->wlba_table[n]=0;ret=1;break;}
								} // 3
							
							}// 4
						}
					}
				}

			wbfs_close_disc(d2);
			}
		///////////////
		if(ret)
			{
			printf("Deleting Disc %s\n", id1);
			discn = d->i;
			memset(d->header,0,p->disc_info_sz);
			p->write_hdsector(p->callback_data,p->part_lba+1+discn*disc_info_sz_lba,disc_info_sz_lba,d->header);
			p->head->disc_table[discn] = 0;
			}
			///////////////
		wbfs_close_disc(d);wbfs_sync(p);
		if(ret) load_freeblocks(p);
		}
if(!bad_error)
	{
	int j;

	printf("Checking for lost blocks... (Press any key to start)\n");
	while(1) {if(!kbhit()) break; getch();}
	getch();
	
	for(i=0;i<p->n_wbfs_sec/(32);i++)
		{
		u32 v = wbfs_ntohl(p->freeblks[i]);
		u32 w = wbfs_ntohl(in_use[i]);
		for(j=0;j<32;j++)
           if (!(v & (1<<j)) && !(w & (1<<j)))
			{
			printf("Block %i marked as 'Used' but really, it is 'Free'. Fixed\n", i+j);
			v|=(1<<j);
			}

		p->freeblks[i] = wbfs_htonl(v);
				
		}

	wbfs_sync(p);

	}
if(in_use) free(in_use);
printf("End\n\nPress Any Key\n");

while(1) {if(!kbhit()) break; getch();}
getch();
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
                                d = wbfs_malloc(sizeof(*d));
                                if(!d)
                                        ERR("allocating memory");
                                d->p = p;
                                d->i = i;
                                d->header = wbfs_ioalloc(p->disc_info_sz);
                                if(!d->header)
                                        ERR("allocating memory");
                                p->read_hdsector(p->callback_data,
                                                  p->part_lba+1+i*disc_info_sz_lba,
                                                  disc_info_sz_lba,d->header);
                                p->n_disc_open ++;

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
// offset is pointing 32bit words to address the whole dvd, although len is in bytes
int wbfs_disc_read(wbfs_disc_t*d,u32 offset, u8 *data, u32 len)
{
 
        wbfs_t *p = d->p;
        u16 wlba = offset>>(p->wbfs_sec_sz_s-2);
        u32 iwlba_shift = p->wbfs_sec_sz_s - p->hd_sec_sz_s;
        u32 lba_mask = (p->wbfs_sec_sz-1)>>(p->hd_sec_sz_s);
        u32 lba = (offset>>(p->hd_sec_sz_s-2))&lba_mask;
        u32 off = offset&((p->hd_sec_sz>>2)-1);
        u16 iwlba = wbfs_ntohs(d->header->wlba_table[wlba]);
        u32 len_copied;
        int err = 0;
        u8  *ptr = data;
        if(unlikely(iwlba==0))
                return 1;
        if(unlikely(off)){
                off*=4;
                err = p->read_hdsector(p->callback_data,
                                       p->part_lba + (iwlba<<iwlba_shift) + lba, 1, p->tmp_buffer);
                if(err)
                        return err;
                len_copied = p->hd_sec_sz - off;
                if(likely(len < len_copied))
                        len_copied = len;
                wbfs_memcpy(ptr, p->tmp_buffer + off, len_copied);
                len -= len_copied;
                ptr += len_copied;
                lba++;
                if(unlikely(lba>lba_mask && len)){
                        lba=0;
                        iwlba =  wbfs_ntohs(d->header->wlba_table[++wlba]);
                        if(unlikely(iwlba==0))
                                return 1;
                }
        }
        while(likely(len>=p->hd_sec_sz))
        {
                u32 nlb = len>>(p->hd_sec_sz_s);
                
                if(unlikely(lba + nlb > p->wbfs_sec_sz)) // dont cross wbfs sectors..
                        nlb = p->wbfs_sec_sz-lba;
                err = p->read_hdsector(p->callback_data,
                                 p->part_lba + (iwlba<<iwlba_shift) + lba, nlb, ptr);
                if(err)
                        return err;
                len -= nlb<<p->hd_sec_sz_s;
                ptr += nlb<<p->hd_sec_sz_s;
                lba += nlb;
                if(unlikely(lba>lba_mask && len)){
                        lba = 0;
                        iwlba =wbfs_ntohs(d->header->wlba_table[++wlba]);
                        if(unlikely(iwlba==0))
                                return 1;
                }
        }
        if(unlikely(len)){
                err = p->read_hdsector(p->callback_data,
                                 p->part_lba + (iwlba<<iwlba_shift) + lba, 1, p->tmp_buffer);
                if(err)
                        return err;
                wbfs_memcpy(ptr, p->tmp_buffer, len);
        }     
        return 0;
}

// disc listing
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
										u8 *header = wbfs_ioalloc(p->disc_info_sz);
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

static void load_freeblocks(wbfs_t*p)
{
        if(p->freeblks)
                return;
        // XXX should handle malloc error..
        p->freeblks = wbfs_ioalloc(ALIGN_LBA(p->n_wbfs_sec/8));
        p->read_hdsector(p->callback_data,p->part_lba+p->freeblks_lba,ALIGN_LBA(p->n_wbfs_sec/8)>>p->hd_sec_sz_s, p->freeblks);
        
}
u32 wbfs_count_usedblocks(wbfs_t*p)
{
        u32 i,j,count=0;
        load_freeblocks(p);
        for(i=0;i<p->n_wbfs_sec/(8*4);i++)
        {
                u32 v = wbfs_ntohl(p->freeblks[i]);
                if(v == ~0U)
                     count+=32;
                else if(v!=0)
                        for(j=0;j<32;j++)
                                if (v & (1<<j))
                                        count++;
        }
        return count;
}


// write access


static int block_used(u8 *used,u32 i,u32 wblk_sz)
{
        u32 k;
        i*=wblk_sz;
        for(k=0;k<wblk_sz;k++)
                if(i+k<143432*2 && used[i+k])
                        return 1;
        return 0;
}

static u32 alloc_block(wbfs_t*p)
{
        u32 i,j;
        for(i=0;i<p->n_wbfs_sec/(8*4);i++)
        {
                u32 v = wbfs_ntohl(p->freeblks[i]);
                if(v != 0)
                {
                        for(j=0;j<32;j++)
                                if (v & (1<<j))
                                {
                                        p->freeblks[i] = wbfs_htonl(v & ~(1<<j));
                                        return (i*32)+j+1;
                                }
                }
        }
        return ~0;
}


static void free_block(wbfs_t *p,int bl)
{
  
        int i = (bl-1)/(32);
        int j = (bl-1)&31;
        
        u32 v = wbfs_ntohl(p->freeblks[i]);
        p->freeblks[i] = wbfs_htonl(v | 1<<j);
}

u32 wbfs_add_disc
	(
		wbfs_t *p,
		read_wiidisc_callback_t read_src_wii_disc,
		void *callback_data,
		progress_callback_t spinner,
		partition_selector_t sel,
		int copy_1_1
	)
{
	int i, discn;
	u32 tot, cur;
	u32 wii_sec_per_wbfs_sect = 1 << (p->wbfs_sec_sz_s-p->wii_sec_sz_s);
	wiidisc_t *d = 0;
	u8 *used = 0;
	wbfs_disc_info_t *info = 0;
	u8* copy_buffer = 0;
	u8 *b;
	int disc_info_sz_lba;
	used = wbfs_malloc(p->n_wii_sec_per_disc);
	
	if (!used)
	{
		ERR("unable to alloc memory");
	}
	
	if (!copy_1_1)
	{
		d = wd_open_disc(read_src_wii_disc, callback_data);
		if(!d)
		{
			ERR("unable to open wii disc");
		}
		wd_build_disc_usage(d, sel, used);
		wd_close_disc(d);
		d = 0;
	}
	
	for (i = 0; i < p->max_disc; i++) // find a free slot.
	{
		if (p->head->disc_table[i] == 0)
		{
			break;
		}
	}
	
	if (i == p->max_disc)
	{
		ERR("no space left on device (table full)");
	}
	
	p->head->disc_table[i] = 1;
	discn = i;
	load_freeblocks(p);
	
	// build disc info
	info = wbfs_ioalloc(p->disc_info_sz);
	b = (u8 *)info;
	read_src_wii_disc(callback_data, 0, 0x100, info->disc_header_copy);
	fprintf(stderr, "adding %c%c%c%c%c%c %s...\n",b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20);
	
	copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
	if (!copy_buffer)
	{
		ERR("alloc memory");
	}
	
	tot = 0;
	cur = 0;
	
	if (spinner)
	{
		// count total number to write for spinner
		for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
		{
			if (copy_1_1 || block_used(used, i, wii_sec_per_wbfs_sect))
			{
				tot++;
				spinner(0, tot);
			}
		}
	}
	
	for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
	{
		u16 bl = 0;
		if (copy_1_1 || block_used(used, i, wii_sec_per_wbfs_sect))
		{
			bl = alloc_block(p);
			if (bl == 0xffff)
			{
				ERR("no space left on device (disc full)");
			}

			read_src_wii_disc(callback_data, i * (p->wbfs_sec_sz >> 2), p->wbfs_sec_sz, copy_buffer);
			
			// fix the partition table.
			if (i == (0x40000 >> p->wbfs_sec_sz_s))
			{
				wd_fix_partition_table(d, sel, copy_buffer + (0x40000 & (p->wbfs_sec_sz - 1)));
			}
			
			p->write_hdsector(p->callback_data, p->part_lba + bl * (p->wbfs_sec_sz / p->hd_sec_sz), p->wbfs_sec_sz / p->hd_sec_sz, copy_buffer);
			
			if (spinner)
			{
				cur++;
				spinner(cur, tot);
			}
		}

		info->wlba_table[i] = wbfs_htons(bl);
	}
	
	// write disc info
	disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
	p->write_hdsector(p->callback_data, p->part_lba + 1 + discn * disc_info_sz_lba,disc_info_sz_lba, info);
	wbfs_sync(p);

error:
	set_ciso_mode(NULL, 0);
	if(d)
		wd_close_disc(d);
	if(used)
		wbfs_free(used);
	if(info)
		wbfs_iofree(info);
	if(copy_buffer)
		wbfs_iofree(copy_buffer);
	
	// init with all free blocks
	return 0;
}

u32 wbfs_add_cfg
	(
		wbfs_t *p
	)
{
	int i, discn;
	u32 tot, cur;

	wiidisc_t *d = 0;
	u8 *used = 0;
	wbfs_disc_info_t *info = 0;
	u8* copy_buffer = 0;
	u8 *b;
	int disc_info_sz_lba;

	set_ciso_mode(NULL, 0);

	used = wbfs_malloc(p->n_wii_sec_per_disc);
	
	if (!used)
	{
		ERR("unable to alloc memory");
	}
	
	
	wbfs_memset(used,0,p->n_wii_sec_per_disc);
	
	
	for (i = 0; i < p->max_disc; i++) // find a free slot.
	{
		if (p->head->disc_table[i] == 0)
		{
			break;
		}
	}
	
	if (i == p->max_disc)
	{
		ERR("no space left on device (table full)");
	}
	
	p->head->disc_table[i] = 1;
	discn = i;
	load_freeblocks(p);
	
	// build disc info
	info = wbfs_ioalloc(p->disc_info_sz);
	b = (u8 *)info;
	//read_src_wii_disc(callback_data, 0, 0x100, info->disc_header_copy);
    wbfs_memset(info->disc_header_copy,0,0x100);
	{
	char *p=(char *) info->disc_header_copy;
	wbfs_memcpy(p,"__CFG_",6);
	
	p[0x18]=0x5D;p[0x19]=0x1C;p[0x1A]=0x9E;p[0x1B]=0xA3;

	wbfs_memcpy(p+0x20,"uLoader CFG Entry",17);
	}
	fprintf(stderr, "adding %c%c%c%c%c%c %s...\n",b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20);
	
	copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
	if (!copy_buffer)
	{
		ERR("alloc memory");
	}
	
	tot = 0;
	cur = 0;
	
	
	for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
	{
		u16 bl = 0;
		
		if(i==0) 
			{
			bl = alloc_block(p); // only one block
				if (bl == 0xffff)
				{
				ERR("no space left on device (disc full)");
				}

			wbfs_memset(copy_buffer, 0, p->wbfs_sec_sz);
			
			p->write_hdsector(p->callback_data, p->part_lba + bl * (p->wbfs_sec_sz / p->hd_sec_sz), p->wbfs_sec_sz / p->hd_sec_sz, copy_buffer);
			}
		

		info->wlba_table[i] = wbfs_htons(bl);
	}
	
	// write disc info
	disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
	p->write_hdsector(p->callback_data, p->part_lba + 1 + discn * disc_info_sz_lba,disc_info_sz_lba, info);
        
	wbfs_sync(p);

error:
	set_ciso_mode(NULL, 0);
	if(d)
		wd_close_disc(d);
	if(used)
		wbfs_free(used);
	if(info)
		wbfs_iofree(info);
	if(copy_buffer)
		wbfs_iofree(copy_buffer);
	
	// init with all free blocks
	return 0;
}

u32 wbfs_estimate_disc
	(
		wbfs_t *p, read_wiidisc_callback_t read_src_wii_disc,
		void *callback_data,
		partition_selector_t sel
	)
{
	u8 *b;

	int i;
	u32 tot;
	u32 wii_sec_per_wbfs_sect = 1 << (p->wbfs_sec_sz_s-p->wii_sec_sz_s);
	wiidisc_t *d = 0;
	u8 *used = 0;
	wbfs_disc_info_t *info = 0;
	
	tot = 0;
	
	used = wbfs_malloc(p->n_wii_sec_per_disc);
	if (!used)
	{
		ERR("unable to alloc memory");
	}
	
	d = wd_open_disc(read_src_wii_disc, callback_data);
	if (!d)
	{
		ERR("unable to open wii disc");
	}
	
	wd_build_disc_usage(d,sel,used);
	wd_close_disc(d);
	d = 0;
	
	info = wbfs_ioalloc(p->disc_info_sz);
	b = (u8 *)info;
	read_src_wii_disc(callback_data, 0, 0x100, info->disc_header_copy);
	
	fprintf(stderr, "estimating %c%c%c%c%c%c %s...\n",b[0], b[1], b[2], b[3], b[4], b[5], b + 0x20);
	
	for (i = 0; i < p->n_wbfs_sec_per_disc; i++)
	{
		if (block_used(used, i, wii_sec_per_wbfs_sect))
		{
			tot++;
		}
	}

	if (d)
		wd_close_disc(d);
	
	if (used)
		wbfs_free(used);
	
	if (info)
		wbfs_iofree(info);
	
	return tot * ((p->wbfs_sec_sz / p->hd_sec_sz));
error:
	if (d)
		wd_close_disc(d);
	
	if (used)
		wbfs_free(used);
	
	if (info)
		wbfs_iofree(info);
	
	return 0xffffffff;
}

u32 wbfs_rm_disc(wbfs_t*p, u8* discid)
{
        wbfs_disc_t *d = wbfs_open_disc(p,discid);
        int i;
        int discn = 0;
        int disc_info_sz_lba = p->disc_info_sz>>p->hd_sec_sz_s;
        if(!d)
                return 1;
        load_freeblocks(p);
        discn = d->i;
        for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                        free_block(p,iwlba);
        }
        memset(d->header,0,p->disc_info_sz);
        p->write_hdsector(p->callback_data,p->part_lba+1+discn*disc_info_sz_lba,disc_info_sz_lba,d->header);
        p->head->disc_table[discn] = 0;
        wbfs_close_disc(d);
        wbfs_sync(p);
        return 0;
}

/* trim the file-system to its minimum size
 */
u32 wbfs_trim(wbfs_t*p)
{
        u32 maxbl;
        load_freeblocks(p);
        maxbl = alloc_block(p);
        p->n_hd_sec = maxbl<<(p->wbfs_sec_sz_s-p->hd_sec_sz_s);
        p->head->n_hd_sec = wbfs_htonl(p->n_hd_sec);
        // make all block full
        memset(p->freeblks,0,p->n_wbfs_sec/8);
        wbfs_sync(p);
        // os layer will truncate the file.
        return maxbl;
}


u32 wbfs_add_png(wbfs_disc_t*d, char *png)
{
        wbfs_t *p = d->p;
        u8* copy_buffer = 0;
        int i;
		int len;
        int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;


		FILE *fp=fopen(png,"rb");
		u8 * mem_png;
		if(!fp)
		{
			  ERR("Error Opening .png");
		}
        fseek(fp, 0, SEEK_END);
		len=ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if(len>(200*1024-8))
			{
				fclose(fp);
				ERR(".png too big (must be < 200KB)");
			}

		mem_png= wbfs_ioalloc(len);
		if(!mem_png)
			{
				fclose(fp);
				ERR("alloc memory");
			}
		fread(mem_png, 1, len, fp);
        fclose(fp);

        copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
        if(!copy_buffer)
				{
				wbfs_iofree(mem_png);
                ERR("alloc memory");
				}

        for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {
				
				if(i==0)
					{
					p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);

					copy_buffer[1024]='H';copy_buffer[1025]='D';copy_buffer[1026]='R';
					copy_buffer[1027]=((len+8+1023)/1024)-1;
					memcpy(&copy_buffer[1024+8], mem_png, len);

					p->write_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);
					printf(".png inserted\n");
					break;
					}
                
                }
	
				
				
        }
		wbfs_iofree(mem_png);
        wbfs_iofree(copy_buffer);
        return 0;
error:
        return 1;
}


u32 wbfs_remove_cfg(wbfs_disc_t*d)
{
        wbfs_t *p = d->p;
        u8* copy_buffer = 0;
        int i;
	
        int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;
 

        copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
        if(!copy_buffer)
				{
                ERR("alloc memory");
				}

        for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {
				
				if(i==0)
					{
					p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);

					if(copy_buffer[1024]=='H' && copy_buffer[1025]=='D' && copy_buffer[1026]=='R')
						{
						memset(&copy_buffer[1024+8], 0, copy_buffer[1027]*1024);
						memset(&copy_buffer[1024],0,8);

						p->write_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);
					
						printf("game CFG deleted\n");
						}

					break;
					}
                
                }
	
				
				
        }
		
        wbfs_iofree(copy_buffer);
        return 0;
error:
        return 1;
}

// data extraction ISO
u32 wbfs_extract_disc(wbfs_disc_t*d, rw_sector_callback_t write_dst_wii_sector,void *callback_data,progress_callback_t spinner)
{
        wbfs_t *p = d->p;
        u8* copy_buffer = 0;
        int i;
		u32 tot, cur;
        int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;
        int dst_wbs_nlb=p->wbfs_sec_sz/p->wii_sec_sz;

        copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
        if(!copy_buffer)
                ERR("alloc memory");


        cur=tot=0;

		for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {
				tot++;
				}
		}

        for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {
                        
                        if(spinner)
                                spinner(cur, tot);
                        p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);
                        write_dst_wii_sector(callback_data, i*dst_wbs_nlb, dst_wbs_nlb, copy_buffer);
						cur++;
				
                }
				
				
        }

        wbfs_iofree(copy_buffer);
        return 0;
error:
        return 1;
}
// data extraction CISO
u32 wbfs_extract_disc2(wbfs_disc_t*d, rw_sector_callback_t write_dst_wii_sector,void *callback_data,progress_callback_t spinner)
{
        wbfs_t *p = d->p;
        u8* copy_buffer = 0;
        int i;
		u32 tot, cur;
        int src_wbs_nlb=p->wbfs_sec_sz/p->hd_sec_sz;
        int dst_wbs_nlb=p->wbfs_sec_sz/p->wii_sec_sz;

		u32 offset=0;



        copy_buffer = wbfs_ioalloc(p->wbfs_sec_sz);
        if(!copy_buffer)
                ERR("alloc memory");

		memset(copy_buffer,0,0x8000);
        copy_buffer[0]='C';
		copy_buffer[1]='I';
		copy_buffer[2]='S';
		copy_buffer[3]='O';
		
		i=32768*dst_wbs_nlb;

		copy_buffer[4]=i & 255;
		copy_buffer[5]=(i>>8) & 255;
		copy_buffer[6]=(i>>16) & 255;
		copy_buffer[7]=(i>>24) & 255;

		cur=tot=0;

		for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba) {copy_buffer[i+8]=1;tot++;}
				else copy_buffer[i+8]=0;
					
		}
		
		write_dst_wii_sector(callback_data, 0, 1, copy_buffer);
		
		offset=1;
        for( i=0; i< p->n_wbfs_sec_per_disc; i++)
        {
                u32 iwlba = wbfs_ntohs(d->header->wlba_table[i]);
                if (iwlba)
                {   
						 if(spinner)
                                spinner(cur, tot);
                        p->read_hdsector(p->callback_data, p->part_lba + iwlba*src_wbs_nlb, src_wbs_nlb, copy_buffer);
                        write_dst_wii_sector(callback_data, offset, dst_wbs_nlb, copy_buffer);
						cur++;

				offset+=dst_wbs_nlb;
				
                }
				
				
        }


        wbfs_iofree(copy_buffer);
        return 0;
error:
        return 1;
}
u32 wbfs_extract_file(wbfs_disc_t*d, char *path);
