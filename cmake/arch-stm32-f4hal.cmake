set(BL_SIZE          16384)
set(FLASH_WRITE_SIZE 16384)
set(FLASH_ERASE_SIZE 16384)
set(FLASH_APP_OFFSET ${BL_SIZE})

target_sources(bootloader PRIVATE
    stm32-f4hal/Clock.cpp
    stm32-f4hal/Gpio.c
    stm32-f4hal/otp.cpp
    stm32-f4hal/Rs485.cpp
    stm32-f4hal/security_features.cpp
    stm32-f4hal/SelfProgram.cpp
    stm32-f4hal/setup_stdout.c
    stm32-f4hal/system_stm32f4xx.c
    stm32-f4hal/startup_stm32f427xx.s

    stm32-common/iwdg.cpp
    stm32-common/power_panic.cpp
    stm32-common/Reset.cpp
)

set(HAL_SOURCES
    stm32f4xx_hal_driver/Src/stm32f4xx_hal.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_cortex.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_flash.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_flash_ex.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_gpio.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_rcc.c
    stm32f4xx_hal_driver/Src/stm32f4xx_hal_uart.c
    stm32f4xx_hal_driver/Src/stm32f4xx_ll_gpio.c
    stm32f4xx_hal_driver/Src/stm32f4xx_ll_rcc.c
    stm32f4xx_hal_driver/Src/stm32f4xx_ll_usart.c
    stm32f4xx_hal_driver/Src/stm32f4xx_ll_utils.c
)
target_sources(bootloader PRIVATE ${HAL_SOURCES})
set_source_files_properties(${HAL_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")

target_include_directories(bootloader PRIVATE
    ${CMAKE_SOURCE_DIR}/stm32-f4hal
    ${CMAKE_SOURCE_DIR}/stm32-common
    ${CMAKE_SOURCE_DIR}/stm32f4xx_hal_driver/Inc
    ${CMAKE_SOURCE_DIR}/stm32-f4hal/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_SOURCE_DIR}/stm32-f4hal/CMSIS/Include
)

target_compile_options(bootloader PRIVATE
    -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb
    -flto -ffat-lto-objects
    -Wno-address-of-packed-member
)
target_compile_definitions(bootloader PRIVATE
    STM32F427xx
    STM32F4
    USE_HAL_DRIVER
    USE_FULL_LL_DRIVER
    BL_SIZE=${BL_SIZE}
    FLASH_ERASE_SIZE=${FLASH_ERASE_SIZE}
    FLASH_WRITE_SIZE=${FLASH_WRITE_SIZE}
    FLASH_APP_OFFSET=${FLASH_APP_OFFSET}
    "APPLICATION_SIZE=(2048*1024-FLASH_APP_OFFSET)"
)

set(LDSCRIPT ${CMAKE_SOURCE_DIR}/stm32-f4hal/STM32F427ZITx_FLASH.ld)
target_link_options(bootloader PRIVATE
    -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb
    -flto -ffat-lto-objects
    -T${LDSCRIPT}
    -nostartfiles
    -specs=nano.specs
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
