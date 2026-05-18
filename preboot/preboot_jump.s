    .syntax unified
    .cpu cortex-m0plus
    .fpu softvfp
    .thumb

    // On entry, r0 contains vector_table pointer
    // Set stack from vector_table[0]
    // Branch to reset vector from vector_table[1].
    // Interrupts are already disabled after reset
    // It is the target's responsibility to setup VTOR.
    .section .text.preboot_jump,"ax",%progbits
    .global preboot_jump
    .type preboot_jump, %function
    .thumb_func
preboot_jump:
    ldr  r1, [r0, #4]
    ldr  r0, [r0]
    mov  sp, r0
    bx   r1
    .size preboot_jump, .-preboot_jump
