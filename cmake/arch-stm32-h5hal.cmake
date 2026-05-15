set(BL_SIZE          8192)
set(FLASH_WRITE_SIZE 8192)
set(FLASH_ERASE_SIZE 8192)
set(FLASH_APP_OFFSET ${BL_SIZE})

target_sources(bootloader PRIVATE
    stm32-h5hal/Clock.cpp
    stm32-h5hal/otp.cpp
    stm32-h5hal/security_features.cpp
    stm32-h5hal/SelfProgram.cpp
    stm32-h5hal/system_stm32h5xx.c
    stm32-h5hal/startup_stm32h503cbux.s

    stm32-common/iwdg.cpp
    stm32-common/power_panic.cpp
    stm32-common/Reset.cpp
    stm32-common/Rs485.cpp
    stm32-common/uart.cpp

    stm32h5xx_hal_driver/Src/stm32h5xx_hal_cortex.c
    stm32h5xx_hal_driver/Src/stm32h5xx_hal_flash.c
    stm32h5xx_hal_driver/Src/stm32h5xx_hal_flash_ex.c
    stm32h5xx_hal_driver/Src/stm32h5xx_hal.c
    stm32h5xx_hal_driver/Src/stm32h5xx_ll_rcc.c
    stm32h5xx_hal_driver/Src/stm32h5xx_ll_usart.c
    stm32h5xx_hal_driver/Src/stm32h5xx_ll_gpio.c
    stm32h5xx_hal_driver/Src/stm32h5xx_ll_utils.c
)

target_include_directories(bootloader PRIVATE
    ${CMAKE_SOURCE_DIR}/stm32-h5hal
    ${CMAKE_SOURCE_DIR}/stm32-common
    ${CMAKE_SOURCE_DIR}/stm32h5xx_hal_driver/Inc
    ${CMAKE_SOURCE_DIR}/stm32-h5hal/CMSIS/Device/ST/STM32H5xx/Include
    ${CMAKE_SOURCE_DIR}/stm32-h5hal/CMSIS/Include
)

target_compile_options(bootloader PRIVATE
    -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16
    -flto -ffat-lto-objects
    -Wno-address-of-packed-member
)
target_compile_definitions(bootloader PRIVATE
    STM32H503xx
    STM32H5
    USE_HAL_DRIVER
    USE_FULL_LL_DRIVER
    FLASH_ERASE_SIZE=${FLASH_ERASE_SIZE}
    FLASH_WRITE_SIZE=${FLASH_WRITE_SIZE}
    FLASH_APP_OFFSET=${FLASH_APP_OFFSET}
    "APPLICATION_SIZE=(128*1024-FLASH_APP_OFFSET)"
)

set(LDSCRIPT ${CMAKE_SOURCE_DIR}/stm32-h5hal/stm32h503cbux.ld)
target_link_options(bootloader PRIVATE
    -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16
    -flto -ffat-lto-objects
    -T${LDSCRIPT}
    -nostartfiles
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,--defsym=BL_SIZE=${BL_SIZE}
)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS ${LDSCRIPT})

set(OUT_BIN ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.bin)
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -j .isr_vector -j .text -j .text.* -j .rodata -j .data
            -O binary $<TARGET_FILE:bootloader> ${OUT_BIN}
    COMMAND chmod a-x ${OUT_BIN}
    BYPRODUCTS ${OUT_BIN}
    VERBATIM
)
