OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(_start)

/* Bases */
BOOT_PHYS_BASE = 0x00100000;   /* physical load base */
KERNEL_VIRT_BASE = 0xC0000000;   /* virtual base */

SECTIONS
{
  /* ---- Boot-only sections (identity mapped) ---- */
  . = BOOT_PHYS_BASE;
  linker_kernel_begin = BOOT_PHYS_BASE;

  .text.boot ALIGN(4K) : AT(ADDR(.text.boot)) {
    KEEP(*(.multiboot .multiboot.*))
    KEEP(*(.text.boot*))
  }

  .data.boot ALIGN(4K) : AT(ADDR(.data.boot)) {
    KEEP(*(.data.boot*))
  }

  .bss.boot ALIGN(4K) (NOLOAD) : AT(ADDR(.bss.boot)) {
    KEEP(*(.bss.boot*))
  }

  __boot_phys_end = .;
  __kernel_phys_base = ALIGN(__boot_phys_end, 4K);
  
  /* VMA shift so first kernel symbol is already “after boot” virtually */
  __kernel_virt_base = KERNEL_VIRT_BASE + (__kernel_phys_base - BOOT_PHYS_BASE);

  /* ---- kernel proper (high, shifted VMA) ---- */
  . = __kernel_virt_base;

  __kernel_start = .;

  .text ALIGN(4K) : AT(ADDR(.text) - __kernel_virt_base + __kernel_phys_base) {
    *(.text .text.*)
  }
  __text_lma = LOADADDR(.text);

  .rodata ALIGN(4K) : AT(ADDR(.rodata) - __kernel_virt_base + __kernel_phys_base) {
    *(.rodata .rodata.*)
  }
  __rodata_lma = LOADADDR(.rodata);

  .data ALIGN(4K) : AT(ADDR(.data) - __kernel_virt_base + __kernel_phys_base) {
    *(.data .data.*)
  }
  __data_lma = LOADADDR(.data);

  .bss ALIGN(4K) (NOLOAD) : AT(ADDR(.bss) - __kernel_virt_base + __kernel_phys_base) {
    *(COMMON)
    *(.bss .bss.*)
    . = ALIGN(16);
    kernel_stack_bottom = .;
    . += 0x4000;            /* 16 KiB */
    kernel_stack_top = .;
  }
  __bss_lma = LOADADDR(.bss);

  . = ALIGN(4096);
  __image_end = .;
  __kernel_end = .;
  
  __kernel_end_phys = __kernel_end - __va_pa_off;
  linker_kernel_end = __kernel_end_phys;
}

/* Convenience phys aliases */
PROVIDE(__kernel_virt_base_sym = __kernel_virt_base);
PROVIDE(__kernel_phys_base_sym = __kernel_phys_base);
PROVIDE(__va_pa_off = __kernel_virt_base - __kernel_phys_base);

PROVIDE(entry_main_PHYSSYM  = entry_main  - __kernel_phys_base);
PROVIDE(kernel_main_PHYSSYM = kernel_main - __kernel_phys_base);
PROVIDE(__kernel_start_phys = __kernel_start - __kernel_phys_base);
PROVIDE(__kernel_end_phys   = __kernel_end   - __kernel_phys_base);
