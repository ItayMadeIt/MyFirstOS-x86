#include <drivers/io.h>
#include <core/idt.h>
#include <stdbool.h>

#define PS2_DATA 0x60
#define PS2_CMD 0x64

void IRQ1_handler();

char* KEYCODES[128];

void init_keycodes()
{
    for (uint16_t i = 0; i < 128; i++)
    {
        KEYCODES[i] = "";
    }
    
    KEYCODES[0x1E] = "A";
    KEYCODES[0x30] = "B";
    KEYCODES[0x2E] = "C";
    KEYCODES[0x20] = "D";
    KEYCODES[0x12] = "E";
    KEYCODES[0x21] = "F";
    KEYCODES[0x22] = "G";
    KEYCODES[0x23] = "H";
    KEYCODES[0x17] = "I";
    KEYCODES[0x24] = "J";
    KEYCODES[0x25] = "K";
    KEYCODES[0x26] = "L";
    KEYCODES[0x32] = "M";
    KEYCODES[0x31] = "N";
    KEYCODES[0x18] = "O";
    KEYCODES[0x19] = "P";
    KEYCODES[0x10] = "Q";
    KEYCODES[0x13] = "R";
    KEYCODES[0x1F] = "S";
    KEYCODES[0x14] = "T";
    KEYCODES[0x16] = "U";
    KEYCODES[0x2F] = "V";
    KEYCODES[0x11] = "W";
    KEYCODES[0x2D] = "X";
    KEYCODES[0x15] = "Y";
    KEYCODES[0x2C] = "Z";

    KEYCODES[0x02] = "1";
    KEYCODES[0x03] = "2";
    KEYCODES[0x04] = "3";
    KEYCODES[0x05] = "4";
    KEYCODES[0x06] = "5";
    KEYCODES[0x07] = "6";
    KEYCODES[0x08] = "7";
    KEYCODES[0x09] = "8";
    KEYCODES[0x0A] = "9";
    KEYCODES[0x0B] = "0";
    
    KEYCODES[0x39] = " ";
    KEYCODES[0x1C] = "\n";
}

void setup_ps2()
{
    init_keycodes();

    set_idt_entry(0x21, IRQ1_handler, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
}

void key_callback()
{
    bool extended = false;
    uint16_t key = inb(PS2_DATA);

    // Extended key
    if (key == 0xE0)
    {
        extended = true;
        io_wait();
        key = inb(PS2_DATA);
    }

    if (KEYCODES[key & 0x7F] && !(key & 0x80))
    {
        //debug_print_str(key & 0x80 ? "Release: " : "Pressed: ");
        debug_print_str(KEYCODES[key & 0x7F]);
        //debug_print_str("\n");
    }

    io_wait();
    PIC_sendEOI(0x21 - 0x20);
}