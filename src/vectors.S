.section .vectors, "ax"
.align 11
.global vectors

vectors:
    // Synchronous EL1t
    b .
    // IRQ EL1t
    b .
    // FIQ EL1t
    b .
    // SError EL1t
    b .
    // Synchronous EL1h
    b .
    // IRQ EL1h
    b .
    // FIQ EL1h
    b .
    // SError EL1h
    b .
    // Synchronous EL0_64
    b .
    // IRQ EL0_64
    b .
    // FIQ EL0_64
    b .
    // SError EL0_64
    b .
    // Synchronous EL0_32
    b .
    // IRQ EL0_32
    b .
    // FIQ EL0_32
    b .
    // SError EL0_32
    b .