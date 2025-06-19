ENTRY(_start)

SECTIONS
{
    . = 0x80000;  /* Load address for kernel on Pi (512KB into memory) */

    .text : {
        *(.text)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}