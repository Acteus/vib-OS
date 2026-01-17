/*
 * UnixOS - Minimal C Library Implementation
 * Standard I/O Functions
 */

#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/string.h"
#include "../include/unistd.h"

/* ===================================================================== */
/* FILE structure */
/* ===================================================================== */

struct _FILE {
    int fd;
    int flags;
    int error;
    int eof;
    char *buf;
    size_t buf_size;
    size_t buf_pos;
    int buf_mode;
    int ungetc_char;
    int has_ungetc;
};

#define FILE_READ   1
#define FILE_WRITE  2

/* Standard streams */
static FILE _stdin  = { .fd = 0, .flags = FILE_READ,  .buf_mode = _IOLBF };
static FILE _stdout = { .fd = 1, .flags = FILE_WRITE, .buf_mode = _IOLBF };
static FILE _stderr = { .fd = 2, .flags = FILE_WRITE, .buf_mode = _IONBF };

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* ===================================================================== */
/* Basic I/O */
/* ===================================================================== */

int fgetc(FILE *stream)
{
    if (!stream) return EOF;
    
    if (stream->has_ungetc) {
        stream->has_ungetc = 0;
        return stream->ungetc_char;
    }
    
    unsigned char c;
    ssize_t n = read(stream->fd, &c, 1);
    
    if (n <= 0) {
        if (n == 0) stream->eof = 1;
        else stream->error = 1;
        return EOF;
    }
    
    return c;
}

char *fgets(char *s, int size, FILE *stream)
{
    if (!s || size <= 0 || !stream) return NULL;
    
    int i = 0;
    int c;
    
    while (i < size - 1) {
        c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        s[i++] = c;
        if (c == '\n') break;
    }
    
    s[i] = '\0';
    return s;
}

int fputc(int c, FILE *stream)
{
    if (!stream) return EOF;
    
    unsigned char ch = c;
    ssize_t n = write(stream->fd, &ch, 1);
    
    if (n != 1) {
        stream->error = 1;
        return EOF;
    }
    
    return (unsigned char)c;
}

int fputs(const char *s, FILE *stream)
{
    if (!s || !stream) return EOF;
    
    size_t len = strlen(s);
    ssize_t n = write(stream->fd, s, len);
    
    if (n != (ssize_t)len) {
        stream->error = 1;
        return EOF;
    }
    
    return 0;
}

int getc(FILE *stream) { return fgetc(stream); }
int getchar(void) { return fgetc(stdin); }
int putc(int c, FILE *stream) { return fputc(c, stream); }
int putchar(int c) { return fputc(c, stdout); }

int puts(const char *s)
{
    if (fputs(s, stdout) == EOF) return EOF;
    if (fputc('\n', stdout) == EOF) return EOF;
    return 0;
}

int ungetc(int c, FILE *stream)
{
    if (!stream || c == EOF) return EOF;
    stream->ungetc_char = c;
    stream->has_ungetc = 1;
    stream->eof = 0;
    return c;
}

/* ===================================================================== */
/* Read/Write */
/* ===================================================================== */

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    
    size_t total = size * nmemb;
    ssize_t n = read(stream->fd, ptr, total);
    
    if (n <= 0) {
        if (n == 0) stream->eof = 1;
        else stream->error = 1;
        return 0;
    }
    
    return n / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0) return 0;
    
    size_t total = size * nmemb;
    ssize_t n = write(stream->fd, ptr, total);
    
    if (n < 0) {
        stream->error = 1;
        return 0;
    }
    
    return n / size;
}

int fflush(FILE *stream)
{
    (void)stream;
    /* No buffering implemented yet */
    return 0;
}

/* ===================================================================== */
/* Seek/Tell */
/* ===================================================================== */

int fseek(FILE *stream, long offset, int whence)
{
    if (!stream) return -1;
    
    off_t result = lseek(stream->fd, offset, whence);
    if (result < 0) return -1;
    
    stream->eof = 0;
    stream->has_ungetc = 0;
    return 0;
}

long ftell(FILE *stream)
{
    if (!stream) return -1;
    return lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE *stream)
{
    if (stream) {
        fseek(stream, 0, SEEK_SET);
        stream->error = 0;
    }
}

/* ===================================================================== */
/* Status */
/* ===================================================================== */

int feof(FILE *stream)
{
    return stream ? stream->eof : 0;
}

int ferror(FILE *stream)
{
    return stream ? stream->error : 0;
}

void clearerr(FILE *stream)
{
    if (stream) {
        stream->error = 0;
        stream->eof = 0;
    }
}

int fileno(FILE *stream)
{
    return stream ? stream->fd : -1;
}

/* ===================================================================== */
/* Formatted output */
/* ===================================================================== */

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    char *p = str;
    char *end = size > 0 ? str + size - 1 : str;
    char numbuf[32];
    
    while (*format && p < end) {
        if (*format != '%') {
            *p++ = *format++;
            continue;
        }
        
        format++;  /* Skip '%' */
        
        /* Flags */
        int zero_pad = 0;
        int width = 0;
        int is_long = 0;
        
        while (*format == '0') {
            zero_pad = 1;
            format++;
        }
        
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }
        
        if (*format == 'l') {
            is_long = 1;
            format++;
            if (*format == 'l') {
                format++;
            }
        } else if (*format == 'z') {
            is_long = 1;
            format++;
        }
        
        switch (*format) {
            case 'd':
            case 'i': {
                long val = is_long ? va_arg(ap, long) : va_arg(ap, int);
                int neg = val < 0;
                if (neg) val = -val;
                
                int i = 0;
                do {
                    numbuf[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val > 0);
                
                if (neg) numbuf[i++] = '-';
                
                while (i < width && p < end && zero_pad) {
                    *p++ = neg ? (--i == 0 ? '-' : '0') : '0';
                }
                
                while (i > 0 && p < end) {
                    *p++ = numbuf[--i];
                }
                break;
            }
            
            case 'u': {
                unsigned long val = is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                int i = 0;
                do {
                    numbuf[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val > 0);
                
                while (i < width && p < end) {
                    *p++ = zero_pad ? '0' : ' ';
                }
                while (i > 0 && p < end) {
                    *p++ = numbuf[--i];
                }
                break;
            }
            
            case 'x':
            case 'X': {
                unsigned long val = is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                const char *digits = (*format == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                int i = 0;
                do {
                    numbuf[i++] = digits[val & 0xf];
                    val >>= 4;
                } while (val > 0);
                
                while (i < width && p < end) {
                    *p++ = zero_pad ? '0' : ' ';
                }
                while (i > 0 && p < end) {
                    *p++ = numbuf[--i];
                }
                break;
            }
            
            case 'p': {
                unsigned long val = (unsigned long)va_arg(ap, void *);
                if (p < end) *p++ = '0';
                if (p < end) *p++ = 'x';
                
                int i = 0;
                for (int j = 0; j < 16; j++) {
                    numbuf[i++] = "0123456789abcdef"[val & 0xf];
                    val >>= 4;
                }
                while (i > 0 && p < end) {
                    *p++ = numbuf[--i];
                }
                break;
            }
            
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s && p < end) {
                    *p++ = *s++;
                }
                break;
            }
            
            case 'c':
                if (p < end) *p++ = (char)va_arg(ap, int);
                break;
                
            case '%':
                if (p < end) *p++ = '%';
                break;
                
            default:
                if (p < end) *p++ = '%';
                if (p < end) *p++ = *format;
                break;
        }
        
        format++;
    }
    
    if (size > 0) *p = '\0';
    return p - str;
}

int vsprintf(char *str, const char *format, va_list ap)
{
    return vsnprintf(str, (size_t)-1, format, ap);
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(str, format, ap);
    va_end(ap);
    return ret;
}

int vfprintf(FILE *stream, const char *format, va_list ap)
{
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), format, ap);
    if (len > 0) {
        write(stream->fd, buf, len);
    }
    return len;
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int vprintf(const char *format, va_list ap)
{
    return vfprintf(stdout, format, ap);
}

int printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

/* ===================================================================== */
/* Error handling */
/* ===================================================================== */

void perror(const char *s)
{
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs("Error\n", stderr);
}
