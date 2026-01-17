/*
 * UnixOS Kernel - Apple Silicon GPU Driver (AGX)
 * 
 * Based on Asahi Linux DRM driver.
 * This provides basic framebuffer support for Apple GPUs.
 */

#include "printk.h"
#include "types.h"
#include "mm/vmm.h"

/* ===================================================================== */
/* AGX GPU Register Definitions (Apple M2) */
/* ===================================================================== */

/* These addresses come from the Apple Device Tree and Asahi RE */
#define AGX_BASE                0x206400000UL   /* M2 GPU MMIO base */
#define AGX_SIZE                0x4000000UL     /* 64MB GPU region */

/* AGX register offsets (from Asahi Linux) */
#define AGX_CTRL_OFFSET         0x00000
#define AGX_STATUS_OFFSET       0x00004
#define AGX_VERSION_OFFSET      0x00008
#define AGX_IRQ_STATUS          0x00010
#define AGX_IRQ_ENABLE          0x00014

/* DCP (Display Coprocessor) addresses */
#define DCP_BASE                0x228200000UL
#define DCP_SIZE                0x8000000UL

/* ===================================================================== */
/* GPU State */
/* ===================================================================== */

static volatile uint32_t *agx_regs = NULL;
static bool agx_initialized = false;

/* Framebuffer info */
static struct {
    phys_addr_t base;
    size_t size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
} framebuffer_info;

/* ===================================================================== */
/* MMIO Helpers */
/* ===================================================================== */

static inline uint32_t agx_read32(uint32_t offset)
{
    if (!agx_regs) return 0;
    return agx_regs[offset / 4];
}

static inline void agx_write32(uint32_t offset, uint32_t val)
{
    if (!agx_regs) return;
    agx_regs[offset / 4] = val;
}

/* ===================================================================== */
/* Initialization */
/* ===================================================================== */

int agx_gpu_init(void)
{
    printk(KERN_INFO "AGX: Initializing Apple Silicon GPU driver\n");
    
    /* Map GPU registers */
    /* Note: On real hardware, we get the address from device tree */
    
    /* For QEMU, skip actual GPU init */
    #ifdef __QEMU__
    printk(KERN_INFO "AGX: Running in QEMU, GPU init skipped\n");
    return 0;
    #endif
    
    /* Map the GPU MMIO region */
    vmm_map_range(AGX_BASE, AGX_BASE, AGX_SIZE, VM_DEVICE);
    agx_regs = (volatile uint32_t *)AGX_BASE;
    
    /* Read GPU version */
    uint32_t version = agx_read32(AGX_VERSION_OFFSET);
    printk(KERN_INFO "AGX: GPU version: 0x%x\n", version);
    
    /* Enable GPU interrupts */
    agx_write32(AGX_IRQ_ENABLE, 0xFFFFFFFF);
    
    agx_initialized = true;
    printk(KERN_INFO "AGX: GPU initialization complete\n");
    
    return 0;
}

/* ===================================================================== */
/* Framebuffer Operations */
/* ===================================================================== */

int agx_fb_init(phys_addr_t fb_base, uint32_t width, uint32_t height, 
                uint32_t pitch, uint32_t bpp)
{
    framebuffer_info.base = fb_base;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.pitch = pitch;
    framebuffer_info.bpp = bpp;
    framebuffer_info.size = pitch * height;
    
    printk(KERN_INFO "AGX FB: %ux%u, %u bpp, pitch %u\n",
           width, height, bpp, pitch);
    
    /* Map framebuffer into kernel space */
    vmm_map_range(fb_base, fb_base, framebuffer_info.size, VM_WRITE);
    
    return 0;
}

void agx_fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, 
                      uint32_t color)
{
    volatile uint32_t *fb = (volatile uint32_t *)framebuffer_info.base;
    
    for (uint32_t row = y; row < y + h && row < framebuffer_info.height; row++) {
        for (uint32_t col = x; col < x + w && col < framebuffer_info.width; col++) {
            fb[row * (framebuffer_info.pitch / 4) + col] = color;
        }
    }
}

void agx_fb_clear(uint32_t color)
{
    agx_fb_fill_rect(0, 0, framebuffer_info.width, framebuffer_info.height, color);
}

/* ===================================================================== */
/* Power Management */
/* ===================================================================== */

int agx_power_on(void)
{
    if (!agx_initialized) return -1;
    
    printk(KERN_INFO "AGX: Powering on GPU\n");
    agx_write32(AGX_CTRL_OFFSET, 0x1);  /* Enable */
    
    return 0;
}

int agx_power_off(void)
{
    if (!agx_initialized) return -1;
    
    printk(KERN_INFO "AGX: Powering off GPU\n");
    agx_write32(AGX_CTRL_OFFSET, 0x0);  /* Disable */
    
    return 0;
}
