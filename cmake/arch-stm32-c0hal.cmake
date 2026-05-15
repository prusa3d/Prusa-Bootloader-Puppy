set(PREBOOT_SIZE     2048)
set(BOOTLOADER_SIZE  6144)
math(EXPR BL_SIZE "${PREBOOT_SIZE} + ${BOOTLOADER_SIZE}")
set(FLASH_WRITE_SIZE 256)
set(FLASH_ERASE_SIZE 2048)
set(FLASH_APP_OFFSET ${BL_SIZE})

target_sources(bootloader PRIVATE
    stm32-c0hal/Clock.cpp
    stm32-c0hal/otp.cpp
    stm32-c0hal/security_features.cpp
    stm32-c0hal/SelfProgram.cpp
    stm32-c0hal/system_stm32c0xx.c
    stm32-c0hal/startup_stm32c092xx.s

    stm32-common/iwdg.cpp
    stm32-common/power_panic.cpp
    stm32-common/Reset.cpp
    stm32-common/Rs485.cpp
    stm32-common/uart.cpp

    stm32c0xx-hal-driver/Src/stm32c0xx_hal_cortex.c
    stm32c0xx-hal-driver/Src/stm32c0xx_hal_flash.c
    stm32c0xx-hal-driver/Src/stm32c0xx_hal_flash_ex.c
    stm32c0xx-hal-driver/Src/stm32c0xx_hal_gpio.c
    stm32c0xx-hal-driver/Src/stm32c0xx_hal_rcc.c
    stm32c0xx-hal-driver/Src/stm32c0xx_hal.c
    stm32c0xx-hal-driver/Src/stm32c0xx_ll_rcc.c
    stm32c0xx-hal-driver/Src/stm32c0xx_ll_usart.c
    stm32c0xx-hal-driver/Src/stm32c0xx_ll_gpio.c
    stm32c0xx-hal-driver/Src/stm32c0xx_ll_utils.c
)

target_include_directories(bootloader PRIVATE
    ${CMAKE_SOURCE_DIR}/stm32-c0hal
    ${CMAKE_SOURCE_DIR}/stm32-common
    ${CMAKE_SOURCE_DIR}/stm32c0xx-hal-driver/Inc
    ${CMAKE_SOURCE_DIR}/stm32-c0hal/CMSIS/Device/ST/STM32C0xx/Include
    ${CMAKE_SOURCE_DIR}/stm32-c0hal/CMSIS/Include
)

target_compile_options(bootloader PRIVATE
    -mcpu=cortex-m0plus
    -flto -ffat-lto-objects
    -Wno-address-of-packed-member
)
target_compile_definitions(bootloader PRIVATE
    STM32C092xx
    STM32C0
    USE_HAL_DRIVER
    USE_FULL_LL_DRIVER
    PREBOOT_SIZE=${PREBOOT_SIZE}
    FLASH_ERASE_SIZE=${FLASH_ERASE_SIZE}
    FLASH_WRITE_SIZE=${FLASH_WRITE_SIZE}
    FLASH_APP_OFFSET=${FLASH_APP_OFFSET}
    "APPLICATION_SIZE=(256*1024-FLASH_APP_OFFSET)"
)

set(LDSCRIPT ${CMAKE_SOURCE_DIR}/stm32-c0hal/stm32c092kcux.ld)
target_link_options(bootloader PRIVATE
    -mcpu=cortex-m0plus
    -flto -ffat-lto-objects
    -T${LDSCRIPT}
    -nostartfiles
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,--defsym=PREBOOT_SIZE=${PREBOOT_SIZE}
    -Wl,--defsym=BOOTLOADER_SIZE=${BOOTLOADER_SIZE}
)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS ${LDSCRIPT})

add_subdirectory(preboot)

set(NOPREBOOT_NOCRC ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.nopreboot.nocrc.bin)
set(NOPREBOOT_BIN   ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.nopreboot.bin)
set(OUT_BIN         ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.bin)
set(PREBOOT_BIN     $<TARGET_FILE_DIR:preboot>/preboot.bin)

add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -j .isr_vector -j .text -j .text.* -j .rodata -j .data
            -O binary $<TARGET_FILE:bootloader> ${NOPREBOOT_NOCRC}
    COMMAND chmod a-x ${NOPREBOOT_NOCRC}
    COMMAND ${CMAKE_SOURCE_DIR}/pad_with_crc32.py
            ${NOPREBOOT_NOCRC} ${BOOTLOADER_SIZE} ${NOPREBOOT_BIN}
    COMMAND sh -c "cat ${PREBOOT_BIN} ${NOPREBOOT_BIN} > ${OUT_BIN}"
    BYPRODUCTS ${NOPREBOOT_NOCRC} ${NOPREBOOT_BIN} ${OUT_BIN}
    VERBATIM
)

add_dependencies(bootloader preboot)
