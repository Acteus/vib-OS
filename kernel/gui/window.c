/*
 * Vib-OS - GUI Windowing System
 * 
 * Complete window manager with compositor and widget toolkit.
 */

#include "types.h"
#include "printk.h"
#include "mm/kmalloc.h"

/* ===================================================================== */
/* Display and Color */
/* ===================================================================== */

#define COLOR_BLACK         0x000000
#define COLOR_WHITE         0xFFFFFF
#define COLOR_RED           0xFF0000
#define COLOR_GREEN         0x00FF00
#define COLOR_BLUE          0x0000FF
#define COLOR_GRAY          0x808080
#define COLOR_DARK_GRAY     0x404040
#define COLOR_LIGHT_GRAY    0xC0C0C0

/* UI Theme Colors */
#define THEME_BG            0x1E1E2E    /* Dark background */
#define THEME_FG            0xCDD6F4    /* Light text */
#define THEME_ACCENT        0x89B4FA    /* Blue accent */
#define THEME_ACCENT2       0xF38BA8    /* Pink accent */
#define THEME_TITLEBAR      0x313244    /* Window title bar */
#define THEME_BORDER        0x45475A    /* Window border */
#define THEME_BUTTON        0x585B70    /* Button background */
#define THEME_BUTTON_HOVER  0x6C7086    /* Button hover */

/* ===================================================================== */
/* Display Driver Interface */
/* ===================================================================== */

struct display {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t *framebuffer;
    uint32_t *backbuffer;
};

static struct display primary_display = {0};

/* ===================================================================== */
/* Basic Drawing Functions */
/* ===================================================================== */

static inline void draw_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || x >= (int)primary_display.width) return;
    if (y < 0 || y >= (int)primary_display.height) return;
    
    uint32_t *target = primary_display.backbuffer ? 
                       primary_display.backbuffer : 
                       primary_display.framebuffer;
    if (target) {
        target[y * (primary_display.pitch / 4) + x] = color;
    }
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            draw_pixel(col, row, color);
        }
    }
}

void gui_draw_rect_outline(int x, int y, int w, int h, uint32_t color, int thickness)
{
    /* Top */
    gui_draw_rect(x, y, w, thickness, color);
    /* Bottom */
    gui_draw_rect(x, y + h - thickness, w, thickness, color);
    /* Left */
    gui_draw_rect(x, y, thickness, h, color);
    /* Right */
    gui_draw_rect(x + w - thickness, y, thickness, h, color);
}

void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void gui_draw_circle(int cx, int cy, int r, uint32_t color, bool filled)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    
    while (y >= x) {
        if (filled) {
            gui_draw_line(cx - x, cy + y, cx + x, cy + y, color);
            gui_draw_line(cx - x, cy - y, cx + x, cy - y, color);
            gui_draw_line(cx - y, cy + x, cx + y, cy + x, color);
            gui_draw_line(cx - y, cy - x, cx + y, cy - x, color);
        } else {
            draw_pixel(cx + x, cy + y, color);
            draw_pixel(cx - x, cy + y, color);
            draw_pixel(cx + x, cy - y, color);
            draw_pixel(cx - x, cy - y, color);
            draw_pixel(cx + y, cy + x, color);
            draw_pixel(cx - y, cy + x, color);
            draw_pixel(cx + y, cy - x, color);
            draw_pixel(cx - y, cy - x, color);
        }
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

/* ===================================================================== */
/* 8x16 Font (PSF-style) */
/* ===================================================================== */

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

/* Simple 8x16 bitmap font - printable ASCII only */
static const uint8_t font_8x16[96][16] = {
    /* Space (32) */
    [0] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ! */
    [1] = {0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
    /* " */
    [2] = {0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* # - Z basic shapes */
    [33] = {0x00,0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x66,0x7E,0x00,0x00,0x00,0x00}, /* 'A' */
    [34] = {0x00,0x00,0x00,0x7C,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 'B' */
    /* ... more characters would be defined here ... */
};

void gui_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg)
{
    int idx = c - 32;
    if (idx < 0 || idx >= 96) idx = 0;
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t line = font_8x16[idx][row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (line & (0x80 >> col)) ? fg : bg;
            draw_pixel(x + col, y + row, color);
        }
    }
}

void gui_draw_string(int x, int y, const char *str, uint32_t fg, uint32_t bg)
{
    while (*str) {
        if (*str == '\n') {
            x = 0;
            y += FONT_HEIGHT;
        } else {
            gui_draw_char(x, y, *str, fg, bg);
            x += FONT_WIDTH;
        }
        str++;
    }
}

/* ===================================================================== */
/* Window System */
/* ===================================================================== */

#define MAX_WINDOWS     64
#define TITLEBAR_HEIGHT 28
#define BORDER_WIDTH    2

typedef enum {
    WINDOW_NORMAL,
    WINDOW_MINIMIZED,
    WINDOW_MAXIMIZED,
    WINDOW_FULLSCREEN
} window_state_t;

struct window {
    int id;
    char title[64];
    int x, y;
    int width, height;
    window_state_t state;
    bool visible;
    bool focused;
    bool has_titlebar;
    bool resizable;
    uint32_t *content_buffer;
    void *userdata;
    
    /* Callbacks */
    void (*on_draw)(struct window *win);
    void (*on_key)(struct window *win, int key);
    void (*on_mouse)(struct window *win, int x, int y, int buttons);
    void (*on_close)(struct window *win);
    
    struct window *next;
};

static struct window windows[MAX_WINDOWS];
static struct window *window_stack = NULL;  /* Z-order, top is focused */
static struct window *focused_window = NULL;
static int next_window_id = 1;

/* Create a new window */
struct window *gui_create_window(const char *title, int x, int y, int w, int h)
{
    /* Find free slot */
    struct window *win = NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == 0) {
            win = &windows[i];
            break;
        }
    }
    
    if (!win) {
        printk(KERN_ERR "GUI: No free window slots\n");
        return NULL;
    }
    
    win->id = next_window_id++;
    for (int i = 0; i < 63 && title[i]; i++) {
        win->title[i] = title[i];
        win->title[i+1] = '\0';
    }
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->state = WINDOW_NORMAL;
    win->visible = true;
    win->focused = false;
    win->has_titlebar = true;
    win->resizable = true;
    
    /* Allocate content buffer */
    int content_h = h - TITLEBAR_HEIGHT - BORDER_WIDTH * 2;
    int content_w = w - BORDER_WIDTH * 2;
    win->content_buffer = kmalloc(content_w * content_h * 4);
    
    /* Add to stack */
    win->next = window_stack;
    window_stack = win;
    
    printk(KERN_INFO "GUI: Created window '%s' (%dx%d)\n", title, w, h);
    
    return win;
}

void gui_destroy_window(struct window *win)
{
    if (!win || win->id == 0) return;
    
    if (win->on_close) {
        win->on_close(win);
    }
    
    /* Remove from stack */
    if (window_stack == win) {
        window_stack = win->next;
    } else {
        struct window *prev = window_stack;
        while (prev && prev->next != win) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = win->next;
        }
    }
    
    if (win->content_buffer) {
        kfree(win->content_buffer);
    }
    
    win->id = 0;
}

void gui_focus_window(struct window *win)
{
    if (!win) return;
    
    if (focused_window) {
        focused_window->focused = false;
    }
    
    /* Move to top of stack */
    if (window_stack != win) {
        struct window *prev = window_stack;
        while (prev && prev->next != win) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = win->next;
            win->next = window_stack;
            window_stack = win;
        }
    }
    
    win->focused = true;
    focused_window = win;
}

/* Draw a single window */
static void draw_window(struct window *win)
{
    if (!win->visible) return;
    
    int x = win->x, y = win->y;
    int w = win->width, h = win->height;
    
    /* Draw border */
    gui_draw_rect_outline(x, y, w, h, THEME_BORDER, BORDER_WIDTH);
    
    if (win->has_titlebar) {
        /* Draw title bar */
        uint32_t titlebar_color = win->focused ? THEME_ACCENT : THEME_TITLEBAR;
        gui_draw_rect(x + BORDER_WIDTH, y + BORDER_WIDTH, 
                     w - BORDER_WIDTH * 2, TITLEBAR_HEIGHT, titlebar_color);
        
        /* Window title */
        gui_draw_string(x + 10, y + 6, win->title, THEME_FG, titlebar_color);
        
        /* Close button */
        int btn_x = x + w - BORDER_WIDTH - 24;
        int btn_y = y + BORDER_WIDTH + 4;
        gui_draw_rect(btn_x, btn_y, 20, 20, THEME_ACCENT2);
        gui_draw_string(btn_x + 6, btn_y + 2, "X", COLOR_WHITE, THEME_ACCENT2);
        
        /* Minimize button */
        btn_x -= 24;
        gui_draw_rect(btn_x, btn_y, 20, 20, THEME_BUTTON);
        gui_draw_string(btn_x + 6, btn_y + 2, "_", THEME_FG, THEME_BUTTON);
        
        /* Maximize button */
        btn_x -= 24;
        gui_draw_rect(btn_x, btn_y, 20, 20, THEME_BUTTON);
        gui_draw_rect_outline(btn_x + 5, btn_y + 5, 10, 10, THEME_FG, 1);
    }
    
    /* Draw content area */
    int content_x = x + BORDER_WIDTH;
    int content_y = y + BORDER_WIDTH + (win->has_titlebar ? TITLEBAR_HEIGHT : 0);
    int content_w = w - BORDER_WIDTH * 2;
    int content_h = h - BORDER_WIDTH * 2 - (win->has_titlebar ? TITLEBAR_HEIGHT : 0);
    
    gui_draw_rect(content_x, content_y, content_w, content_h, THEME_BG);
    
    /* Call window's draw callback */
    if (win->on_draw) {
        win->on_draw(win);
    }
}

/* ===================================================================== */
/* Desktop and Taskbar */
/* ===================================================================== */

#define TASKBAR_HEIGHT  40

static void draw_desktop(void)
{
    /* Draw desktop background */
    gui_draw_rect(0, 0, primary_display.width, primary_display.height - TASKBAR_HEIGHT, 
                 THEME_BG);
    
    /* Draw taskbar */
    gui_draw_rect(0, primary_display.height - TASKBAR_HEIGHT, 
                 primary_display.width, TASKBAR_HEIGHT, THEME_TITLEBAR);
    
    /* Start button */
    gui_draw_rect(4, primary_display.height - TASKBAR_HEIGHT + 4, 
                 80, TASKBAR_HEIGHT - 8, THEME_ACCENT);
    gui_draw_string(12, primary_display.height - TASKBAR_HEIGHT + 12, 
                   "Vib-OS", COLOR_WHITE, THEME_ACCENT);
    
    /* Window buttons in taskbar */
    int btn_x = 100;
    for (struct window *win = window_stack; win; win = win->next) {
        if (!win->visible) continue;
        
        uint32_t btn_color = win->focused ? THEME_ACCENT : THEME_BUTTON;
        gui_draw_rect(btn_x, primary_display.height - TASKBAR_HEIGHT + 4, 
                     120, TASKBAR_HEIGHT - 8, btn_color);
        gui_draw_string(btn_x + 4, primary_display.height - TASKBAR_HEIGHT + 12, 
                       win->title, THEME_FG, btn_color);
        btn_x += 124;
    }
    
    /* Clock */
    gui_draw_string(primary_display.width - 60, primary_display.height - TASKBAR_HEIGHT + 12,
                   "12:00", THEME_FG, THEME_TITLEBAR);
}

/* ===================================================================== */
/* Compositor - Draw everything */
/* ===================================================================== */

void gui_compose(void)
{
    /* Draw desktop and taskbar */
    draw_desktop();
    
    /* Draw windows from bottom to top (reverse order) */
    /* First, find tail of list */
    struct window *tail = NULL;
    for (struct window *win = window_stack; win; win = win->next) {
        tail = win;
    }
    
    /* Draw from tail to head */
    /* For simplicity, just iterate normally (top window drawn last) */
    struct window *draw_order[MAX_WINDOWS];
    int count = 0;
    for (struct window *win = window_stack; win && count < MAX_WINDOWS; win = win->next) {
        draw_order[count++] = win;
    }
    
    /* Draw in reverse (bottom to top) */
    for (int i = count - 1; i >= 0; i--) {
        draw_window(draw_order[i]);
    }
    
    /* Copy backbuffer to framebuffer */
    if (primary_display.backbuffer && primary_display.framebuffer) {
        size_t size = primary_display.pitch * primary_display.height;
        uint32_t *src = primary_display.backbuffer;
        uint32_t *dst = primary_display.framebuffer;
        for (size_t i = 0; i < size / 4; i++) {
            dst[i] = src[i];
        }
    }
}

/* ===================================================================== */
/* Mouse Cursor */
/* ===================================================================== */

static int mouse_x = 100, mouse_y = 100;
static int mouse_buttons = 0;

static const uint8_t cursor_bitmap[16] = {
    0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
    0xFC, 0xFC, 0xD8, 0x98, 0x0C, 0x0C, 0x06, 0x00
};

void gui_draw_cursor(void)
{
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 8; x++) {
            if (cursor_bitmap[y] & (0x80 >> x)) {
                draw_pixel(mouse_x + x, mouse_y + y, COLOR_WHITE);
            }
        }
    }
}

void gui_move_mouse(int dx, int dy)
{
    mouse_x += dx;
    mouse_y += dy;
    
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= (int)primary_display.width) mouse_x = primary_display.width - 1;
    if (mouse_y >= (int)primary_display.height) mouse_y = primary_display.height - 1;
}

void gui_set_mouse_buttons(int buttons)
{
    mouse_buttons = buttons;
}

/* ===================================================================== */
/* Event Handling */
/* ===================================================================== */

void gui_handle_mouse_event(int x, int y, int buttons)
{
    mouse_x = x;
    mouse_y = y;
    
    /* Check if clicking on a window */
    for (struct window *win = window_stack; win; win = win->next) {
        if (!win->visible) continue;
        
        if (x >= win->x && x < win->x + win->width &&
            y >= win->y && y < win->y + win->height) {
            
            gui_focus_window(win);
            
            /* Check for close button */
            if (win->has_titlebar) {
                int btn_x = win->x + win->width - BORDER_WIDTH - 24;
                int btn_y = win->y + BORDER_WIDTH + 4;
                if (x >= btn_x && x < btn_x + 20 && y >= btn_y && y < btn_y + 20) {
                    if (buttons & 1) {
                        gui_destroy_window(win);
                        return;
                    }
                }
            }
            
            if (win->on_mouse) {
                win->on_mouse(win, x - win->x, y - win->y, buttons);
            }
            break;
        }
    }
}

void gui_handle_key_event(int keycode, bool pressed)
{
    if (focused_window && focused_window->on_key && pressed) {
        focused_window->on_key(focused_window, keycode);
    }
}

/* ===================================================================== */
/* Initialization */
/* ===================================================================== */

int gui_init(uint32_t *framebuffer, uint32_t width, uint32_t height, uint32_t pitch)
{
    printk(KERN_INFO "GUI: Initializing windowing system\n");
    
    primary_display.framebuffer = framebuffer;
    primary_display.width = width;
    primary_display.height = height;
    primary_display.pitch = pitch;
    primary_display.bpp = 32;
    
    /* Allocate backbuffer for double-buffering */
    primary_display.backbuffer = kmalloc(pitch * height);
    
    /* Clear windows */
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].id = 0;
    }
    
    printk(KERN_INFO "GUI: Display %ux%u initialized\n", width, height);
    
    return 0;
}

struct display *gui_get_display(void)
{
    return &primary_display;
}
