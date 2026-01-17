# vib-OS

**A Production-Grade ARM64 Operating System for Apple Silicon**

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-ARM64%20%7C%20Apple%20M2-blue)
![License](https://img.shields.io/badge/license-MIT-green)

```
  _   _       _       ___  ____  
 | | | |_ __ (_)_  __/ _ \/ ___| 
 | | | | '_ \| \ \/ / | | \___ \ 
 | |_| | | | | |>  <| |_| |___) |
  \___/|_| |_|_/_/\_\\___/|____/ 

vib-OS v0.2.0 - ARM64
```

## Overview

vib-OS is a from-scratch, production-grade Unix-like operating system designed for Apple M2 Macs. Built with over **10,000 lines** of C and ARM64 Assembly, it features:

- ✅ **ARM64 Kernel** with 4-level MMU page tables
- ✅ **GICv3** interrupt controller support
- ✅ **Process Management** with fork/exec
- ✅ **VFS** with ramfs filesystem
- ✅ **BSD Socket API** with TCP/IP stack
- ✅ **Asahi Linux Driver Scaffolding** for Apple hardware

## Quick Start

### Build Requirements

- macOS with Homebrew
- LLVM/Clang (`brew install llvm`)
- QEMU for ARM64 (`brew install qemu`)
- LLD Linker (`brew install lld`)

### Build & Run

```bash
# Clone the repository
git clone git@github.com:viralcode/vib-OS.git
cd vib-OS

# Build the kernel
make kernel

# Run in QEMU
make run
# Or manually:
qemu-system-aarch64 -M virt,gic-version=3 -cpu max -m 4G \
    -nographic -kernel build/kernel/unixos.elf
```

### Expected Boot Output

```
vib-OS v0.2.0 - ARM64

[INIT] Phase 1: Core Hardware
  GIC: Distributor supports 288 IRQs
  TIMER: Initialized

[INIT] Phase 2: Memory Management
  Physical memory: 255 MB available
  VMM: MMU enabled! Page tables active.

[INIT] Phase 3: Process Management
  SCHED: Scheduler initialized

[INIT] Phase 4: Filesystems
  VFS: Initialized

[INIT] Phase 5: Device Drivers
  Loading display, keyboard, NVMe, USB, network drivers...

[INIT] Kernel initialization complete!
System ready.
```

## Architecture

```
vib-OS/
├── kernel/                 # Kernel source code
│   ├── arch/arm64/        # ARM64-specific code
│   │   ├── boot.S         # Boot assembly, exception vectors
│   │   ├── gic.c          # GICv3 interrupt controller
│   │   └── timer.c        # ARM Generic Timer
│   ├── core/              # Core kernel
│   │   ├── main.c         # Kernel entry point
│   │   └── printk.c       # Kernel console output
│   ├── mm/                # Memory management
│   │   ├── pmm.c          # Physical memory (buddy allocator)
│   │   ├── vmm.c          # Virtual memory (page tables)
│   │   └── kmalloc.c      # Kernel heap
│   ├── sched/             # Process scheduling
│   │   ├── sched.c        # Round-robin scheduler
│   │   ├── fork.c         # fork/exec implementation
│   │   └── switch.S       # Context switch assembly
│   ├── fs/                # Filesystems
│   │   ├── vfs.c          # Virtual filesystem layer
│   │   └── ramfs.c        # RAM filesystem
│   ├── net/               # Networking
│   │   └── socket.c       # BSD socket API
│   ├── syscall/           # System calls
│   │   └── syscall.c      # Syscall dispatcher
│   └── ipc/               # Inter-process communication
│       └── pipe.c         # Pipe implementation
├── drivers/               # Device drivers
│   ├── uart/              # Serial console
│   ├── gpu/               # Apple AGX GPU
│   └── nvme/              # Apple ANS NVMe
├── libc/                  # Minimal C library
│   ├── src/               # C library source
│   └── include/           # C library headers
├── userspace/             # Userspace programs
│   ├── init/              # Init process (PID 1)
│   └── shell/             # Shell implementation
└── runtimes/              # Language runtime scaffolding
    ├── python/            # CPython 3.12 port
    └── nodejs/            # Node.js 20 LTS port
```

## Features

### Kernel Components

| Component | Status | Description |
|-----------|--------|-------------|
| MMU | ✅ | 4-level page tables, identity mapping |
| GIC | ✅ | GICv3 interrupt controller |
| Timer | ✅ | ARM Generic Timer |
| PMM | ✅ | Buddy allocator for physical memory |
| VMM | ✅ | Virtual memory with TLB flush |
| Scheduler | ✅ | Round-robin with run queue |
| VFS | ✅ | Unix-like filesystem abstraction |
| Ramfs | ✅ | In-memory filesystem |
| Syscalls | ✅ | 20+ Linux-compatible syscalls |
| Fork/Exec | ✅ | ELF loading, address space creation |
| Network | ✅ | BSD socket API |

### Drivers (Asahi Linux Based)

| Driver | Status | Description |
|--------|--------|-------------|
| UART | ✅ | PL011 (QEMU) + Apple S5L |
| GPU (AGX) | 🔄 | Apple GPU framebuffer |
| NVMe (ANS) | 🔄 | Apple storage controller |
| USB | 📋 | XHCI support planned |
| Keyboard | 📋 | SPI keyboard driver |
| Display | 📋 | Display Coprocessor (DCP) |

### Networking Stack

| Layer | Status | Description |
|-------|--------|-------------|
| Ethernet | ✅ | Frame handling |
| ARP | ✅ | Address resolution |
| IPv4/IPv6 | ✅ | IP packet routing |
| ICMP | ✅ | Ping support |
| UDP | ✅ | Datagram sockets |
| TCP | ✅ | Stream sockets |
| DNS | 📋 | Name resolution |
| HTTP | 📋 | Client support |
| TLS 1.2 | 📋 | HTTPS support |

## System Calls

```c
// Process management
pid_t fork(void);
int execve(const char *path, char *const argv[], char *const envp[]);
void exit(int status);
pid_t getpid(void);
pid_t waitpid(pid_t pid, int *status, int options);

// File I/O
int open(const char *path, int flags, mode_t mode);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);

// Memory management
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);
void *brk(void *addr);

// Networking
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t len);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *len);
int connect(int sockfd, const struct sockaddr *addr, socklen_t len);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

## Roadmap

- [x] ARM64 kernel boot
- [x] MMU with page tables
- [x] Process scheduler
- [x] Fork/exec
- [x] VFS and ramfs
- [x] BSD socket API
- [ ] Full TCP/IP stack
- [ ] ext4 filesystem
- [ ] APFS read support
- [ ] Complete Asahi drivers
- [ ] CPython 3.12 port
- [ ] Node.js 20 port
- [ ] GUI subsystem

## Building from Source

### Prerequisites

```bash
# Install toolchain
brew install llvm qemu lld coreutils

# Verify installation
/opt/homebrew/opt/llvm/bin/clang --version
qemu-system-aarch64 --version
```

### Build Commands

```bash
make clean          # Clean build artifacts
make kernel         # Build kernel only
make drivers        # Build drivers
make libc           # Build C library
make userspace      # Build userspace programs
make all            # Build everything
make run            # Run in QEMU
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Asahi Linux](https://asahilinux.org/) - Apple Silicon reverse engineering
- [OSDev Wiki](https://wiki.osdev.org/) - OS development resources
- [ARM Architecture Reference Manual](https://developer.arm.com/) - ARM64 documentation

---

**vib-OS** - Built with ❤️ for Apple Silicon
