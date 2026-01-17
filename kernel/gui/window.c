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

/* UI Theme Colors - macOS Inspired */
#define THEME_BG            0x1E1E2E    /* Dark background */
#define THEME_FG            0xCDD6F4    /* Light text */
#define THEME_ACCENT        0x007AFF    /* macOS blue */
#define THEME_ACCENT2       0xF38BA8    /* Pink accent */
#define THEME_TITLEBAR      0x3C3C3C    /* Window title bar */
#define THEME_TITLEBAR_INACTIVE 0x4A4A4A
#define THEME_BORDER        0x45475A    /* Window border */
#define THEME_BUTTON        0x585B70    /* Button background */
#define THEME_BUTTON_HOVER  0x6C7086    /* Button hover */

/* macOS Traffic Light Colors */
#define COLOR_BTN_CLOSE     0xFF5F57    /* Red */
#define COLOR_BTN_MINIMIZE  0xFFBD2E    /* Yellow */
#define COLOR_BTN_ZOOM      0x28C840    /* Green */

/* Menu Bar */
#define COLOR_MENU_BG       0x2D2D2D    /* Dark menu bar */
#define COLOR_MENU_TEXT     0xFFFFFF    /* White text */
#define MENU_BAR_HEIGHT     28

/* Dock */
#define COLOR_DOCK_BG       0x3C3C3C    /* Dark dock */
#define COLOR_DOCK_BORDER   0x5C5C5C
#define DOCK_HEIGHT         70

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
/* 8x16 Font - use external complete font */
/* ===================================================================== */

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

/* External font data from font.c - 256 characters */
extern const uint8_t font_data[256][16];

void gui_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg)
{
    unsigned char idx = (unsigned char)c;
    const uint8_t *glyph = font_data[idx];
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t line = glyph[row];
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

/* Draw a filled circle (for traffic light buttons) */
static void draw_circle(int cx, int cy, int r, uint32_t color)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                draw_pixel(cx + x, cy + y, color);
            }
        }
    }
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
        /* Draw title bar - macOS style gray */
        uint32_t titlebar_color = win->focused ? THEME_TITLEBAR : THEME_TITLEBAR_INACTIVE;
        gui_draw_rect(x + BORDER_WIDTH, y + BORDER_WIDTH, 
                     w - BORDER_WIDTH * 2, TITLEBAR_HEIGHT, titlebar_color);
        
        /* Traffic light buttons on LEFT side - macOS style */
        int btn_cx = x + BORDER_WIDTH + 18;  /* First circle center X */
        int btn_cy = y + BORDER_WIDTH + TITLEBAR_HEIGHT / 2;  /* Center Y */
        int btn_r = 6;  /* Button radius */
        
        /* Close button - Red */
        draw_circle(btn_cx, btn_cy, btn_r, COLOR_BTN_CLOSE);
        
        /* Minimize button - Yellow */
        btn_cx += 20;
        draw_circle(btn_cx, btn_cy, btn_r, COLOR_BTN_MINIMIZE);
        
        /* Zoom button - Green */
        btn_cx += 20;
        draw_circle(btn_cx, btn_cy, btn_r, COLOR_BTN_ZOOM);
        
        /* Window title - centered */
        int title_len = 0;
        for (const char *p = win->title; *p; p++) title_len++;
        int title_x = x + (w - title_len * 8) / 2;
        gui_draw_string(title_x, y + 6, win->title, THEME_FG, titlebar_color);
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
/* Desktop with Menu Bar and Dock */
/* ===================================================================== */

static void draw_menu_bar(void)
{
    /* Menu bar background */
    gui_draw_rect(0, 0, primary_display.width, MENU_BAR_HEIGHT, COLOR_MENU_BG);
    
    /* Apple logo placeholder */
    gui_draw_string(12, 6, "@", COLOR_MENU_TEXT, COLOR_MENU_BG);
    
    /* App name */
    gui_draw_string(36, 6, "Vib-OS", COLOR_MENU_TEXT, COLOR_MENU_BG);
    
    /* Menu items */
    gui_draw_string(96, 6, "File", COLOR_MENU_TEXT, COLOR_MENU_BG);
    gui_draw_string(144, 6, "Edit", COLOR_MENU_TEXT, COLOR_MENU_BG);
    gui_draw_string(192, 6, "View", COLOR_MENU_TEXT, COLOR_MENU_BG);
    gui_draw_string(240, 6, "Help", COLOR_MENU_TEXT, COLOR_MENU_BG);
    
    /* Clock on right */
    gui_draw_string(primary_display.width - 60, 6, "12:00", COLOR_MENU_TEXT, COLOR_MENU_BG);
}

/* Dock icons */
#include "icons.h"

static const char *dock_labels[] = {
    "Term", "Files", "Calc", "Edit", "Help"
};
#define NUM_DOCK_ICONS 5
#define DOCK_ICON_SIZE 40   /* Display size */
#define DOCK_ICON_MARGIN 4  /* Padding inside dock pill */
#define DOCK_PADDING 16     /* Space between icons */

/* Draw a 32x32 bitmap icon scaled to display size */
static void draw_icon(int x, int y, int size, const unsigned char *bitmap, uint32_t fg, uint32_t bg)
{
    for (int py = 0; py < 32; py++) {
        int draw_y = y + (py * size) / 32;
        for (int px = 0; px < 32; px++) {
            int draw_x = x + (px * size) / 32;
            uint32_t color = bitmap[py * 32 + px] ? fg : bg;
            /* Draw a small block for scaling */
            int next_x = x + ((px + 1) * size) / 32;
            int next_y = y + ((py + 1) * size) / 32;
            for (int dy = draw_y; dy < next_y; dy++) {
                for (int dx = draw_x; dx < next_x; dx++) {
                    draw_pixel(dx, dy, color);
                }
            }
        }
    }
}

static void draw_dock(void)
{
    int dock_content_w = NUM_DOCK_ICONS * (DOCK_ICON_SIZE + DOCK_PADDING) - DOCK_PADDING + 40;
    int dock_x = (primary_display.width - dock_content_w) / 2;
    int dock_y = primary_display.height - DOCK_HEIGHT + 10;
    int dock_h = DOCK_HEIGHT - 20;
    
    /* Dock background pill */
    gui_draw_rect(dock_x, dock_y, dock_content_w, dock_h, COLOR_DOCK_BG);
    gui_draw_rect_outline(dock_x, dock_y, dock_content_w, dock_h, COLOR_DOCK_BORDER, 1);
    
    /* Draw icons */
    int icon_x = dock_x + 20;
    int icon_y = dock_y + (dock_h - DOCK_ICON_SIZE) / 2;
    
    for (int i = 0; i < NUM_DOCK_ICONS; i++) {
        /* Draw icon bitmap */
        draw_icon(icon_x, icon_y, DOCK_ICON_SIZE, dock_icons_bmp[i], THEME_FG, COLOR_DOCK_BG);
        
        icon_x += DOCK_ICON_SIZE + DOCK_PADDING;
    }
}

static void draw_desktop(void)
{
    /* Desktop background */
    gui_draw_rect(0, MENU_BAR_HEIGHT, 
                  primary_display.width, 
                  primary_display.height - MENU_BAR_HEIGHT - DOCK_HEIGHT, 
                  THEME_BG);
    
    /* Draw menu bar at top */
    draw_menu_bar();
    
    /* Draw dock at bottom */
    draw_dock();
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
/* Mouse Cursor (Mac-style arrow with background save/restore) */
/* ===================================================================== */

#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 19

/* Classic Mac arrow: 1=black, 2=white, 0=transparent */
static const uint8_t cursor_data[CURSOR_HEIGHT][CURSOR_WIDTH] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,2,2,2,2,1,0},
    {1,2,2,2,2,2,2,1,1,1,1,1},
    {1,2,2,2,1,2,2,1,0,0,0,0},
    {1,2,2,1,0,1,2,2,1,0,0,0},
    {1,2,1,0,0,1,2,2,1,0,0,0},
    {1,1,0,0,0,0,1,2,2,1,0,0},
    {1,0,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,0,1,2,1,0,0},
    {0,0,0,0,0,0,0,1,1,0,0,0},
};

static int mouse_x = 512, mouse_y = 384;
static int mouse_buttons = 0;
static uint32_t saved_bg[CURSOR_HEIGHT][CURSOR_WIDTH];
static int saved_x = -1, saved_y = -1;
static int cursor_visible = 0;

static void save_cursor_background(int x, int y)
{
    for (int row = 0; row < CURSOR_HEIGHT; row++) {
        for (int col = 0; col < CURSOR_WIDTH; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < (int)primary_display.width && 
                py >= 0 && py < (int)primary_display.height) {
                uint32_t *target = primary_display.framebuffer;
                if (target) {
                    saved_bg[row][col] = target[py * (primary_display.pitch / 4) + px];
                }
            }
        }
    }
    saved_x = x;
    saved_y = y;
}

static void restore_cursor_background(void)
{
    if (saved_x < 0) return;
    
    for (int row = 0; row < CURSOR_HEIGHT; row++) {
        for (int col = 0; col < CURSOR_WIDTH; col++) {
            int px = saved_x + col;
            int py = saved_y + row;
            if (px >= 0 && px < (int)primary_display.width && 
                py >= 0 && py < (int)primary_display.height) {
                uint32_t *target = primary_display.framebuffer;
                if (target) {
                    target[py * (primary_display.pitch / 4) + px] = saved_bg[row][col];
                }
            }
        }
    }
    saved_x = -1;
}

static void draw_cursor_at(int x, int y)
{
    for (int row = 0; row < CURSOR_HEIGHT; row++) {
        for (int col = 0; col < CURSOR_WIDTH; col++) {
            uint8_t pixel = cursor_data[row][col];
            if (pixel == 0) continue;
            
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < (int)primary_display.width && 
                py >= 0 && py < (int)primary_display.height) {
                uint32_t color = (pixel == 1) ? 0x00000000 : 0x00FFFFFF;
                uint32_t *target = primary_display.framebuffer;
                if (target) {
                    target[py * (primary_display.pitch / 4) + px] = color;
                }
            }
        }
    }
}

void gui_draw_cursor(void)
{
    extern void mouse_get_position(int *x, int *y);
    int new_x, new_y;
    mouse_get_position(&new_x, &new_y);
    
    /* Only update if position changed */
    if (new_x == mouse_x && new_y == mouse_y && cursor_visible) {
        return;
    }
    
    /* Restore old background */
    if (cursor_visible) {
        restore_cursor_background();
    }
    
    /* Update position */
    mouse_x = new_x;
    mouse_y = new_y;
    
    /* Save and draw new cursor */
    save_cursor_background(mouse_x, mouse_y);
    draw_cursor_at(mouse_x, mouse_y);
    cursor_visible = 1;
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
/* Event Handling with Window Dragging */
/* ===================================================================== */

/* Dragging state */
static struct window *dragging_window = 0;
static int drag_offset_x = 0, drag_offset_y = 0;
static int prev_buttons = 0;

void gui_handle_mouse_event(int x, int y, int buttons)
{
    int prev_x = mouse_x;
    int prev_y = mouse_y;
    mouse_x = x;
    mouse_y = y;
    
    int left_click = (buttons & 1) && !(prev_buttons & 1);  /* Just pressed */
    int left_held = (buttons & 1);
    int left_release = !(buttons & 1) && (prev_buttons & 1);
    
    /* Handle window dragging */
    if (dragging_window && left_held) {
        /* Move window with mouse */
        dragging_window->x = x - drag_offset_x;
        dragging_window->y = y - drag_offset_y;
        
        /* Clamp to screen */
        if (dragging_window->y < MENU_BAR_HEIGHT) 
            dragging_window->y = MENU_BAR_HEIGHT;
        if (dragging_window->y > (int)primary_display.height - DOCK_HEIGHT - TITLEBAR_HEIGHT)
            dragging_window->y = primary_display.height - DOCK_HEIGHT - TITLEBAR_HEIGHT;
        if (dragging_window->x < 0) dragging_window->x = 0;
        if (dragging_window->x > (int)primary_display.width - 100) 
            dragging_window->x = primary_display.width - 100;
    }
    
    if (left_release) {
        dragging_window = 0;
    }
    
    prev_buttons = buttons;
    
    /* Check if clicking on a window */
    if (!left_click) return;
    
    for (struct window *win = window_stack; win; win = win->next) {
        if (!win->visible) continue;
        
        if (x >= win->x && x < win->x + win->width &&
            y >= win->y && y < win->y + win->height) {
            
            gui_focus_window(win);
            
            /* Check for traffic light buttons (on LEFT side now) */
            if (win->has_titlebar) {
                int btn_cy = win->y + BORDER_WIDTH + TITLEBAR_HEIGHT / 2;
                int btn_r = 8;  /* Click radius slightly larger than visual */
                
                /* Close button (first) */
                int close_cx = win->x + BORDER_WIDTH + 18;
                if ((x - close_cx) * (x - close_cx) + (y - btn_cy) * (y - btn_cy) <= btn_r * btn_r) {
                    gui_destroy_window(win);
                    return;
                }
                
                /* Minimize button (second) - skip for now */
                /* Zoom button (third) - skip for now */
                
                /* Start dragging if clicking on title bar */
                if (y >= win->y + BORDER_WIDTH && 
                    y < win->y + BORDER_WIDTH + TITLEBAR_HEIGHT &&
                    x >= win->x + BORDER_WIDTH + 70) {  /* After traffic lights */
                    dragging_window = win;
                    drag_offset_x = x - win->x;
                    drag_offset_y = y - win->y;
                    return;
                }
            }
            
            if (win->on_mouse) {
                win->on_mouse(win, x - win->x, y - win->y, buttons);
            }
            break;
        }
    }
    
    /* Check dock click */
    int dock_content_w = NUM_DOCK_ICONS * (DOCK_ICON_SIZE + DOCK_PADDING) - DOCK_PADDING + 40;
    int dock_x = (primary_display.width - dock_content_w) / 2;
    int dock_y = primary_display.height - DOCK_HEIGHT + 10;
    int dock_h = DOCK_HEIGHT - 20;
    
    if (y >= dock_y && y < dock_y + dock_h) {
        int icon_x = dock_x + 20;
        int icon_y_start = dock_y + (dock_h - DOCK_ICON_SIZE) / 2;
        
        for (int i = 0; i < NUM_DOCK_ICONS; i++) {
            if (x >= icon_x && x < icon_x + DOCK_ICON_SIZE &&
                y >= icon_y_start && y < icon_y_start + DOCK_ICON_SIZE) {
                /* Clicked on icon i - create a window for it */
                static int app_x = 100;
                static int app_y = 80;
                
                switch (i) {
                    case 0: /* Terminal */
                        gui_create_window("Terminal", app_x, app_y, 400, 300);
                        break;
                    case 1: /* Files */
                        gui_create_window("Files", app_x + 30, app_y + 30, 450, 350);
                        break;
                    case 2: /* Calculator */
                        gui_create_window("Calculator", app_x + 60, app_y + 60, 250, 300);
                        break;
                    case 3: /* Paint */
                        gui_create_window("Paint", app_x + 90, app_y + 90, 500, 400);
                        break;
                    case 4: /* Help */
                        gui_create_window("Help", app_x + 120, app_y + 120, 350, 280);
                        break;
                }
                app_x = (app_x + 50) % 300 + 100;
                app_y = (app_y + 50) % 200 + 80;
                return;
            }
            icon_x += DOCK_ICON_SIZE + DOCK_PADDING;
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
