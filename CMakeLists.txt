cmake_minimum_required(VERSION 3.21)
project(prusa-bootloader-puppy LANGUAGES C CXX ASM)

set(BOARD "" CACHE STRING "dwarf | modularbed | xbuddy_extension | indx_head")
set(CURRENT_HW_REVISION    "0x10" CACHE STRING "HW revision byte, e.g. 0x10")
set(COMPATIBLE_HW_REVISION "0x10" CACHE STRING "HW revision byte, e.g. 0x10")
set(VERSION                "1"    CACHE STRING "Bootloader version (integer)")
set(VERSION_PREFIX         ""     CACHE STRING "Bootloader version prefix string")

math(EXPR HW_MAJOR "${CURRENT_HW_REVISION} / 16" OUTPUT_FORMAT DECIMAL)
math(EXPR HW_MINOR "${CURRENT_HW_REVISION} % 16" OUTPUT_FORMAT DECIMAL)
set(FILE_NAME "bootloader-${VERSION_PREFIX}v${VERSION}-${BOARD}-${HW_MAJOR}.${HW_MINOR}")

add_executable(bootloader
    BaseProtocol.cpp
    bootloader.cpp
    led.cpp
    main.cpp
    rtt.cpp
    SelfProgramCommon.cpp
    sha256.cpp
)
target_include_directories(bootloader PRIVATE ${CMAKE_SOURCE_DIR})

set_target_properties(bootloader PROPERTIES
    OUTPUT_NAME "${FILE_NAME}"
    C_STANDARD   99
    C_EXTENSIONS ON
    CXX_STANDARD   11
    CXX_EXTENSIONS ON
)

target_compile_options(bootloader PRIVATE
    -Wall -Wextra -Werror
    -fpack-struct -fshort-enums
    -ggdb3 -Os
    -ffunction-sections -fdata-sections
    -fno-exceptions
)
target_link_options(bootloader PRIVATE
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

target_compile_definitions(bootloader PRIVATE
    STM32
    VERSION_SIZE=7
    PROTOCOL_VERSION=0x0302
    FW_DESCRIPTOR_SIZE=128
    HARDWARE_REVISION=${CURRENT_HW_REVISION}
    HARDWARE_COMPATIBLE_REVISION=${COMPATIBLE_HW_REVISION}
    BL_VERSION=${VERSION}
)

if(BOARD STREQUAL "dwarf")
    set(ARCH stm32-ocm3)
    target_compile_definitions(bootloader PRIVATE BOARD_TYPE_prusa_dwarf DISABLE_WATCHDOG)
elseif(BOARD STREQUAL "modularbed")
    set(ARCH stm32-ocm3)
    target_compile_definitions(bootloader PRIVATE BOARD_TYPE_prusa_modular_bed DISABLE_WATCHDOG)
elseif(BOARD STREQUAL "xbuddy_extension")
    set(ARCH stm32-h5hal)
    target_compile_definitions(bootloader PRIVATE BOARD_TYPE_prusa_xbuddy_extension FIXED_ADDRESS=17 DISABLE_WATCHDOG)
elseif(BOARD STREQUAL "indx_head")
    set(ARCH stm32-c0hal)
    target_compile_definitions(bootloader PRIVATE BOARD_TYPE_prusa_indx_head FIXED_ADDRESS=18)
else()
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'")
endif()

include(cmake/arch-${ARCH}.cmake)
