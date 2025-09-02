#define EARLY_TEXT_SECTION __attribute__((section(".text.boot"), noinline, used))
#define EARLY_BSS_SECTION  __attribute__((section(".bss.boot")))
#define EARLY_DATA_SECTION  __attribute__((section(".data.boot")))

EARLY_TEXT_SECTION
static inline void e_sti()
{
    asm volatile("sti");
}
EARLY_TEXT_SECTION
static inline void e_cli()
{
    asm volatile("cli");
}
