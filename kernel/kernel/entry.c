
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

    virt_kernel_main();

    debug_print_str("\"What the hell nah\"");

    while (1);
}