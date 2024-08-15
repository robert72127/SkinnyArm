#pragma once

/**
 * CPU 64-bit ARM-8 Cortex-A5
 * 1.2 GHz clk freq
 * 1 GB of RAM
 * 40 GPIO
 * microSD car slot 
 */


// rpi3b has 4 cpu's
#define NCPU 4

// memory map
// see BCM2837-ARM-Peripherals.pdf
// thank's to https://github.com/bztsrc/raspi3-tutorial

// page 6 of PDF
#define MMIO_BASE       0x3F000000
#define MMIO_END        0x3FFFFFFF
//Thus a peripheral advertised here at bus address 0x7Ennnnnn is available at physical address 0x3Fnnnnnn.

/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

// gpio pins
#define GPFSEL0         ((volatile unsigned int*)(MMIO_BASE+0x00200000))
#define GPFSEL1         ((volatile unsigned int*)(MMIO_BASE+0x00200004))
#define GPFSEL2         ((volatile unsigned int*)(MMIO_BASE+0x00200008))
#define GPFSEL3         ((volatile unsigned int*)(MMIO_BASE+0x0020000C))
#define GPFSEL4         ((volatile unsigned int*)(MMIO_BASE+0x00200010))
#define GPFSEL5         ((volatile unsigned int*)(MMIO_BASE+0x00200014))
#define GPSET0          ((volatile unsigned int*)(MMIO_BASE+0x0020001C))
#define GPSET1          ((volatile unsigned int*)(MMIO_BASE+0x00200020))
#define GPCLR0          ((volatile unsigned int*)(MMIO_BASE+0x00200028))
#define GPLEV0          ((volatile unsigned int*)(MMIO_BASE+0x00200034))
#define GPLEV1          ((volatile unsigned int*)(MMIO_BASE+0x00200038))
#define GPEDS0          ((volatile unsigned int*)(MMIO_BASE+0x00200040))
#define GPEDS1          ((volatile unsigned int*)(MMIO_BASE+0x00200044))
#define GPHEN0          ((volatile unsigned int*)(MMIO_BASE+0x00200064))
#define GPHEN1          ((volatile unsigned int*)(MMIO_BASE+0x00200068))
#define GPPUD           ((volatile unsigned int*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((volatile unsigned int*)(MMIO_BASE+0x00200098))
#define GPPUDCLK1       ((volatile unsigned int*)(MMIO_BASE+0x0020009C))


// random number generator, used in scheduler
#define RNG_CTRL        ((volatile unsigned int*)(MMIO_BASE+0x00104000))
#define RNG_STATUS      ((volatile unsigned int*)(MMIO_BASE+0x00104004))
#define RNG_DATA        ((volatile unsigned int*)(MMIO_BASE+0x00104008))
#define RNG_INT_MASK    ((volatile unsigned int*)(MMIO_BASE+0x00104010))

// Delays
#define SYSTMR_LO        ((volatile unsigned int*)(MMIO_BASE+0x00003004))
#define SYSTMR_HI        ((volatile unsigned int*)(MMIO_BASE+0x00003008))

// SD_CARD
#define EMMC_ARG2           ((volatile unsigned int*)(MMIO_BASE+0x00300000))
#define EMMC_BLKSIZECNT     ((volatile unsigned int*)(MMIO_BASE+0x00300004))
#define EMMC_ARG1           ((volatile unsigned int*)(MMIO_BASE+0x00300008))
#define EMMC_CMDTM          ((volatile unsigned int*)(MMIO_BASE+0x0030000C))
#define EMMC_RESP0          ((volatile unsigned int*)(MMIO_BASE+0x00300010))
#define EMMC_RESP1          ((volatile unsigned int*)(MMIO_BASE+0x00300014))
#define EMMC_RESP2          ((volatile unsigned int*)(MMIO_BASE+0x00300018))
#define EMMC_RESP3          ((volatile unsigned int*)(MMIO_BASE+0x0030001C))
#define EMMC_DATA           ((volatile unsigned int*)(MMIO_BASE+0x00300020))
#define EMMC_STATUS         ((volatile unsigned int*)(MMIO_BASE+0x00300024))
#define EMMC_CONTROL0       ((volatile unsigned int*)(MMIO_BASE+0x00300028))
#define EMMC_CONTROL1       ((volatile unsigned int*)(MMIO_BASE+0x0030002C))
#define EMMC_INTERRUPT      ((volatile unsigned int*)(MMIO_BASE+0x00300030))
#define EMMC_INT_MASK       ((volatile unsigned int*)(MMIO_BASE+0x00300034))
#define EMMC_INT_EN         ((volatile unsigned int*)(MMIO_BASE+0x00300038))
#define EMMC_CONTROL2       ((volatile unsigned int*)(MMIO_BASE+0x0030003C))
#define EMMC_SLOTISR_VER    ((volatile unsigned int*)(MMIO_BASE+0x003000FC))

