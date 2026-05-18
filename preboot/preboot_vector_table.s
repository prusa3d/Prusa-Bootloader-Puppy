    .syntax unified
    .cpu cortex-m0plus
    .fpu softvfp
    .thumb

    .global preboot_vector_table
    .section .preboot_vector_table,"a",%progbits
    .type preboot_vector_table, %object
preboot_vector_table:
    .word stack_end
    .word preboot_reset_handler
    .word preboot_default_handler // NMI
    .word preboot_default_handler // HardFault
    // SVC, PendSV, SysTick, and peripheral interrupts cannot fire unless
    // explicitly enabled. Don't waste space by including handlers in the table.
    .size preboot_vector_table, .-preboot_vector_table
