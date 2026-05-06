ENTRY(preboot_reset_handler)

MEMORY {
    PREBOOT (rx) : ORIGIN = 0x08000000, LENGTH = PREBOOT_SIZE
}

stack_end = 0x20007800;
bootloader_start = ORIGIN(PREBOOT) + PREBOOT_SIZE;
application_start = ORIGIN(PREBOOT) + PREBOOT_SIZE + BOOTLOADER_SIZE;

SECTIONS {
    .preboot_vector_table ORIGIN(PREBOOT) : {
        KEEP(*(.preboot_vector_table))
    } >PREBOOT

    .text : {
        . = ALIGN(4);
        *(.text*)
    } >PREBOOT

    /* our startup code doesn't initialize them */
    .data : { *(.data*) } >PREBOOT
    .bss  : { *(.bss*) *(COMMON) } >PREBOOT
}

/* our startup code doesn't initialize them */
ASSERT(SIZEOF(.data) == 0, "preboot must not have globals")
ASSERT(SIZEOF(.bss) == 0, "preboot must not have globals")
