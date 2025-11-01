#include <kernel/devices/keyboard.h>
#include <kernel/interrupts/irq.h>
#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/drivers/pic/ps2_key.h>
#include <arch/i386/drivers/io/io.h>

extern void ps2_handler();

u8 ps2_read_data(void)
{
    return inb(PS2_DATA);;
}
void ps2_send_eoi(void)
{    
    pic_send_eoi_vector(PS2_IRQ_VECTOR);
}

static u16 ps2_kc_map[128];
static u16 ps2_kc_map_ext[128];

void unmask_ps2_key()
{
    // unmask IRQ1
    io_wait();
    outb(PIC1_DATA, inb(PIC1_DATA) & ~0x02);
}

void ps2_init()
{
    for (u16 i = 0; i < 128; i++)
    {
        ps2_kc_map[i] = KEYCODE_NONE;
        ps2_kc_map_ext[i] = KEYCODE_NONE;
    }
    
    // letters
    ps2_kc_map[0x1E] = KEYCODE_A;
    ps2_kc_map[0x30] = KEYCODE_B;
    ps2_kc_map[0x2E] = KEYCODE_C;
    ps2_kc_map[0x20] = KEYCODE_D;
    ps2_kc_map[0x12] = KEYCODE_E;
    ps2_kc_map[0x21] = KEYCODE_F;
    ps2_kc_map[0x22] = KEYCODE_G;
    ps2_kc_map[0x23] = KEYCODE_H;
    ps2_kc_map[0x17] = KEYCODE_I;
    ps2_kc_map[0x24] = KEYCODE_J;
    ps2_kc_map[0x25] = KEYCODE_K;
    ps2_kc_map[0x26] = KEYCODE_L;
    ps2_kc_map[0x32] = KEYCODE_M;
    ps2_kc_map[0x31] = KEYCODE_N;
    ps2_kc_map[0x18] = KEYCODE_O;
    ps2_kc_map[0x19] = KEYCODE_P;
    ps2_kc_map[0x10] = KEYCODE_Q;
    ps2_kc_map[0x13] = KEYCODE_R;
    ps2_kc_map[0x1F] = KEYCODE_S;
    ps2_kc_map[0x14] = KEYCODE_T;
    ps2_kc_map[0x16] = KEYCODE_U;
    ps2_kc_map[0x2F] = KEYCODE_V;
    ps2_kc_map[0x11] = KEYCODE_W;
    ps2_kc_map[0x2D] = KEYCODE_X;
    ps2_kc_map[0x15] = KEYCODE_Y;
    ps2_kc_map[0x2C] = KEYCODE_Z;

    // numbers
    ps2_kc_map[0x02] = KEYCODE_1;
    ps2_kc_map[0x03] = KEYCODE_2;
    ps2_kc_map[0x04] = KEYCODE_3;
    ps2_kc_map[0x05] = KEYCODE_4;
    ps2_kc_map[0x06] = KEYCODE_5;
    ps2_kc_map[0x07] = KEYCODE_6;
    ps2_kc_map[0x08] = KEYCODE_7;
    ps2_kc_map[0x09] = KEYCODE_8;
    ps2_kc_map[0x0A] = KEYCODE_9;
    ps2_kc_map[0x0B] = KEYCODE_0;

    // symbols
    ps2_kc_map[0x0C] = KEYCODE_MINUS;
    ps2_kc_map[0x0D] = KEYCODE_EQUAL;
    ps2_kc_map[0x29] = KEYCODE_BACKTICK;
    ps2_kc_map[0x2B] = KEYCODE_BACKSLASH;
    ps2_kc_map[0x1A] = KEYCODE_BRACKET_LEFT;
    ps2_kc_map[0x1B] = KEYCODE_BRACKET_RIGHT;
    ps2_kc_map[0x27] = KEYCODE_SEMICOLON;
    ps2_kc_map[0x28] = KEYCODE_APOSTROPHE;
    ps2_kc_map[0x33] = KEYCODE_COMMA;
    ps2_kc_map[0x34] = KEYCODE_DOT;
    ps2_kc_map[0x35] = KEYCODE_SLASH;

    // modifiers
    ps2_kc_map[0x2A] = KEYCODE_SHIFT_LEFT;
    ps2_kc_map[0x36] = KEYCODE_SHIFT_RIGHT;
    ps2_kc_map[0x1D] = KEYCODE_CTRL_LEFT;
    ps2_kc_map[0x38] = KEYCODE_ALT_LEFT;
    ps2_kc_map[0x3A] = KEYCODE_CAPS;
    ps2_kc_map[0x45] = KEYCODE_NUMLOCK;
    ps2_kc_map[0x46] = KEYCODE_SCROLLLOCK;

    // Moving
    ps2_kc_map[0x39] = KEYCODE_SPACE;
    ps2_kc_map[0x0F] = KEYCODE_TAB;
    ps2_kc_map[0x1C] = KEYCODE_ENTER;
    ps2_kc_map[0x0E] = KEYCODE_BACKSPACE;
    ps2_kc_map[0x01] = KEYCODE_ESCAPE;

    // Functions
    ps2_kc_map[0x3B] = KEYCODE_F1;
    ps2_kc_map[0x3C] = KEYCODE_F2;
    ps2_kc_map[0x3D] = KEYCODE_F3;
    ps2_kc_map[0x3E] = KEYCODE_F4;
    ps2_kc_map[0x3F] = KEYCODE_F5;
    ps2_kc_map[0x40] = KEYCODE_F6;
    ps2_kc_map[0x41] = KEYCODE_F7;
    ps2_kc_map[0x42] = KEYCODE_F8;
    ps2_kc_map[0x43] = KEYCODE_F9;
    ps2_kc_map[0x44] = KEYCODE_F10;
    ps2_kc_map[0x57] = KEYCODE_F11;
    ps2_kc_map[0x58] = KEYCODE_F12;

    // Numpad 
    ps2_kc_map[0x52] = KEYCODE_NUMPAD_0;   // also Ins when NumLock off
    ps2_kc_map[0x4F] = KEYCODE_NUMPAD_1;   // End
    ps2_kc_map[0x50] = KEYCODE_NUMPAD_2;   // Down
    ps2_kc_map[0x51] = KEYCODE_NUMPAD_3;   // PgDn
    ps2_kc_map[0x4B] = KEYCODE_NUMPAD_4;   // Left
    ps2_kc_map[0x4C] = KEYCODE_NUMPAD_5;
    ps2_kc_map[0x4D] = KEYCODE_NUMPAD_6;   // Right
    ps2_kc_map[0x47] = KEYCODE_NUMPAD_7;   // Home
    ps2_kc_map[0x48] = KEYCODE_NUMPAD_8;   // Up
    ps2_kc_map[0x49] = KEYCODE_NUMPAD_9;   // PgUp
    ps2_kc_map[0x53] = KEYCODE_NUMPAD_DOT;
    ps2_kc_map[0x37] = KEYCODE_NUMPAD_STAR;
    ps2_kc_map[0x4A] = KEYCODE_NUMPAD_MINUS;
    ps2_kc_map[0x4E] = KEYCODE_NUMPAD_PLUS;

    // extended set
    ps2_kc_map_ext[0x1C] = KEYCODE_NUMPAD_ENTER;
    ps2_kc_map_ext[0x35] = KEYCODE_NUMPAD_SLASH;

    ps2_kc_map_ext[0x1D] = KEYCODE_CTRL_RIGHT;
    ps2_kc_map_ext[0x38] = KEYCODE_ALT_RIGHT;

    ps2_kc_map_ext[0x52] = KEYCODE_INSERT;
    ps2_kc_map_ext[0x53] = KEYCODE_DELETE;
    ps2_kc_map_ext[0x47] = KEYCODE_HOME;
    ps2_kc_map_ext[0x4F] = KEYCODE_END;
    ps2_kc_map_ext[0x49] = KEYCODE_PAGE_UP;
    ps2_kc_map_ext[0x51] = KEYCODE_PAGE_DOWN;

    ps2_kc_map_ext[0x48] = KEYCODE_ARROW_UP;
    ps2_kc_map_ext[0x50] = KEYCODE_ARROW_DOWN;
    ps2_kc_map_ext[0x4B] = KEYCODE_ARROW_LEFT;
    ps2_kc_map_ext[0x4D] = KEYCODE_ARROW_RIGHT;

    ps2_kc_map_ext[0x5B] = KEYCODE_WIN_LEFT;
    ps2_kc_map_ext[0x5C] = KEYCODE_WIN_RIGHT;
    ps2_kc_map_ext[0x5D] = KEYCODE_MENU;
}

key_event_cb_t key_event_callback;

void ps2_key_dispatch(irq_frame_t* frame)
{
    (void)frame;

    static bool extended = false;

    u8 ps2_scancode = ps2_read_data();

    // extended key
    if (ps2_scancode == 0xE0)
    {
        extended = true;
        io_wait();
        ps2_send_eoi();

        return;
    }
    if (ps2_scancode == 0xE1) 
    { 
        io_wait();
        ps2_send_eoi();
    } 
    
    bool released    = ps2_scancode & 0x80;
    u8 code     = ps2_scancode & 0x7F;
    u16 keycode = KEYCODE_NONE;

    if (extended) 
    {
        keycode = ps2_kc_map_ext[code];
        extended = false;
    } 
    else 
    {
        keycode = ps2_kc_map[code];
    }

    raw_key_event_t key_event = {
        .code = keycode,
        .pressed = !released,
    };

    key_event_callback(key_event);

    io_wait();
    ps2_send_eoi();
}