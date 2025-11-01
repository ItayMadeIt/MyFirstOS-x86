#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/drivers/io/io.h>
#include <drivers/pci.h>
#include <memory/heap/heap.h>
#include <arch/i386/memory/paging_utils.h>
#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pci/ide.h>
#include <kernel/devices/storage.h>
#include <kernel/drivers/pci_ops.h>
#include "core/num_defs.h"
#include <stdio.h>
#include <kernel/interrupts/irq.h>
#include <drivers/storage.h>

#define INVALID_ID (~0)

#define COMPABILITY_PRIMARY_PORT      0x1F0
#define COMPABILITY_PRIMARY_CTRL_PORT 0x3F6
#define COMPABILITY_PRIMARY_IRQ       14
#define COMPABILITY_SECOND_PORT       0x170
#define COMPABILITY_SECOND_CTRL_PORT  0x376
#define COMPABILITY_SECOND_IRQ        15

#define PCI_CMD_DMA_BIT (1 << 2)

#define REG_OFF_DATA       0x00
#define REG_OFF_ERROR      0x01
#define REG_OFF_FEATURES   0x01
#define REG_OFF_SECCOUNT0  0x02
#define REG_OFF_LBA0       0x03
#define REG_OFF_LBA1       0x04
#define REG_OFF_LBA2       0x05
#define REG_OFF_HDDEVSEL   0x06
#define REG_OFF_COMMAND    0x07
#define REG_OFF_STATUS     0x07

#define REG_CTRL_DEVICE 0x02

typedef struct ide_channel 
{
    u16 base;       // BAR0 or BAR2
    u16 ctrl;       // BAR1 or BAR3
    u32 bus_master; // BAR4 + 0x0 (primary) or +0x8 (secondary)
    bool has_dma;        // does this channel support DMA?
    bool dma_io;         // is DMA IO or MMIO? True - IO, MMIO - False
    u32 irq;        // irq number
    bool pic_enabled;    // enabled PIC on this channel
} ide_channel_t;

enum ide_size_types {
    IDE_SIZE_CHS = 0,
    IDE_SIZE_LBA28 = 1,
    IDE_SIZE_LBA48 = 2,
};

#define IDE_STR_INDEN_LENGTH 40
typedef struct ide_device 
{
    bool present;                           // did IDENTIFY succeed?
    bool slave;                             // 0 = master, 1 = slave
    u8 size_type;                      // 0 = chs (not supported), 1 = LBA28, 2 = LBA48
    union {
        struct {
            u16 cylinders;
            u8 heads;
            u8 sectors;
        } chs;
    };
    bool supports_dma;                      // supports DMA? Yes true, No false
    u8 channel;                        // 0 = primary, 1 = secondary
    u8 type;                           // ATA or ATAPI
    u16 signature;                     // Drive signature
    u16 features;                      // device capabilities
    u32 command_sets;                  // command sets supported
    u64 size;                          // in sectors 
    char model[IDE_STR_INDEN_LENGTH + 1];   // from IDENTIFY string

    u64 external_dev_id;
} ide_device_t;

typedef struct __attribute__((packed)) ide_prd_entry
{
    u32 phys_addr;
    u16 bytes_size;
    u16 flags; // reserved except MSB

} ide_prd_entry_t; 

#define PRD_ENTRIES_PER_CHANNEL 8

#define CHANNELS 2 // pri/sec
#define DEVS_PER_CHANNEL 2   // for each channel - master/slave 
#define DEVICES (CHANNELS * DEVS_PER_CHANNEL) 

typedef struct __attribute__((aligned(SECTOR_SIZE))) ide_dma_vars
{   
    ide_prd_entry_t* prdt;   // dynamic 
    u64 prdt_capacity;  // allocated (not used)
    
    // only needed for first entry if not aligned
    u16 head_offset;
    u16 head_size;
    u16 tail_valid;

    __attribute__((aligned(2))) u8 head_buffer[SECTOR_SIZE];
    __attribute__((aligned(2))) u8 tail_buffer[SECTOR_SIZE];  

} ide_dma_vars_t;

typedef struct ide_request_item
{
    stor_request_t request;
    struct ide_request_item* prev;
} ide_request_item_t;

typedef struct ide_request_queue 
{
    ide_request_item_t* head;
    ide_request_item_t* tail;
} ide_request_queue_t;
typedef struct ide_vars
{
    ide_channel_t channels[CHANNELS];  
    union 
    {
        // DMA only
        ide_dma_vars_t dma[CHANNELS];
    };
    ide_request_queue_t queue[CHANNELS];
    ide_device_t devices[DEVICES]; 
} ide_vars_t;

static ide_vars_t ide;

#define ATA_STATUS_BSY     0x80    // Busy
#define ATA_STATUS_DRDY    0x40    // Drive ready
#define ATA_STATUS_DF      0x20    // Drive write fault
#define ATA_STATUS_DSC     0x10    // Drive seek complete
#define ATA_STATUS_DRQ     0x08    // Data request ready
#define ATA_STATUS_CORR    0x04    // Corrected data
#define ATA_STATUS_IDX     0x02    // Index
#define ATA_STATUS_ERR     0x01    // Error

#define ATA_ERROR_BBK      0x80    // Bad block
#define ATA_ERROR_UNC      0x40    // Uncorrectable data
#define ATA_ERROR_MC       0x20    // Media changed
#define ATA_ERROR_IDNF     0x10    // ID mark not found
#define ATA_ERROR_MCR      0x08    // Media change request
#define ATA_ERROR_ABRT     0x04    // Command aborted
#define ATA_ERROR_TK0NF    0x02    // Track 0 not found
#define ATA_ERROR_AMNF     0x01    // No address mark

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATA_CTRL_nIEN 0x02
#define ATA_CTRL_SRST 0x04


static inline u8 ide_cmd_inb(usize_ptr channel, u8 reg) 
{
    return inb(ide.channels[channel].base + reg);
}

static inline void ide_cmd_outb(usize_ptr channel, u8 reg, u8 val) 
{
    outb(ide.channels[channel].base + reg, val);
}

static inline void ide_select(u8 channel, bool slave, u8 lba_high4) 
{
    u8 val = 0xE0                     // mandatory bits
                | (slave ? 0x10 : 0x00)    // bit 4: master/slave
                | (lba_high4 & 0x0F);      // upper 4 bits of LBA28
    ide_cmd_outb(channel, REG_OFF_HDDEVSEL, val);
    io_wait(); io_wait(); // spec requires delay
}

static inline void ide_send_cmd(u8 channel, u8 cmd) 
{
    ide_cmd_outb(channel, REG_OFF_COMMAND, cmd);
}

static inline u8 ide_get_status(u8 channel) 
{
    return ide_cmd_inb(channel, REG_OFF_STATUS);
}

static inline void ide_set_lba28(u8 channel, bool slave, u32 lba, u8 sector_count) 
{
    ide_select(channel, slave, (lba >> 24) & 0x0F);

    ide_cmd_outb(channel, REG_OFF_SECCOUNT0, sector_count); 
    ide_cmd_outb(channel, REG_OFF_LBA0, (u8)(lba & 0xFF));
    ide_cmd_outb(channel, REG_OFF_LBA1, (u8)((lba >> 8) & 0xFF));
    ide_cmd_outb(channel, REG_OFF_LBA2, (u8)((lba >> 16) & 0xFF));
}

static inline void ide_set_lba48(u8 channel, bool slave, u64 lba, u16 sector_count) 
{
    ide_select(channel, slave, (lba >> 24) & 0x0F);

    // High-order bytes first
    ide_cmd_outb(channel, REG_OFF_SECCOUNT0, (sector_count >> 8) & 0xFF); 
    ide_cmd_outb(channel, REG_OFF_LBA0, (lba >> 24) & 0xFF);
    ide_cmd_outb(channel, REG_OFF_LBA1, (lba >> 32) & 0xFF);
    ide_cmd_outb(channel, REG_OFF_LBA2, (lba >> 40) & 0xFF);

    // Then low-order bytes
    ide_cmd_outb(channel, REG_OFF_SECCOUNT0, sector_count & 0xFF); 
    ide_cmd_outb(channel, REG_OFF_LBA0, lba & 0xFF);
    ide_cmd_outb(channel, REG_OFF_LBA1, (lba >> 8) & 0xFF);
    ide_cmd_outb(channel, REG_OFF_LBA2, (lba >> 16) & 0xFF);
}

static inline void ide_read_buffer(usize_ptr channel, void* buf, unsigned words) 
{
    insw(ide.channels[channel].base + REG_OFF_DATA, buf, words);
}


#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define ATA_IDENT_SIZE         512

static inline void get_channel_identity(usize_ptr channel, u16 output[ATA_IDENT_SIZE / sizeof(u16)])
{
    ide_read_buffer(channel, output, ATA_IDENT_SIZE / sizeof(u16));
}

#define ATA_BM_OFF_CMD    0x00
#define ATA_BM_OFF_STATUS 0x02
#define ATA_BM_OFF_PRDT   0x04

#define ATA_BM_CMD_START  0x01
#define ATA_BM_CMD_READ   0x08
#define ATA_BM_CMD_STOP   0x00

#define ATA_BM_STATUS_IRQ   0x02
#define ATA_BM_STATUS_ERR   0x01

static inline void send_bm_cmd(usize_ptr channel, u8 cmd)
{
    if (ide.channels[channel].dma_io)
    {
        outb(ide.channels[channel].bus_master + ATA_BM_OFF_CMD, cmd);
    }
    else
    {
        // not supported yet
        abort();
    }
}

static inline u8 recv_bm_status(usize_ptr channel)
{
    if (ide.channels[channel].dma_io)
    {
        return inb(ide.channels[channel].bus_master + ATA_BM_OFF_STATUS);
    }
    else
    {
        // not supported yet
        abort();
        return 0;
    }
}

static inline void send_bm_status(usize_ptr channel, u8 status)
{
    if (ide.channels[channel].dma_io)
    {
        outb(ide.channels[channel].bus_master + ATA_BM_OFF_STATUS, status);
    }
    else
    {
        // not supported yet
        abort();
    }
}

static inline void set_prdt_addr(usize_ptr channel, u32 prdt_phys_addr)
{
    if (ide.channels[channel].dma_io)
    {
        outl(ide.channels[channel].bus_master + ATA_BM_OFF_PRDT, prdt_phys_addr);
    }
    else 
    {
        // not supported yet
        abort();
    }
}

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

#define ATA_PRIMARY    0x00
#define ATA_SECONDARY  0x01

static void init_ide_channels(pci_driver_t* driver)
{
    bool pri_compat = (driver->device_header.header.progif & (1 << 0)) == 0;
    bool sec_compat = (driver->device_header.header.progif & (1 << 2)) == 0;
    bool support_dma = driver->device_header.header.progif & (1 << 7);
    
    // Set both channel's common vars
    u32 bus_master_base = driver->device_header.base_addr[4] & ~0xF; 
    ide.channels[ATA_PRIMARY  ].bus_master = bus_master_base + 0;
    ide.channels[ATA_SECONDARY].bus_master = bus_master_base + 8;
    ide.channels[ATA_PRIMARY  ].has_dma    = support_dma;
    ide.channels[ATA_SECONDARY].has_dma    = support_dma;
    ide.channels[ATA_PRIMARY  ].dma_io     = driver->device_header.base_addr[4] & 0x01;
    ide.channels[ATA_SECONDARY].dma_io     = driver->device_header.base_addr[4] & 0x01;

    // Set primary channel vars
    ide.channels[ATA_PRIMARY].base = pri_compat ? 
        COMPABILITY_PRIMARY_PORT : (u16) (driver->device_header.base_addr[0] & (~0b1));
    ide.channels[ATA_PRIMARY].ctrl = pri_compat ? 
        COMPABILITY_PRIMARY_CTRL_PORT : (u16) (driver->device_header.base_addr[1] & (~0b1));
    ide.channels[ATA_PRIMARY].irq = pri_compat ?
                                    COMPABILITY_PRIMARY_IRQ : (u16)(driver->device_header.interrupt_line);
    ide.channels[ATA_PRIMARY].irq += PIC_IRQ_OFFSET;

    // Set secondary channel vars
    ide.channels[ATA_SECONDARY].base = sec_compat ? 
        COMPABILITY_SECOND_PORT : (u16) (driver->device_header.base_addr[2] & (~0b1));
    ide.channels[ATA_SECONDARY].ctrl = sec_compat ? 
        COMPABILITY_SECOND_CTRL_PORT : (u16) (driver->device_header.base_addr[3] & (~0b1));
    ide.channels[ATA_SECONDARY].irq = sec_compat ?
                                    COMPABILITY_SECOND_IRQ : (u16)(driver->device_header.interrupt_line);
    ide.channels[ATA_SECONDARY].irq += PIC_IRQ_OFFSET;

    if (ide.channels[ATA_PRIMARY].has_dma)
    {
        send_bm_cmd(ATA_PRIMARY, ATA_BM_CMD_STOP);
        send_bm_status(ATA_PRIMARY, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);
    }

    if (ide.channels[ATA_SECONDARY].has_dma)
    {
        send_bm_cmd(ATA_SECONDARY, ATA_BM_CMD_STOP);
        send_bm_status(ATA_SECONDARY, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);
    }
    ide.channels[ATA_PRIMARY  ].pic_enabled = false;
    ide.channels[ATA_SECONDARY].pic_enabled = false;

    ide.queue[ATA_PRIMARY  ].head = NULL;
    ide.queue[ATA_SECONDARY].head = NULL;
    ide.queue[ATA_PRIMARY  ].tail = NULL;
    ide.queue[ATA_SECONDARY].tail = NULL;
}

static u8 ide_ident_buffer[2048] = {0};

static void init_ide_device(usize_ptr device_index, usize_ptr channel_index, usize_ptr drive_select)
{
    ide.devices[device_index].external_dev_id = INVALID_ID;

    u8 err = 0;
    u8 type = IDE_ATA;
    u8 status;
    ide.devices[device_index].present = false; // Assuming no drive
    ide.devices[device_index].slave = drive_select == ATA_SLAVE;

    // (I) Select Drive:
    ide_select(channel_index, ide.devices[device_index].slave, 0); 
    io_wait(); io_wait();

    // (II) Send ATA Identify Command:
    ide_send_cmd(channel_index, ATA_CMD_IDENTIFY);
    io_wait(); io_wait();

    // (III) Polling: 
    
    // If Status = 0, No Device.
    if (ide_get_status(channel_index) == 0) 
        return;

    while(1) 
    {
        status = ide_get_status(channel_index);
        
        // If Err, Device is not ATA.
        if ((status & ATA_STATUS_ERR)) 
        {
            err = 1; 
            break;
        } 
        // Everything is right.
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ))
        {
            break; 
        }
    }

    // (IV) Probe for ATAPI Devices:
    if (err != 0) 
    {
        return;
        // keep it if ever ATAPI will be implemented 
        
        u8 lba1 = ide_cmd_inb(channel_index, REG_OFF_LBA1);
        u8 lba2 = ide_cmd_inb(channel_index, REG_OFF_LBA2);

        if ((lba1 == 0x14 && lba2== 0xEB) ||
            (lba1 == 0x69 && lba2 == 0x96))
        {   
            type = IDE_ATAPI;
        }
        else
        {
            return;
        }

        ide_send_cmd(channel_index, ATA_CMD_IDENTIFY_PACKET);
        io_wait(); io_wait();
    }

    // (V) Read Identification Space of the Device:
    get_channel_identity(channel_index, (u16*)ide_ident_buffer);

    // (VI) Read Device Parameters:
    ide.devices[device_index].present      = true;
    ide.devices[device_index].type         = type;
    ide.devices[device_index].channel      = channel_index;
    ide.devices[device_index].slave        = drive_select == ATA_SLAVE;
    ide.devices[device_index].signature    = *((u16*)(ide_ident_buffer + ATA_IDENT_DEVICETYPE));
    ide.devices[device_index].features     = *((u16*)(ide_ident_buffer + ATA_IDENT_CAPABILITIES));
    ide.devices[device_index].command_sets = *((u32*)(ide_ident_buffer + ATA_IDENT_COMMANDSETS));
    ide.devices[device_index].supports_dma = (ide.devices[device_index].features & (1 << 8)) != 0;
    
    // (VII) Get Size:
    if (ide.devices[device_index].command_sets & (1 << 26))
    {
        // Device uses 48-Bit Addressing:
        u64 lba48 = *((u64*)(ide_ident_buffer + ATA_IDENT_MAX_LBA_EXT)) & ((1ull<<48)-1);
        ide.devices[device_index].size = lba48;

        ide.devices[device_index].size_type = IDE_SIZE_LBA48;
    }
    else if(ide.devices[device_index].features & 0x200) // does support LBA
    {
        // Device uses CHS or 28-bit Addressing::
        u32 lba28 = *((u32*)(ide_ident_buffer + ATA_IDENT_MAX_LBA));
        ide.devices[device_index].size = lba28 & ((1<<28)-1);

        ide.devices[device_index].size_type = IDE_SIZE_LBA28;
    }
    else // get using chs 
    {
        ide.devices[device_index].chs.heads     = *((u8* )(ide_ident_buffer + ATA_IDENT_HEADS));
        ide.devices[device_index].chs.sectors   = *((u8* )(ide_ident_buffer + ATA_IDENT_SECTORS));
        ide.devices[device_index].chs.cylinders = *((u16*)(ide_ident_buffer + ATA_IDENT_CYLINDERS));

        ide.devices[device_index].size = 
            (u64)ide.devices[device_index].chs.cylinders *
            ide.devices[device_index].chs.heads *
            ide.devices[device_index].chs.sectors;

        ide.devices[device_index].size_type = IDE_SIZE_CHS;
    }
    // String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
    u16* ident_data = (u16*) ide_ident_buffer;
    for (u32 i = 0; i < IDE_STR_INDEN_LENGTH / 2; ++i)
    {
        ide.devices[device_index].model[i * 2 + 0] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 8) & 0xFF;
        ide.devices[device_index].model[i * 2 + 1] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 0) & 0xFF;
    }

    ide.devices[device_index].model[IDE_STR_INDEN_LENGTH] = '\0';
}

static void init_ide_devices()
{
    // 3- Detect ATA-ATAPI Devices:
    for (usize_ptr device_index = 0; device_index < DEVICES; device_index++)
    {
        usize_ptr channel_index = device_index / DEVS_PER_CHANNEL;
        usize_ptr drive_select  = device_index % DEVS_PER_CHANNEL;

        init_ide_device(device_index, channel_index, drive_select);
    }     

    for (u32 i = 0; i < DEVICES; i++)
    {
        if (ide.devices[i].present)
        {
            printf("Device #%d\n- %s\n- %lluMiB\n- Name:%s\n",
                i, 
                (const char* []){"ATA", "ATAPI"}[ide.devices[i].type],
                ide.devices[i].size * SECTOR_SIZE / STOR_1MiB, 
                ide.devices[i].model);
        }
    }   
}

static void init_dma_devices()
{
    // Init dma per channel
    for (usize_ptr ch = 0; ch < CHANNELS; ch++)
    {
        bool dev_supports_dma = false;
        for (usize_ptr dev = 0; dev < DEVS_PER_CHANNEL; dev++)
        {
            if (ide.devices[ch*DEVS_PER_CHANNEL + dev].supports_dma)
            {
                dev_supports_dma = true;
                break;
            }
        }

        if (!dev_supports_dma)
            continue;

        ide.dma[ch].prdt = kalloc_aligned(
            64,  // prdt addr must be aligned 64 bytes
            sizeof(ide_prd_entry_t) * PRD_ENTRIES_PER_CHANNEL
        );

        assert(ide.dma[ch].prdt);

        ide.dma[ch].prdt_capacity = PRD_ENTRIES_PER_CHANNEL;
    }
}

#define PRD_MAX_BOUNDARY STOR_64KiB
#define PRD_LAST_BIT     0x8000

static void prdt_iterate_chunk_prd(u64 prd_index, ide_dma_vars_t* dma, usize_ptr* remaining, usize_ptr* va)
{
    if (prd_index >= dma->prdt_capacity) 
    {
        // grow dynamically
        u64 new_capacity = dma->prdt_capacity * 2;
        dma->prdt = krealloc(dma->prdt, sizeof(ide_prd_entry_t) * new_capacity);
        dma->prdt_capacity = new_capacity;
    }

    usize_ptr cur_chunk_size = 0;
    usize_ptr start_pa = (usize_ptr)virt_to_phys((void*)*va);
    usize_ptr cur_pa   = start_pa;

    // Until no bytes remain, or some policy stopped this prd entry
    while (*remaining > 0) 
    {
        usize_ptr page_offset = ((*va) + cur_chunk_size) & (PAGE_SIZE - 1);
        usize_ptr page_space  = PAGE_SIZE - page_offset;

        usize_ptr take = (*remaining < page_space) ? *remaining : page_space;

        usize_ptr boundary = PRD_MAX_BOUNDARY -
                                ((cur_pa + cur_chunk_size) &
                                (PRD_MAX_BOUNDARY - 1));
        if (take > boundary)
            take = boundary;

        // must be sector aligned
        take -= take % SECTOR_SIZE;
        if (take == 0)
            break;

        cur_chunk_size += take;
        *remaining     -= take;

        if (*remaining == 0)
            break;

        // phys must be continous
        usize_ptr next_pa =
            (usize_ptr)virt_to_phys((void*)(*va + cur_chunk_size));
        if (next_pa != (cur_pa & PAGE_MASK) + PAGE_SIZE)
            break;

        // Must not pass the MAX BOUNDARY
        usize_ptr cur_prd_max_boundary  = cur_pa  & (~(PRD_MAX_BOUNDARY-1));
        usize_ptr next_prd_max_boundary = next_pa & (~(PRD_MAX_BOUNDARY-1));
        if (cur_prd_max_boundary != next_prd_max_boundary)
            break;

        cur_pa = next_pa;
    }

    dma->prdt[prd_index].phys_addr  = (u32)start_pa;
    dma->prdt[prd_index].bytes_size =
        (cur_chunk_size == PRD_MAX_BOUNDARY ? 
            0 : (u16)cur_chunk_size);
    dma->prdt[prd_index].flags = 0;

    *va += cur_chunk_size;
}

// Returns the amount of sectors that are needed to be copied
static u64 fill_prdt(u8 channel, stor_request_chunk_entry_t* chunk_arr, u64 chunk_length) 
{
    ide_dma_vars_t* dma = &ide.dma[channel];
    u64 total_sectors = 0;
    u64 prd_index = 0;

    for (u64 ci = 0; ci < chunk_length; ci++) 
    {
        usize_ptr va        = (usize_ptr)chunk_arr[ci].va_buffer;
        usize_ptr remaining = chunk_arr[ci].sectors * SECTOR_SIZE;

        assert((va % SECTOR_SIZE) == 0);

        while (remaining > 0) 
        {
            prdt_iterate_chunk_prd(prd_index, dma, &remaining, &va);

            prd_index++;
        }

        total_sectors += chunk_arr[ci].sectors;
    }

    if (prd_index > 0)
    {
        ide.dma[channel].prdt[prd_index - 1].flags |= PRD_LAST_BIT;
        set_prdt_addr(channel, (u32)virt_to_phys(ide.dma[channel].prdt));
    }

    return total_sectors;
}

static void ide_read_sectors_async(usize_ptr device, u64 lba, stor_request_chunk_entry_t* chunk_arr, u64 chunk_length)
{
    u8 channel = ide.devices[device].channel;

    if (!ide.channels[channel].has_dma)
    {    
        abort();
    }

    u64 sector_count = fill_prdt(
        channel, 
        chunk_arr, 
        chunk_length
    );

    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    u8 command = ~0;
    if (ide.devices[device].size_type == IDE_SIZE_LBA48) 
    {
        ide_set_lba48(channel, ide.devices[device].slave, lba, sector_count);
        command = ATA_CMD_READ_DMA_EXT;
    } 
    else if (ide.devices[device].size_type == IDE_SIZE_LBA28) 
    {
        ide_cmd_outb(channel, REG_OFF_SECCOUNT0, 0x00);
        ide_set_lba28(channel, ide.devices[device].slave, lba, sector_count);
        command = ATA_CMD_READ_DMA;
    } 
    else 
    {
        abort();
    }

    ide_send_cmd(channel, command);

    u8 bm_cmd = ATA_BM_CMD_READ | ATA_BM_CMD_START;
    send_bm_cmd(channel, bm_cmd);
}

static void ide_write_sectors_async(usize_ptr device, u64 lba, stor_request_chunk_entry_t* chunk_arr, u64 chunk_length)
{
    u8 channel = ide.devices[device].channel;

    if (!ide.channels[channel].has_dma)
    {
        abort();
    }

    // just for PRDT, we're not modifying the buffer
    u64 sector_count = fill_prdt(
        channel, 
        chunk_arr, 
        chunk_length
    );
    assert(sector_count);

    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    u8 command = ~0;
    if (ide.devices[device].size_type == IDE_SIZE_LBA48) 
    {
        ide_set_lba48(channel, ide.devices[device].slave, lba, sector_count);
        command = ATA_CMD_WRITE_DMA_EXT;
    } 
    else if (ide.devices[device].size_type == IDE_SIZE_LBA28)
    {
        ide_cmd_outb(channel, REG_OFF_SECCOUNT0, 0x00);
        ide_set_lba28(channel, ide.devices[device].slave, lba, sector_count);
        command = ATA_CMD_WRITE_DMA;
    } 
    else 
    {
        abort();
    }

    ide_send_cmd(channel, command);

    u8 bm_cmd = ATA_BM_CMD_START; // no READ bit = write
    send_bm_cmd(channel, bm_cmd);
}

static void ide_push_queue(stor_request_t* request)
{
    ide_device_t* dev = (ide_device_t*) request->dev->dev_data;
    u16 channel = dev->channel;

    ide_request_item_t* item = kalloc(sizeof(ide_request_item_t));
    assert(item);

    item->prev = NULL;
    item->request = *request;

    // Insert new item
    if (! ide.queue[channel].tail)
    {
        ide.queue[channel].tail = item;
        ide.queue[channel].head = item;
    }
    else
    {
        ide.queue[channel].tail->prev = item;
        ide.queue[channel].tail = item;
    }
}

// kfree is needed by respondent
static ide_request_item_t* ide_pop_queue(u16 channel)
{
    if (!ide.queue[channel].head)
    {
        return NULL;
    }
    
    ide_request_item_t* item = ide.queue[channel].head;
        
    ide.queue[channel].head = ide.queue[channel].head->prev;

    if (!ide.queue[channel].head)
    {
        ide.queue[channel].tail = NULL;
    }

    return item;

}

static void make_request(stor_request_t* request)
{
    ide_device_t* dev = (ide_device_t*) request->dev->dev_data;
    u64 lba = request->lba;
    
    switch (request->action)
    {
    case STOR_REQ_READ:
        ide_read_sectors_async(dev->channel, lba, request->chunk_list, request->chunk_length);
        break;
    
    case STOR_REQ_WRITE:
        ide_write_sectors_async(dev->channel, lba, request->chunk_list, request->chunk_length);
        break;
    
    default:
        printf("Invalid Storage Request\n\n");
        abort();
    }
}

static void ide_submit(stor_request_t* request)
{
    ide_device_t* dev = (ide_device_t*) request->dev->dev_data;
    
    usize_ptr irq_data = irq_save();
    
    bool empty_queue = ide.queue[dev->channel].head == NULL;

    ide_push_queue(request);

    // The callback will queue the next task
    if (empty_queue == true)
    {
        make_request(request);
    }

    irq_restore(irq_data);
}


static void primary_irq(irq_frame_t* irq_frame)
{
    (void)irq_frame;

    u16 channel = ATA_PRIMARY;
    
    ide_request_item_t* item = ide_pop_queue(channel);

    if (!item) 
    {
        send_bm_cmd(channel, ATA_BM_CMD_STOP);
        send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);
    
        pic_send_eoi_vector(ide.channels[channel].irq);
    
        return;
    }

    send_bm_cmd(channel, ATA_BM_CMD_STOP);
    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    u32 bm_status = recv_bm_status(channel);
    u16 drv_status = ide_get_status(channel);

    int64_t result;
    if ((bm_status & ATA_BM_STATUS_ERR) || (drv_status & ATA_STATUS_ERR)) 
    {
        result = -1;
    }
    else
    {
        result = 1;
    }
    
    if (item->request.callback)
    {
        item->request.callback(&item->request, result);
    }
    
    if (ide.queue[channel].head)
    {
        make_request(&ide.queue[channel].head->request);
    }

    kfree(item);
    item = NULL;

    pic_send_eoi_vector(ide.channels[channel].irq);
}

static void secondary_irq(irq_frame_t* irq_frame)
{
    (void)irq_frame;
    
    u16 channel = ATA_SECONDARY;

    ide_request_item_t* item = ide_pop_queue(channel);

    if (!item) 
    {
        send_bm_cmd(channel, ATA_BM_CMD_STOP);
        send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);
    
        pic_send_eoi_vector(ide.channels[channel].irq);
    
        return;
    }

    send_bm_cmd(channel, ATA_BM_CMD_STOP);
    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    u32 bm_status = recv_bm_status(channel);
    u16 drv_status = ide_get_status(channel);

    int64_t result;
    if ((bm_status & ATA_BM_STATUS_ERR) || (drv_status & ATA_STATUS_ERR)) 
    {
        result = -1;
    }
    else
    {
        result = 1;
    }

    item->request.callback(&item->request, result);

    if (ide.queue[channel].head)
    {
        make_request(&ide.queue[channel].head->request);
    }

    kfree(item);
    item = NULL;

    pic_send_eoi_vector(ide.channels[channel].irq);
}

static void enable_pic()
{
    for (u32 i = 0; i < DEVICES; i++)
    {
        u16 dev_channel = ide.devices[i].channel;
        
        // Only channels allowed
        if (dev_channel != ATA_PRIMARY && dev_channel != ATA_SECONDARY)
        {
            abort();
        }
        
        if (ide.devices[i].present && ide.channels[dev_channel].pic_enabled == false)
        {
            
            send_bm_cmd(dev_channel, ATA_BM_CMD_STOP);
            send_bm_status(dev_channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);
            (void)ide_get_status(dev_channel);

            irq_register_handler(
                ide.channels[dev_channel].irq,
                dev_channel == ATA_PRIMARY ? primary_irq : secondary_irq
            );

            pic_unmask_vector(ide.channels[dev_channel].irq);

            ide.channels[dev_channel].pic_enabled = true;
        }
    }    
}

void init_ide(storage_add_device add_func, pci_driver_t *driver)
{
    u32 cmd = pci_config_read_dword(driver->bus, driver->slot, driver->func, 0x4);
    if (!(cmd & PCI_CMD_DMA_BIT)) 
    {
        cmd |= PCI_CMD_DMA_BIT;
        pci_config_write_dword(
            driver->bus, driver->slot, driver->func, PCI_CMD_OFF, cmd
        );
    }

    assert(driver);

    init_ide_channels(driver);

    init_ide_devices();

    init_dma_devices();

    for (u32 i = 0; i < DEVICES; i++)
    {
        if (ide.devices[i].present)
        {
            ide.devices[i].external_dev_id = add_func(
                &ide.devices[i],
                SECTOR_SIZE,
                ide_submit,
                ide.devices[i].size
            );
        }
    }
    
    enable_pic();
}