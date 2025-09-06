void cpu_halt()
{
    asm volatile(
        "hlt\n\t"
    ); 
}