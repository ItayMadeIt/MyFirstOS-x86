
void virt_kernel_main()
{
    asm volatile (
        "mov $kernel_main, %%eax\n\t"
        "add $0xBFF00000, %%eax\n\t"
        "call *%%eax\n\t"
        : 
        : 
        : "eax", "memory"
    );
}


void entry_main()
{
    setup_gdt();
    setup_idt();
    setup_paging();
    
    setup_isr();

    asm volatile (
        "mov %0, %%edx\n\t"
        "div %%edx\n\t"
        :
        : "r"(0)
        : "eax", "edx"
    );

    virt_kernel_main();
}