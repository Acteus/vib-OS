/*
 * UnixOS - Minimal C Library (libc)
 * 
 * Provides basic C runtime support for userspace programs.
 * Based on musl libc design.
 */

#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H

/* Standard file descriptors */
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

/* Types */
typedef long ssize_t;
typedef unsigned long size_t;
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long off_t;

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* ===================================================================== */
/* System call wrappers */
/* ===================================================================== */

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int open(const char *pathname, int flags, ...);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);

pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
void _exit(int status) __attribute__((noreturn));

pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);

int pipe(int pipefd[2]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);

unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usec);

/* ===================================================================== */
/* Seek whence values */
/* ===================================================================== */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif /* _LIBC_UNISTD_H */
