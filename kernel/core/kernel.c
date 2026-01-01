#include "arch/i386/core/paging.h"
#include "arch/i386/memory/paging_utils.h"
#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/disk.h"
#include "filesystem/drivers/Ext2/features.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "firmware/pci/pci.h"
#include "drivers/storage.h"
#include "memory/virt/virt_alloc.h"
#include "memory/virt/virt_region.h"
#include "services/block/device.h"
#include "services/block/manager.h"
#include "services/block/request.h"
#include "vfs/core/errors.h"
#include "vfs/core/mount.h"
#include "vfs/core/path.h"
#include "vfs/core/superblock.h"
#include "vfs/core/vfs.h"
#include "vfs/inode/inode.h"
#include <stdbool.h>
#include <stdio.h>
#include <core/defs.h>
#include <drivers/tty.h>
#include <firmware/acpi/acpi.h>
#include <kernel/boot/boot_data.h>
#include <kernel/devices/keyboard.h>
#include <kernel/devices/int_timer.h>
#include <kernel/interrupts/irq.h>
#include <kernel/core/cpu.h>
#include <string.h>

void init_memory(boot_data_t* data);

static bool dummy_printing_time;

static void dummy_key_handler(raw_key_event_t key_event)
{
    if (key_event.pressed == false)
    {
        return;
    }    

    switch (key_event.code) {
        // digits
        case KEYCODE_0: printf("0"); break;
        case KEYCODE_1: printf("1"); break;
        case KEYCODE_2: printf("2"); break;
        case KEYCODE_3: printf("3"); break;
        case KEYCODE_4: printf("4"); break;
        case KEYCODE_5: printf("5"); break;
        case KEYCODE_6: printf("6"); break;
        case KEYCODE_7: printf("7"); break;
        case KEYCODE_8: printf("8"); break;
        case KEYCODE_9: printf("9"); break;

        // letters
        case KEYCODE_A: printf("a"); break;
        case KEYCODE_B: printf("b"); break;
        case KEYCODE_C: printf("c"); break;
        case KEYCODE_D: printf("d"); break;
        case KEYCODE_E: printf("e"); break;
        case KEYCODE_F: printf("f"); break;
        case KEYCODE_G: printf("g"); break;
        case KEYCODE_H: printf("h"); break;
        case KEYCODE_I: printf("i"); break;
        case KEYCODE_J: printf("j"); break;
        case KEYCODE_K: printf("k"); break;
        case KEYCODE_L: printf("l"); break;
        case KEYCODE_M: printf("m"); break;
        case KEYCODE_N: printf("n"); break;
        case KEYCODE_O: printf("o"); break;
        case KEYCODE_P: printf("p"); break;
        case KEYCODE_Q: printf("q"); break;
        case KEYCODE_R: printf("r"); break;
        case KEYCODE_S: printf("s"); break;
        case KEYCODE_T: printf("t"); break;
        case KEYCODE_U: printf("u"); break;
        case KEYCODE_V: printf("v"); break;
        case KEYCODE_W: printf("w"); break;
        case KEYCODE_X: printf("x"); break;
        case KEYCODE_Y: printf("y"); break;
        case KEYCODE_Z: printf("z"); break;
        case KEYCODE_ENTER: printf("\n"); break;
        case KEYCODE_SPACE: printf(" "); break;
        case KEYCODE_MINUS: dummy_printing_time = !dummy_printing_time; break;

        default:
            break;
    }
}

static void dummy_time_event(int_timer_event_t time_event)
{
    if (dummy_printing_time == false)
        return;
    printf(
        "Tick #%llu Time:%llu [hz:%d]\n", 
        time_event.tick_count, 
        time_event.timestamp_ns,
        time_event.frequency_hz
    );
}

void kernel_main(boot_data_t* boot_data)
{
    cpu_init();

	terminal_initialize();

    // Setup interrupt handlers
	init_memory(boot_data);

    // Setup firmware
    init_pci();
    init_acpi();

    // basic drivers
    init_int_timer(10, dummy_time_event);
    init_keyboard(dummy_key_handler);
    init_storage();

    init_block_manager();

	irq_enable();

    block_device_t* block_dev = block_manager_get_device(2);
    init_vfs(block_dev);
    vfs_mount_map_t* map = vfs_get_mount_data();
    dir_entry_t *result = NULL;
    i32 res = vfs_lookup_path(map, "/file.txt", &result);
    inode_t* inode = result->inode;
    
    if (result)
    {
        for (usize i = 0; i < inode->size; i += sizeof(u8)) 
        {
            u8 ch;
            inode->ops->read(inode, &ch, sizeof(ch), i);
            printf("%c", ch);
        }
    }

    while(1) cpu_halt();
}