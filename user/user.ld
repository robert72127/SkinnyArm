SECTIONS
{
    . = 0x0;
    _start = .;
    .text : {
         KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) 
    } 
    .rodata : {
         *(.rodata .rodata.* .gnu.linkonce.r*) 
    }
    PROVIDE(_data = .);
    .data : {
         *(.data .data.* .gnu.linkonce.d*) 
    }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    _end = .;
    /* align to mage size */
   /* /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) } */
}
__bss_size = (__bss_end - __bss_start);