/*
 * UnixOS Kernel - ARM64 Boot Assembly
 * Entry point for the kernel after UEFI bootloader
 * 
 * This file handles:
 * - Initial CPU state setup
 * - Stack initialization  
 * - BSS clearing
 * - Jump to C kernel main
 */

.section .text.boot
.global _start
.extern kernel_main
.extern __bss_start
.extern __bss_end
.extern __stack_top

/*
 * Kernel entry point
 * Called by UEFI or bootloader with:
 * - x0: Pointer to device tree blob (DTB)
 * - x1: 0 (reserved)
 * - x2: 0 (reserved)
 * - x3: 0 (reserved)
 * 
 * CPU state on entry:
 * - EL1 (Exception Level 1) or EL2
 * - MMU may be on with identity mapping
 * - Caches enabled
 */
_start:
    /* ================================================================= */
    /* Disable interrupts during initialization */
    /* ================================================================= */
    msr     daifset, #0xf
    
    /* ================================================================= */
    /* Save device tree pointer before we use x0 */
    /* ================================================================= */
    mov     x19, x0             /* Save DTB pointer in callee-saved register */
    
    /* ================================================================= */
    /* Check current exception level */
    /* ================================================================= */
    mrs     x1, CurrentEL
    and     x1, x1, #0xC        /* Extract EL bits */
    cmp     x1, #0x8            /* EL2? */
    beq     el2_setup
    cmp     x1, #0x4            /* EL1? */
    beq     el1_setup
    b       halt                /* Unknown EL, halt */

el2_setup:
    /* ================================================================= */
    /* Configure EL2 -> EL1 transition */
    /* ================================================================= */
    
    /* Set EL1 to use AArch64 */
    mov     x0, #(1 << 31)      /* RW bit: EL1 is AArch64 */
    orr     x0, x0, #(1 << 1)   /* SWIO: Set/Way Invalidation Override */
    msr     hcr_el2, x0
    
    /* Set up SPSR for return to EL1 */
    mov     x0, #0x3c5          /* DAIF masked, EL1h (SP_EL1) */
    msr     spsr_el2, x0
    
    /* Set return address to el1_setup */
    adr     x0, el1_setup
    msr     elr_el2, x0
    
    /* Return to EL1 */
    eret

el1_setup:
    /* ================================================================= */
    /* Configure EL1 System Registers */
    /* ================================================================= */
    
    /* Configure SCTLR_EL1 (System Control Register) */
    /* Start with MMU disabled for initial boot */
    mrs     x0, sctlr_el1
    bic     x0, x0, #(1 << 0)   /* M: Disable MMU initially */
    bic     x0, x0, #(1 << 2)   /* C: Disable data cache initially */
    bic     x0, x0, #(1 << 12)  /* I: Disable instruction cache initially */
    bic     x0, x0, #(1 << 1)   /* A: Disable alignment check */
    msr     sctlr_el1, x0
    isb
    
    /* ================================================================= */
    /* Install exception vector table */
    /* ================================================================= */
    ldr     x0, =exception_vectors
    msr     vbar_el1, x0
    isb
    
    /* ================================================================= */
    /* Set up kernel stack */
    /* ================================================================= */
    ldr     x0, =__stack_top
    mov     sp, x0
    
    /* ================================================================= */
    /* Clear BSS section */
    /* ================================================================= */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    
clear_bss:
    cmp     x0, x1
    bge     bss_done
    str     xzr, [x0], #8       /* Store zero and increment */
    b       clear_bss
    
bss_done:
    /* ================================================================= */
    /* Initialize FPU/SIMD */
    /* ================================================================= */
    mrs     x0, cpacr_el1
    orr     x0, x0, #(3 << 20)  /* FPEN: Enable FP/SIMD at EL1 */
    msr     cpacr_el1, x0
    isb
    
    /* ================================================================= */
    /* Call kernel_main(dtb_ptr) */
    /* ================================================================= */
    mov     x0, x19             /* Restore DTB pointer as first argument */
    bl      kernel_main
    
    /* ================================================================= */
    /* kernel_main returned - halt the system */
    /* ================================================================= */
halt:
    wfi                         /* Wait for interrupt (low power) */
    b       halt                /* Loop forever */

/* ===================================================================== */
/* Exception Vector Table */
/* Must be aligned to 2KB (0x800) boundary */
/* ===================================================================== */
.balign 0x800
.global exception_vectors
exception_vectors:
    /* ----------------------------------------------------------------- */
    /* Current EL with SP_EL0 */
    /* ----------------------------------------------------------------- */
    .balign 0x80
    b       sync_exception_handler      /* Synchronous */
    .balign 0x80
    b       irq_handler                 /* IRQ */
    .balign 0x80
    b       fiq_handler                 /* FIQ */
    .balign 0x80
    b       serror_handler              /* SError */
    
    /* ----------------------------------------------------------------- */
    /* Current EL with SP_ELx */
    /* ----------------------------------------------------------------- */
    .balign 0x80
    b       sync_exception_handler
    .balign 0x80
    b       irq_handler
    .balign 0x80
    b       fiq_handler
    .balign 0x80
    b       serror_handler
    
    /* ----------------------------------------------------------------- */
    /* Lower EL using AArch64 */
    /* ----------------------------------------------------------------- */
    .balign 0x80
    b       sync_exception_lower        /* Synchronous (syscalls) */
    .balign 0x80
    b       irq_handler
    .balign 0x80
    b       fiq_handler
    .balign 0x80
    b       serror_handler
    
    /* ----------------------------------------------------------------- */
    /* Lower EL using AArch32 */
    /* ----------------------------------------------------------------- */
    .balign 0x80
    b       sync_exception_handler
    .balign 0x80
    b       irq_handler
    .balign 0x80
    b       fiq_handler
    .balign 0x80
    b       serror_handler

/* ===================================================================== */
/* Exception Handlers (stubs - implemented in C) */
/* ===================================================================== */

.extern handle_sync_exception
.extern handle_irq
.extern handle_syscall

sync_exception_handler:
    /* Save all registers */
    sub     sp, sp, #272
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x30, x0, [sp, #240]
    str     x1, [sp, #256]
    
    /* Call C handler */
    mov     x0, sp
    bl      handle_sync_exception
    
    /* Restore registers */
    ldp     x30, x0, [sp, #240]
    msr     elr_el1, x0
    ldr     x0, [sp, #256]
    msr     spsr_el1, x0
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldp     x8, x9, [sp, #64]
    ldp     x10, x11, [sp, #80]
    ldp     x12, x13, [sp, #96]
    ldp     x14, x15, [sp, #112]
    ldp     x16, x17, [sp, #128]
    ldp     x18, x19, [sp, #144]
    ldp     x20, x21, [sp, #160]
    ldp     x22, x23, [sp, #176]
    ldp     x24, x25, [sp, #192]
    ldp     x26, x27, [sp, #208]
    ldp     x28, x29, [sp, #224]
    add     sp, sp, #272
    eret

sync_exception_lower:
    /* System call from userspace */
    sub     sp, sp, #272
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x30, x0, [sp, #240]
    str     x1, [sp, #256]
    
    /* Call syscall handler - x8 contains syscall number */
    mov     x0, sp
    bl      handle_syscall
    
    /* Store return value */
    str     x0, [sp, #0]
    
    /* Restore registers */
    ldp     x30, x0, [sp, #240]
    msr     elr_el1, x0
    ldr     x0, [sp, #256]
    msr     spsr_el1, x0
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldp     x8, x9, [sp, #64]
    ldp     x10, x11, [sp, #80]
    ldp     x12, x13, [sp, #96]
    ldp     x14, x15, [sp, #112]
    ldp     x16, x17, [sp, #128]
    ldp     x18, x19, [sp, #144]
    ldp     x20, x21, [sp, #160]
    ldp     x22, x23, [sp, #176]
    ldp     x24, x25, [sp, #192]
    ldp     x26, x27, [sp, #208]
    ldp     x28, x29, [sp, #224]
    add     sp, sp, #272
    eret

irq_handler:
    sub     sp, sp, #272
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x30, x0, [sp, #240]
    str     x1, [sp, #256]
    
    mov     x0, sp
    bl      handle_irq
    
    ldp     x30, x0, [sp, #240]
    msr     elr_el1, x0
    ldr     x0, [sp, #256]
    msr     spsr_el1, x0
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldp     x8, x9, [sp, #64]
    ldp     x10, x11, [sp, #80]
    ldp     x12, x13, [sp, #96]
    ldp     x14, x15, [sp, #112]
    ldp     x16, x17, [sp, #128]
    ldp     x18, x19, [sp, #144]
    ldp     x20, x21, [sp, #160]
    ldp     x22, x23, [sp, #176]
    ldp     x24, x25, [sp, #192]
    ldp     x26, x27, [sp, #208]
    ldp     x28, x29, [sp, #224]
    add     sp, sp, #272
    eret

fiq_handler:
    b       fiq_handler         /* FIQ not used, spin */

serror_handler:
    b       serror_handler      /* SError - critical fault */
