/*
 * UnixOS - Init Program (PID 1)
 * 
 * The first userspace process started by the kernel.
 * Responsible for:
 * - Mounting filesystems
 * - Starting essential services
 * - Spawning login/shell
 */

/* Minimal syscall interface */

#define SYS_write       64
#define SYS_exit        93
#define SYS_getpid      172
#define SYS_uname       160

/* Inline syscall */
static inline long syscall0(long nr)
{
    register long x0 __asm__("x0");
    register long x8 __asm__("x8") = nr;
    
    __asm__ volatile(
        "svc #0"
        : "=r" (x0)
        : "r" (x8)
        : "memory"
    );
    
    return x0;
}

static inline long syscall1(long nr, long a0)
{
    register long x0 __asm__("x0") = a0;
    register long x8 __asm__("x8") = nr;
    
    __asm__ volatile(
        "svc #0"
        : "+r" (x0)
        : "r" (x8)
        : "memory"
    );
    
    return x0;
}

static inline long syscall3(long nr, long a0, long a1, long a2)
{
    register long x0 __asm__("x0") = a0;
    register long x1 __asm__("x1") = a1;
    register long x2 __asm__("x2") = a2;
    register long x8 __asm__("x8") = nr;
    
    __asm__ volatile(
        "svc #0"
        : "+r" (x0)
        : "r" (x1), "r" (x2), "r" (x8)
        : "memory"
    );
    
    return x0;
}

/* Simple string length */
static int strlen(const char *s)
{
    int len = 0;
    while (*s++) len++;
    return len;
}

/* Write to stdout */
static void print(const char *msg)
{
    syscall3(SYS_write, 1, (long)msg, strlen(msg));
}

/* Get PID */
static long getpid(void)
{
    return syscall0(SYS_getpid);
}

/* Exit */
static void exit(int code)
{
    syscall1(SYS_exit, code);
    while (1) {}  /* Never reached */
}

/* ===================================================================== */
/* Init main */
/* ===================================================================== */

void _start(void)
{
    long pid = getpid();
    
    print("\n");
    print("=====================================\n");
    print("  UnixOS Init System\n");
    print("=====================================\n");
    print("\n");
    
    print("[init] Starting as PID ");
    /* Simple number print */
    char num[2] = { '0' + (pid % 10), '\0' };
    print(num);
    print("\n");
    
    print("[init] UnixOS initialized successfully!\n");
    print("\n");
    print("[init] System ready.\n");
    print("\n");
    
    /* Display system info */
    print("  _   _       _       ___  ____  \n");
    print(" | | | |_ __ (_)_  __/ _ \\/ ___| \n");
    print(" | | | | '_ \\| \\ \\/ / | | \\___ \\ \n");
    print(" | |_| | | | | |>  <| |_| |___) |\n");
    print("  \\___/|_| |_|_/_/\\_\\\\___/|____/ \n");
    print("\n");
    print("Welcome to UnixOS!\n");
    print("Type 'help' for available commands.\n");
    print("\n");
    
    /* In a real init, we would:
     * 1. Mount /proc, /sys, /dev
     * 2. Start essential services
     * 3. Fork and exec /bin/login or /bin/sh
     * 4. Wait for children and handle orphans
     */
    
    print("[init] Entering main loop...\n");
    
    /* Init main loop - never exits */
    while (1) {
        /* Wait for children, handle signals */
        /* For now, just idle */
        __asm__ volatile("wfi");
    }
    
    /* Never reached */
    exit(0);
}
