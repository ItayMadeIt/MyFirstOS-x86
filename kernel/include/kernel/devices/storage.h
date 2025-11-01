#ifndef __ARCH_STORAGE_H__
#define __ARCH_STORAGE_H__

#include "core/num_defs.h"

#define SECTOR_SIZE 512

u64 stor_get_total_lba  (usize_ptr dev_id);  
u64 stor_get_sector_size(usize_ptr dev_id); 
u64 stor_get_size_bytes (usize_ptr dev_id); 

#endif // __STORAGE_H__