#include <stdbool.h>
#include <stdint.h>

struct boot_data;
typedef struct boot_data boot_data_t;

bool boot_has_memory(const boot_data_t* boot_data);
uintptr_t get_max_memory(const boot_data_t* boot_data);

void boot_foreach_free_page(const boot_data_t* boot_data, void(*callback)(void* pa));

void boot_foreach_free_page_region(
    const boot_data_t* boot_data, void(*callback) (void* from_pa,void* to_pa));

void boot_foreach_reserved_region(const boot_data_t* boot_data,
                                void(*callback)(void* from_pa, void* to_pa));

void boot_foreach_page_struct(void(*callback)(void* pa));