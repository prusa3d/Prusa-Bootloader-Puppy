set(BL_SIZE          8192)
set(FLASH_WRITE_SIZE 256)
set(FLASH_ERASE_SIZE 2048)
set(FLASH_APP_OFFSET ${BL_SIZE})

target_sources(bootloader PRIVATE
    stm32-ocm3/Clock.cpp
    stm32-ocm3/iwdg.cpp
    stm32-ocm3/otp.cpp
    stm32-ocm3/power_panic.cpp
    stm32-ocm3/Reset.cpp
    stm32-ocm3/Rs485.cpp
    stm32-ocm3/security_features.cpp
    stm32-ocm3/SelfProgram.cpp
    stm32-ocm3/uart.cpp
)

target_include_directories(bootloader PRIVATE
    ${CMAKE_SOURCE_DIR}/stm32-ocm3
    ${CMAKE_SOURCE_DIR}/libopencm3/include
)

# Flags that libopencm3's mk/genlink-config.mk emits for DEVICE=STM32G070RBT6.
# Kept inline to avoid invoking make at configure time.
target_compile_options(bootloader PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -msoft-float
)
target_compile_definitions(bootloader PRIVATE
    STM32G0
    FLASH_ERASE_SIZE=${FLASH_ERASE_SIZE}
    FLASH_WRITE_SIZE=${FLASH_WRITE_SIZE}
    FLASH_APP_OFFSET=${FLASH_APP_OFFSET}
    "APPLICATION_SIZE=(128*1024-FLASH_APP_OFFSET)"
)

set(LDSCRIPT ${CMAKE_SOURCE_DIR}/stm32-ocm3/stm32g070rbt6.ld)
target_link_options(bootloader PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -msoft-float
    -T${LDSCRIPT}
    -nostartfiles
    -specs=nano.specs
    -Wl,--no-warn-rwx-segments
    -Wl,--defsym=BL_SIZE=${BL_SIZE}
)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS ${LDSCRIPT})

include(ExternalProject)
set(OPENCM3_LIB ${CMAKE_SOURCE_DIR}/libopencm3/lib/libopencm3_stm32g0.a)
ExternalProject_Add(libopencm3_ext
    SOURCE_DIR        ${CMAKE_SOURCE_DIR}/libopencm3
    BUILD_IN_SOURCE   TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     make lib/stm32/g0 "CFLAGS=-flto -ffat-lto-objects"
    INSTALL_COMMAND   ""
    BUILD_BYPRODUCTS  ${OPENCM3_LIB}
    BUILD_ALWAYS      FALSE
)
add_dependencies(bootloader libopencm3_ext)
target_link_libraries(bootloader PRIVATE ${OPENCM3_LIB})

set(OUT_BIN ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.bin)
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -j .isr_vector -j .text -j .text.* -j .rodata -j .data
            -O binary $<TARGET_FILE:bootloader> ${OUT_BIN}
    COMMAND chmod a-x ${OUT_BIN}
    BYPRODUCTS ${OUT_BIN}
    VERBATIM
)
