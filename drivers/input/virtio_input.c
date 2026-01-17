/*
 * Vib-OS - Virtio Input Driver
 * 
 * Handles keyboard and mouse input from virtio-input devices.
 */

#include "types.h"
#include "printk.h"

/* ===================================================================== */
/* Virtio Input Device */
/* ===================================================================== */

/* Virtio MMIO base for input devices (after GPU at 0x0A003000) */
#define VIRTIO_INPUT_BASE   0x0A003200UL

/* Virtio register offsets */
#define VIRTIO_MMIO_MAGIC           0x000
#define VIRTIO_MMIO_VERSION         0x004
#define VIRTIO_MMIO_DEVICE_ID       0x008
#define VIRTIO_MMIO_VENDOR_ID       0x00C
#define VIRTIO_MMIO_STATUS          0x070

#define VIRTIO_MAGIC            0x74726976
#define VIRTIO_DEV_INPUT        18

/* Input event structure */
struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

/* Event types */
#define EV_SYN      0x00
#define EV_KEY      0x01
#define EV_REL      0x02  /* Relative movement (mouse) */
#define EV_ABS      0x03  /* Absolute position */

/* Key codes */
#define KEY_ESC     1
#define KEY_1       2
#define KEY_2       3
#define KEY_Q       16
#define KEY_W       17
#define KEY_E       18
#define KEY_A       30
#define KEY_S       31
#define KEY_D       32
#define KEY_ENTER   28
#define KEY_SPACE   57

/* Mouse relative codes */
#define REL_X       0x00
#define REL_Y       0x01

/* ===================================================================== */
/* Input State */
/* ===================================================================== */

static struct {
    int mouse_x;
    int mouse_y;
    int mouse_buttons;
    bool initialized;
} input_state = {
    .mouse_x = 512,
    .mouse_y = 384,
    .mouse_buttons = 0,
    .initialized = false
};

/* Keyboard to ASCII mapping (simplified) */
static const char keycode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Callback function pointers */
static void (*key_callback)(int key) = 0;
static void (*mouse_callback)(int x, int y, int buttons) = 0;

/* ===================================================================== */
/* Input API */
/* ===================================================================== */

void input_set_key_callback(void (*callback)(int key))
{
    key_callback = callback;
}

void input_set_mouse_callback(void (*callback)(int x, int y, int buttons))
{
    mouse_callback = callback;
}

void input_get_mouse(int *x, int *y, int *buttons)
{
    if (x) *x = input_state.mouse_x;
    if (y) *y = input_state.mouse_y;
    if (buttons) *buttons = input_state.mouse_buttons;
}

/* Process a single input event */
void input_process_event(uint16_t type, uint16_t code, uint32_t value)
{
    if (type == EV_KEY && value == 1) {
        /* Key press */
        if (code < 128) {
            char ascii = keycode_to_ascii[code];
            if (ascii && key_callback) {
                key_callback(ascii);
            }
        }
    }
    else if (type == EV_REL) {
        /* Relative mouse movement */
        if (code == REL_X) {
            input_state.mouse_x += (int32_t)value;
            if (input_state.mouse_x < 0) input_state.mouse_x = 0;
            if (input_state.mouse_x > 1023) input_state.mouse_x = 1023;
        }
        else if (code == REL_Y) {
            input_state.mouse_y += (int32_t)value;
            if (input_state.mouse_y < 0) input_state.mouse_y = 0;
            if (input_state.mouse_y > 767) input_state.mouse_y = 767;
        }
        
        if (mouse_callback) {
            mouse_callback(input_state.mouse_x, input_state.mouse_y, 
                          input_state.mouse_buttons);
        }
    }
    else if (type == EV_ABS) {
        /* Absolute mouse position */
        if (code == 0) {
            input_state.mouse_x = value;
        }
        else if (code == 1) {
            input_state.mouse_y = value;
        }
        
        if (mouse_callback) {
            mouse_callback(input_state.mouse_x, input_state.mouse_y,
                          input_state.mouse_buttons);
        }
    }
}

/* ===================================================================== */
/* Polling (for simple implementation without interrupts) */
/* ===================================================================== */

/* For now, input comes through the terminal's serial interface */
/* This is a placeholder for full virtio-input implementation */

void input_poll(void)
{
    /* Read from UART for keyboard input (simpler than full virtio) */
    extern int uart_getc_nonblock(void);
    
    int c = uart_getc_nonblock();
    if (c >= 0 && key_callback) {
        key_callback(c);
    }
}

/* ===================================================================== */
/* Initialization */
/* ===================================================================== */

int input_init(void)
{
    printk(KERN_INFO "INPUT: Initializing input system\n");
    
    input_state.mouse_x = 512;
    input_state.mouse_y = 384;
    input_state.mouse_buttons = 0;
    input_state.initialized = true;
    
    printk(KERN_INFO "INPUT: Ready, mouse at (%d, %d)\n",
           input_state.mouse_x, input_state.mouse_y);
    
    return 0;
}
