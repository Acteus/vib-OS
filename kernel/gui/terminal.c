/*
 * Vib-OS - Terminal Emulator
 * 
 * VT100-compatible terminal emulator for the GUI.
 */

#include "types.h"
#include "printk.h"
#include "mm/kmalloc.h"

/* Forward declare window type */
struct window;

/* External GUI functions */
extern void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
extern void gui_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
extern struct window *gui_create_window(const char *title, int x, int y, int w, int h);

/* ===================================================================== */
/* Terminal Configuration */
/* ===================================================================== */

#define TERM_COLS       80
#define TERM_ROWS       24
#define TERM_CHAR_W     8
#define TERM_CHAR_H     16
#define TERM_PADDING    4

/* Terminal colors (VT100/ANSI) */
static const uint32_t term_colors[16] = {
    0x1E1E2E,   /* 0 - Black (background) */
    0xF38BA8,   /* 1 - Red */
    0xA6E3A1,   /* 2 - Green */
    0xF9E2AF,   /* 3 - Yellow */
    0x89B4FA,   /* 4 - Blue */
    0xCBA6F7,   /* 5 - Magenta */
    0x94E2D5,   /* 6 - Cyan */
    0xCDD6F4,   /* 7 - White (foreground) */
    0x585B70,   /* 8 - Bright Black */
    0xF38BA8,   /* 9 - Bright Red */
    0xA6E3A1,   /* 10 - Bright Green */
    0xF9E2AF,   /* 11 - Bright Yellow */
    0x89B4FA,   /* 12 - Bright Blue */
    0xCBA6F7,   /* 13 - Bright Magenta */
    0x94E2D5,   /* 14 - Bright Cyan */
    0xFFFFFF,   /* 15 - Bright White */
};

/* ===================================================================== */
/* Terminal State */
/* ===================================================================== */

struct terminal {
    /* Character buffer */
    char *chars;
    uint8_t *fg_colors;
    uint8_t *bg_colors;
    
    /* Dimensions */
    int cols;
    int rows;
    
    /* Cursor */
    int cursor_x;
    int cursor_y;
    bool cursor_visible;
    bool cursor_blink;
    
    /* Current colors */
    uint8_t current_fg;
    uint8_t current_bg;
    
    /* Escape sequence state */
    bool in_escape;
    char escape_buf[32];
    int escape_len;
    
    /* Scrollback */
    int scroll_offset;
    
    /* Associated window */
    struct window *window;
    int content_x, content_y;
    
    /* Input buffer */
    char input_buf[256];
    int input_len;
    int input_pos;
    
    /* Shell process */
    int shell_pid;
    int pty_fd;
};

static struct terminal *active_terminal = NULL;

/* ===================================================================== */
/* Terminal Buffer Operations */
/* ===================================================================== */

static void term_clear_line(struct terminal *term, int row)
{
    for (int col = 0; col < term->cols; col++) {
        int idx = row * term->cols + col;
        term->chars[idx] = ' ';
        term->fg_colors[idx] = term->current_fg;
        term->bg_colors[idx] = term->current_bg;
    }
}

static void term_scroll_up(struct terminal *term)
{
    /* Move all lines up by one */
    for (int row = 0; row < term->rows - 1; row++) {
        for (int col = 0; col < term->cols; col++) {
            int src = (row + 1) * term->cols + col;
            int dst = row * term->cols + col;
            term->chars[dst] = term->chars[src];
            term->fg_colors[dst] = term->fg_colors[src];
            term->bg_colors[dst] = term->bg_colors[src];
        }
    }
    
    /* Clear last line */
    term_clear_line(term, term->rows - 1);
}

static void term_newline(struct terminal *term)
{
    term->cursor_x = 0;
    term->cursor_y++;
    
    if (term->cursor_y >= term->rows) {
        term_scroll_up(term);
        term->cursor_y = term->rows - 1;
    }
}

/* ===================================================================== */
/* Escape Sequence Processing */
/* ===================================================================== */

static void term_process_escape(struct terminal *term)
{
    if (term->escape_len < 1) return;
    
    /* CSI sequences start with [ */
    if (term->escape_buf[0] == '[') {
        char *seq = term->escape_buf + 1;
        char cmd = term->escape_buf[term->escape_len - 1];
        
        int params[8] = {0};
        int param_count = 0;
        int num = 0;
        bool in_num = false;
        
        for (int i = 0; i < term->escape_len - 1 && param_count < 8; i++) {
            char c = seq[i];
            if (c >= '0' && c <= '9') {
                num = num * 10 + (c - '0');
                in_num = true;
            } else if (c == ';') {
                if (in_num) params[param_count++] = num;
                num = 0;
                in_num = false;
            }
        }
        if (in_num) params[param_count++] = num;
        
        switch (cmd) {
            case 'A': /* Cursor Up */
                term->cursor_y -= (params[0] > 0) ? params[0] : 1;
                if (term->cursor_y < 0) term->cursor_y = 0;
                break;
                
            case 'B': /* Cursor Down */
                term->cursor_y += (params[0] > 0) ? params[0] : 1;
                if (term->cursor_y >= term->rows) term->cursor_y = term->rows - 1;
                break;
                
            case 'C': /* Cursor Forward */
                term->cursor_x += (params[0] > 0) ? params[0] : 1;
                if (term->cursor_x >= term->cols) term->cursor_x = term->cols - 1;
                break;
                
            case 'D': /* Cursor Back */
                term->cursor_x -= (params[0] > 0) ? params[0] : 1;
                if (term->cursor_x < 0) term->cursor_x = 0;
                break;
                
            case 'H': /* Cursor Position */
            case 'f':
                term->cursor_y = (params[0] > 0) ? params[0] - 1 : 0;
                term->cursor_x = (param_count > 1 && params[1] > 0) ? params[1] - 1 : 0;
                if (term->cursor_y >= term->rows) term->cursor_y = term->rows - 1;
                if (term->cursor_x >= term->cols) term->cursor_x = term->cols - 1;
                break;
                
            case 'J': /* Erase Display */
                if (params[0] == 2) {
                    /* Clear entire screen */
                    for (int row = 0; row < term->rows; row++) {
                        term_clear_line(term, row);
                    }
                    term->cursor_x = 0;
                    term->cursor_y = 0;
                }
                break;
                
            case 'K': /* Erase Line */
                for (int col = term->cursor_x; col < term->cols; col++) {
                    int idx = term->cursor_y * term->cols + col;
                    term->chars[idx] = ' ';
                }
                break;
                
            case 'm': /* SGR - Select Graphic Rendition */
                for (int i = 0; i < param_count; i++) {
                    int p = params[i];
                    if (p == 0) {
                        term->current_fg = 7;
                        term->current_bg = 0;
                    } else if (p >= 30 && p <= 37) {
                        term->current_fg = p - 30;
                    } else if (p >= 40 && p <= 47) {
                        term->current_bg = p - 40;
                    } else if (p >= 90 && p <= 97) {
                        term->current_fg = p - 90 + 8;
                    } else if (p >= 100 && p <= 107) {
                        term->current_bg = p - 100 + 8;
                    }
                }
                break;
        }
    }
    
    term->in_escape = false;
    term->escape_len = 0;
}

/* ===================================================================== */
/* Character Output */
/* ===================================================================== */

void term_putc(struct terminal *term, char c)
{
    if (term->in_escape) {
        term->escape_buf[term->escape_len++] = c;
        
        /* Check for end of escape sequence */
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~') {
            term_process_escape(term);
        } else if (term->escape_len >= 31) {
            term->in_escape = false;
            term->escape_len = 0;
        }
        return;
    }
    
    switch (c) {
        case '\033': /* ESC */
            term->in_escape = true;
            term->escape_len = 0;
            break;
            
        case '\n':
            term_newline(term);
            break;
            
        case '\r':
            term->cursor_x = 0;
            break;
            
        case '\b':
            if (term->cursor_x > 0) {
                term->cursor_x--;
            }
            break;
            
        case '\t':
            term->cursor_x = (term->cursor_x + 8) & ~7;
            if (term->cursor_x >= term->cols) {
                term_newline(term);
            }
            break;
            
        default:
            if (c >= 32 && c < 127) {
                int idx = term->cursor_y * term->cols + term->cursor_x;
                term->chars[idx] = c;
                term->fg_colors[idx] = term->current_fg;
                term->bg_colors[idx] = term->current_bg;
                
                term->cursor_x++;
                if (term->cursor_x >= term->cols) {
                    term_newline(term);
                }
            }
            break;
    }
}

void term_puts(struct terminal *term, const char *str)
{
    while (*str) {
        term_putc(term, *str++);
    }
}

/* ===================================================================== */
/* Rendering */
/* ===================================================================== */

void term_render(struct terminal *term)
{
    if (!term || !term->window) return;
    
    int base_x = term->content_x + TERM_PADDING;
    int base_y = term->content_y + TERM_PADDING;
    
    /* Draw background */
    gui_draw_rect(term->content_x, term->content_y,
                 term->cols * TERM_CHAR_W + TERM_PADDING * 2,
                 term->rows * TERM_CHAR_H + TERM_PADDING * 2,
                 term_colors[0]);
    
    /* Draw characters */
    for (int row = 0; row < term->rows; row++) {
        for (int col = 0; col < term->cols; col++) {
            int idx = row * term->cols + col;
            char c = term->chars[idx];
            uint32_t fg = term_colors[term->fg_colors[idx] & 0xF];
            uint32_t bg = term_colors[term->bg_colors[idx] & 0xF];
            
            int x = base_x + col * TERM_CHAR_W;
            int y = base_y + row * TERM_CHAR_H;
            
            gui_draw_char(x, y, c, fg, bg);
        }
    }
    
    /* Draw cursor */
    if (term->cursor_visible) {
        int x = base_x + term->cursor_x * TERM_CHAR_W;
        int y = base_y + term->cursor_y * TERM_CHAR_H;
        gui_draw_rect(x, y, TERM_CHAR_W, TERM_CHAR_H, term_colors[7]);
    }
}

/* ===================================================================== */
/* Input Handling */
/* ===================================================================== */

void term_handle_key(struct terminal *term, int key)
{
    if (!term) return;
    
    if (key == '\n' || key == '\r') {
        /* Process command */
        term->input_buf[term->input_len] = '\0';
        term_putc(term, '\n');
        
        /* Execute command (placeholder) */
        if (term->input_len > 0) {
            term_puts(term, "$ ");
            term_puts(term, term->input_buf);
            term_puts(term, "\n");
            
            /* Simple built-in commands */
            if (term->input_buf[0] == 'c' && term->input_buf[1] == 'l' && 
                term->input_buf[2] == 'e' && term->input_buf[3] == 'a' &&
                term->input_buf[4] == 'r') {
                for (int row = 0; row < term->rows; row++) {
                    term_clear_line(term, row);
                }
                term->cursor_x = 0;
                term->cursor_y = 0;
            } else if (term->input_buf[0] == 'h' && term->input_buf[1] == 'e' &&
                       term->input_buf[2] == 'l' && term->input_buf[3] == 'p') {
                term_puts(term, "Vib-OS Terminal\n");
                term_puts(term, "Commands: clear, help, exit\n");
            } else if (term->input_buf[0] == 'e' && term->input_buf[1] == 'x' &&
                       term->input_buf[2] == 'i' && term->input_buf[3] == 't') {
                term_puts(term, "Goodbye!\n");
            } else {
                term_puts(term, "Unknown command: ");
                term_puts(term, term->input_buf);
                term_puts(term, "\n");
            }
        }
        
        /* Show new prompt */
        term_puts(term, "vib-os $ ");
        
        term->input_len = 0;
        term->input_pos = 0;
    } else if (key == '\b' || key == 127) {
        if (term->input_len > 0) {
            term->input_len--;
            term->cursor_x--;
            int idx = term->cursor_y * term->cols + term->cursor_x;
            term->chars[idx] = ' ';
        }
    } else if (key >= 32 && key < 127) {
        if (term->input_len < 255) {
            term->input_buf[term->input_len++] = key;
            term_putc(term, key);
        }
    }
}

/* ===================================================================== */
/* Terminal Creation */
/* ===================================================================== */

struct terminal *term_create(int x, int y, int cols, int rows)
{
    struct terminal *term = kmalloc(sizeof(struct terminal));
    if (!term) return NULL;
    
    term->cols = cols;
    term->rows = rows;
    
    size_t buf_size = cols * rows;
    term->chars = kmalloc(buf_size);
    term->fg_colors = kmalloc(buf_size);
    term->bg_colors = kmalloc(buf_size);
    
    if (!term->chars || !term->fg_colors || !term->bg_colors) {
        if (term->chars) kfree(term->chars);
        if (term->fg_colors) kfree(term->fg_colors);
        if (term->bg_colors) kfree(term->bg_colors);
        kfree(term);
        return NULL;
    }
    
    /* Initialize */
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->cursor_visible = true;
    term->current_fg = 7;
    term->current_bg = 0;
    term->in_escape = false;
    term->escape_len = 0;
    term->input_len = 0;
    term->input_pos = 0;
    term->content_x = x;
    term->content_y = y;
    
    /* Clear buffer */
    for (int row = 0; row < rows; row++) {
        term_clear_line(term, row);
    }
    
    /* Print welcome message */
    term_puts(term, "\033[32mVib-OS Terminal v1.0\033[0m\n");
    term_puts(term, "Type 'help' for commands.\n\n");
    term_puts(term, "vib-os $ ");
    
    printk(KERN_INFO "TERM: Created terminal %dx%d\n", cols, rows);
    
    return term;
}

void term_destroy(struct terminal *term)
{
    if (!term) return;
    
    if (term->chars) kfree(term->chars);
    if (term->fg_colors) kfree(term->fg_colors);
    if (term->bg_colors) kfree(term->bg_colors);
    kfree(term);
}

struct terminal *term_get_active(void)
{
    return active_terminal;
}

void term_set_active(struct terminal *term)
{
    active_terminal = term;
}
