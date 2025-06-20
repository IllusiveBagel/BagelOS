.section .text
.global _start

_start:
    // Zero .bss
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    mov x2, #0
bss_loop:
    cmp x0, x1
    b.ge bss_done
    str x2, [x0], #8
    b bss_loop
bss_done:

    // Set up stack pointer
    ldr x0, =_stack_top
    mov sp, x0

    // Call main()
    bl main

1:  wfe
    b 1b

// Stack (32KB)
.section .bss
.align 12
_stack:
    .skip 0x8000
_stack_top: