add_executable(preboot
    preboot.cpp
    preboot_vector_table.s
    preboot_jump.s
)

target_compile_features(preboot PRIVATE cxx_std_23)

target_include_directories(preboot PRIVATE
    ${CMAKE_SOURCE_DIR}/stm32-c0hal/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/stm32-c0hal/CMSIS/Device/ST/STM32C0xx/Include
)

target_compile_options(preboot PRIVATE
    -Werror -Wall -Wextra -pedantic
    -Os
    -ffunction-sections -fdata-sections -fno-exceptions
    -mcpu=cortex-m0plus
)
target_compile_definitions(preboot PRIVATE STM32C092xx)

set(PREBOOT_LDSCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/preboot.ld)
target_link_options(preboot PRIVATE
    -mcpu=cortex-m0plus
    -nostartfiles
    -Wl,-T,${PREBOOT_LDSCRIPT},--print-memory-usage,--gc-sections
    -Wl,--defsym=PREBOOT_SIZE=${PREBOOT_SIZE}
    -Wl,--defsym=BOOTLOADER_SIZE=${BOOTLOADER_SIZE}
)
set_target_properties(preboot PROPERTIES LINK_DEPENDS ${PREBOOT_LDSCRIPT})

math(EXPR PAD_TO "0x08000000 + ${PREBOOT_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
set(PREBOOT_BIN ${CMAKE_CURRENT_BINARY_DIR}/preboot.bin)
add_custom_command(TARGET preboot POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary --pad-to=${PAD_TO} --gap-fill=0xff
            $<TARGET_FILE:preboot> ${PREBOOT_BIN}
    BYPRODUCTS ${PREBOOT_BIN}
    VERBATIM
)
