/*
 * UnixOS - Minimal C Library Implementation
 * System Call Interface
 */

#include "../include/unistd.h"

/* ARM64 syscall numbers */
#define __NR_read           63
#define __NR_write          64
#define __NR_openat         56
#define __NR_close          57
#define __NR_lseek          62
#define __NR_exit           93
#define __NR_exit_group     94
#define __NR_getpid         172
#define __NR_getppid        173
#define __NR_getuid         174
#define __NR_geteuid        175
#define __NR_getgid         176
#define __NR_getegid        177
#define __NR_clone          220
#define __NR_execve         221
#define __NR_chdir          49
#define __NR_getcwd         17
#define __NR_pipe2          59
#define __NR_dup            23
#define __NR_dup3           24
#define __NR_nanosleep      101

/* Inline syscall macros for ARM64 */
static inline long __syscall0(long n)
{
    register long x0 __asm__("x0");
    register long x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "=r"(x0) : "r"(x8) : "memory");
    return x0;
}

static inline long __syscall1(long n, long a)
{
    register long x0 __asm__("x0") = a;
    register long x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
}

static inline long __syscall2(long n, long a, long b)
{
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    return x0;
}

static inline long __syscall3(long n, long a, long b, long c)
{
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}

static inline long __syscall4(long n, long a, long b, long c, long d)
{
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x8 __asm__("x8") = n;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
    return x0;
}

/* ===================================================================== */
/* System call wrappers */
/* ===================================================================== */

ssize_t read(int fd, void *buf, size_t count)
{
    return __syscall3(__NR_read, fd, (long)buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    return __syscall3(__NR_write, fd, (long)buf, count);
}

int open(const char *pathname, int flags, ...)
{
    /* Use openat with AT_FDCWD */
    return __syscall4(__NR_openat, -100, (long)pathname, flags, 0);
}

int close(int fd)
{
    return __syscall1(__NR_close, fd);
}

off_t lseek(int fd, off_t offset, int whence)
{
    return __syscall3(__NR_lseek, fd, offset, whence);
}

pid_t fork(void)
{
    /* Clone with SIGCHLD */
    return __syscall4(__NR_clone, 17, 0, 0, 0);  /* 17 = SIGCHLD */
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
    return __syscall3(__NR_execve, (long)pathname, (long)argv, (long)envp);
}

void _exit(int status)
{
    __syscall1(__NR_exit_group, status);
    while (1) {}  /* Never reached */
}

pid_t getpid(void)
{
    return __syscall0(__NR_getpid);
}

pid_t getppid(void)
{
    return __syscall0(__NR_getppid);
}

uid_t getuid(void)
{
    return __syscall0(__NR_getuid);
}

uid_t geteuid(void)
{
    return __syscall0(__NR_geteuid);
}

gid_t getgid(void)
{
    return __syscall0(__NR_getgid);
}

gid_t getegid(void)
{
    return __syscall0(__NR_getegid);
}

int chdir(const char *path)
{
    return __syscall1(__NR_chdir, (long)path);
}

char *getcwd(char *buf, size_t size)
{
    long ret = __syscall2(__NR_getcwd, (long)buf, size);
    return ret > 0 ? buf : NULL;
}

int pipe(int pipefd[2])
{
    return __syscall2(__NR_pipe2, (long)pipefd, 0);
}

int dup(int oldfd)
{
    return __syscall1(__NR_dup, oldfd);
}

int dup2(int oldfd, int newfd)
{
    return __syscall3(__NR_dup3, oldfd, newfd, 0);
}

unsigned int sleep(unsigned int seconds)
{
    struct {
        long tv_sec;
        long tv_nsec;
    } ts = { seconds, 0 };
    
    __syscall2(__NR_nanosleep, (long)&ts, 0);
    return 0;
}

int usleep(unsigned int usec)
{
    struct {
        long tv_sec;
        long tv_nsec;
    } ts = { usec / 1000000, (usec % 1000000) * 1000 };
    
    return __syscall2(__NR_nanosleep, (long)&ts, 0);
}
