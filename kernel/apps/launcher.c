/*
 * Vib-OS Application Launcher
 * 
 * Provides kernel API and launches embedded applications
 */

#include "apps/kapi.h"
#include "printk.h"
#include "mm/kmalloc.h"

/* Display structure from window.c */
struct display {
    uint32_t *framebuffer;
    uint32_t *backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
};

/* External references */
extern struct display *gui_get_display(void);
extern void mouse_get_position(int *x, int *y);
extern int mouse_get_buttons(void);
extern int uart_getc_nonblock(void);
extern void uart_putc(char c);

/* Timer ticks counter */
static volatile uint64_t uptime_ticks = 0;

/* Global kernel API instance */
static kapi_t global_kapi;

/* ===================================================================== */
/* KAPI Implementation Functions */
/* ===================================================================== */

static void kapi_putc(char c) {
    uart_putc(c);
}

static void kapi_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

static int kapi_getc(void) {
    return uart_getc_nonblock();
}

static int kapi_has_key(void) {
    return uart_getc_nonblock() >= 0 ? 1 : 0;
}

static void kapi_clear(void) {
    /* Clear framebuffer to black */
    struct display *d = gui_get_display();
    if (d && d->framebuffer) {
        for (uint32_t i = 0; i < d->width * d->height; i++) {
            d->framebuffer[i] = 0;
        }
    }
}

static int kapi_get_key(void) {
    return uart_getc_nonblock();
}

static void kapi_mouse_get_pos(int *x, int *y) {
    mouse_get_position(x, y);
}

static uint8_t kapi_mouse_get_buttons(void) {
    return (uint8_t)mouse_get_buttons();
}

static int last_mouse_x = 0, last_mouse_y = 0;
static void kapi_mouse_get_delta(int *dx, int *dy) {
    int x, y;
    mouse_get_position(&x, &y);
    *dx = x - last_mouse_x;
    *dy = y - last_mouse_y;
    last_mouse_x = x;
    last_mouse_y = y;
}

static uint64_t kapi_get_uptime_ticks(void) {
    return uptime_ticks;
}

static void kapi_sleep_ms(uint32_t ms) {
    /* Simple busy-wait sleep */
    for (volatile uint32_t i = 0; i < ms * 10000; i++) { }
}

static void *kapi_malloc(size_t size) {
    return kmalloc(size);
}

static void kapi_free(void *ptr) {
    kfree(ptr);
}

/* File I/O stubs - TODO: implement with VFS */
static void *kapi_open(const char *path) {
    (void)path;
    return NULL; /* Not implemented yet */
}

static void kapi_close(void *handle) {
    (void)handle;
}

static int kapi_read(void *handle, void *buf, size_t count, size_t offset) {
    (void)handle; (void)buf; (void)count; (void)offset;
    return -1;
}

static int kapi_write(void *handle, const void *buf, size_t count) {
    (void)handle; (void)buf; (void)count;
    return -1;
}

static int kapi_file_size(void *handle) {
    (void)handle;
    return 0;
}

static int kapi_create(const char *path) {
    (void)path;
    return -1;
}

static int kapi_delete(const char *path) {
    (void)path;
    return -1;
}

static int kapi_rename(const char *old, const char *new) {
    (void)old; (void)new;
    return -1;
}

static void kapi_exit(int status) {
    printk(KERN_INFO "[APP] Exit with status %d\n", status);
    /* Return to kernel - in real userspace, this would terminate the process */
}

/* Run an app and wait for completion */
static int kapi_exec(const char *path) {
    printk(KERN_INFO "[KAPI] exec: %s\n", path);
    return app_run(path, 0, 0);
}

/* Run an app in background */
static int kapi_spawn(const char *path) {
    printk(KERN_INFO "[KAPI] spawn: %s\n", path);
    /* For now, same as exec - no true multitasking yet */
    return app_run(path, 0, 0);
}

/* Yield CPU to other tasks */
static void kapi_yield(void) {
    /* Placeholder - would call scheduler */
    for (volatile int i = 0; i < 1000; i++) { }
}

static void kapi_uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

/* ===================================================================== */
/* Initialize Kernel API */
/* ===================================================================== */

void kapi_init(kapi_t *api) {
    struct display *d = gui_get_display();
    api->version = 1;
    
    /* Framebuffer */
    api->fb_base = d ? d->framebuffer : NULL;
    api->fb_width = d ? d->width : 0;
    api->fb_height = d ? d->height : 0;
    api->fb_pitch = d ? d->pitch : 0;
    
    /* Memory */
    api->malloc = kapi_malloc;
    api->free = kapi_free;
    
    /* Console */
    api->putc = kapi_putc;
    api->puts = kapi_puts;
    api->getc = kapi_getc;
    api->has_key = kapi_has_key;
    api->clear = kapi_clear;
    
    /* Keyboard */
    api->get_key = kapi_get_key;
    
    /* Mouse */
    api->mouse_get_pos = kapi_mouse_get_pos;
    api->mouse_get_buttons = kapi_mouse_get_buttons;
    api->mouse_get_delta = kapi_mouse_get_delta;
    
    /* Timing */
    api->get_uptime_ticks = kapi_get_uptime_ticks;
    api->sleep_ms = kapi_sleep_ms;
    
    /* File I/O */
    api->open = kapi_open;
    api->close = kapi_close;
    api->read = kapi_read;
    api->write = kapi_write;
    api->file_size = kapi_file_size;
    api->create = kapi_create;
    api->delete = kapi_delete;
    api->rename = kapi_rename;
    
    /* Process */
    api->exit = kapi_exit;
    api->exec = kapi_exec;
    api->spawn = kapi_spawn;
    api->yield = kapi_yield;
    
    /* Debug */
    api->uart_puts = kapi_uart_puts;
    
    printk(KERN_INFO "[KAPI] Kernel API initialized (fb=%dx%d)\n", api->fb_width, api->fb_height);
}

/* ===================================================================== */
/* Application Registry - Embedded Apps */
/* ===================================================================== */

/* Tick counter for timing */
void kapi_tick(void) {
    uptime_ticks++;
}

/* Get the global kapi */
kapi_t *kapi_get(void) {
    static int initialized = 0;
    if (!initialized) {
        kapi_init(&global_kapi);
        initialized = 1;
    }
    return &global_kapi;
}

/* ===================================================================== */
/* Demo Application: Clock */
/* ===================================================================== */
static int clock_app_main(kapi_t *api, int argc, char **argv) {
    (void)argc; (void)argv;
    
    api->puts("\n=== Vib-OS Clock ===\n");
    
    if (!api->fb_base) {
        api->puts("No framebuffer available\n");
        return -1;
    }
    
    /* Draw clock interface */
    int cx = api->fb_width / 2;
    int cy = api->fb_height / 2;
    int radius = 100;
    
    /* Draw clock face (circle) */
    for (int angle = 0; angle < 360; angle++) {
        /* Simplified circle using fixed-point math */
        int x = cx + (radius * (angle % 90 < 45 ? angle % 45 : 45 - (angle % 45))) / 45;
        int y = cy + (radius * (45 - (angle % 45))) / 45;
        if (y >= 0 && y < (int)api->fb_height && x >= 0 && x < (int)api->fb_width) {
            api->fb_base[y * api->fb_width + x] = 0xFFFFFF;
        }
    }
    
    /* Draw hour markers */
    for (int h = 0; h < 12; h++) {
        int mx = cx + ((h < 6 ? h : 12 - h) * radius / 6);
        int my = cy - (h < 3 || h > 9 ? radius - 10 : (h == 6 ? -radius + 10 : 0));
        if (my >= 0 && my < (int)api->fb_height && mx >= 0 && mx < (int)api->fb_width) {
            api->fb_base[my * api->fb_width + mx] = 0xFFFF00;
        }
    }
    
    /* Draw center dot */
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            int px = cx + dx;
            int py = cy + dy;
            if (py >= 0 && py < (int)api->fb_height && px >= 0 && px < (int)api->fb_width) {
                api->fb_base[py * api->fb_width + px] = 0xFF0000;
            }
        }
    }
    
    /* Draw clock hands based on uptime */
    uint64_t ticks = api->get_uptime_ticks();
    int seconds = (ticks / 100) % 60;
    int minutes = (ticks / 6000) % 60;
    int hours = (ticks / 360000) % 12;
    
    /* Second hand (red, long) */
    for (int i = 0; i < radius - 10; i++) {
        int sx = cx + (i * (seconds % 30 < 15 ? seconds % 30 : 30 - seconds % 30)) / 30;
        int sy = cy - (i * (seconds < 30 ? 1 : -1));
        if (sy >= 0 && sy < (int)api->fb_height && sx >= 0 && sx < (int)api->fb_width) {
            api->fb_base[sy * api->fb_width + sx] = 0xFF0000;
        }
    }
    
    api->puts("Clock drawn! Uptime: ");
    char buf[32];
    int idx = 0;
    buf[idx++] = '0' + (hours / 10);
    buf[idx++] = '0' + (hours % 10);
    buf[idx++] = ':';
    buf[idx++] = '0' + (minutes / 10);
    buf[idx++] = '0' + (minutes % 10);
    buf[idx++] = ':';
    buf[idx++] = '0' + (seconds / 10);
    buf[idx++] = '0' + (seconds % 10);
    buf[idx++] = '\n';
    buf[idx] = '\0';
    api->puts(buf);
    
    return 0;
}

/* ===================================================================== */
/* Demo Application: Snake Game */
/* ===================================================================== */
#define SNAKE_GRID_SIZE 20
#define SNAKE_MAX_LEN 100

static int snake_app_main(kapi_t *api, int argc, char **argv) {
    (void)argc; (void)argv;
    
    api->puts("\n=== Vib-OS Snake ===\n");
    api->puts("Use mouse to control direction!\n");
    
    if (!api->fb_base) {
        api->puts("No framebuffer available\n");
        return -1;
    }
    
    /* Snake state */
    int snake_x[SNAKE_MAX_LEN];
    int snake_y[SNAKE_MAX_LEN];
    int snake_len = 5;
    int dir_x = 1, dir_y = 0;
    
    /* Initialize snake in center */
    int grid_w = api->fb_width / SNAKE_GRID_SIZE;
    int grid_h = api->fb_height / SNAKE_GRID_SIZE;
    int start_x = grid_w / 2;
    int start_y = grid_h / 2;
    
    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = start_x - i;
        snake_y[i] = start_y;
    }
    
    /* Food position */
    int food_x = start_x + 5;
    int food_y = start_y;
    
    int score = 0;
    int game_over = 0;
    
    /* Game loop - run for limited iterations */
    for (int frame = 0; frame < 200 && !game_over; frame++) {
        /* Clear screen to dark */
        for (uint32_t y = 0; y < api->fb_height; y++) {
            for (uint32_t x = 0; x < api->fb_width; x++) {
                api->fb_base[y * api->fb_width + x] = 0x1E1E2E;
            }
        }
        
        /* Check mouse for direction */
        int mx, my;
        api->mouse_get_pos(&mx, &my);
        int head_px = snake_x[0] * SNAKE_GRID_SIZE;
        int head_py = snake_y[0] * SNAKE_GRID_SIZE;
        
        /* Change direction based on mouse relative to head */
        if (mx > head_px + SNAKE_GRID_SIZE && dir_x == 0) { dir_x = 1; dir_y = 0; }
        else if (mx < head_px - SNAKE_GRID_SIZE && dir_x == 0) { dir_x = -1; dir_y = 0; }
        else if (my > head_py + SNAKE_GRID_SIZE && dir_y == 0) { dir_x = 0; dir_y = 1; }
        else if (my < head_py - SNAKE_GRID_SIZE && dir_y == 0) { dir_x = 0; dir_y = -1; }
        
        /* Move snake */
        int new_x = snake_x[0] + dir_x;
        int new_y = snake_y[0] + dir_y;
        
        /* Wrap around */
        if (new_x < 0) new_x = grid_w - 1;
        if (new_x >= grid_w) new_x = 0;
        if (new_y < 0) new_y = grid_h - 1;
        if (new_y >= grid_h) new_y = 0;
        
        /* Self collision check */
        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == new_x && snake_y[i] == new_y) {
                game_over = 1;
                break;
            }
        }
        
        if (!game_over) {
            /* Move body */
            for (int i = snake_len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i-1];
                snake_y[i] = snake_y[i-1];
            }
            snake_x[0] = new_x;
            snake_y[0] = new_y;
            
            /* Check food */
            if (snake_x[0] == food_x && snake_y[0] == food_y) {
                score++;
                if (snake_len < SNAKE_MAX_LEN) snake_len++;
                /* New food position */
                food_x = (food_x + 7) % grid_w;
                food_y = (food_y + 11) % grid_h;
            }
        }
        
        /* Draw snake */
        for (int i = 0; i < snake_len; i++) {
            uint32_t color = (i == 0) ? 0x00FF00 : 0x00AA00;  /* Head brighter */
            int sx = snake_x[i] * SNAKE_GRID_SIZE;
            int sy = snake_y[i] * SNAKE_GRID_SIZE;
            for (int dy = 1; dy < SNAKE_GRID_SIZE - 1; dy++) {
                for (int dx = 1; dx < SNAKE_GRID_SIZE - 1; dx++) {
                    int px = sx + dx;
                    int py = sy + dy;
                    if ((uint32_t)py < api->fb_height && (uint32_t)px < api->fb_width) {
                        api->fb_base[py * api->fb_width + px] = color;
                    }
                }
            }
        }
        
        /* Draw food (red) */
        for (int dy = 2; dy < SNAKE_GRID_SIZE - 2; dy++) {
            for (int dx = 2; dx < SNAKE_GRID_SIZE - 2; dx++) {
                int px = food_x * SNAKE_GRID_SIZE + dx;
                int py = food_y * SNAKE_GRID_SIZE + dy;
                if ((uint32_t)py < api->fb_height && (uint32_t)px < api->fb_width) {
                    api->fb_base[py * api->fb_width + px] = 0xFF0000;
                }
            }
        }
        
        api->sleep_ms(100);  /* Game speed */
    }
    
    /* Show score */
    api->puts("Game Over! Score: ");
    char sbuf[16];
    sbuf[0] = '0' + (score / 10);
    sbuf[1] = '0' + (score % 10);
    sbuf[2] = '\n';
    sbuf[3] = '\0';
    api->puts(sbuf);
    
    return 0;
}

/* ===================================================================== */
/* Demo Application: System Monitor */
/* ===================================================================== */
static int sysmon_app_main(kapi_t *api, int argc, char **argv) {
    (void)argc; (void)argv;
    
    api->puts("\n=== Vib-OS System Monitor ===\n\n");
    
    /* Display system information */
    api->puts("SYSTEM INFO\n");
    api->puts("-----------\n");
    api->puts("OS:       Vib-OS v0.5.0\n");
    api->puts("Arch:     ARM64 (AArch64)\n");
    api->puts("Platform: QEMU virt\n\n");
    
    api->puts("DISPLAY\n");
    api->puts("-------\n");
    char buf[64];
    int idx = 0;
    api->puts("Resolution: ");
    idx = 0;
    uint32_t w = api->fb_width;
    buf[idx++] = '0' + (w / 1000) % 10;
    buf[idx++] = '0' + (w / 100) % 10;
    buf[idx++] = '0' + (w / 10) % 10;
    buf[idx++] = '0' + w % 10;
    buf[idx++] = 'x';
    uint32_t h = api->fb_height;
    buf[idx++] = '0' + (h / 1000) % 10;
    buf[idx++] = '0' + (h / 100) % 10;
    buf[idx++] = '0' + (h / 10) % 10;
    buf[idx++] = '0' + h % 10;
    buf[idx++] = '\n';
    buf[idx] = '\0';
    api->puts(buf);
    api->puts("Color:      32-bit ARGB\n");
    api->puts("Compositor: Double-buffered\n\n");
    
    api->puts("MEMORY\n");
    api->puts("------\n");
    api->puts("Heap:     8 MB\n");
    api->puts("PMM:      Buddy allocator\n\n");
    
    api->puts("UPTIME\n");
    api->puts("------\n");
    uint64_t ticks = api->get_uptime_ticks();
    int secs = (int)(ticks / 100);
    int mins = secs / 60;
    int hrs = mins / 60;
    secs %= 60;
    mins %= 60;
    
    idx = 0;
    buf[idx++] = '0' + (hrs / 10);
    buf[idx++] = '0' + (hrs % 10);
    buf[idx++] = ':';
    buf[idx++] = '0' + (mins / 10);
    buf[idx++] = '0' + (mins % 10);
    buf[idx++] = ':';
    buf[idx++] = '0' + (secs / 10);
    buf[idx++] = '0' + (secs % 10);
    buf[idx++] = '\n';
    buf[idx] = '\0';
    api->puts(buf);
    
    /* Draw system bars if framebuffer available */
    if (api->fb_base) {
        int bar_x = 50;
        int bar_y = api->fb_height - 150;
        int bar_w = 200;
        int bar_h = 20;
        
        /* CPU bar (simulated 45%) */
        for (int y = 0; y < bar_h; y++) {
            for (int x = 0; x < bar_w; x++) {
                int px = bar_x + x;
                int py = bar_y + y;
                uint32_t color = (x < bar_w * 45 / 100) ? 0x00FF00 : 0x333333;
                if ((uint32_t)py < api->fb_height && (uint32_t)px < api->fb_width) {
                    api->fb_base[py * api->fb_width + px] = color;
                }
            }
        }
        
        /* Memory bar (simulated 62%) */
        bar_y += 30;
        for (int y = 0; y < bar_h; y++) {
            for (int x = 0; x < bar_w; x++) {
                int px = bar_x + x;
                int py = bar_y + y;
                uint32_t color = (x < bar_w * 62 / 100) ? 0x00AAFF : 0x333333;
                if ((uint32_t)py < api->fb_height && (uint32_t)px < api->fb_width) {
                    api->fb_base[py * api->fb_width + px] = color;
                }
            }
        }
    }
    
    return 0;
}

/* ===================================================================== */
/* Demo Application: Mandelbrot Fractal */
/* ===================================================================== */
static int mandelbrot_app_main(kapi_t *api, int argc, char **argv) {
    (void)argc; (void)argv;
    
    api->puts("\n=== Vib-OS Mandelbrot Viewer ===\n");
    api->puts("Rendering fractal...\n");
    
    if (!api->fb_base) {
        api->puts("No framebuffer available\n");
        return -1;
    }
    
    int width = api->fb_width;
    int height = api->fb_height;
    int max_iter = 50;
    
    /* Fixed-point math (16.16 format) */
    #define FP_SHIFT 16
    #define FP_ONE (1 << FP_SHIFT)
    #define FP_MUL(a, b) (((long long)(a) * (b)) >> FP_SHIFT)
    
    /* View: x from -2.5 to 1, y from -1 to 1 */
    int x_min = -2 * FP_ONE - FP_ONE / 2;  /* -2.5 */
    int x_max = 1 * FP_ONE;                 /* 1.0 */
    int y_min = -1 * FP_ONE;                /* -1.0 */
    int y_max = 1 * FP_ONE;                 /* 1.0 */
    
    int x_scale = (x_max - x_min) / width;
    int y_scale = (y_max - y_min) / height;
    
    /* Color palette */
    uint32_t palette[16] = {
        0x000764, 0x206BCB, 0xEDFFFF, 0xFFAA00,
        0x000200, 0x0C2161, 0x1E81B0, 0x76E5FC,
        0xFBFECC, 0xED8A0A, 0x9A0200, 0x280000,
        0x2D0070, 0x6600AA, 0x9900FF, 0xCC00FF
    };
    
    for (int py = 0; py < height; py++) {
        int y0 = y_min + py * y_scale;
        
        for (int px = 0; px < width; px++) {
            int x0 = x_min + px * x_scale;
            
            int x = 0, y = 0;
            int iter = 0;
            
            while (iter < max_iter) {
                int x2 = FP_MUL(x, x);
                int y2 = FP_MUL(y, y);
                
                if (x2 + y2 > 4 * FP_ONE) break;
                
                int xtemp = x2 - y2 + x0;
                y = 2 * FP_MUL(x, y) + y0;
                x = xtemp;
                iter++;
            }
            
            uint32_t color;
            if (iter == max_iter) {
                color = 0x000000;  /* Black for set */
            } else {
                color = palette[iter % 16];
            }
            
            api->fb_base[py * width + px] = color;
        }
        
        /* Yield every 50 rows to keep system responsive */
        if (py % 50 == 0) {
            api->yield();
        }
    }
    
    #undef FP_SHIFT
    #undef FP_ONE
    #undef FP_MUL
    
    api->puts("Fractal rendered!\n");
    return 0;
}

/* ===================================================================== */
/* Simple test app */
/* ===================================================================== */
static int test_app_main(kapi_t *api, int argc, char **argv) {
    (void)argc; (void)argv;
    
    api->puts("Hello from test app!\n");
    api->puts("Framebuffer: ");
    
    /* Draw a red rectangle on screen */
    if (api->fb_base) {
        for (int y = 100; y < 200; y++) {
            for (int x = 100; x < 300; x++) {
                api->fb_base[y * api->fb_width + x] = 0xFF0000;  /* Red */
            }
        }
        api->puts("Drew red rectangle!\n");
    }
    
    return 0;
}

/* ===================================================================== */
/* App Registry */
/* ===================================================================== */
typedef struct {
    const char *name;
    app_main_fn main_fn;
} app_entry_t;

static app_entry_t app_registry[] = {
    { "test",       test_app_main },
    { "clock",      clock_app_main },
    { "snake",      snake_app_main },
    { "sysmon",     sysmon_app_main },
    { "mandelbrot", mandelbrot_app_main },
    { NULL, NULL }
};

/* Run an embedded application by name */
int app_run(const char *name, int argc, char **argv) {
    printk(KERN_INFO "[APP] Running: %s\n", name);
    
    /* Find app in registry */
    for (int i = 0; app_registry[i].name != NULL; i++) {
        /* Simple strcmp */
        const char *a = name;
        const char *b = app_registry[i].name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == *b) {
            /* Match found */
            kapi_t *api = kapi_get();
            return app_registry[i].main_fn(api, argc, argv);
        }
    }
    
    printk(KERN_WARNING "[APP] App not found: %s\n", name);
    return -1;
}
