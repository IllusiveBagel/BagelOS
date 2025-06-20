.section .text
.global _start

_start:
    // Set up stack pointer (e.g., at 0x80000 + 0x8000 = 0x88000)
    ldr x0, =_stack_top
    mov sp, x0

    // Call main()
    bl main

    // Infinite loop if main returns
1:  wfe
    b 1b

// Define stack (32KB)
.section .bss
.align 12
_stack:
    .skip 0x8000
_stack_top: