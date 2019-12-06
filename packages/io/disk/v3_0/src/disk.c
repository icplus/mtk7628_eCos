//==========================================================================
//
//      io/disk/disk.c
//
//      High level disk driver
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2003, 2004, 2005, 2006 Free Software Foundation, Inc.      
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):     savin 
// Date:          2003-06-10
// Purpose:       Top level disk driver
// Description: 
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/io.h>
#include <pkgconf/io_disk.h>

#include <cyg/io/io.h>
#include <cyg/io/devtab.h>
#include <cyg/io/disk.h>
#include <cyg/infra/cyg_ass.h>      // assertion support
#include <cyg/infra/diag.h>         // diagnostic output

// ---------------------------------------------------------------------------

#ifdef CYGDBG_IO_DISK_DEBUG
#define DEBUG 0
#endif

#ifdef DEBUG
# define D(_args_) diag_printf _args_
#else
# define D(_args_)
#endif

// ---------------------------------------------------------------------------

// Master Boot Record defines
#define MBR_SIG_ADDR  0x1FE   // signature address 
#define MBR_SIG_BYTE0 0x55    // signature first byte value
#define MBR_SIG_BYTE1 0xAA    // signature second byte value
#define MBR_PART_ADDR 0x1BE   // first partition address
#define MBR_PART_SIZE 0x10    // partition size
#define MBR_PART_NUM  4       // number of partitions
#define EBR 1
// Get cylinders, heads and sectors from data (MBR partition format)
#define READ_CHS(_data_, _c_, _h_, _s_)                     \
    do {                                                    \
        _h_ = (*((cyg_uint8 *)_data_));                     \
        _s_ = (*(((cyg_uint8 *)_data_)+1) &  0x3F);         \
        _c_ = (*(((cyg_uint8 *)_data_)+1) & ~0x3F) << 2 |   \
              (*(((cyg_uint8 *)_data_)+2));                 \
    } while (0)

// Get double word from data (MBR partition format)
#define READ_DWORD(_data_, _val_)                           \
    do {                                                    \
        _val_ = *((cyg_uint8 *)_data_)           |          \
                *(((cyg_uint8 *)_data_)+1) << 8  |          \
                *(((cyg_uint8 *)_data_)+2) << 16 |          \
                *(((cyg_uint8 *)_data_)+3) << 24;           \
    } while (0)

// Convert cylinders, heads and sectors to LBA sectors 
#define CHS_TO_LBA(_info_, _c_, _h_, _s_, _lba_) \
    (_lba_=(((_c_)*(_info_)->heads_num+(_h_))*(_info_)->sectors_num)+(_s_)-1)

// ---------------------------------------------------------------------------

static Cyg_ErrNo disk_bread(cyg_io_handle_t  handle, 
                            void            *buf, 
                            cyg_uint32      *len, 
                            cyg_uint64       pos);

static Cyg_ErrNo disk_bwrite(cyg_io_handle_t  handle, 
                             const void      *buf, 
                             cyg_uint32      *len, 
                             cyg_uint64       pos);

static Cyg_ErrNo disk_select(cyg_io_handle_t handle, 
                             cyg_uint32      which, 
                             CYG_ADDRWORD    info);

static Cyg_ErrNo disk_get_config(cyg_io_handle_t  handle, 
                                 cyg_uint32       key, 
                                 void            *buf, 
                                 cyg_uint32      *len);

static Cyg_ErrNo disk_set_config(cyg_io_handle_t  handle, 
                                 cyg_uint32       key, 
                                 const void      *buf, 
                                 cyg_uint32      *len);

BLOCK_DEVIO_TABLE(cyg_io_disk_devio,
                  disk_bwrite,
                  disk_bread,
                  disk_select,
                  disk_get_config,
                  disk_set_config
);

static cyg_bool disk_init(struct cyg_devtab_entry *tab);

static Cyg_ErrNo disk_connected(struct cyg_devtab_entry *tab,
                                cyg_disk_identify_t     *ident);

static Cyg_ErrNo disk_disconnected(struct disk_channel *chan);

static Cyg_ErrNo disk_lookup(struct cyg_devtab_entry **tab,
                             struct cyg_devtab_entry  *sub_tab,
                             const char               *name);

static void disk_transfer_done(struct disk_channel *chan, Cyg_ErrNo res);

DISK_CALLBACKS(cyg_io_disk_callbacks, 
               disk_init,
               disk_connected,
               disk_disconnected,
               disk_lookup,
               disk_transfer_done
); 

// ---------------------------------------------------------------------------
//
// Read partition from data
// 

static void 
read_partition(cyg_uint8            *data,
               cyg_disk_info_t      *info,
               cyg_disk_partition_t *part)
{
    cyg_disk_identify_t *ident = &info->ident;
    cyg_uint16 c, h, s;
    cyg_uint32 start, end, size;

#ifdef DEBUG
    diag_printf("Partition data:\n");
    diag_dump_buf( data, 16 );
    diag_printf("Disk geometry: %d/%d/%d\n",info->ident.cylinders_num,
                info->ident.heads_num, info->ident.sectors_num );
#endif
    
    // Retrieve basic information
    part->type  = data[4];
    part->state = data[0];
    READ_DWORD(&data[12], part->size);

    READ_DWORD(&data[8], start);        
    READ_DWORD(&data[12], size);

    // Use the LBA start and size fields if they are valid. Otherwise
    // fall back to CHS.
    
    if( start > 0 && size > 0 )
    {
        READ_DWORD(&data[8], start);    
        end = start + size - 1;

#ifdef DEBUG
        diag_printf("Using LBA partition parameters\n");
        diag_printf("      LBA start %d\n",start);
        diag_printf("      LBA size  %d\n",size);
        diag_printf("      LBA end   %d\n",end);
#endif
        
    }
    else
    {
        READ_CHS(&data[1], c, h, s);
        CHS_TO_LBA(ident, c, h, s, start);
#ifdef DEBUG
        diag_printf("Using CHS partition parameters\n");
        diag_printf("      CHS start %d/%d/%d => %d\n",c,h,s,start);
#endif
    
        READ_CHS(&data[5], c, h, s);
        CHS_TO_LBA(ident, c, h, s, end);
#ifdef DEBUG
        diag_printf("      CHS end %d/%d/%d => %d\n",c,h,s,end);
        diag_printf("      CHS size %d\n",size);
#endif

    }

    part->size = size;
    part->start = start;
    part->end = end;
}
#if EBR
static Cyg_ErrNo 
read_ebr(disk_channel *chan,cyg_disk_partition_t *part)
{
    cyg_disk_info_t *info = chan->info;
    disk_funs       *funs = chan->funs;
    disk_controller *ctlr = chan->controller;
    cyg_uint8 *buf = (cyg_uint8*)malloc(info->block_size);
    Cyg_ErrNo res = ENOERR;
    cyg_uint32 start=0;
    int i,index = 5;
    cyg_disk_partition_t *oldpart,*newpart=part;
    D(("read EBR\n"));

	if (buf == NULL)
		{
		diag_printf("%s:%d malloc fail\n",__FUNCTION__,__LINE__);
		return -1;
		}
    while(1)
    {
        cyg_drv_mutex_lock( &ctlr->lock );
        while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );
        ctlr->busy = true;
        ctlr->result = -EWOULDBLOCK;
        res = (funs->read)(chan, (void *)buf, 1, newpart->start);
        if( res == -EWOULDBLOCK )
        {
            // If the driver replys EWOULDBLOCK, then the transfer is
            // being handled asynchronously and when it is finished it
            // will call disk_transfer_done(). This will wake us up here
            // to continue.
            while( ctlr->result == -EWOULDBLOCK )
            cyg_drv_cond_wait( &ctlr->async );
            res = ctlr->result;
        } 
        ctlr->busy = false;
        cyg_drv_mutex_unlock( &ctlr->lock );
        if (ENOERR != res)
            return res;
#ifdef DEBUG
        diag_dump_buf_with_offset( buf, 512, buf );
#endif
        if (MBR_SIG_BYTE0 == buf[MBR_SIG_ADDR+0] && MBR_SIG_BYTE1 == buf[MBR_SIG_ADDR+1])
        {
            D(("disk EBR found\n")); 
            start = newpart->start;       
            read_partition(&buf[MBR_PART_ADDR+MBR_PART_SIZE*0], info, newpart);
            //read_partition(&buftest[MBR_PART_SIZE*0], info, newpart);
            newpart->start += start;
            newpart->end += start;
#ifdef DEBUG
            if (0x00 != part->type)
            {
               // D(("\ndisk MBR partition %d:\n", i));
                D(("      type  = %02X\n", newpart->type));
                D(("      state = %02X\n", newpart->state));
                D(("      start = %d\n",   newpart->start));
                D(("      end   = %d\n",   newpart->end));
                D(("      size  = %d\n\n", newpart->size));
            }
#endif
            if(0x00 != buf[MBR_PART_ADDR+MBR_PART_SIZE+4])
            //if(0x00 != buftest[MBR_PART_SIZE*1+4])
            {    
                oldpart = newpart;
                newpart = (cyg_disk_partition_t *)malloc(sizeof(struct cyg_disk_partition_t));
                memset(newpart,0,sizeof(struct cyg_disk_partition_t));
                oldpart->pnext = newpart;
                newpart->index = index;
                index++;
                info->partitions_num++;
                newpart->pnext = NULL;
                read_partition(&buf[MBR_PART_ADDR+MBR_PART_SIZE], info, newpart);
                newpart->start += start;
                newpart->end += start;
            }
            else
                break;
        }
        else break;
    }
    free(buf);
    return res;
}
#endif
// ---------------------------------------------------------------------------
//
// Read Master Boot Record (partitions)
//
static Cyg_ErrNo 
read_mbr(disk_channel *chan)
{
    cyg_disk_info_t *info = chan->info;
    disk_funs       *funs = chan->funs;
    disk_controller *ctlr = chan->controller;
    cyg_uint8 *buf = (cyg_uint8*)malloc(info->block_size);
    Cyg_ErrNo res = ENOERR;
    int i;

    D(("read MBR\n"));

	if (buf == NULL)
		{
		diag_printf("%s:%d malloc fail\n",__FUNCTION__,__LINE__);
		return -1;
		}
    for (i = 0; i < info->partitions_num; i++)
        info->partitions[i].type = 0x00;  


    
    cyg_drv_mutex_lock( &ctlr->lock );

    while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );

    ctlr->busy = true;
    
    ctlr->result = -EWOULDBLOCK;

    for( i = 0; i < sizeof(buf); i++ )
        buf[i] = 0;
    //diag_printf("buf = %p\n",buf);
    res = (funs->read)(chan, (void *)buf, 1, 0);
    
    if( res == -EWOULDBLOCK )
    {
        // If the driver replys EWOULDBLOCK, then the transfer is
        // being handled asynchronously and when it is finished it
        // will call disk_transfer_done(). This will wake us up here
        // to continue.

        while( ctlr->result == -EWOULDBLOCK )
            cyg_drv_cond_wait( &ctlr->async );

        res = ctlr->result;
    }
        
    ctlr->busy = false;
    
    cyg_drv_mutex_unlock( &ctlr->lock );

    if (ENOERR != res)
        return res;

#ifdef DEBUG
    diag_dump_buf_with_offset( buf, 512, buf );
#endif
    //for test
    //buf[MBR_SIG_ADDR+0]=0x55;buf[MBR_SIG_ADDR+1]=0xAA;
    if (MBR_SIG_BYTE0 == buf[MBR_SIG_ADDR+0] && MBR_SIG_BYTE1 == buf[MBR_SIG_ADDR+1])
    {
        int npart;

        D(("disk MBR found\n")); 
 
        npart = info->partitions_num < MBR_PART_NUM ? 
            info->partitions_num : MBR_PART_NUM;

        for (i = 0; i < MBR_PART_NUM; i++)
        {
            cyg_disk_partition_t *part = &info->partitions[i];
            part->index = i+1;
            
            read_partition(&buf[MBR_PART_ADDR+MBR_PART_SIZE*i], info, part);
            //read_partition(&buftest[MBR_PART_SIZE*i], info, part);
#ifdef DEBUG
            if (0x00 != part->type)
            {
                D(("\ndisk MBR partition %d:\n", i));
                D(("      type  = %02X\n", part->type));
                D(("      state = %02X\n", part->state));
                D(("      start = %d\n",   part->start));
                D(("      end   = %d\n",   part->end));
                D(("      size  = %d\n\n", part->size));
            }
#endif    
#if EBR
            if(part->type == 0x05 || part->type == 0x0F)
            read_ebr(chan,part);
#endif
        } 
    }
    free(buf);
    return ENOERR;
}

// ---------------------------------------------------------------------------

static cyg_bool 
disk_init(struct cyg_devtab_entry *tab)
{
    disk_channel    *chan = (disk_channel *) tab->priv;
    cyg_disk_info_t *info = chan->info;
    int i;
    //diag_printf("file test:disk_init\n");
    if (!chan->init)
    {
        disk_controller *controller = chan->controller;
        
        if( !controller->init )
        {
            cyg_drv_mutex_init( &controller->lock );
            cyg_drv_cond_init( &controller->queue, &controller->lock );
            cyg_drv_cond_init( &controller->async, &controller->lock );
            
            controller->init = true;
        }
        
        info->connected = false;
        
        // clear partition data
        for (i = 0; i < info->partitions_num; i++)
            info->partitions[i].type = 0x00;
            //info->partitions.type = 0x00;

        chan->init = true;
    }
    //diag_printf("file test:disk_init end\n");
    return true;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo
disk_connected(struct cyg_devtab_entry *tab,
               cyg_disk_identify_t     *ident)
{
    disk_channel    *chan = (disk_channel *) tab->priv;
    cyg_disk_info_t *info = chan->info;
    Cyg_ErrNo res = ENOERR;
     //diag_printf("file test:disk_connected\n");
    if (!chan->init)
        {D(("chan->init is false\n"));return -EINVAL;}

    // If the device is already connected, nothing more to do
    if( info->connected )
        {D(("info->connected is true\n"));return ENOERR;}

    // If any of these assertions fire, it is probable that the
    // hardware driver has not been updated to match the current disk
    // API.
    CYG_ASSERT( ident->lba_sectors_num > 0, "Bad LBA sector count" );
    CYG_ASSERT( ident->phys_block_size > 0, "Bad physical block size");
    CYG_ASSERT( ident->max_transfer > 0, "Bad max transfer size");
    
    info->ident      = *ident;
    info->block_size = ident->phys_block_size;
    info->blocks_num = ident->lba_sectors_num;
    info->phys_block_size = ident->phys_block_size;
    D(("disk connected\n")); 
    //D(("    serial            = '%s'\n", ident->serial)); 
    //D(("    firmware rev      = '%s'\n", ident->firmware_rev)); 
    //D(("    model num         = '%s'\n", ident->model_num)); 
    D(("    block_size        = %d\n",   info->block_size));
    D(("    blocks_num        = %u\n",   info->blocks_num));
    D(("    phys_block_size   = %d\n",   info->phys_block_size));
    
    if (chan->mbr_support)
    {    
        // read disk master boot record
        res = read_mbr(chan);
    }

    if (ENOERR == res)
    {    
        // now declare that we are connected
        info->connected = true;
        chan->valid     = true; 
    }
    D(("file test:disk_connected end\n"));
    return res;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo
disk_disconnected(disk_channel *chan)
{
    cyg_disk_info_t *info = chan->info;
    int i;

    if (!chan->init)
        return -EINVAL;

    info->connected = false;
    chan->valid     = false;
     
    // clear partition data and invalidate partition devices 
    for (i = 0; i < info->partitions_num; i++)
    {
        info->partitions[i].type  = 0x00;
        chan->pdevs_chan->valid = false;
    }

    
    D(("disk disconnected\n")); 

    return ENOERR;    
}
    
// ---------------------------------------------------------------------------

static Cyg_ErrNo
disk_lookup(struct cyg_devtab_entry **tab,
            struct cyg_devtab_entry  *sub_tab,
            const char *name)
{
    disk_channel    *chan = (disk_channel *) (*tab)->priv;
    cyg_disk_info_t *info = chan->info;
    struct cyg_devtab_entry *new_tab;
    disk_channel            *new_chan;
    int dev_num;
    
    if (!info->connected)
        return -EINVAL;

    dev_num = 0;

    while ('\0' != *name)
    {
        if (*name < '0' || *name > '9')
            return -EINVAL;

        dev_num = 10 * dev_num + (*name - '0');
        name++;
    }
   
    if (dev_num > info->partitions_num)
        return -EINVAL;

    D(("disk lookup dev number = %d\n", dev_num)); 

    if (0 == dev_num)
    {
        // 0 is the root device number
        return ENOERR; 
    }
    if(dev_num <= 4 )
    {
        if (0x00 == info->partitions[dev_num-1].type)
        {
            D(("disk NO partition for dev\n")); 
            return -EINVAL;
        }
        /*if (0x0B == info->partitions[dev_num-1].type || 0x0C == info->partitions[dev_num-1].type)
        {
            chan->partition = &info->partitions[dev_num-1];
        }   
        else
        {
            D(("disk file system is not FAT32\n")); 
            return -EINVAL;
        }*/
        chan->partition = &info->partitions[dev_num-1];
   // new_tab  = &chan->pdevs_dev[dev_num-1];
    //new_chan = &chan->pdevs_chan[dev_num-1];
    
    // copy device data from parent
    //*new_tab  = **tab; 
    //*new_chan = *chan;

    //new_tab->priv = (void *)new_chan;

    // set partition ptr
    //new_chan->partition = &info->partitions[dev_num-1];
    }
    else
    {    
        int i;
        cyg_disk_partition_t *part;
        for(i= 0;i < 4;i++)
        {
            part = info->partitions[i].pnext;
            while(part)
            {
                D(("i= %d exten partion\n",i));
                if(part->index == dev_num)
                {

                    //if (0x0B == part->type || 0x0C == part->type)
                    {chan->partition = part;return ENOERR;}
                }
                part = part->pnext;
            }
        }
        return -EINVAL;
    }
        
    return ENOERR;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo 
disk_bread(cyg_io_handle_t  handle, 
           void            *buf, 
           cyg_uint32      *len,  // In blocks
           cyg_uint64       pos)  // In blocks
{
    cyg_devtab_entry_t *t    = (cyg_devtab_entry_t *) handle;
    disk_channel       *chan = (disk_channel *) t->priv;
    disk_controller    *ctlr = chan->controller;
    disk_funs          *funs = chan->funs;
    cyg_disk_info_t    *info = chan->info;
    cyg_uint64  size = *len;
    cyg_uint8  *bbuf = (cyg_uint8  *)buf;
    Cyg_ErrNo   res  = ENOERR;
    cyg_uint64  last;
    
    //cyg_drv_mutex_lock( &ctlr->lock );

    /*while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );*/

    if (info->connected && chan->valid)
    {
        //ctlr->busy = true;
    
        if (NULL != chan->partition)
        {
            pos += chan->partition->start;
            last = chan->partition->end;
        }
        else
        {
            last = info->blocks_num-1;
        }
#ifdef DEBUG
            if (0x00 != chan->partition->type)
            {
                D(("\ndisk MBR partition %d:\n", chan->partition->index));
                D(("      type  = %02X\n", chan->partition->type));
                D(("      state = %02X\n", chan->partition->state));
                D(("      start = %d\n",   chan->partition->start));
                D(("      end   = %d\n",   chan->partition->end));
                D(("      size  = %d\n\n", chan->partition->size));
            }
#endif
        //D(("disk read block=%d len=%d buf=%p\n", pos, *len, buf));

        while( size > 0 )
        {
            cyg_uint32 tfr = size;
            
            if (pos > last)
            {
                res = -EIO;
                break;
            }

            if( tfr > ((info->ident.max_transfer)/info->block_size) )
                tfr = (info->ident.max_transfer)/info->block_size;
            
            ctlr->result = -EWOULDBLOCK;

            //cyg_drv_dsr_lock();
            
            res = (funs->read)(chan, (void*)bbuf, tfr, pos);

            /*if( res == -EWOULDBLOCK )
            {
                // If the driver replys EWOULDBLOCK, then the transfer is
                // being handled asynchronously and when it is finished it
                // will call disk_transfer_done(). This will wake us up here
                // to continue.

                while( ctlr->result == -EWOULDBLOCK )
                    cyg_drv_cond_wait( &ctlr->async );

                res = ctlr->result;
            }

            cyg_drv_dsr_unlock();*/
            
            if (ENOERR != res)
                goto done;

            if (!info->connected)
            {
                res = -EINVAL;
                goto done;
            }

            bbuf        += tfr * info->block_size;
            pos         += tfr;
            size        -= tfr;
        }

        
    }
    else
        res = -EINVAL;

done:
    //ctlr->busy = false;
    //cyg_drv_cond_signal( &ctlr->queue );
    
    //cyg_drv_mutex_unlock( &ctlr->lock );
/*#ifdef CYGPKG_KERNEL
    cyg_thread_yield();
#endif*/
    
    *len -= size;
    return res;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo 
disk_bwrite(cyg_io_handle_t  handle, 
            const void      *buf, 
            cyg_uint32      *len,   // In blocks
            cyg_uint64       pos)   // In blocks
{
    cyg_devtab_entry_t *t    = (cyg_devtab_entry_t *) handle;
    disk_channel       *chan = (disk_channel *) t->priv;
    disk_controller    *ctlr = chan->controller;    
    disk_funs          *funs = chan->funs;
    cyg_disk_info_t    *info = chan->info;
    cyg_uint64  size = *len;
    cyg_uint8  *bbuf = (cyg_uint8 * const) buf;
    Cyg_ErrNo   res  = ENOERR;
    cyg_uint64  last;

    //cyg_drv_mutex_lock( &ctlr->lock );
    /*while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );
    diag_printf("enter disk_bwrite 1\n");*/
    if (info->connected && chan->valid)
    {
        //ctlr->busy = true;
        
        if (NULL != chan->partition)
        {
            pos += chan->partition->start;
            last = chan->partition->end;
        }
        else
        {
            last = info->blocks_num-1;
        }
    
       // D(("disk write block=%d len=%d buf=%p\n", pos, *len, buf));
       

        while( size > 0 )
        {
            cyg_uint32 tfr = size;
        
            if (pos > last)
            {
                res = -EIO;
                {diag_printf("pos > last\n");goto done;}
            }

            if( tfr > ((info->ident.max_transfer)/info->block_size) )
                tfr = (info->ident.max_transfer)/info->block_size;

            ctlr->result = -EWOULDBLOCK;

            //cyg_drv_dsr_lock();
            res = (funs->write)(chan, (void*)bbuf, tfr, pos);

            /*if( res == -EWOULDBLOCK )
            {
                // If the driver replys EWOULDBLOCK, then the transfer is
                // being handled asynchronously and when it is finished it
                // will call disk_transfer_done(). This will wake us up here
                // to continue.

                while( ctlr->result == -EWOULDBLOCK )
                    cyg_drv_cond_wait( &ctlr->async );

                res = ctlr->result;
            }*/

            //cyg_drv_dsr_unlock();
            
            if (ENOERR != res)
                {diag_printf("cyg_drv_dsr_unlock \n");goto done;}
 
            if (!info->connected)
            {
                res = -EINVAL;
                {diag_printf("!info->connected \n");goto done;}
            }

            bbuf        += tfr * info->block_size;
            pos         += tfr;
            size        -= tfr;
            
        }
 
        
    }
    else
        res = -EINVAL;

done:
    //ctlr->busy = false;
    //cyg_drv_cond_signal( &ctlr->queue );
    
    //cyg_drv_mutex_unlock( &ctlr->lock );
/*#ifdef CYGPKG_KERNEL    
    cyg_thread_yield();
#endif*/
    
    *len -= size;
    return res;
}

// ---------------------------------------------------------------------------

static void disk_transfer_done(struct disk_channel *chan, Cyg_ErrNo res)
{
    disk_controller    *ctlr = chan->controller;    

    ctlr->result = res;
    
    cyg_drv_cond_signal( &ctlr->async );
}

// ---------------------------------------------------------------------------

static cyg_bool
disk_select(cyg_io_handle_t handle, cyg_uint32 which, CYG_ADDRWORD info)
{
    cyg_devtab_entry_t *t     = (cyg_devtab_entry_t *) handle;
    disk_channel       *chan  = (disk_channel *) t->priv;
    cyg_disk_info_t    *cinfo = chan->info;
 
    if (!cinfo->connected || !chan->valid)
        return false;
    else
        return true;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo 
disk_get_config(cyg_io_handle_t  handle, 
                cyg_uint32       key, 
                void            *xbuf,
                cyg_uint32      *len)
{
    cyg_devtab_entry_t *t    = (cyg_devtab_entry_t *) handle;
    disk_channel       *chan = (disk_channel *) t->priv;
    disk_controller    *ctlr = chan->controller;    
    cyg_disk_info_t    *info = chan->info;
    cyg_disk_info_t    *buf  = (cyg_disk_info_t *) xbuf;
    disk_funs          *funs = chan->funs;
    Cyg_ErrNo res = ENOERR;
 
    cyg_drv_mutex_lock( &ctlr->lock );

    while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );

    if (info->connected && chan->valid)
    {
        ctlr->busy = true;
    
        D(("disk get config key=%d\n", key)); 
    
        switch (key) {
        case CYG_IO_GET_CONFIG_DISK_INFO:
            if (*len < sizeof(cyg_disk_info_t)) {
                res = -EINVAL;
                break;
            }
            D(("chan->info->block_size %u\n", chan->info->block_size ));
            D(("chan->info->blocks_num %u\n", chan->info->blocks_num ));
            D(("chan->info->phys_block_size %u\n", chan->info->phys_block_size ));
            *buf = *chan->info;
            *len = sizeof(cyg_disk_info_t);
            break;       

        default:
            // pass down to lower layers
            res = (funs->get_config)(chan, key, xbuf, len);
        }
        
        ctlr->busy = false;
        cyg_drv_cond_signal( &ctlr->queue );
    }
    else
        res = -EINVAL;

    cyg_drv_mutex_unlock( &ctlr->lock );    
    
    return res;
}

// ---------------------------------------------------------------------------

static Cyg_ErrNo 
disk_set_config(cyg_io_handle_t  handle, 
                cyg_uint32       key, 
                const void      *xbuf, 
                cyg_uint32      *len)
{
    cyg_devtab_entry_t *t    = (cyg_devtab_entry_t *) handle;
    disk_channel       *chan = (disk_channel *) t->priv;
    disk_controller    *ctlr = chan->controller;    
    cyg_disk_info_t    *info = chan->info;
    disk_funs          *funs = chan->funs;
    Cyg_ErrNo res = ENOERR;
    
    cyg_drv_mutex_lock( &ctlr->lock );

    while( ctlr->busy )
        cyg_drv_cond_wait( &ctlr->queue );

    if (info->connected && chan->valid)
    {
        ctlr->busy = true;
        
        D(("disk set config key=%d\n", key)); 

        switch ( key )
        {
        case CYG_IO_SET_CONFIG_DISK_MOUNT:
            chan->mounts++;
            info->mounts++;
            D(("disk mount: chan %d disk %d\n",chan->mounts, info->mounts));
            break;
            
        case CYG_IO_SET_CONFIG_DISK_UMOUNT:
            chan->mounts--;
            info->mounts--;
            D(("disk umount: chan %d disk %d\n",chan->mounts, info->mounts));            
            break;
            
        default:
            break;
        }
        
        // pass down to lower layers
        res = (funs->set_config)(chan, key, xbuf, len);
        
        ctlr->busy = false;
        cyg_drv_cond_signal( &ctlr->queue );
    }
    else
        res = -EINVAL;
    
    cyg_drv_mutex_unlock( &ctlr->lock );

    return res;
    
}

// ---------------------------------------------------------------------------
// EOF disk.c
