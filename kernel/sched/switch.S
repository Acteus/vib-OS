/*
 * UnixOS Kernel - Context Switch Assembly (ARM64)
 * 
 * Performs low-level context switch between tasks.
 */

.global cpu_switch_to
.type cpu_switch_to, %function

/*
 * cpu_switch_to(prev, next)
 * 
 * Save callee-saved registers of prev task and restore those of next task.
 * 
 * Arguments:
 *   x0 - prev task_struct pointer
 *   x1 - next task_struct pointer
 * 
 * cpu_context offset in task_struct should match the C structure.
 * Assuming it's at offset 96 (adjust based on actual layout):
 *   x19-x28, fp (x29), sp, pc
 */

#define THREAD_CPU_CONTEXT  96  /* Offset of cpu_context in task_struct */

/* Register offsets within cpu_context */
#define CTX_X19     0
#define CTX_X20     8
#define CTX_X21     16
#define CTX_X22     24
#define CTX_X23     32
#define CTX_X24     40
#define CTX_X25     48
#define CTX_X26     56
#define CTX_X27     64
#define CTX_X28     72
#define CTX_FP      80
#define CTX_SP      88
#define CTX_PC      96

cpu_switch_to:
    /* Save context of prev task */
    add     x8, x0, #THREAD_CPU_CONTEXT
    
    /* Save callee-saved registers */
    stp     x19, x20, [x8, #CTX_X19]
    stp     x21, x22, [x8, #CTX_X21]
    stp     x23, x24, [x8, #CTX_X23]
    stp     x25, x26, [x8, #CTX_X25]
    stp     x27, x28, [x8, #CTX_X27]
    
    /* Save frame pointer and stack pointer */
    stp     x29, x30, [x8, #CTX_FP]
    
    /* Save stack pointer */
    mov     x9, sp
    str     x9, [x8, #CTX_SP]
    
    /* Save return address as PC for when we switch back */
    str     x30, [x8, #CTX_PC]
    
    /* ----------------------------------------------------------------- */
    /* Now restore context of next task */
    /* ----------------------------------------------------------------- */
    
    add     x8, x1, #THREAD_CPU_CONTEXT
    
    /* Restore callee-saved registers */
    ldp     x19, x20, [x8, #CTX_X19]
    ldp     x21, x22, [x8, #CTX_X21]
    ldp     x23, x24, [x8, #CTX_X23]
    ldp     x25, x26, [x8, #CTX_X25]
    ldp     x27, x28, [x8, #CTX_X27]
    
    /* Restore frame pointer and link register */
    ldp     x29, x30, [x8, #CTX_FP]
    
    /* Restore stack pointer */
    ldr     x9, [x8, #CTX_SP]
    mov     sp, x9
    
    /* Return - this returns to the new task's saved PC */
    ret

/* ===================================================================== */
/* Task entry wrapper */
/* ===================================================================== */

.global task_entry_wrapper
.type task_entry_wrapper, %function

/*
 * task_entry_wrapper
 * 
 * Entry point for new tasks. Sets up the environment and calls the
 * actual task function. Never returns.
 * 
 * x19 contains the entry function pointer (saved by create_task)
 * x20 contains the argument to pass
 */
task_entry_wrapper:
    /* Enable interrupts for user tasks */
    msr     daifclr, #2
    
    /* Call the actual task function: entry(arg) */
    mov     x0, x20
    blr     x19
    
    /* If task function returns, exit */
    mov     x0, #0          /* Exit code 0 */
    bl      exit_task
    
    /* Should never reach here */
1:
    wfi
    b       1b
