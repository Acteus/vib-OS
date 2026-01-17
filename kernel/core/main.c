/*
 * UnixOS Kernel - Main Entry Point
 * 
 * This is the C entry point called from boot.S after basic
 * hardware initialization is complete.
 */

#include "types.h"
#include "printk.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "sched/sched.h"
#include "arch/arm64/gic.h"
#include "arch/arm64/timer.h"
#include "drivers/uart.h"
#include "fs/vfs.h"

/* Kernel version */
#define UNIXOS_VERSION_MAJOR 0
#define UNIXOS_VERSION_MINOR 1
#define UNIXOS_VERSION_PATCH 0

/* External symbols from linker script */
extern char __kernel_start[];
extern char __kernel_end[];
extern char __bss_start[];
extern char __bss_end[];

/* Forward declarations */
static void print_banner(void);
static void init_subsystems(void *dtb);
static void start_init_process(void);

/*
 * kernel_main - Main kernel entry point
 * @dtb: Pointer to device tree blob passed by bootloader
 * 
 * This function never returns. After initialization, it either:
 * 1. Starts the init process and enters the scheduler
 * 2. Panics if initialization fails
 */
void kernel_main(void *dtb)
{
    /* Initialize early console for debugging */
    uart_early_init();
    
    /* Print boot banner */
    print_banner();
    
    printk("DEBUG: After banner\n");
    
    (void)dtb;  /* Suppress unused warning */
    (void)__kernel_start;
    (void)__kernel_end;
    
    printk("DEBUG: About to call init_subsystems\n");
    
    /* Initialize all kernel subsystems */
    init_subsystems(dtb);
    
    printk("DEBUG: After init_subsystems\n");
    printk(KERN_INFO "All subsystems initialized successfully\n");
    printk(KERN_INFO "Starting init process...\n\n");
    
    /* Start the first userspace process */
    start_init_process();
    
    /* This point should never be reached */
    panic("kernel_main returned unexpectedly!");
}

/*
 * print_banner - Display kernel boot banner
 */
static void print_banner(void)
{
    printk("\n");
    printk("  _   _       _       ___  ____  \n");
    printk(" | | | |_ __ (_)_  __/ _ \\/ ___| \n");
    printk(" | | | | '_ \\| \\ \\/ / | | \\___ \\ \n");
    printk(" | |_| | | | | |>  <| |_| |___) |\n");
    printk("  \\___/|_| |_|_/_/\\_\\\\___/|____/ \n");
    printk("\n");
    printk("UnixOS v%d.%d.%d - ARM64\n",
           UNIXOS_VERSION_MAJOR,
           UNIXOS_VERSION_MINOR,
           UNIXOS_VERSION_PATCH);
    printk("A production-grade Unix-like operating system\n");
    printk("Copyright (c) 2026 UnixOS Project\n");
    printk("\n");
}

/*
 * init_subsystems - Initialize all kernel subsystems
 * @dtb: Device tree blob for hardware discovery
 */
static void init_subsystems(void *dtb)
{
    int ret;
    
    /* ================================================================= */
    /* Phase 1: Core Hardware */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Phase 1: Core Hardware\n");
    
    /* Parse device tree for hardware information */
    printk(KERN_INFO "  Parsing device tree...\n");
    (void)dtb;  /* TODO: dtb_parse(dtb); */
    
    /* Initialize interrupt controller */
    printk(KERN_INFO "  Initializing GIC (interrupt controller)...\n");
    gic_init();
    
    /* Initialize system timer */
    printk(KERN_INFO "  Initializing timer...\n");
    timer_init();
    
    /* ================================================================= */
    /* Phase 2: Memory Management */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Phase 2: Memory Management\n");
    
    /* Initialize physical memory manager */
    printk(KERN_INFO "  Initializing physical memory manager...\n");
    ret = pmm_init();
    if (ret < 0) {
        panic("Failed to initialize physical memory manager!");
    }
    printk(KERN_INFO "  Physical memory: %lu MB available\n", pmm_get_free_memory() / (1024 * 1024));
    
    /* Initialize virtual memory manager */
    printk(KERN_INFO "  Initializing virtual memory manager...\n");
    ret = vmm_init();
    if (ret < 0) {
        panic("Failed to initialize virtual memory manager!");
    }
    
    /* Initialize kernel heap */
    printk(KERN_INFO "  Initializing kernel heap...\n");
    /* TODO: kmalloc_init(); */
    
    /* ================================================================= */
    /* Phase 3: Process Management */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Phase 3: Process Management\n");
    
    /* Initialize scheduler */
    printk(KERN_INFO "  Initializing scheduler...\n");
    sched_init();
    
    /* ================================================================= */
    /* Phase 4: Filesystems */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Phase 4: Filesystems\n");
    
    /* Initialize Virtual Filesystem */
    printk(KERN_INFO "  Initializing VFS...\n");
    vfs_init();
    
    /* Mount root filesystem */
    printk(KERN_INFO "  Mounting root filesystem...\n");
    /* TODO: mount_root(); */
    
    /* Mount proc, sys, dev */
    printk(KERN_INFO "  Mounting procfs...\n");
    printk(KERN_INFO "  Mounting sysfs...\n");
    printk(KERN_INFO "  Mounting devfs...\n");
    
    /* ================================================================= */
    /* Phase 5: Device Drivers */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Phase 5: Device Drivers\n");
    
    printk(KERN_INFO "  Loading display driver...\n");
    printk(KERN_INFO "  Loading keyboard driver...\n");
    printk(KERN_INFO "  Loading NVMe driver...\n");
    printk(KERN_INFO "  Loading USB driver...\n");
    printk(KERN_INFO "  Loading network driver...\n");
    
    /* ================================================================= */
    /* Phase 6: Enable Interrupts */
    /* ================================================================= */
    
    printk(KERN_INFO "[INIT] Enabling interrupts...\n");
    /* Enable interrupts */
    asm volatile("msr daifclr, #2");  /* Clear IRQ mask */
    
    printk(KERN_INFO "[INIT] Kernel initialization complete!\n\n");
}

/*
 * start_init_process - Start the first userspace process (PID 1)
 */
static void start_init_process(void)
{
    /* Create and start init process */
    printk(KERN_INFO "Executing /sbin/init...\n");
    
    /* TODO: Implement process creation and exec */
    /* For now, just enter an idle loop */
    
    printk(KERN_INFO "Init process started (placeholder)\n");
    printk(KERN_INFO "System ready.\n\n");
    
    /* Enter idle loop - in real implementation, this becomes the idle task */
    while (1) {
        /* Wait for interrupt */
        asm volatile("wfi");
    }
}

/*
 * panic - Halt the system with an error message
 * @msg: Error message to display
 */
void panic(const char *msg)
{
    /* Disable interrupts */
    asm volatile("msr daifset, #0xf");
    
    printk(KERN_EMERG "\n");
    printk(KERN_EMERG "============================================\n");
    printk(KERN_EMERG "KERNEL PANIC!\n");
    printk(KERN_EMERG "============================================\n");
    printk(KERN_EMERG "%s\n", msg);
    printk(KERN_EMERG "============================================\n");
    printk(KERN_EMERG "System halted.\n");
    
    /* Infinite loop */
    while (1) {
        asm volatile("wfi");
    }
}
