# AGENTS.md - Vib-OS Development Guide

This document contains essential information for AI agents working with the Vib-OS codebase.

## Project Overview

Vib-OS is a from-scratch, production-grade Unix-like operating system for ARM64 (AArch64). Built with 18,000+ lines of C and Assembly, it features a complete GUI, TCP/IP stack, and runs on Apple Silicon and Raspberry Pi.

### Key Characteristics
- **Architecture**: ARM64 (AArch64)
- **Target Platforms**: QEMU virt (primary), Apple Silicon M1/M2/M3, Raspberry Pi 4/5
- **Build System**: Custom Makefiles
- **Toolchain**: LLVM/Clang (preferred) or GNU toolchain for ARM64
- **Features**: Full GUI with window manager, TCP/IP networking, filesystem support (VFS, ramfs, ext4, APFS)

## Essential Commands

### Building
```bash
make clean          # Clean all build artifacts
make kernel         # Build kernel only
make all            # Build everything (kernel, drivers, libc, userspace, runtimes)
make image          # Create bootable disk image
```

### Platform-Specific Setup
```bash
# macOS (Homebrew)
make toolchain     # Runs scripts/setup-toolchain.sh

# Linux (Ubuntu/Debian)
make toolchain     # Runs scripts/setup-toolchain-linux.sh
# or manually: sudo apt install clang lld qemu-system-arm make
```

### Running
```bash
make run            # Run in QEMU (terminal/text mode only)
make run-gui        # Run in QEMU with GUI display (includes virtio input devices)
make qemu           # Run with boot image (QEMU virt)
make qemu-debug     # Run with GDB server on port 1234
```

### Testing
```bash
make test           # Run test suite (uses scripts/run-tests.sh)
```

### Toolchain Setup
```bash
make toolchain      # Install build dependencies (runs scripts/setup-toolchain.sh)
```

### Component-Specific
```bash
make drivers        # Build device drivers
make libc           # Build C library (musl-based)
make userspace      # Build userspace programs
make runtimes       # Build Python and Node.js runtimes
```

## Code Organization

### Directory Structure
```
vib-OS/
├── kernel/              # Kernel source
│   ├── core/           # Main entry, printk, boot config
│   ├── gui/            # Window manager, compositor, apps (terminal, calculator, etc.)
│   ├── mm/             # Memory management (PMM, VMM, kmalloc)
│   ├── sched/          # Scheduler, fork, signal, context switch
│   ├── net/            # TCP/IP stack (socket, tcp_ip, dns)
│   ├── fs/             # Filesystems (VFS, ramfs, ext4, APFS)
│   ├── ipc/            # IPC mechanisms (pipes)
│   ├── syscall/        # System call handling
│   ├── arch/           # Architecture-specific code (ARM64)
│   │   └── arm64/      # Boot, GIC, timer
│   └── include/        # Kernel headers
├── drivers/            # Device drivers
│   ├── gpu/            # AGX GPU
│   ├── video/          # Bochs, framebuffer, ramfb
│   ├── input/          # Virtio tablet & keyboard
│   ├── usb/            # XHCI controller
│   ├── uart/           # Serial console
│   ├── platform/       # RPi, Apple Silicon platform code
│   ├── nvme/           # NVMe storage
│   └── bluetooth/      # HCI
├── libc/               # C library (musl-based)
├── userspace/          # Userspace programs (init, shell)
├── boot/               # Bootloader support (EFI, GRUB)
├── runtimes/           # Runtime environments (Node.js, Python)
└── scripts/            # Build and setup scripts
```

### Kernel Initialization Flow
See `kernel/core/main.c:kernel_main()`:
1. Early console initialization (`uart_early_init()`)
2. Print banner
3. `init_subsystems()`:
   - Phase 1: Core hardware (GIC, timer)
   - Phase 2: Memory management (PMM, VMM, kmalloc)
   - Phase 3: Process management (scheduler)
   - Phase 4: Filesystems (VFS, mount root)
   - Phase 5: Device drivers & GUI (framebuffer, GUI init)
   - Phase 6: Enable interrupts
4. `start_init_process()`: Main GUI event loop

## Code Conventions

### Style Guidelines
- **Language**: C (kernel), Assembly (ARM64 boot code)
- **Comments**: Multi-line C-style `/* ... */` for file headers and major sections
- **Indentation**: 4 spaces (tabs converted to spaces)
- **Line Length**: Generally under 120 characters
- **File Headers**: Standard format:
  ```c
  /*
   * Vib-OS - Component Name
   *
   * Brief description of what this file does.
   */
  ```

### Naming Conventions
- **Functions**: `snake_case()` - e.g., `kmalloc_init()`, `gui_create_window()`
- **Variables**: `snake_case` - e.g., `heap_start`, `current_task`
- **Types**: `snake_case_t` - e.g., `task_state_t`, `pid_t`
- **Constants/Defines**: `UPPER_SNAKE_CASE` - e.g., `KERNEL_BASE`, `GFP_KERNEL`
- **Structs**: `snake_case` - e.g., `struct task_struct`, `struct socket`
- **Macros**: `UPPER_SNAKE_CASE` - e.g., `MAX_TASKS`, `PAGE_SIZE`
- **Enum values**: `UPPER_SNAKE_CASE` - e.g., `TASK_RUNNING`, `SOCK_STREAM`

### File Organization Patterns
- **Headers**: Located in `kernel/include/` organized by subsystem
- **Source**: `.c` files in subsystem directories
- **Separation**: Interface in headers, implementation in `.c` files
- **Section dividers**: Use comment blocks for logical sections:
  ```c
  /* ===================================================================== */
  /* Section Name */
  /* ===================================================================== */
  ```

### Compiler Flags
```makefile
# Kernel
CFLAGS_KERNEL := -Wall -Wextra -Wno-unused-function -ffreestanding \
                 -fno-stack-protector -fno-pic -mcpu=apple-m2 -O2 -g \
                 -fno-builtin -nostdlib -nostdinc

# Userspace
CFLAGS_USER := -Wall -Wextra -O2 -g --target=aarch64-linux-musl --sysroot=$(SYSROOT)
```

### Memory Management
- **Kernel Heap**: `kmalloc()` / `kfree()` (fixed 8MB at 0x42000000)
- **Physical Pages**: `pmm_alloc_pages()` / `pmm_free_pages()` (buddy allocator)
- **Virtual Memory**: `vmalloc()` / `vfree()` (page table management)
- **Allocation Flags**: `GFP_KERNEL` (normal), `GFP_ATOMIC` (interrupt context), `GFP_ZERO` (zero memory)

### Error Handling
```c
/* Use printk with log levels */
printk(KERN_ERR "Failed to allocate memory\n");

/* Panic on fatal errors */
if (!ptr) {
    panic("Out of memory!");
}

/* Return negative error codes */
return -ENODEV;  /* Common error codes: ENOMEM, EINVAL, ENODEV, etc. */
```

### Debugging
```c
/* Debug print (only when DEBUG is defined) */
#ifdef DEBUG
    printk(KERN_DEBUG "Debug message\n");
#endif

/* Helper macros */
pr_info("Informational message\n");
pr_warn("Warning message\n");
pr_err("Error message: %d\n", error_code);
```

### Assertions
```c
/* Fatal assertion */
BUG_ON(condition);  /* Calls panic() */

/* Non-fatal warning */
WARN_ON(condition);  /* Prints warning */

/* Use (void) to suppress unused warnings */
(void)unused_variable;
```

## Important Patterns

### Function Documentation
```c
/**
 * function_name - Brief description
 * @param1: Description of first parameter
 * @param2: Description of second parameter
 *
 * Extended description (optional).
 *
 * Return: Description of return value
 */
```

### Inline Functions
```c
static inline void helper_function(void) {
    /* ... */
}
```

### Atomic Operations
```c
static volatile int lock = 0;

/* Acquire lock */
while (__atomic_test_and_set(&lock, __ATOMIC_ACQUIRE)) {
    asm volatile("yield");
}

/* Release lock */
__atomic_clear(&lock, __ATOMIC_RELEASE);
```

### Memory Barriers
```c
barrier();   /* Compiler barrier */
mb();        /* Full memory barrier */
rmb();       /* Read memory barrier */
wmb();       /* Write memory barrier */
```

### Container Macros
```c
/* Get struct from member pointer */
container_of(ptr, type, member)

/* Calculate offsets */
offsetof(type, member)

/* Array size */
ARRAY_SIZE(arr)
```

### Alignment Macros
```c
ALIGN(x, a)        /* Align up */
ALIGN_DOWN(x, a)   /* Align down */
IS_ALIGNED(x, a)   /* Check alignment */
```

### Type Conversions
```c
/* Physical to virtual and vice versa */
phys_to_virt(addr)
virt_to_phys(addr)

/* Page frame number conversions */
PHYS_TO_PFN(addr)
PFN_TO_PHYS(pfn)
```

## Memory Layout

### Kernel Memory Map
```
0x40000000        - Kernel physical base (QEMU virt)
0x40200000        - Kernel load address
0x42000000        - Kernel heap base (8MB)
0x42800000        - Kernel heap end
0xFFFF000000000000 - Kernel virtual base
```

### Page Size
- Standard: 4KB pages
- Use `PAGE_SIZE` constant (4096)

### Address Spaces
- **Physical Memory**: Identity-mapped during early boot
- **Kernel Virtual**: High canonical memory (0xFFFF...)
- **User Virtual**: Separate per-process address space

## Architecture-Specific Notes

### ARM64 Context
- **Exception Levels**: EL1 (kernel), EL2 (hypervisor)
- **System Registers**: Access via `mrs`/`msr` assembly
- **Context Switch**: See `kernel/arch/arm64/switch.S`
- **Interrupt Controller**: GIC v3 (Generic Interrupt Controller)
- **Boot Entry**: `_start` in `kernel/arch/arm64/boot.S`

### Inline Assembly
```c
/* Example: Disable interrupts */
asm volatile("msr daifset, #0xf");

/* Example: Enable interrupts */
asm volatile("msr daifclr, #2");

/* Example: Memory barrier */
asm volatile("dmb sy" ::: "memory");

/* Example: Wait for interrupt */
asm volatile("wfi");
```

## GUI System

### Window Management
- **Location**: `kernel/gui/window.c`
- **Key Functions**:
  - `gui_init()` - Initialize GUI system
  - `gui_create_window()` - Create new window
  - `gui_compose()` - Compose and render desktop
  - `gui_draw_cursor()` - Draw mouse cursor

### Event Handling
- **Mouse**: `gui_handle_mouse_event(x, y, buttons)` in `kernel/gui/window.c`
- **Keyboard**: `gui_handle_key_event(key)` in `kernel/gui/window.c`
- **Event Loop**: See `kernel/core/main.c:start_init_process()` line 268-330

### Applications
- **Location**: `kernel/gui/app.c`
- **Structure**: `struct application` with lifecycle callbacks (`on_init`, `on_draw`, `on_exit`)
- **Types**: Terminal, File Manager, Calculator, Notepad, Help, etc.

### Double Buffering
- Implemented in GUI compositor
- Flicker-free rendering
- Use frame buffer for composition

## Network Stack

### Socket API
- **Location**: `kernel/net/socket.c`
- **Functions**: `socket_create()`, `socket_bind()`, `socket_connect()`, `socket_send()`, `socket_recv()`, `socket_close()`
- **Supported Types**: `SOCK_STREAM` (TCP), `SOCK_DGRAM` (UDP)

### Protocol Implementation
- **TCP**: `kernel/net/tcp_ip.c`
- **UDP**: `kernel/net/tcp_ip.c`
- **DNS**: `kernel/net/dns.c`
- **ARP**: `kernel/net/tcp_ip.c`

## Filesystem

### VFS Layer
- **Location**: `kernel/fs/vfs.c`
- **Key Functions**: `vfs_init()`, `vfs_mount()`, `vfs_open()`, `vfs_read()`, `vfs_write()`

### Supported Filesystems
- **ramfs**: `kernel/fs/ramfs.c` - In-memory filesystem
- **ext4**: `kernel/fs/ext4.c` - Linux ext4 filesystem
- **APFS**: `kernel/fs/apfs.c` - Apple filesystem (read-only)

## Testing

### Running Tests
```bash
make test
```

### Test Script
- **Location**: `scripts/run-tests.sh`
- **Coverage**: Build infrastructure, toolchain checks, kernel binary validation

### Manual Testing
- **QEMU virt**: Primary test environment
- **GUI Test**: Use `make run-gui` to test window manager and applications
- **Network Test**: Requires QEMU with networking configured

## Common Gotchas

### Memory Allocation
- **No `malloc()` in kernel**: Use `kmalloc()` instead
- **Interrupt Context**: Use `GFP_ATOMIC` flag, can't sleep
- **Zeroing**: Use `kzalloc()` or manually zero with `memset()`

### Interrupts
- **Disabled During Init**: Interrupts enabled at end of `init_subsystems()`
- **Context Switch**: Can only sleep in process context, not interrupt context
- **Spinlocks**: Use for short critical sections, avoid holding across schedule

### Build System
- **Path Dependent**: Must run from root directory
- **Toolchain**: Expects LLVM at `/opt/homebrew/opt/llvm/bin/` on macOS
- **Cross-Compile**: Always use `--target=aarch64-unknown-none-elf` for kernel

### Drivers
- **No Userspace**: All drivers run in kernel space
- **Memory-Mapped I/O**: Use `volatile` pointers for hardware registers
- **Polling vs Interrupts**: Many drivers still use polling (see TODOs)

### GUI
- **Event Loop**: Must call `input_poll()` continuously (see main.c:276)
- **Double Buffering**: Always use back buffer for rendering
- **Input**: Virtio devices for mouse/keyboard in GUI mode

### Device Tree
- **Not Fully Implemented**: DTB parsing is TODO (see main.c:109)
- **Hardcoded Addresses**: Many hardware addresses are currently hardcoded

## Known Issues and TODOs

### High Priority
- Device tree parsing for hardware discovery
- Proper interrupt handling in drivers
- Complete filesystem implementations (ext4, APFS)
- Full signal handling implementation

### Medium Priority
- DNS timestamp implementation (needs `get_time()`)
- ARP cache aging
- Socket blocking operations
- File manager actual file listing

### Low Priority
- Bluetooth HCI implementation
- NVMe driver completion
- Settings application functionality

## Platform-Specific Notes

### QEMU virt Machine
- **Memory**: Default 4GB
- **CPU**: `max` CPU type
- **UART**: PL011 at 0x09000000
- **Input**: Virtio tablet (mouse) and keyboard
- **Display**: RAMFB for graphics

### Apple Silicon
- **UART**: Samsung S5L compatible at 0x235200000
- **Boot**: UEFI bootloader
- **Framebuffer**: AGX GPU driver

### Raspberry Pi 4/5
- Platform code in `drivers/platform/rpi.c`
- Similar to QEMU virt with actual hardware

## Adding New Features

### Adding a New Kernel Module
1. Create header in `kernel/include/`
2. Create implementation in appropriate subsystem directory
3. Add to main Makefile's source discovery
4. Add initialization call to `init_subsystems()`

### Adding a GUI Application
1. Define app type in `kernel/gui/app.c`
2. Implement `on_init()`, `on_draw()` callbacks
3. Add to application registration
4. Create dock icon in `gui_init()`

### Adding a Device Driver
1. Create driver file in `drivers/`
2. Implement init/cleanup functions
3. Register in `init_subsystems()` phase 5
4. Add to Makefile's driver discovery

## Resources

### Key Files to Understand
- `kernel/core/main.c` - Main entry and initialization
- `kernel/linker.ld` - Kernel memory layout
- `kernel/arch/arm64/boot.S` - Boot assembly
- `kernel/gui/window.c` - GUI window manager
- `kernel/mm/kmalloc.c` - Kernel heap allocator
- `kernel/sched/sched.c` - Scheduler implementation
- `Makefile` - Build system overview

### Header Files for Patterns
- `kernel/include/types.h` - Basic types and macros
- `kernel/include/printk.h` - Logging macros
- `kernel/include/mm/kmalloc.h` - Memory allocation interface
- `kernel/include/sched/sched.h` - Task structure and scheduler
- `kernel/include/net/net.h` - Network structures and constants

## Build Artifacts

- **Kernel Binary**: `build/kernel/unixos.elf`
- **Boot Image**: `image/unixos.img`
- **Sysroot**: `build/sysroot/` (for userspace compilation)
- **Object Files**: `build/kernel/`, `build/drivers/`, `build/userspace/`, etc.

## Debugging Tips

### Using QEMU GDB
```bash
make qemu-debug          # Start QEMU with GDB server on port 1234
aarch64-elf-gdb          # In another terminal
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### Adding Debug Output
```c
printk(KERN_DEBUG "Variable value: %d\n", variable);
```

### Common Issues
- **Kernel Doesn't Boot**: Check linker script addresses and boot entry point
- **GUI Not Rendering**: Verify framebuffer initialization and virtio devices
- **Memory Allocation Fails**: Check PMM initialization and available memory
- **Build Failures**: Verify LLVM toolchain installation and PATH

## Version Information

- **Current Version**: v0.5.0 (see README)
- **Kernel Major/Minor**: Defined in `kernel/core/main.c` lines 19-21
- **Architecture**: ARM64 (AArch64)
- **License**: MIT
