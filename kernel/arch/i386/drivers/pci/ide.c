#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/drivers/io/io.h>
#include <drivers/pci.h>
#include <memory/heap/heap.h>
#include <arch/i386/memory/paging_utils.h>
#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pci/ide.h>
#include <kernel/devices/storage.h>
#include <kernel/drivers/pci_ops.h>
#include <stdint.h>
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
    uint16_t base;       // BAR0 or BAR2
    uint16_t ctrl;       // BAR1 or BAR3
    uint32_t bus_master; // BAR4 + 0x0 (primary) or +0x8 (secondary)
    bool has_dma;        // does this channel support DMA?
    bool dma_io;         // is DMA IO or MMIO? True - IO, MMIO - False
    uint32_t irq;        // irq number
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
    uint8_t size_type;                      // 0 = chs (not supported), 1 = LBA28, 2 = LBA48
    union {
        struct {
            uint16_t cylinders;
            uint8_t heads;
            uint8_t sectors;
        } chs;
    };
    bool supports_dma;                      // supports DMA? Yes true, No false
    uint8_t channel;                        // 0 = primary, 1 = secondary
    uint8_t type;                           // ATA or ATAPI
    uint16_t signature;                     // Drive signature
    uint16_t features;                      // device capabilities
    uint32_t command_sets;                  // command sets supported
    uint64_t size;                          // in sectors 
    char model[IDE_STR_INDEN_LENGTH + 1];   // from IDENTIFY string

    uint64_t external_dev_id;
} ide_device_t;

typedef struct __attribute__((packed)) ide_prd_entry
{
    uint32_t phys_addr;
    uint16_t bytes_size;
    uint16_t flags; // reserved except MSB

} ide_prd_entry_t; 

#define PRD_ENTRIES_PER_CHANNEL 8
typedef ide_prd_entry_t ide_prd_entries[PRD_ENTRIES_PER_CHANNEL];

#define CHANNELS 2 // pri/sec
#define DEVS_PER_CHANNEL 2   // for each channel - master/slave 
#define DEVICES (CHANNELS * DEVS_PER_CHANNEL) 

typedef struct __attribute__((aligned(SECTOR_SIZE))) ide_dma_vars
{
    __attribute__((aligned(64))) ide_prd_entries prd;  
    // only needed for first entry if not aligned
    uint16_t head_offset;
    uint16_t head_size;
    uint16_t tail_valid;
    __attribute__((aligned(2))) uint8_t head_buffer[SECTOR_SIZE];
    __attribute__((aligned(2))) uint8_t tail_buffer[SECTOR_SIZE];  

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

ide_vars_t ide;

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


static inline uint8_t ide_cmd_inb(uintptr_t channel, uint8_t reg) 
{
    return inb(ide.channels[channel].base + reg);
}

static inline void ide_cmd_outb(uintptr_t channel, uint8_t reg, uint8_t val) 
{
    outb(ide.channels[channel].base + reg, val);
}

static inline void ide_select(uint8_t channel, bool slave, uint8_t lba_high4) 
{
    uint8_t val = 0xE0                     // mandatory bits
                | (slave ? 0x10 : 0x00)    // bit 4: master/slave
                | (lba_high4 & 0x0F);      // upper 4 bits of LBA28
    ide_cmd_outb(channel, REG_OFF_HDDEVSEL, val);
    io_wait(); io_wait(); // spec requires delay
}

static inline void ide_send_cmd(uint8_t channel, uint8_t cmd) 
{
    ide_cmd_outb(channel, REG_OFF_COMMAND, cmd);
}

static inline uint8_t ide_get_status(uint8_t channel) 
{
    return ide_cmd_inb(channel, REG_OFF_STATUS);
}

static inline void ide_set_lba28(uint8_t channel, bool slave, uint32_t lba, uint8_t sector_count) 
{
    ide_select(channel, slave, (lba >> 24) & 0x0F);

    ide_cmd_outb(channel, REG_OFF_SECCOUNT0, sector_count); 
    ide_cmd_outb(channel, REG_OFF_LBA0, (uint8_t)(lba & 0xFF));
    ide_cmd_outb(channel, REG_OFF_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    ide_cmd_outb(channel, REG_OFF_LBA2, (uint8_t)((lba >> 16) & 0xFF));
}

static inline void ide_set_lba48(uint8_t channel, bool slave, uint64_t lba, uint16_t sector_count) 
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

static inline void ide_read_buffer(uintptr_t channel, void* buf, unsigned words) 
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

static inline void get_channel_identity(uintptr_t channel, uint16_t output[ATA_IDENT_SIZE / sizeof(uint16_t)])
{
    ide_read_buffer(channel, output, ATA_IDENT_SIZE / sizeof(uint16_t));
}

#define ATA_BM_OFF_CMD    0x00
#define ATA_BM_OFF_STATUS 0x02
#define ATA_BM_OFF_PRDT   0x04

#define ATA_BM_CMD_START  0x01
#define ATA_BM_CMD_READ   0x08
#define ATA_BM_CMD_STOP   0x00

#define ATA_BM_STATUS_IRQ   0x02
#define ATA_BM_STATUS_ERR   0x01

static inline void send_bm_cmd(uintptr_t channel, uint8_t cmd)
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

static inline uint8_t recv_bm_status(uintptr_t channel)
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

static inline void send_bm_status(uintptr_t channel, uint8_t status)
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

static inline void set_prdt_addr(uintptr_t channel, uint32_t prdt_phys_addr)
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
    uint32_t bus_master_base = driver->device_header.base_addr[4] & ~0xF; 
    ide.channels[ATA_PRIMARY  ].bus_master = bus_master_base + 0;
    ide.channels[ATA_SECONDARY].bus_master = bus_master_base + 8;
    ide.channels[ATA_PRIMARY  ].has_dma    = support_dma;
    ide.channels[ATA_SECONDARY].has_dma    = support_dma;
    ide.channels[ATA_PRIMARY  ].dma_io     = driver->device_header.base_addr[4] & 0x01;
    ide.channels[ATA_SECONDARY].dma_io     = driver->device_header.base_addr[4] & 0x01;

    // Set primary channel vars
    ide.channels[ATA_PRIMARY].base = pri_compat ? 
        COMPABILITY_PRIMARY_PORT : (uint16_t) (driver->device_header.base_addr[0] & (~0b1));
    ide.channels[ATA_PRIMARY].ctrl = pri_compat ? 
        COMPABILITY_PRIMARY_CTRL_PORT : (uint16_t) (driver->device_header.base_addr[1] & (~0b1));
    ide.channels[ATA_PRIMARY].irq = pri_compat ?
                                    COMPABILITY_PRIMARY_IRQ : (uint16_t)(driver->device_header.interrupt_line);
    ide.channels[ATA_PRIMARY].irq += PIC_IRQ_OFFSET;

    // Set secondary channel vars
    ide.channels[ATA_SECONDARY].base = sec_compat ? 
        COMPABILITY_SECOND_PORT : (uint16_t) (driver->device_header.base_addr[2] & (~0b1));
    ide.channels[ATA_SECONDARY].ctrl = sec_compat ? 
        COMPABILITY_SECOND_CTRL_PORT : (uint16_t) (driver->device_header.base_addr[3] & (~0b1));
    ide.channels[ATA_SECONDARY].irq = sec_compat ?
                                    COMPABILITY_SECOND_IRQ : (uint16_t)(driver->device_header.interrupt_line);
    ide.channels[ATA_SECONDARY].irq += PIC_IRQ_OFFSET;

    if (ide.channels[ATA_PRIMARY].has_dma)
    {
        send_bm_cmd(ATA_PRIMARY, 0x00);
    }

    if (ide.channels[ATA_SECONDARY].has_dma)
    {
        send_bm_cmd(ATA_SECONDARY, 0x00);
    }
    ide.channels[ATA_PRIMARY  ].pic_enabled = false;
    ide.channels[ATA_SECONDARY].pic_enabled = false;

    ide.queue[ATA_PRIMARY  ].head = NULL;
    ide.queue[ATA_SECONDARY].head = NULL;
    ide.queue[ATA_PRIMARY  ].tail = NULL;
    ide.queue[ATA_SECONDARY].tail = NULL;
}

uint8_t ide_buf[2048] = {0};

static void init_ide_device(uintptr_t device_index, uintptr_t channel_index, uintptr_t drive_select)
{
    ide.devices[device_index].external_dev_id = INVALID_ID;

    uint8_t err = 0;
    uint8_t type = IDE_ATA;
    uint8_t status;
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
        
        uint8_t lba1 = ide_cmd_inb(channel_index, REG_OFF_LBA1);
        uint8_t lba2 = ide_cmd_inb(channel_index, REG_OFF_LBA2);

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
    get_channel_identity(channel_index, (uint16_t*)ide_buf);

    // (VI) Read Device Parameters:
    ide.devices[device_index].present      = true;
    ide.devices[device_index].type         = type;
    ide.devices[device_index].channel      = channel_index;
    ide.devices[device_index].slave        = drive_select == ATA_SLAVE;
    ide.devices[device_index].signature    = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
    ide.devices[device_index].features     = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
    ide.devices[device_index].command_sets = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

    // (VII) Get Size:
    if (ide.devices[device_index].command_sets & (1 << 26))
    {
        // Device uses 48-Bit Addressing:
        uint64_t lba48 = *((uint64_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT)) & ((1ull<<48)-1);
        ide.devices[device_index].size = lba48;

        ide.devices[device_index].size_type = IDE_SIZE_LBA48;
    }
    else if(ide.devices[device_index].features & 0x200) // does support LBA
    {
        // Device uses CHS or 28-bit Addressing::
        uint32_t lba28 = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA));
        ide.devices[device_index].size = lba28 & ((1<<28)-1);

        ide.devices[device_index].size_type = IDE_SIZE_LBA28;
    }
    else // get using chs 
    {
        ide.devices[device_index].chs.heads     = *((uint8_t* )(ide_buf + ATA_IDENT_HEADS));
        ide.devices[device_index].chs.sectors   = *((uint8_t* )(ide_buf + ATA_IDENT_SECTORS));
        ide.devices[device_index].chs.cylinders = *((uint16_t*)(ide_buf + ATA_IDENT_CYLINDERS));

        ide.devices[device_index].size = 
            (uint64_t)ide.devices[device_index].chs.cylinders *
            ide.devices[device_index].chs.heads *
            ide.devices[device_index].chs.sectors;

        ide.devices[device_index].size_type = IDE_SIZE_CHS;
    }
    // String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
    uint16_t* ident_data = (uint16_t*) ide_buf;
    for (uint32_t i = 0; i < IDE_STR_INDEN_LENGTH / 2; ++i)
    {
        ide.devices[device_index].model[i * 2 + 0] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 8) & 0xFF;
        ide.devices[device_index].model[i * 2 + 1] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 0) & 0xFF;
    }

    ide.devices[device_index].model[IDE_STR_INDEN_LENGTH] = '\0';
}

static void init_ide_devices()
{
    // 3- Detect ATA-ATAPI Devices:
    for (uintptr_t device_index = 0; device_index < DEVICES; device_index++)
    {
        uintptr_t channel_index = device_index / DEVS_PER_CHANNEL;
        uintptr_t drive_select  = device_index % DEVS_PER_CHANNEL;

        init_ide_device(device_index, channel_index, drive_select);
    }

    for (uint32_t i = 0; i < DEVICES; i++)
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

#define PRD_MAX_BOUNDARY STOR_64KiB
#define PRD_LAST_BIT     0x8000

// Returns the amount of sectors that are needed to be copied// Returns the amount of sectors that are needed to be copied
static uint32_t fill_prdt(uint8_t channel, void* virt_buffer, uintptr_t byte_count) 
{
    assert(byte_count % SECTOR_SIZE == 0);
    assert(((uintptr_t)virt_buffer % SECTOR_SIZE) == 0);

    uintptr_t va        = (uintptr_t)virt_buffer;
    uintptr_t remaining = byte_count;
    uintptr_t prd_index = 0;

    uint32_t sectors = byte_count / SECTOR_SIZE;

    while (remaining > 0) 
    {
        if (prd_index >= PRD_ENTRIES_PER_CHANNEL) 
            abort();

        uintptr_t cur_chunk_size = 0;
        uintptr_t start_pa = (uintptr_t)get_phys_addr((void*)va); 
        uintptr_t cur_pa   = start_pa;

        while (remaining > 0) 
        {
            uintptr_t page_offset = (va + cur_chunk_size) & (PAGE_SIZE - 1);
            uintptr_t page_space  = PAGE_SIZE - page_offset;

            uintptr_t take = (remaining < page_space) ? remaining : page_space;

            uintptr_t boundary = PRD_MAX_BOUNDARY - ((cur_pa + cur_chunk_size) & (PRD_MAX_BOUNDARY - 1));
            if (take > boundary) 
                take = boundary;

            // stop if we’d break sector alignment
            take -= take % SECTOR_SIZE;
            if (take == 0) 
                break;

            cur_chunk_size += take;
            remaining      -= take;

            if (remaining == 0) 
                break;

            uintptr_t next_pa = (uintptr_t)get_phys_addr((void*)(va + cur_chunk_size));
            if (next_pa != (cur_pa & PAGE_MASK) + PAGE_SIZE) 
                break;

            uintptr_t cur_prd_max_boundary  = cur_pa  & (~(PRD_MAX_BOUNDARY-1));
            uintptr_t next_prd_max_boundary = next_pa & (~(PRD_MAX_BOUNDARY-1));
            if (cur_prd_max_boundary != next_prd_max_boundary) 
                break;

            cur_pa = next_pa;
        }

        ide.dma[channel].prd[prd_index].phys_addr  = (uint32_t)start_pa;
        ide.dma[channel].prd[prd_index].bytes_size =
            (cur_chunk_size == PRD_MAX_BOUNDARY ? 0 : (uint16_t)cur_chunk_size);
        ide.dma[channel].prd[prd_index].flags = 0;

        va += cur_chunk_size;
        prd_index++;
    }

    if (prd_index > 0)
    {
        ide.dma[channel].prd[prd_index - 1].flags |= PRD_LAST_BIT;
        set_prdt_addr(channel, (uint32_t)get_phys_addr(&ide.dma[channel].prd[0]));
    }

    return sectors;
}

static void ide_read_sectors_async(uintptr_t device, void* virt_buffer, uint64_t lba, uint64_t bytes_amount)
{
    uint8_t channel = ide.devices[device].channel;

    if (!ide.channels[channel].has_dma)
    {    
        abort();
    }

    uint64_t sector_count = fill_prdt(channel, virt_buffer, bytes_amount);

    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    uint8_t command = ~0;
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

    uint8_t bm_cmd = ATA_BM_CMD_READ | ATA_BM_CMD_START;
    send_bm_cmd(channel, bm_cmd);
}

static void ide_write_sectors_async(uintptr_t device, const void* virt_buffer, uint64_t lba, uint64_t bytes_amount)
{
    uint8_t channel = ide.devices[device].channel;

    if (!ide.channels[channel].has_dma)
    {
        abort();
    }

    uint64_t sector_count = fill_prdt(channel, (void*)virt_buffer, bytes_amount); // just for PRDT, we're not modifying the buffer

    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    uint8_t command = ~0;
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

    uint8_t bm_cmd = ATA_BM_CMD_START; // no READ bit = write
    send_bm_cmd(channel, bm_cmd);
}

static void ide_push_queue(stor_request_t* request)
{
    ide_device_t* dev = (ide_device_t*) request->dev->dev_data;
    uint16_t channel = dev->channel;

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
static ide_request_item_t* ide_pop_queue(uint16_t channel)
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
    void* va_buf = request->va_buffer;
    uint64_t lba = request->lba;
    uint64_t bytes = request->sectors * SECTOR_SIZE;
    
    switch (request->action)
    {
    case STOR_REQ_READ:
        ide_read_sectors_async(dev->channel, va_buf, lba, bytes);
        break;
    
    case STOR_REQ_WRITE:
        ide_write_sectors_async(dev->channel, va_buf, lba, bytes);
        break;
    
    default:
        printf("Invalid Storage Request\n\n");
        abort();
    }
}

static void ide_submit(stor_request_t* request)
{
    ide_device_t* dev = (ide_device_t*) request->dev->dev_data;
    bool empty_queue = ide.queue[dev->channel].head == NULL;

    irq_disable();

    ide_push_queue(request);

    // The callback will queue the next task
    if (empty_queue == true)
    {
        make_request(request);
    }

    irq_enable();
}


static void primary_irq(irq_frame_t* irq_frame)
{
    (void)irq_frame;
    
    uint16_t channel = ATA_PRIMARY;

    ide_request_item_t* item = ide_pop_queue(channel);

    send_bm_cmd(channel, ATA_BM_CMD_STOP);

    uint32_t bm_status = recv_bm_status(channel);
    uint16_t drv_status = ide_get_status(channel);

    int64_t bytes_count = item->request.sectors * SECTOR_SIZE;
    int64_t result;
    if ((bm_status & ATA_BM_STATUS_ERR) || (drv_status & ATA_STATUS_ERR)) 
    {
        result = -1;
    }
    else
    {
        result = bytes_count;
    }

    item->request.callback(&item->request, result);

    if (ide.queue[channel].head)
    {
        make_request(&ide.queue[channel].head->request);
    }

    kfree(item);
    item = NULL;
}
static void secondary_irq(irq_frame_t* irq_frame)
{
    (void)irq_frame;
    
    uint16_t channel = ATA_SECONDARY;

    ide_request_item_t* item = ide_pop_queue(channel);

    send_bm_cmd(channel, ATA_BM_CMD_STOP);

    uint32_t bm_status = recv_bm_status(channel);
    uint16_t drv_status = ide_get_status(channel);

    int64_t bytes_count = item->request.sectors * SECTOR_SIZE;
    int64_t result;
    if ((bm_status & ATA_BM_STATUS_ERR) || (drv_status & ATA_STATUS_ERR)) 
    {
        result = -1;
    }
    else
    {
        result = bytes_count;
    }

    item->request.callback(&item->request, result);

    if (ide.queue[channel].head)
    {
        make_request(&ide.queue[channel].head->request);
    }

    kfree(item);
    item = NULL;
}

static void enable_pic()
{
    for (uint32_t i = 0; i < DEVICES; i++)
    {
        uint16_t dev_channel = ide.devices[i].channel;
        
        // Only channels allowed
        if (dev_channel != ATA_PRIMARY && dev_channel != ATA_SECONDARY)
        {
            abort();
        }
        
        if (ide.devices[i].present && ide.channels[dev_channel].pic_enabled == false)
        {
            ide.channels[dev_channel].pic_enabled = true;
            pic_unmask_vector(ide.channels[dev_channel].irq);

            irq_register_handler(
                ide.channels[dev_channel].irq,
                dev_channel == ATA_PRIMARY ? primary_irq : secondary_irq 
            );
        }
    }    
}

static void dummy_callback(stor_request_t* request, int64_t result)
{
    if (result > 0)
    {
        printf("Success\n\n");
    }

    uint8_t* buf = request->va_buffer;
    
    for (uint32_t i = 0; i < SECTOR_SIZE; i++)
    {
        printf("%02X ", (uint32_t)buf[i]);
        if (i % 0x10 + 1 == 0x10)
        {
            printf("\n");
        }
    }
}

__attribute__((aligned(PAGE_SIZE))) uint8_t buffer[SECTOR_SIZE];

void init_ide(storage_add_device add_func, pci_driver_t *driver)
{
    uint32_t cmd = pci_config_read_dword(driver->bus, driver->slot, driver->func, 0x4);
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

    for (uint32_t i = 0; i < DEVICES; i++)
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

    for (uint32_t i = 0; i < DEVICES; i++)
    {
        if (ide.devices[i].present)
        {
            stor_device_t* dev = stor_get_device(ide.devices[i].external_dev_id);
            
            stor_request_t request = {
                .callback = dummy_callback,
                .dev = dev,
                .action = STOR_REQ_READ,
                .lba = 0,
                .sectors = 1,
                .user_data = NULL,
                .va_buffer = buffer
            };

            dev->submit(&request);

            request.action = STOR_REQ_WRITE;
            buffer[0] = 'A';
            buffer[1] = 'B';
            buffer[2] = 'C';
            buffer[3] = 'D';
            buffer[4] = 'E';
            dev->submit(&request);
        }
    }


}