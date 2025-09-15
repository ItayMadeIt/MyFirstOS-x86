#include <stdint.h>

#define SECTOR_SIZE 512

void init_stor();
uint64_t stor_get_total_lba();  
uint64_t stor_get_block_size(); 
uint64_t stor_get_size_bytes(); 

void stor_write(uint64_t lba_begin, uint64_t lba_count, const uint8_t* data);
void stor_read (uint64_t lba_begin, uint64_t lba_count, uint8_t* data);