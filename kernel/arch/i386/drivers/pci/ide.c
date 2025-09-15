#include "drivers/pci.h"
#include <arch/i386/memory/paging_utils.h>
#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pci/ide.h>
#include <kernel/devices/storage.h>
#include <kernel/drivers/pci_ops.h>
#include <stdint.h>
#include <stdio.h>

#define COMPABILITY_PRIMARY_PORT      0x1F0
#define COMPABILITY_PRIMARY_CTRL_PORT 0x3F6
#define COMPABILITY_SECOND_PORT       0x170
#define COMPABILITY_SECOND_CTRL_PORT  0x376

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
typedef struct ide_vars
{
    ide_channel_t channels[CHANNELS];  
    union 
    {
        // DMA only
        ide_prd_entries prd_entries[CHANNELS];  
    };
    ide_device_t devices[DEVICES]; 
} ide_vars_t;

ide_vars_t vars;

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
    return inb(vars.channels[channel].base + reg);
}

static inline void ide_cmd_outb(uintptr_t channel, uint8_t reg, uint8_t val) 
{
    outb(vars.channels[channel].base + reg, val);
}



static inline uint8_t ide_ctrl_inb(uintptr_t channel) 
{
    return inb(vars.channels[channel].ctrl + 2);
}

static inline void ide_ctrl_outb(uintptr_t channel, uint8_t val) 
{
    outb(vars.channels[channel].ctrl + 2, val);
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

static inline uint8_t ide_get_alt_status(uint8_t channel) 
{
    return ide_ctrl_inb(channel);
}

static inline uint8_t ide_get_error(uint8_t channel) 
{
    return ide_cmd_inb(channel, REG_OFF_ERROR);
}


static inline void ide_set_lba28(uint8_t channel, bool slave, uint32_t lba, uint8_t sector_count) 
{
    ide_select(channel, slave, (lba >> 24) & 0x0F);

    io_wait();

    ide_cmd_outb(channel, REG_OFF_SECCOUNT0, sector_count); 
    io_wait();
    ide_cmd_outb(channel, REG_OFF_LBA0, (uint8_t)(lba & 0xFF));
    io_wait();
    ide_cmd_outb(channel, REG_OFF_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    io_wait();
    ide_cmd_outb(channel, REG_OFF_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    io_wait();
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

static inline int ide_poll_status(uint8_t channel, bool check_drq) 
{
    uint8_t status = ATA_STATUS_BSY;
    do 
    {
        status = ide_cmd_inb(channel, REG_OFF_STATUS);

    } while (status & ATA_STATUS_BSY);

    if (check_drq) 
    {
        if (status & ATA_STATUS_ERR) 
            return -1;
        
        if (!(status & ATA_STATUS_DRQ)) 
            return -2;
    }
    return 0;
}

static inline void ide_read_buffer(uintptr_t channel, void* buf, unsigned words) 
{
    insw(vars.channels[channel].base + REG_OFF_DATA, buf, words);
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
    if (vars.channels[channel].dma_io)
    {
        outb(vars.channels[channel].bus_master + ATA_BM_OFF_CMD, cmd);
    }
    else
    {
        // not supported yet
        abort();
    }
}

static inline uint8_t recv_bm_status(uintptr_t channel)
{
    if (vars.channels[channel].dma_io)
    {
        return inb(vars.channels[channel].bus_master + ATA_BM_OFF_STATUS);
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
    if (vars.channels[channel].dma_io)
    {
        outb(vars.channels[channel].bus_master + ATA_BM_OFF_STATUS, status);
    }
    else
    {
        // not supported yet
        abort();
    }
}

static inline void set_prdt_addr(uintptr_t channel, uint32_t prdt_phys_addr)
{
    uint32_t bm_base = vars.channels[channel].bus_master;

    if (vars.channels[channel].dma_io)
    {
        outl(vars.channels[channel].bus_master + ATA_BM_OFF_PRDT, prdt_phys_addr);
    }
    else 
    {
        // not supported yet
        abort();
    }
}
static inline uint32_t get_prdt_addr(uintptr_t channel)
{
    uint32_t bm_base = vars.channels[channel].bus_master;
    uint32_t prdt_phys_addr = 0;

    if (vars.channels[channel].dma_io)
    {
        for (uintptr_t i = 0; i < sizeof(uint32_t); i++)
        {
            prdt_phys_addr |= ((uint32_t)inb(bm_base + ATA_BM_OFF_PRDT + i) << (i * 8));
        }

        return prdt_phys_addr;
    }
    else 
    {
        // not supported yet
        abort(); 
        return 0;
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
    vars.channels[ATA_PRIMARY  ].bus_master = bus_master_base + 0;
    vars.channels[ATA_SECONDARY].bus_master = bus_master_base + 8;
    vars.channels[ATA_PRIMARY  ].has_dma    = support_dma;
    vars.channels[ATA_SECONDARY].has_dma    = support_dma;
    vars.channels[ATA_PRIMARY  ].dma_io     = driver->device_header.base_addr[4] & 0x01;
    vars.channels[ATA_SECONDARY].dma_io     = driver->device_header.base_addr[4] & 0x01;

    // Set primary channel vars
    vars.channels[ATA_PRIMARY].base = pri_compat ? 
        COMPABILITY_PRIMARY_PORT : (uint16_t) (driver->device_header.base_addr[0] & (~0b1));
    vars.channels[ATA_PRIMARY].ctrl = pri_compat ? 
        COMPABILITY_PRIMARY_CTRL_PORT : (uint16_t) (driver->device_header.base_addr[1] & (~0b1));

    // Set secondary channel vars
    vars.channels[ATA_SECONDARY].base = sec_compat ? 
        COMPABILITY_SECOND_PORT : (uint16_t) (driver->device_header.base_addr[2] & (~0b1));
    vars.channels[ATA_SECONDARY].ctrl = sec_compat ? 
        COMPABILITY_SECOND_CTRL_PORT : (uint16_t) (driver->device_header.base_addr[3] & (~0b1));

    if (vars.channels[ATA_PRIMARY].has_dma)
    {
        send_bm_cmd(ATA_PRIMARY, 0x00);
    }

    if (vars.channels[ATA_SECONDARY].has_dma)
    {
        send_bm_cmd(ATA_SECONDARY, 0x00);
    }
}

uint8_t ide_buf[2048] = {0};

static void init_ide_device(uintptr_t device_index, uintptr_t channel_index, uintptr_t drive_select)
{
    uint8_t err = 0;
    uint8_t type = IDE_ATA;
    uint8_t status;
    vars.devices[device_index].present = false; // Assuming no drive
    vars.devices[device_index].slave = drive_select == ATA_SLAVE;

    // (I) Select Drive:
    ide_select(channel_index, vars.devices[device_index].slave, 0); 
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
    vars.devices[device_index].present      = true;
    vars.devices[device_index].type         = type;
    vars.devices[device_index].channel      = channel_index;
    vars.devices[device_index].slave        = drive_select == ATA_SLAVE;
    vars.devices[device_index].signature    = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
    vars.devices[device_index].features     = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
    vars.devices[device_index].command_sets = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

    // (VII) Get Size:
    if (vars.devices[device_index].command_sets & (1 << 26))
    {
        // Device uses 48-Bit Addressing:
        uint64_t lba48 = 0;
        for (uint8_t bits = 0; bits < 48; bits+=BIT_TO_BYTE)
        {
            lba48 |= ((uint64_t)ide_buf[ATA_IDENT_MAX_LBA_EXT + bits/8]) << bits;
        }
        vars.devices[device_index].size = lba48 & ((1ull << 48)-1);

        vars.devices[device_index].size_type = IDE_SIZE_LBA48;
    }
    else if(vars.devices[device_index].features & 0x200) // does support LBA
    {
        // Device uses CHS or 28-bit Addressing::
        uint32_t lba28 = 0;
        for (uint8_t bits = 0; bits < 28; bits+=BIT_TO_BYTE)
        {
            lba28 |= ((uint64_t)ide_buf[ATA_IDENT_MAX_LBA + bits/8]) << bits;
        }
        vars.devices[device_index].size = lba28 & ((1ull << 28)-1);

        vars.devices[device_index].size_type = IDE_SIZE_LBA28;
    }
    else // get using chs 
    {
        vars.devices[device_index].chs.heads     = *((uint8_t* )(ide_buf + ATA_IDENT_HEADS));
        vars.devices[device_index].chs.sectors   = *((uint8_t* )(ide_buf + ATA_IDENT_SECTORS));
        vars.devices[device_index].chs.cylinders = *((uint16_t*)(ide_buf + ATA_IDENT_CYLINDERS));

        vars.devices[device_index].size_type = IDE_SIZE_CHS;
    }
    // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
    uint16_t* ident_data = (uint16_t*) ide_buf;
    for (uint32_t i = 0; i < IDE_STR_INDEN_LENGTH / 2; ++i)
    {
        vars.devices[device_index].model[i * 2 + 0] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 8) & 0xFF;
        vars.devices[device_index].model[i * 2 + 1] = (ident_data[ATA_IDENT_MODEL/2 + i] >> 0) & 0xFF;
    }

    vars.devices[device_index].model[IDE_STR_INDEN_LENGTH] = '\0';
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
        if (vars.devices[i].present)
        {
            printf("Device #%d\n- %s\n- %lluMiB\n- Name:%s\n",
                i, 
                (const char* []){"ATA", "ATAPI"}[vars.devices[i].type],
                vars.devices[i].size * SECTOR_SIZE / STOR_1MiB, 
                vars.devices[i].model);
        }
    }   
}

static void ide_readwrite_sectors_sync(bool write, uintptr_t device, void* virt_buffer, uint64_t lba, uint64_t sector_count)
{
    uint8_t channel = vars.devices[device].channel;

    // Prepare a PRDT 
    vars.prd_entries[channel][0].phys_addr  = (uint32_t)get_phys_addr(virt_buffer);
    vars.prd_entries[channel][0].bytes_size = sector_count * SECTOR_SIZE;
    vars.prd_entries[channel][0].flags      = 0x8000; // MSB = last entry

    set_prdt_addr(channel, (uint32_t)get_phys_addr(&vars.prd_entries[channel][0]));


    // Clear BM status (IRQ + ERR)
    send_bm_status(channel, ATA_BM_STATUS_IRQ | ATA_BM_STATUS_ERR);

    // Select drive, program LBA + sector count
    uint8_t command = 0;
    if (vars.devices[device].size_type == IDE_SIZE_LBA48)
    {
        ide_set_lba48(channel, vars.devices[device].slave, lba, sector_count);
        command = write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;
    }
    else if (vars.devices[device].size_type == IDE_SIZE_LBA28)
    {
        ide_cmd_outb(channel, REG_OFF_SECCOUNT0, 0x00);
        ide_set_lba28(channel, vars.devices[device].slave, lba, sector_count);
        command = write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
    }
    else 
    {
        // not supported yet
        abort();
    }

    // Issue the ATA DMA command
    ide_send_cmd(channel, command);

    uint8_t bm_cmd = (write ? 0 : ATA_BM_CMD_READ) | ATA_BM_CMD_START;
    send_bm_cmd(channel, bm_cmd);

    int drv_status = ide_poll_status(channel, true);
    if (drv_status < 0) 
    {
        printf("Drive error before DMA\n");
        return;
    }
    
    // Stop DMA 
    send_bm_cmd(channel, ATA_BM_CMD_STOP);

    uint32_t bm_status  = recv_bm_status(channel);

    if ((bm_status & ATA_BM_STATUS_ERR) || (drv_status & ATA_STATUS_ERR)) 
    {
        abort();
    } 
}

__attribute__((aligned(4096))) uint8_t ide_data_arr[512];

void init_ide(pci_driver_t *driver)
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

    //test_ata();

    for (uint32_t i = 0; i < DEVICES; i++)
    {
        if (vars.devices[i].present)
        {
            ide_readwrite_sectors_sync(
                false, 
                i, 
                ide_data_arr, 
                0, 
                1
            );

            for (uint32_t i = 0; i < sizeof(ide_data_arr); i++) 
            {
                printf("%02X ", (uint32_t)ide_data_arr[i]);

                if ((i + 1) % 16 == 0) {
                    printf("\n"); // new row every 16 values
                }
            }
            
        }
    }
    
}