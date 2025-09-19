#ifndef __ARCH_STORAGE_H__
#define __ARCH_STORAGE_H__

#include <stdint.h>

#define SECTOR_SIZE 512

uint64_t stor_get_total_lba  (uintptr_t dev_id);  
uint64_t stor_get_sector_size(uintptr_t dev_id); 
uint64_t stor_get_size_bytes (uintptr_t dev_id); 

#endif // __STORAGE_H__